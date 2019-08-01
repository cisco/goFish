#include "includes/Processor.h"
#include "includes/JsonBuilder.h"
#include "includes/EventDetector.h"
#include "includes/Calibration.h"
#include "includes/Tracker.h"

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
    : Success{false}, _tracker{nullptr}, _detected_events{nullptr}
{
    for(int i = 0; i < 2; i++)
        videos[i] = nullptr;
        
    Calibration::Input input;
    input.image_size = cv::Size(1920, 1440);

    _calib = new Calibration(input, CalibrationType::STEREO, "StereoCalibration.yaml");
}

Processor::Processor(std::string left_file, std::string right_file)
    : Success{false}, _tracker{nullptr}, _detected_events{nullptr}
{
    for(int i = 0; i < 2; i++)
        videos[i] = nullptr;

    if(left_file != "" && right_file != "")
    {
        if(left_file != right_file)
        {
            videos[0] = new Video(left_file);
            videos[1] = new Video(right_file);
        }

        Tracker::Settings t_conf;
        t_conf.bDrawContours = false;
        t_conf.MinThreshold = 200;
        _tracker = new Tracker(t_conf);

        _detected_events = new JSON("DetectedEvents");
    }

    Calibration::Input input;
    input.image_size = cv::Size(1920, 1440);

    _calib = new Calibration(input, CalibrationType::STEREO, "StereoCalibration.yaml");
}

Processor::~Processor()
{
    for(int i = 0; i < 2; i++)
        delete videos[i];

    delete _tracker;
    delete _detected_events;
}

void Processor::ProcessVideos()
{
    auto time_start = cv::getTickCount();
    std::string file_name = "";
    if (videos[0]->FileName == videos[1]->FileName)
    {
        // Create a save location for the new combined video.
        file_name = "./static/proc_videos/" + videos[0]->FileName + ".mp4";
        std::cout << "=== Creating \"" << file_name << "\" ===" << std::endl;

        // Setup QR Code detection events for the left and right videos.
        if(!SyncVideos()) return;

        // Create a writer for the new combined video.
        cv::VideoWriter writer(
            file_name,
            videos[0]->FOURCC,
            videos[0]->FPS,
            cv::Size(videos[0]->Width + videos[1]->Width,
                     std::max(videos[0]->Height, videos[1]->Height)),
            true);

        int frame_num = 0;
        while (!videos[0]->Ended() && !videos[1]->Ended())
        {
            cv::Mat frames[2];
            for(int i = 0; i < 2; i++)
                videos[i]->Read();
            
            if(videos[0]->Get() && videos[1]->Get())
            {
                for(int i = 0; i < 2; i++)
                {
                    // Undistort the frames using camera calibration data.
                    frames[i] = *videos[i]->Get();
                    UndistortImage(frames[i], 0);

                    // Run the tracker on the undistorted frames.
                    _tracker->CreateMask(frames[i]);
                    _tracker->CheckForActivity(frame_num);
                }

                // Write the concatenated undistorted frames.
                cv::Mat res = ConcatenateMatrices(frames[0], frames[1]);
                writer << res;
                frame_num++;
            }
        }

        cv::destroyAllWindows();
        
        std::cout << "=== Finished Concatenating ===\n";
        std::cout << "=== Time taken: " << (double)(cv::getTickCount() - time_start)/cv::getTickFrequency() << " seconds ===\n";

        AssembleEvents(frame_num);

        // Construct the JSON object array of all events detected.
        _detected_events->BuildJSONObjectArray();

        // Create the JSON file for this video.
        std::ofstream configFile;
        configFile.open("static/video-info/DE_" + videos[0]->FileName + ".json");
        configFile << _detected_events->GetJSON();
        configFile.close();

        std::cout << "=== Finished Processing for \"" << file_name << "\"===\n";
        Success = true;
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
    
    _calib->input = input;
    if(input.image_points[0].size() ==  input.image_points[0].size())
    {
        _calib->ReadCalibration();
        _calib->TriangulatePoints();
    }  
}

void Processor::UndistortImage(cv::Mat& frame, int index) const
{
    _calib->ReadCalibration();
    _calib->UndistortImage(frame, index);
}

void Processor::AssembleEvents(int& last_frame) const
{
    for(auto event : _tracker->ActivityRange)
    {
        if(event->IsActive())
            event->EndEvent(last_frame);
        _detected_events->AddObject(event->GetAsJSON());
    }
}

bool Processor::SyncVideos() const
{
    for(int i = 0 ; i < 2; i++)
    {
        QREvent detect_QR;
        while(true)
        {
            if(!detect_QR.DetectedQR())
            {
                videos[i]->Read();
                if(videos[i]->Get())
                    detect_QR.CheckFrame(*videos[i]->Get(), videos[i]->Frame);
            }
            else break;
        }
        if(!detect_QR.DetectedQR()) return false;
    }
    return true;
}


Video::Video(std::string file)
    : FileName{""}, Frame{0}, TotalFrames{0}, _filepath{file}, _frame{nullptr}, _vid_cap{nullptr}
{
    try {
        FileName = _filepath.substr(_filepath.find_last_of("/") + 1, _filepath.length());
        FileName = FileName.substr(0,FileName.find_last_of("_"));
        
        _vid_cap = new cv::VideoCapture(_filepath);
        if (!_vid_cap->isOpened())
            throw "Video \"" + FileName + "\" could not be opened!";

        TotalFrames = _vid_cap->get(cv::CAP_PROP_FRAME_COUNT);
        Width       = _vid_cap->get(cv::CAP_PROP_FRAME_WIDTH);
        Height      = _vid_cap->get(cv::CAP_PROP_FRAME_HEIGHT);
        FPS         = _vid_cap->get(cv::CAP_PROP_FPS);
        FOURCC      = _vid_cap->get(cv::CAP_PROP_FOURCC);
    } catch(const char* msg) {
        std::cerr << "*** " << msg << std::endl;
    }
}

Video::~Video()
{
    if(_vid_cap) _vid_cap->release();  
    delete _frame;
    delete _vid_cap;
}

void Video::Read()
{
    if(_vid_cap)
    {
        if(Frame <= TotalFrames)
        {
            cv::Mat frame;
            if (_vid_cap->isOpened()) 
                _vid_cap->read(frame);
            
            if(frame.empty())
                Read();
            
            _frame = new cv::Mat(frame);
            Frame++;
        }
        else _frame = nullptr;
    }
}

cv::Mat* Video::Get() const
{
    if(_frame != nullptr)
        if(!_frame->empty())
            return _frame;

    return nullptr;
}

bool Video::Ended()
{
    return Frame >= TotalFrames;
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