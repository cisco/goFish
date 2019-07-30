#include "includes/Processor.h"
#include "includes/JsonBuilder.h"
#include "includes/EventDetector.h"
#include "includes/Calibration.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <time.h>

///////////////////////////////////////////////////////////////////////////////
// Forward Declarations
///////////////////////////////////////////////////////////////////////////////
cv::Mat ConcatenateMatrices(cv::Mat&, cv::Mat&);
void ReadVectorOfVector(cv::FileStorage&, std::string, std::vector<std::vector<cv::Point2f>>&);

Processor::Processor()
    : frame_num_{0}, total_frames_{-1}
{

}

Processor::~Processor()
{

}

void Processor::ProcessVideos(std::string left_file, std::string right_file)
{
    clock_t time_start = clock();
    if(left_file != right_file)
    {
        cv::VideoCapture left_cap(left_file), right_cap(right_file);
        if (!left_cap.isOpened() && !right_cap.isOpened())
        {
            std::cerr << "*** Could not open video file(s)" << std::endl;
            return;
        }

        // Clean up the video filenames.
        {
            // Now that we've opened the file, scrub it clean of directory names to get the tag name.
            left_file = left_file.substr(left_file.find_last_of("/") + 1, left_file.length());
            left_file = left_file.substr(0, left_file.find_last_of("_"));
            right_file = right_file.substr(right_file.find_last_of("/") + 1, right_file.length());
            right_file = right_file.substr(0, right_file.find_last_of("_"));
        }

        std::string file_name = "";
        if (left_file == right_file) 
        {
            // Create a save location for the new combined video.
            file_name = "./static/proc_videos/" + left_file + ".mp4";
            std::cout << "=== Creating \"" << file_name << "\" ===" << std::endl;

            // Create a writer for the new combined video.
            cv::VideoWriter writer(
                file_name,
                int(left_cap.get(cv::CAP_PROP_FOURCC)),
                left_cap.get(cv::CAP_PROP_FPS),
                cv::Size(
                    left_cap.get(cv::CAP_PROP_FRAME_WIDTH) + right_cap.get(cv::CAP_PROP_FRAME_WIDTH),
                    std::max(left_cap.get(cv::CAP_PROP_FRAME_HEIGHT),
                             right_cap.get(cv::CAP_PROP_FRAME_HEIGHT))),
                true);

            // Create the JSON object which will hold all of the detected events.
            auto DE = std::make_unique<JSON>("DetectedEvents");

            // Setup QR Code detection events for the left and right videos.
            auto detect_QR_left = std::make_unique<QREvent>();
            auto detect_QR_right = std::make_unique<QREvent>();

            // Setup the tracker.
            Tracker::Settings t_conf;
            t_conf.bDrawContours = false;
            t_conf.MinThreshold = 200;
            
            auto tracker = std::make_unique<Tracker>(t_conf);

            total_frames_ = std::min(left_cap.get(cv::CAP_PROP_FRAME_COUNT), right_cap.get(cv::CAP_PROP_FRAME_COUNT));
            int frame_offset = -1;
            while (true) 
            {
                cv::Mat left_frame, right_frame;

                // Read the frames only as long as either no QR code is detected, or both are detected. Keeps reading the one that isn't detected until it is.
                if((!detect_QR_left->DetectedQR()) || (detect_QR_left->DetectedQR() && detect_QR_right->DetectedQR()) || left_frame.empty())
                    left_cap.read(left_frame);
                if((!detect_QR_right->DetectedQR()) || (detect_QR_left->DetectedQR() && detect_QR_right->DetectedQR()) || right_frame.empty())
                    right_cap.read(right_frame);

                if ((left_frame.empty() || right_frame.empty()) && (frame_num_ < total_frames_ - 1))
                    continue;

                frame_num_++;
                if (frame_num_ >= total_frames_)
                    break;

                // Detect QR codes in each video.
                if(!detect_QR_left->DetectedQR()) detect_QR_left->CheckFrame(left_frame, frame_num_);
                if(!detect_QR_right->DetectedQR()) detect_QR_right->CheckFrame(right_frame, frame_num_);
                
                // Undistort the frames using camera calibration data.
                UndistortImage(left_frame, 0); // Index 0 means get Left camera calibration data.
                UndistortImage(right_frame, 1);// Index 1 means get Right camera calibration data.

                // Only start writing if both videos are synced up.
                if(detect_QR_left->DetectedQR() && detect_QR_right->DetectedQR())
                {
                    //std::cout << "Writing frame " << frame_num_ << " of " << total_frames_ << "\n";

                    // Account for frame offset from the last detected QR code frame.
                    if(frame_offset == -1) frame_offset = frame_num_;
                    int adj_frame = (frame_num_ - frame_offset);                  

                    // Run the tracker on the undistorted frames.
                    tracker->CreateMask(left_frame);
                    tracker->CheckForActivity(adj_frame);
                    tracker->CreateMask(right_frame);
                    tracker->CheckForActivity(adj_frame);

                    // Write the concatenated undistorted frames.
                    cv::Mat res = ConcatenateMatrices(left_frame, right_frame);
                    writer << res;
                }
            }

            std::cout << "=== Finished Concatenating ===\n";
            std::cout << "=== Time taken: " << (double)(clock() - time_start)/CLOCKS_PER_SEC << " seconds ===\n";

            // Release all video files and close any open windows.
            left_cap.release();
            right_cap.release();
            cv::destroyAllWindows();

            AssembleEvents(tracker, DE);

            // Construct the JSON object array of all events detected.
            DE->BuildJSONObjectArray();

            // Create the JSON file for this video.
            std::ofstream configFile;
            configFile.open("static/video-info/DE_" + left_file + ".json");
            configFile << DE->GetJSON();
            configFile.close();

            std::cout << "=== Finished Processing for \"" << file_name << "\"===\n";
        }
    }
}

