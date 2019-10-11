#include <algorithm>
#include <csignal>
#include <dirent.h>
#include <iostream>
#include <string.h>
#include <thread>

#include <Processor.h>
#include <Camera/Camera.h>
#include <Camera/CalibratorFactory.h>

#include "Calibration.h"

using namespace std;

// Directory to save JSON config video_files to.
#define JSON_DIR "static/video-info/"
#define VIDEO_DIR "static/videos/"
#define CALIB_DIR "static/calibrate/"
#define PATH(n, ...) std::string(n) + std::string(__VA_ARGS__)

//#define THREADED

void HandleSignal(int);
std::vector<std::string> GetFilesFromDir(std::string, std::vector<std::string>);

int main(int argc, char** argv)
{
    signal(SIGABRT, HandleSignal);
    signal(SIGINT, HandleSignal);

    auto p_ptr = std::make_unique<Processor>();
    
    // FIXME: This is very hacky, and should not stay. 
    if (argv[1] != NULL) 
    {
        if (std::string(argv[1]) == "TRIANGULATE")
            try 
            {
                Processor p;
                p.TriangulatePoints("calib_config/measure_points.yaml", "stereo_calibration.yaml");
            } 
            catch (const std::exception& e) 
            {
                std::cerr << e.what() << '\n';
            }
        else if (std::string(argv[1]) == "CALIBRATE" )
            try
            {
            #ifdef CALIB
                auto left_cam   = std::make_shared<Camera>(PATH(CALIB_DIR, argv[2]));
                auto right_cam  = std::make_shared<Camera>(PATH(CALIB_DIR, argv[3]));
                auto calib      = CalibratorFactory::MakeCalibrator(left_cam, right_cam);
                calib->RunCalibration();
            #else
                Calibration::Input input;
                input.image_size = cv::Size(1920, 1440);

                Calibration calib(input, CalibrationType::STEREO, "stereo_calibration.yaml");
                
                calib.ReadImages(argv[2], argv[3]);
                calib.RunCalibration();
            #endif
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }

        return 0;
    }

    bool bHasVideos = false;
    std::vector<std::string> video_files;
    auto add = [&p_ptr, &video_files, &bHasVideos](){
        while(true)
        {
            video_files = GetFilesFromDir(VIDEO_DIR, { ".mp4", ".MP4" });
            std::sort(video_files.begin(), video_files.end());

            bHasVideos = !video_files.empty();
            if(!bHasVideos) return;

            if(video_files.size() % 2 == 0)
                for (size_t i = 0; i < video_files.size(); i += 2)
                    if(video_files[i] != "" && video_files[(i + 1)] != "")
                        p_ptr->AddVideos(video_files[i], video_files[(i + 1)]);
        }   
    };

    auto run = [&p_ptr](){
        try
        {
            p_ptr->RunProcesses();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    };
    
    do 
    {
        if(argv[1] == NULL)
        {
            std::thread t1(add), t2(run);
            t1.join(); t2.join();
        }
    } while (bHasVideos);

    return 0;
}

void HandleSignal(int signal)
{
    std::cout << "\r=== Got signal: " << signal << " ===" << endl;
    std::cout << "  > Terminating..." << endl;
    exit(0);
}

std::vector<std::string> GetFilesFromDir(std::string dir, std::vector<std::string> filters)
{
    std::vector<std::string> files;
    DIR* dp;
    struct dirent* d;
    if ((dp = opendir(dir.c_str())) != NULL)
    {
        while ((d = readdir(dp)) != NULL)
            for (auto filter : filters)
                if ((strcmp(string(d->d_name).c_str(), ".") != false &&
                     strcmp(string(d->d_name).c_str(), "..") != false) &&
                     string(d->d_name).find(filter) != string::npos)
                    files.push_back(dir + d->d_name);
        closedir(dp);
    }
    return files;
}