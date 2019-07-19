#include <iostream>
#include <fstream>
#include <dirent.h>
#include <thread>
#include <vector>
#include <csignal>
#include <stdio.h>
#include <algorithm>
#include <time.h>

#include "resources/includes/JsonBuilder.h"
#include "resources/includes/EventDetector.h"
#include "resources/includes/Tracker.h"
#include "resources/includes/Calibration.h"

using namespace std;
using namespace cv;

// Directory to save JSON config video_files to.
#define JSON_DIR "static/video-info/"
#define VIDEO_DIR "static/videos/"

//#define THREADED

void HandleSignal(int);
std::vector<std::string> GetVideosFromDir(std::string, std::vector<std::string>);

cv::Mat ConcatenateMatrices(cv::Mat&, cv::Mat&);
void ProcessVideo(std::string, std::string);

void TriangulateSelectedPoints(Calibration*&);
void ReadVectorOfVector(cv::FileStorage&, std::string, std::vector<std::vector<cv::Point2f>>&);

int main(int argc, char** argv)
{
    signal(SIGABRT, HandleSignal);
    signal(SIGINT, HandleSignal);

    useOptimized();
    setUseOptimized(true);

    bool bHasVideos = false;

    do 
    {
        std::vector<std::string> json_filters = { ".json", ".JSON" };
        auto json_files = GetVideosFromDir(JSON_DIR, json_filters);
        std::vector<std::string> vid_filters = { ".mp4", ".MP4" };
        auto video_files = GetVideosFromDir(VIDEO_DIR, vid_filters);
        std::sort(video_files.begin(), video_files.end());

        bHasVideos = !video_files.empty() ? true : false;

        for (size_t i = 0; i < json_files.size(); i++) 
        {
            string jf = json_files[i].substr(json_files[i].find("DE_") + 3, json_files[i].length());
            jf = jf.substr(0, jf.find_last_of("."));

            for (auto it = video_files.begin(); it != video_files.end(); ++it)
                if ((*it).find(jf) != string::npos) 
                {
                    video_files.erase(it);
                    break;
                }
        }
        std::sort(video_files.begin(), video_files.end());

#ifdef THREADED
        std::vector<std::thread> threads;
        threads.resize(video_files.size());
        for (size_t i = 0; i < video_files.size() / 2; i++)
        {
            std::cout << "!!! Creating Thread: " << i << " !!!\n";
            threads[i] = thread(ProcessVideo, video_files[i], video_files[(i + 1) % video_files.size()]);
            threads[i].join();
        }

#else
        if(argv[1] == NULL)
            for (size_t i = 0; i < video_files.size() / 2; i += 2)
                ProcessVideo(video_files[i], video_files[(i + 1) % video_files.size()]);
#endif
        if(video_files.size() > 1)
            for(std::string video  : video_files)
                std::remove(video.c_str());
            
    } while (bHasVideos);

    if(argv[1] != NULL)
    {
        Calibration *calib = nullptr;
        TriangulateSelectedPoints(calib);
    }

    return 0;
}

