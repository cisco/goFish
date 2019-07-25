#include <iostream>
#include <fstream>
#include <dirent.h>
#include <thread>
#include <vector>
#include <csignal>
#include <stdio.h>
#include <algorithm>
#include <time.h>

#include "resources/includes/Calibration.h"
#include "resources/includes/Processor.h"

using namespace std;
using namespace cv;

// Directory to save JSON config video_files to.
#define JSON_DIR "static/video-info/"
#define VIDEO_DIR "static/videos/"

//#define THREADED

void HandleSignal(int);
std::vector<std::string> GetVideosFromDir(std::string, std::vector<std::string>);
cv::Mat ConcatenateMatrices(cv::Mat&, cv::Mat&);
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

        if(!bHasVideos) break;

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

        // FIXME: Doesn't need to be unique.
        auto p = std::make_unique<Processor>("", "");

#ifdef THREADED
        std::vector<std::thread> threads;
        threads.resize(video_files.size());
        for (size_t i = 0; i < video_files.size() / 2; i++)
        {
            std::cout << "!!! Creating Thread: " << i << " !!!\n";
            threads[i] = thread(ProcessVideos, video_files[i], video_files[(i + 1) % video_files.size()]);
            threads[i].join();
        }

#else
        if(argv[1] == NULL)
            for (size_t i = 0; i < video_files.size() / 2; i += 2)
                p->ProcessVideos(video_files[i], video_files[(i + 1) % video_files.size()]);
#endif
    //    if(video_files.size() > 1)
     //       for(std::string video  : video_files)
      //          std::remove(video.c_str());
            
    } while (bHasVideos);

    if(argv[1] != NULL)
    {
        Calibration *calib = nullptr;
        TriangulateSelectedPoints(calib);
    }

    return 0;
}

void HandleSignal(int signal)
{
    std::cout << "\r=== Got signal: " << signal << " ===" << endl;
    std::cout << "  > Terminating..." << endl;
    exit(0);
}

std::vector<std::string> GetVideosFromDir(std::string dir, std::vector<std::string> filters)
{
    std::vector<std::string> video_files;
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

void TriangulateSelectedPoints(Calibration*& calib)
{
    // Get keypoints saved from browser.
    {
        Calibration::Input input;
        vector<vector<Point2f> > keypoints_l, keypoints_r;
        {
            FileStorage fs("calib_config/measure_points.yaml", FileStorage::READ);
            ReadVectorOfVector(fs, "keypoints_left", keypoints_l);
            ReadVectorOfVector(fs, "keypoints_right", keypoints_r);

            input.image_points[0] = keypoints_l;
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