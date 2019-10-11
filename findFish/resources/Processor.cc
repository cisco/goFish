#include "includes/Processor.h"
#include "includes/JsonBuilder.h"
#include "includes/EventDetector.h"
#include "includes/Calibration.h"
#include "includes/Tracker.h"

#include "Camera/CalibratorFactory.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <time.h>
#include <thread>

cv::Mat ConcatenateMatrices(cv::Mat&, cv::Mat&);
void ReadVectorOfVector(cv::FileStorage&, std::string, std::vector<std::vector<cv::Point2f>>&);

Processor::Processor()
    : Success{false}
{
    Calibration::Input input;
    input.image_size = cv::Size(1920, 1440);
#ifndef CALIB
    _calib = std::make_shared<Calibration>(input, CalibrationType::STEREO, "stereo_calibration.yaml");
    _calib->ReadCalibration();
#endif
}

#ifdef CALIB
Processor::Processor(std::shared_ptr<Camera>c1, std::shared_ptr<Camera> c2)
    : Success{false}
{
    _calib = CalibratorFactory::MakeCalibrator(c1, c2);
    _calib->RunCalibration();
}
#endif

Processor::Processor(std::string left_file, std::string right_file)
    : Success{false}
{
    if(left_file != "" && right_file != "")
    {
        if(left_file != right_file)
        {
            _videos[0] = std::make_unique<Video>(left_file);
            _videos[1] = std::make_unique<Video>(right_file);
        }

        Tracker::Settings t_conf;
        t_conf.bDrawContours = false;
        t_conf.MinThreshold = 200;
        _tracker = std::make_unique<Tracker>(t_conf);

        _detected_events = std::make_shared<JSON>("DetectedEvents");
    }

#ifndef CALIB
    Calibration::Input input;
    input.image_size = cv::Size(1920, 1440);

    _calib = std::make_shared<Calibration>(input, CalibrationType::STEREO, "stereo_calibration.yaml");
    _calib->ReadCalibration();
#else
    auto left_cam = std::make_shared<Camera>(_videos[0]->FileName);
    auto right_cam = std::make_shared<Camera>(_videos[1]->FileName);
    _calib = CalibratorFactory::MakeCalibrator(left_cam, right_cam);
    _calib->RunCalibration();
#endif
}

Processor::~Processor()
{
}

void Processor::ProcessVideos()
{
    try
    {
        if(!_videos[0] || !_videos[1])
            throw std::runtime_error("One or more videos is null");

        auto time_start = cv::getTickCount();
        std::string file_name = "";
        if (_videos[0]->FileName == _videos[1]->FileName &&
            (_videos[0]->FileName != "" || _videos[1]->FileName != ""))
        {
            // Create a save location for the new combined video.
            file_name = "./static/proc_videos/" + _videos[0]->FileName + ".mp4";
            std::cout << "=== Creating \"" << file_name << "\" ===" << std::endl;

            // Setup QR Code detection events for the left and right _videos.
            if (!SyncVideos())
                throw std::runtime_error("Videos did not sync. Either they are "
                                        "missing QR code(s), or none were detected.");

            // Create a writer for the new combined video.
            cv::VideoWriter writer(
                file_name,
                _videos[0]->FOURCC,
                _videos[0]->FPS,
                cv::Size(_videos[0]->Width + _videos[1]->Width,
                        std::max(_videos[0]->Height, _videos[1]->Height)),
                true);

            int frame_num = 0;
            while (!_videos[0]->Ended() && !_videos[1]->Ended())
            {
                std::shared_ptr<cv::Mat> frames[2];
                for(int i = 0; i < 2; i++)
                    _videos[i]->Read();
                
                if(_videos[0]->Get() && _videos[1]->Get())
                {
                    for(int i = 0; i < 2; i++)
                    {
                        // Undistort the frames using camera calibration data.
                        frames[i] = _videos[i]->Get();
                        #ifndef CALIB
                        UndistortImage(*frames[i], 0);
                        #else
                        _calib->UndistortImages(*frames[i].get());
                        #endif

                        // Run the t on the undistorted frames.
                        _tracker->CreateMask(*frames[i]);
                        _tracker->CheckForActivity(frame_num);
                    }

                    // Write the concatenated undistorted frames.
                    cv::Mat res = ConcatenateMatrices(*frames[0], *frames[1]);
                    writer << res;
                    frame_num++;
                }
            }

            cv::destroyAllWindows();
            
            std::cout << "=== Finished Concatenating ===\n";
            std::cout << "=== Time taken: ";
            std::cout << (double)(cv::getTickCount() - time_start)/cv::getTickFrequency() << " seconds ===\n";

            AssembleEvents(frame_num);

            // Construct the JSON object array of all events detected.
            _detected_events->BuildJSONObjectArray();

            // Create the JSON file for this video.
            std::ofstream configFile;
            configFile.open("static/video-info/DE_" + _videos[0]->FileName + ".json");
            configFile << _detected_events->GetJSON();
            configFile.close();

            std::cout << "=== Finished Processing for \"" << file_name << "\" ===\n";
            Success = true;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << " !> " << e.what() << '\n';
    }
}

void Processor::TriangulatePoints(std::string points_file, std::string calib_file)
{
    try
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
#ifndef CALIB
        _calib->_input = input;
        _calib->ReadCalibration();
        _calib->TriangulatePoints();
#endif
    }
    catch(const std::exception& e)
    {
        std::cerr << " !> " << e.what() << '\n';
    }
}

