#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <map>

class Tracker
{
public:
    struct Settings
    {
        // Contour Settings
        bool bDrawContours = false;
        
        // Threshold Settings
        int MaxThreshold = 255;
        int MinThreshold = 250;
    };

public:
    Tracker(Settings);

    void CreateMask(cv::Mat&);
    void GetObjectContours(cv::Mat&);
    void CheckForActivity(int&);
    void GetCascades();

public:
    Settings Config;
    std::map<int, std::pair<int, int>> ActivityRange;

private:
    cv::Mat _mask;
    cv::Ptr<cv::BackgroundSubtractor> bkgd_sub_ptr;
    std::map<int, cv::Ptr<cv::CascadeClassifier>> cascades;
    std::vector<std::vector<cv::Point>> contours;
    bool bIsActive;
};