void Processor::UndistortImage(cv::Mat& frame, int index)
{
    Calibration::Input input;
    input.image_size = cv::Size(1920, 1440);

    Calibration calib(input, CalibrationType::STEREO, "StereoCalibration.yaml");
    calib.ReadCalibration();

    calib.UndistortImage(frame, index);
}

void Processor::AssembleEvents(std::unique_ptr<class Tracker>& tracker, std::unique_ptr<class JSON>& json_ptr)
{
    for(auto event : tracker->ActivityRange)
    {
        if(event->IsActive())
            event->EndEvent(frame_num_);
        json_ptr->AddObject(event->GetAsJSON());
    }
}

void Processor::TriangulatePoints(std::string points_file, std::string calib_file)
{
    Calibration::Input input;
    std::vector<std::vector<cv::Point2f> > keypoints_l, keypoints_r;
    {
        cv::FileStorage fs(points_file, cv::FileStorage::READ);
        ReadVectorOfVector(fs, "keypoints_left", keypoints_l);
        ReadVectorOfVector(fs, "keypoints_right", keypoints_r);

        input.image_points[0] = keypoints_l;
        input.image_points[1] = keypoints_r;
    }
    
    if(input.image_points[0].size() ==  input.image_points[0].size())
    {
        auto calib = new Calibration(input, CalibrationType::STEREO, calib_file);
        calib->ReadCalibration();
        calib->TriangulatePoints();
        delete calib;
    }  
}


///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

cv::Mat ConcatenateMatrices(cv::Mat& left_mat, cv::Mat& right_mat)
{
    int rows = std::max(left_mat.rows, right_mat.rows);
    int cols = left_mat.cols + right_mat.cols;

    cv::Mat3b conc(rows, cols, cv::Vec3b(0, 0, 0));
    cv::Mat3b res(rows, cols, cv::Vec3b(0, 0, 0));

    left_mat.copyTo(res(cv::Rect(0, 0, left_mat.cols, left_mat.rows)));
    right_mat.copyTo(res(cv::Rect(left_mat.cols, 0, right_mat.cols, right_mat.rows)));

    return std::move(res);
}

void ReadVectorOfVector(cv::FileStorage& fs, std::string name, std::vector<std::vector<cv::Point2f>>& data)
{
    data.clear();
    cv::FileNode fn = fs[name];
    if (fn.empty())
        return;

    for (auto node : fn) 
    {
        std::vector<cv::Point2f> temp_vec;
        node >> temp_vec;
        data.push_back(temp_vec);
    }
}
