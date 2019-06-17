#include <iostream>
#include <fstream>
#include <dirent.h>
#include <thread>
#include <map>
#include <vector>
#include <csignal>
#include <algorithm>

#include "resources/includes/JsonBuilder.h"
#include "resources/includes/EventDetector.h"

#include "resources/includes/Calibration.h"

// Hoping to move the OpenCV out of this file.

/* Included in EventDetector.h */
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgcodecs.hpp>
/*******************************/
#include <opencv2/stereo.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/ximgproc.hpp>

using namespace std;
using namespace cv;

// Directory to save JSON config video_files to.
#define JSON_DIR "static/video-info/"
#define VIDEO_DIR "static/videos/"

//#define TRACKING
//#define THREADED

void HandleSignal(int);
vector<string> GetVideosFromDir(string, vector<string>);
void ProcessVideo(string);

Mat ConcatenateMatrices(Mat&, Mat&);
void CreateConcatVideo(string&, string&);

void TriangulateSelectedPoints();
void ReadVectorOfVector(FileStorage&, string, vector<vector<Point2f>>&);

int main()
{
    signal(SIGABRT, HandleSignal);

    useOptimized();
    setUseOptimized(true);

    // Keep the application running to continuously check for more files
    do {
        vector<string> json_filters = { ".json", ".JSON" };
        auto json_files = GetVideosFromDir(JSON_DIR, json_filters);
        vector<string> vid_filters = { ".mp4", ".MP4" };
        auto video_files = GetVideosFromDir(VIDEO_DIR, vid_filters);
        std::sort(video_files.begin(), video_files.end());

        // This looks pretty heavy (and is), so consider refining this to reduce CPU processing.
        for (int i = 0; i < json_files.size(); i++) {
            string jf = json_files[i].substr(json_files[i].find("DE_") + 3, json_files[i].length());
            jf = jf.substr(0, jf.find_last_of("."));

            for (auto it = video_files.begin(); it != video_files.end(); ++it)
                if ((*it).find(jf) != string::npos) {
                    // cout << "\"" << (*it) << "\" has already been processed!"<<endl;
                    video_files.erase(it);
                    break;
                }
        }

#ifdef THREADED

        vector<thread> threads;
        threads.resize(video_files.size());
        for (int i = 0; i < video_files.size(); i++)
            threads[i] = thread(ProcessVideo, video_files[i]);

        for (int i = 0; i < threads.size(); i++)
            threads[i].join();

#else

        for (int i = 0; i < video_files.size(); i++)
            //ProcessVideo(video_files[i]);
            CreateConcatVideo(video_files[i], video_files[(i + 1) % video_files.size()]);

#endif

    } while (true); // Could be helpful to add some event notifier to help close this more gracefully.

    return 0;
}

void HandleSignal(int signal)
{
    cout << endl << "Got signal: " << signal << endl;
    exit(signal);
}

vector<string> GetVideosFromDir(string dir, vector<string> filters)
{
    vector<string> video_files;
    DIR* dp;
    struct dirent* d;
    if ((dp = opendir(dir.c_str())) != NULL) {
        while ((d = readdir(dp)) != NULL)
            for (auto filter : filters)
                if ((strcmp(string(d->d_name).c_str(), ".") != false && strcmp(string(d->d_name).c_str(), "..") != false) && string(d->d_name).find(filter) != string::npos)
                    video_files.push_back(dir + d->d_name);
        closedir(dp);
    }
    return video_files;
}