void Processor::AddVideos(std::string v1, std::string v2)
{
    std::shared_ptr<Video> t[2] = { std::make_shared<Video>(v1),
                                    std::make_shared<Video>(v2) };
    Process p;
    for (int i = 0; i < 2; i++)
        p.videos[i] = std::move(t[i]);

    _buffer.push(std::make_shared<Process>(p));
    std::cout << "ADDED " << _buffer.back()->videos[0]->FileName << " TO BUFFER | ["<<_buffer.size()<<"]\n";

}

void Processor::RunProcesses()
{
    std::mutex l_av, l_wv;
    auto av = [this, &l_av](std::shared_ptr<Process> n){
        std::lock_guard<std::mutex> l(l_av);
        std::cout << "Analyzing...\n";
        this->AnalyzeVideos(n);
        std::cout << "Done Analyzing\n";
    };

    auto wv = [this, &l_wv](std::shared_ptr<Process> n){
        std::lock_guard<std::mutex> l(l_wv);
        std::cout << "Writing...\n";
        this->WriteVideo(n);
        std::cout << "Done Writing\n";
    };

    auto benchmark = [](auto& f, auto& n) {
        auto time_start = cv::getTickCount(); 
        f(n);        
        std::cout << "=== Time taken: ";
        std::cout << (double)(cv::getTickCount() - time_start)/cv::getTickFrequency();
        std::cout << " seconds ===\n";
    };

    auto awv = [av, wv, benchmark](auto n){
        benchmark(av, n);
        benchmark(wv, n);
    };

    std::vector<std::thread> threads(_buffer.size());
    for(int i = 0; i < threads.size(); i++)
    {
        threads[i] = std::thread(awv, _buffer.front());
        _buffer.pop();
    }

    for(int i = 0; i < threads.size(); i++)
        threads[i].join();
}

void Processor::AnalyzeVideos(std::shared_ptr<Process>& n)
{
    bool synced = false;
    for(int i = 0 ; i < 2; i++)
    {
        QREvent detect_QR;
        while(true)
        {
            if(!detect_QR.DetectedQR())
            {
                n->videos[i]->Read();
                if(n->videos[i]->Get())
                    detect_QR.CheckFrame(*n->videos[i]->Get(), n->videos[i]->Frame);
            }
            else break;
        }
        if(!detect_QR.DetectedQR()) synced = false;
        else synced = true;
    }

    if (!synced)
        throw std::runtime_error("Videos did not sync. QR code(s) missing or undetected");

    Tracker::Settings t_conf;
    t_conf.bDrawContours = false;
    t_conf.MinThreshold = 200;
    auto t = std::make_unique<Tracker>(t_conf);

    int frame_num = 0;
    while (!n->videos[0]->Ended() && !n->videos[1]->Ended())
    {
        std::shared_ptr<cv::Mat> frames[2];
        for(int i = 0; i < 2; i++)
            n->videos[i]->Read();
        
        if(n->videos[0]->Get() && n->videos[1]->Get())
            for(int i = 0; i < 2; i++)
            {
                frames[i] = n->videos[i]->Get();
                // TODO: Add new calibrator here.
                t->CreateMask(*frames[0]);
                t->CheckForActivity(frame_num);
            }
            // Write the concatenated undistorted frames.
            auto res = ConcatenateMatrices(*frames[0], *frames[1]);
            n->frames.push_back(res);
            frame_num++;
    }

    n->events = std::make_shared<JSON>("DetectedEvents");
    for(auto event : t->ActivityRange)
    {
        if(event->IsActive())
            event->EndEvent(frame_num);
        n->events->AddObject(event->GetAsJSON());
    }
    n->events->BuildJSONObjectArray();
}