void ProcessVideo(std::string file1, std::string file2)
{
    clock_t time_start = clock();
    if(file1 != file2)
    {
        cv::VideoCapture left_cap(file1), right_cap(file2);
        if (!left_cap.isOpened() && !right_cap.isOpened())
        {
            std::cerr << "*** Could not open video file(s)" << std::endl;
            return;
        }

        // Clean up the video filenames.
        {
            // Now that we've opened the file, scrub it clean of directory names to get the tag name.
            file1 = file1.substr(file1.find_last_of("/") + 1, file1.length());
            file1 = file1.substr(0, file1.find_last_of("_"));
            file2 = file2.substr(file2.find_last_of("/") + 1, file2.length());
            file2 = file2.substr(0, file2.find_last_of("_"));
        }

        std::string file_name = "";
        if (file1 == file2) 
        {
            // Create a save location for the new combined video.
            file_name = "./static/proc_videos/" + file1 + ".mp4";
            std::cout << "=== Creating \"" << file_name << "\" ===" << std::endl;

            // Create a writer for the new combined video.
            VideoWriter writer(file_name,
                            int(left_cap.get(CAP_PROP_FOURCC)),
                            left_cap.get(CAP_PROP_FPS),
                            Size(left_cap.get(CAP_PROP_FRAME_WIDTH) + right_cap.get(CAP_PROP_FRAME_WIDTH),
                                    max(left_cap.get(CAP_PROP_FRAME_HEIGHT), right_cap.get(CAP_PROP_FRAME_HEIGHT))),
                            true);

            // Setup stereo calibration and triangulate points.
            Calibration::Input input;
            input.image_size = cv::Size(1920, 1440);
            Calibration*  calib = new Calibration(input, CalibrationType::STEREO, "StereoCalibration.yaml");
            calib->ReadCalibration();

            // Create the JSON object which will hold all of the detected events.
            JSON DE("DetectedEvents");

            // Setup QR Code detection events for the left and right videos.
            QREvent* detectQR_left = nullptr, *detectQR_right = nullptr;

            // Setup the tracker.
            Tracker::Settings t_conf;
            t_conf.bDrawContours = false;
            t_conf.MinThreshold = 200;
            
            Tracker tracker_l(t_conf), tracker_r(t_conf);

            auto totalFrames = min(left_cap.get(cv::CAP_PROP_FRAME_COUNT), right_cap.get(cv::CAP_PROP_FRAME_COUNT));
            int currentFrame = 0, frameOffset = -1;
            while (true) 
            {
                cv::Mat left_frame, right_frame;

                // Read the frames only as long as either no QR code is detected, or both are detected. Keeps reading the one that isn't detected until it is.
                if((!detectQR_left || !detectQR_left->DetectedQR()) || (detectQR_left->DetectedQR() && detectQR_right->DetectedQR()) || left_frame.empty())
                    left_cap.read(left_frame);
                if((!detectQR_right || !detectQR_right->DetectedQR()) || (detectQR_left->DetectedQR() && detectQR_right->DetectedQR()) || right_frame.empty())
                    right_cap.read(right_frame);

                if ((left_frame.empty() || right_frame.empty()) && (currentFrame < totalFrames - 1))
                    continue;

                currentFrame++;
                if (currentFrame >= totalFrames)
                    break;

                // Detect QR codes in each video.
                {
                    if (!detectQR_left || !detectQR_left->DetectedQR())
                    {
                        if(detectQR_left) delete detectQR_left;
                        detectQR_left = new QREvent(left_frame);
                        detectQR_left->StartEvent(currentFrame);
                    } 

                    if (!detectQR_right || !detectQR_right->DetectedQR())
                    {
                        if(detectQR_right) delete detectQR_right;
                        detectQR_right = new QREvent(right_frame);
                        detectQR_right->StartEvent(currentFrame);
                    } 
                }
                
                // Undistort the frames using camera calibration data.
                if(calib)
                {
                    calib->UndistortImage(left_frame, 0);
                    calib->UndistortImage(right_frame, 1);
                }

                tracker_l.CreateMask(left_frame);
                tracker_r.CreateMask(right_frame);

                // Only start writing if both videos are synced up.
                if(detectQR_left->DetectedQR() && detectQR_right->DetectedQR())
                {
                    cout << "Writing frame " << currentFrame << " of " << totalFrames << "\n";
                    // Account for frame offset from the last detected QR code frame.
                    if(frameOffset == -1) frameOffset = currentFrame;
                    int adj_frame = (currentFrame - frameOffset);                    

                    // Run the tracker_l on the undistorted frames.
                    tracker_l.CreateMask(left_frame);
                    tracker_l.CheckForActivity(adj_frame);
                    tracker_r.CreateMask(right_frame);
                    tracker_r.CheckForActivity(adj_frame);

                    // Write the concatenated undistorted frames.
                    cv::Mat res = ConcatenateMatrices(left_frame, right_frame);
                    
                    /*
                    imshow("res", res);
                    char c = waitKey(25);
                    if(c == 27) break;
                    */
                    writer << res;
                }
            }
            std::cout << "=== Finished Concatenating ===" << endl;
            std::cout << "=== Time taken: " << (double)(clock() - time_start)/CLOCKS_PER_SEC << " seconds ===\n";
            left_cap.release();
            right_cap.release();
            cv::destroyAllWindows();

            for(auto event : tracker_l.ActivityRange)
            {
                ActivityEvent act(int(event.first), event.second.first, event.second.second > 0 ? event.second.second : currentFrame);
                DE.AddObject(act.GetAsJSON());
            }

            for(auto event : tracker_r.ActivityRange)
            {
                ActivityEvent act(int(event.first), event.second.first, event.second.second > 0 ? event.second.second : currentFrame);
                DE.AddObject(act.GetAsJSON());
            }

            // Construct the JSON object array of all events detected.
            DE.BuildJSONObjectArray();

            // Create the JSON file for this video.
            ofstream configFile;
            configFile.open(JSON_DIR "DE_" + file1 + ".json");
            configFile << DE.GetJSON();
            configFile.close();

            // Cleanup pointers
            {
                if (detectQR_left)
                {
                    delete detectQR_left;
                    detectQR_left = nullptr;
                }
                if (detectQR_right)
                {
                    delete detectQR_right;
                    detectQR_right = nullptr;
                }
                if(calib)
                {
                    delete calib;
                    calib = nullptr;
                }
            }

            std::cout << "=== Finished Processing for \"" << file_name << "\"===\n";
        }
    }
}