void ProcessVideo(string file)
{
    string vidFileName = file;
    // Open video.
    VideoCapture cap(vidFileName);
    if (!cap.isOpened()) {
        cerr << "*** Could not open video file: \"" << vidFileName << "\"" << endl;
        return;
    }

    // Clean up the video file name.
    {
        // Now that we've opened the file, scrub it clean of directory names to get the tag name.
        vidFileName = vidFileName.substr(vidFileName.find_last_of("/") + 1, vidFileName.length());

        // Strip the uneeded V_ from the video.
        if (vidFileName.find("V_") != string::npos)
            vidFileName = vidFileName.substr(vidFileName.find_last_of("V_") + 1, vidFileName.length());
    }

    // Get the total number of frames in the video.
    auto totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    cout << "=== Processing \"" << vidFileName << "\" ===" << endl;
    cout << "  > Total Frames: " << totalFrames << endl;

    // Create the JSON object which will hold all of the detected events.
    JSON DE("DetectedEvents");

    QREvent* detectQR = nullptr;
    ActivityEvent* activity = nullptr;

#ifdef TRACKING
    Mat mask;
    Ptr<BackgroundSubtractor> bkgd_sub_ptr = createBackgroundSubtractorKNN();
#endif

    int currentFrame = 0;
    int currentEvent = 1;
    while (true) {
        // Read in the first frame of the video.
        Mat frame;
        cap.read(frame);

        // If the frame is empty, we simply skip this one and move on.
        // TODO: GoPro videos seem to have lots of these due to large amounts of meta-data. This might be the only workaround for now
        if (frame.empty())
            continue;

        currentFrame++;
        if (currentFrame >= totalFrames)
            break;

        // Gets the first QR code it sees, then stops checking once it has one.
        if (!detectQR || !detectQR->DetectedQR()) {
            detectQR = new QREvent(frame);
            detectQR->StartEvent(currentFrame);
            DE.AddObject(detectQR->GetAsJSON());
        }

#ifdef TRACKING
        // Masking to find contours of moving objects.
        {
            bkgd_sub_ptr->apply(frame, mask);

            threshold(mask, mask, 250, 255, THRESH_BINARY);

            int morph_elem = 0;
            int morph_size = 7, erode_size = 2;
            Mat kernel = getStructuringElement(morph_elem, Size(2 * erode_size + 1, 2 * erode_size + 1), Point(erode_size, erode_size));
            Mat element = getStructuringElement(morph_elem, Size(2 * morph_size + 1, 2 * morph_size + 1), Point(morph_size, morph_size));

            dilate(mask, mask, kernel, Point(1, 1));
            morphologyEx(mask, mask, MORPH_CLOSE, element);
            erode(mask, mask, kernel, Point(0, 0));

            medianBlur(mask, mask, 3);

            int thresh = 2000;
            RNG rng(12345);

            Mat canny_output;
            vector<vector<Point> > contours;
            vector<Vec4i> hierarchy;

            // Detect edges using canny
            Canny(mask, canny_output, thresh, thresh * 2, 5);
            findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_TC89_KCOS, Point(0, 0));

            Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3);
            for (int i = 0; i < contours.size(); i++) {
                Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
                drawContours(frame, contours, i, color, 2, 8, hierarchy, 0, Point());
            }

            if (!contours.empty()) {
                if (!activity) {
                    activity = new ActivityEvent(frame, currentEvent);
                    activity->StartEvent(currentFrame);
                }
            } else {
                if (activity) {
                    activity->SetIsActive(false);
                    activity->EndEvent(currentFrame);
                    DE.AddObject(activity->GetAsJSON());
                    delete activity;
                    activity = nullptr;
                    currentEvent++;
                }
            }
        }

        char c = (char)waitKey(25);
        if (c == 27)
            break;

#endif
    }
    if (activity) {
        activity->SetIsActive(false);
        activity->EndEvent(currentFrame);
        DE.AddObject(activity->GetAsJSON());
        delete activity;
        activity = nullptr;
        currentEvent++;
    }
    cout << "=== Finished processing \"" << vidFileName << "\" ===" << endl;

    cap.release();
    destroyAllWindows();

    // Construct the JSON object array of all events detected.
    DE.BuildJSONObjectArray();

    // Create the JSON file for this video.
    ofstream configFile;
    configFile.open(JSON_DIR "DE_" + vidFileName.substr(0, vidFileName.find_last_of(".")) + ".json");
    configFile << DE.GetJSON();
    configFile.close();

    // Cleanup pointers
    if (detectQR) {
        delete detectQR;
        detectQR = nullptr;
    }

    if (activity) {
        delete activity;
        activity = nullptr;
    }
}


Mat ConcatenateMatrices(Mat& left_mat, Mat& right_mat)
{
    int rows = max(left_mat.rows, right_mat.rows);
    int cols = left_mat.cols + right_mat.cols;

    Mat3b conc(rows, cols, Vec3b(0, 0, 0));
    Mat3b res(rows, cols, Vec3b(0, 0, 0));

    left_mat.copyTo(res(Rect(0, 0, left_mat.cols, left_mat.rows)));
    right_mat.copyTo(res(Rect(left_mat.cols, 0, right_mat.cols, right_mat.rows)));
    
    return std::move(res);
}