void Processor::WriteVideo(std::shared_ptr<Process>& n)
{
    std::string file = "./static/proc_videos/" + n->videos[0]->FileName +  ".mp4";
    cv::VideoWriter writer(file,
                n->videos[0]->FOURCC,
                n->videos[0]->FPS,
                cv::Size(n->videos[0]->Width + n->videos[1]->Width,
                        std::max(n->videos[0]->Height, n->videos[1]->Height)),
                true);

    for(auto frame : n->frames)
        writer << frame;

    std::ofstream configFile;
    configFile.open("static/video-info/DE_" + n->videos[0]->FileName + ".json");
    configFile << n->events->GetJSON();
    configFile.close();
}

void Processor::UndistortImage(cv::Mat& frame, int index) const
{
#ifndef CALIB
    _calib->UndistortImage(frame, index);
#endif
}

void Processor::AssembleEvents(int& last_frame) const
{
    for(auto event : _tracker->ActivityRange)
    {
        if(event->IsActive())
            event->EndEvent(last_frame);
        _detected_events->AddObject(event->GetAsJSON());
    }
    _detected_events->BuildJSONObjectArray();
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
                _videos[i]->Read();
                if(_videos[i]->Get())
                    detect_QR.CheckFrame(*_videos[i]->Get(), _videos[i]->Frame);
            }
            else break;
        }
        if(!detect_QR.DetectedQR()) return false;
    }
    std::cout << " > Synced videos\n";
    return true;
}


Video::Video(std::string file)
    : FileName{""}, Frame{0}, TotalFrames{0}, _filepath{file}
{
    try
    {
        FileName = _filepath.substr(_filepath.find_last_of("/") + 1, _filepath.length());
        FileName = FileName.substr(0,FileName.find_last_of("_"));
        
        _vid_cap = std::make_unique<cv::VideoCapture>(_filepath);
        if (!_vid_cap->isOpened())
            throw std::runtime_error("Video \"" + FileName + "\" could not be opened!");

        TotalFrames = _vid_cap->get(cv::CAP_PROP_FRAME_COUNT);
        Width       = _vid_cap->get(cv::CAP_PROP_FRAME_WIDTH);
        Height      = _vid_cap->get(cv::CAP_PROP_FRAME_HEIGHT);
        FPS         = _vid_cap->get(cv::CAP_PROP_FPS);
        FOURCC      = _vid_cap->get(cv::CAP_PROP_FOURCC);
    } 
    catch(std::exception& e)
    {
        std::cerr << " !> " << e.what() << std::endl;
    }
}

Video::~Video()
{
    if(_vid_cap) _vid_cap->release();
}

void Video::Read()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if(_vid_cap)
    {
        if(Frame <= TotalFrames)
        {
            cv::Mat frame;
            if (_vid_cap->isOpened()) 
                _vid_cap->read(frame);
            
            if(frame.empty())
            {
                _mutex.unlock();
                Read();
            }
            
            _frame = std::make_shared<cv::Mat>(frame);
            Frame++;
        }
        else _frame = nullptr;
    }
}

std::shared_ptr<cv::Mat> Video::Get() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    if(_frame != nullptr)
        if(!_frame->empty())
            return _frame;

    return nullptr;
}

bool Video::Ended() const
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