void HandleSignal(int signal)
{
    std::cout << "\r=== Got signal: " << signal << " ===" << endl;
    std::cout << "  > Terminating..." << endl;
    exit(0);
}

std::vector<std::string> GetVideosFromDir(std::string dir, std::vector<std::string> filters)
{
    vector<string> video_files;
    DIR* dp;
    struct dirent* d;
    if ((dp = opendir(dir.c_str())) != NULL)
    {
        while ((d = readdir(dp)) != NULL)
            for (auto filter : filters)
                if ((strcmp(string(d->d_name).c_str(), ".") != false && strcmp(string(d->d_name).c_str(), "..") != false) && string(d->d_name).find(filter) != string::npos)
                    video_files.push_back(dir + d->d_name);
        closedir(dp);
    }
    return video_files;
}

Mat ConcatenateMatrices(Mat& left_mat, Mat& right_mat)
{
    int rows = max(left_mat.rows, right_mat.rows);
    int cols = left_mat.cols + right_mat.cols;

    cv::Mat3b conc(rows, cols, Vec3b(0, 0, 0));
    cv::Mat3b res(rows, cols, Vec3b(0, 0, 0));

    left_mat.copyTo(res(Rect(0, 0, left_mat.cols, left_mat.rows)));
    right_mat.copyTo(res(Rect(left_mat.cols, 0, right_mat.cols, right_mat.rows)));

    return std::move(res);
}

void TriangulateSelectedPoints(Calibration*& calib)
{
    // Get keypoints saved from browser.
    {
        Calibration::Input input;
        vector<vector<Point2f> > keypoints_l, keypoints_r;
        {
            // TODO: Combine the two of these to be read from the same file. This requires the JS post them together.
            FileStorage fs("calib_config/measure_points_left.yaml", FileStorage::READ);
            ReadVectorOfVector(fs, "keypoints", keypoints_l);
            input.image_points[0] = keypoints_l;
        }
        {
            FileStorage fs("calib_config/measure_points_right.yaml", FileStorage::READ);
            ReadVectorOfVector(fs, "keypoints", keypoints_r);
            input.image_points[1] = keypoints_r;
        }
        
        if(input.image_points[0].size() ==  input.image_points[0].size())
        {
            calib = new Calibration(input, CalibrationType::STEREO, "StereoCalibration.yaml");
            calib->ReadCalibration();
            calib->TriangulatePoints();
        }  
    }
}

void ReadVectorOfVector(FileStorage& fs, string name, vector<vector<Point2f> >& data)
{
    data.clear();
    FileNode fn = fs[name];
    if (fn.empty())
        return;

    for (auto node : fn) 
    {
        vector<Point2f> temp_vec;
        node >> temp_vec;
        data.push_back(temp_vec);
    }
}