void CreateConcatVideo(string& file1, string& file2)
{
    VideoCapture cap1(file1), cap2(file2);
    if (!cap1.isOpened() && !cap2.isOpened()) {
        cerr << "*** Could not open video file(s)" << endl;
        return;
    }

    // Clean up the video file name.
    {
        // Now that we've opened the file, scrub it clean of directory names to get the tag name.
        file1 = file1.substr(file1.find_last_of("/") + 1, file1.length());
        file1 = file1.substr(0, file1.find_last_of("_"));
        file2 = file2.substr(file2.find_last_of("/") + 1, file2.length());
        file2 = file2.substr(0, file2.find_last_of("_"));
    }

    if (file1 == file2) {
        /*   
            VideoWriter writer("./static/videos/" + file1 + ".mp4",
            int(cap1.get(CAP_PROP_FOURCC)),
            cap1.get(CAP_PROP_FPS),
            Size(cap1.get(CAP_PROP_FRAME_WIDTH) + cap2.get(CAP_PROP_FRAME_WIDTH),
                max(cap1.get(CAP_PROP_FRAME_HEIGHT), cap2.get(CAP_PROP_FRAME_HEIGHT))),
            true);
        */
        auto totalFrames = max(cap1.get(cv::CAP_PROP_FRAME_COUNT), cap2.get(cv::CAP_PROP_FRAME_COUNT));
        int currentFrame = 0;
        while (true) 
        {
            Mat frame1, frame2;
            cap1.read(frame1);
            cap2.read(frame2);

            if (frame1.empty() || frame2.empty())
                continue;

            currentFrame++;
            if (currentFrame >= totalFrames)
                break;

            

            // Disparity
            {
                Mat K1, K2, D1, D2;
                FileStorage fs("config/StereoCalibration.yaml", FileStorage::READ);
                fs["K1"] >> K1;
                fs["D1"] >> D1;
                fs["K2"] >> K2;
                fs["D2"] >> D2;

                Mat uframe1, uframe2;
                undistort(frame1, uframe1, K1, D1);
                undistort(frame2, uframe2, K2, D2);
                frame1 = uframe1;
                frame2 = uframe2;

                Ptr<cv::StereoBM> SM = StereoBM::create(160, 5);
                Mat disparity;

                Mat grey1, grey2;
                cvtColor(uframe1, grey1, COLOR_BGR2GRAY);
                cvtColor(uframe2, grey2, COLOR_BGR2GRAY);
                SM->compute(grey1, grey2, disparity);

                Mat D;
                float b = 65.f; // mm
                float f = 17.f; // mm

                D = (b*f)/disparity;
                imshow("Disparity DIstance", D);
            }

            Mat res = ConcatenateMatrices(frame1, frame2);
            imshow("Concatented", res);
            
            //writer << res;

            char c = (char)waitKey(25);
            if (c == 27)
                break;
        }
        cout << "Done" << endl;
        cap1.release();
        cap2.release();
        destroyAllWindows();
    }
}

void TriangulateSelectedPoints()
{
    // Get keypoints saved from browser.
    {
        Calibration::Input input;
        vector<vector<Point2f> > keypoints_l, keypoints_r;
        {
            FileStorage fs("config/measure_points_left.yaml", FileStorage::READ);
            ReadVectorOfVector(fs, "keypoints", keypoints_l);
            input.image_points[0] = keypoints_l;
        }
        {
            FileStorage fs("config/measure_points_right.yaml", FileStorage::READ);
            ReadVectorOfVector(fs, "keypoints", keypoints_r);
            input.image_points[1] = keypoints_r;
        }

        Calibration* calib = new Calibration(input, CalibrationType::STEREO, "StereoCalibration.yaml");
        calib->ReadCalibration();
    }
}

void ReadVectorOfVector(FileStorage &fns, string name, vector<vector<Point2f>> &data)
{
     data.clear();
     FileNode fn = fns[name];
     if (fn.empty()){
	  return;
     }

     FileNodeIterator current = fn.begin(), it_end = fn.end();
     for (; current != it_end; ++current)
     {
	  vector<Point2f> tmp;
	  FileNode item = *current;
	  item >> tmp;
	  data.push_back(tmp);
     }
}