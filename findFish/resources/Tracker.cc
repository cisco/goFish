#include "includes/Tracker.h"

#include <opencv2/objdetect.hpp>
#include <opencv2/imgcodecs.hpp>

#include <vector>

Tracker::Tracker(Tracker::Settings s)
{
    Config = s;
    bkgd_sub_ptr = cv::createBackgroundSubtractorKNN();
}

void Tracker::CreateMask(cv::Mat& frame)
{
    bkgd_sub_ptr->apply(frame, _mask);

    cv::threshold(_mask, _mask, Config.MinThreshold, Config.MaxThreshold, cv::THRESH_BINARY);

    int morph_elem = 0;
    int morph_size = 7, erode_size = 2;
    cv::Mat kernel  = cv::getStructuringElement(morph_elem, cv::Size(2 * erode_size + 1, 2 * erode_size + 1), cv::Point(erode_size, erode_size));
    cv::Mat element = cv::getStructuringElement(morph_elem, cv::Size(2 * morph_size + 1, 2 * morph_size + 1), cv::Point(morph_size, morph_size));

    cv::dilate(_mask, _mask, kernel, cv::Point(1, 1));
    cv::morphologyEx(_mask, _mask, cv::MORPH_CLOSE, element);
    cv::erode(_mask, _mask, kernel, cv::Point(0, 0));

    cv::medianBlur(_mask, _mask, 3);

    GetObjectContours(frame);
}

void Tracker::GetObjectContours(cv::Mat& frame)
{
    contours.clear();
    int thresh = 8500;

    cv::Mat canny_output;
    std::vector<cv::Vec4i> hierarchy;

    // Detect edges using canny
    cv::Canny(_mask, canny_output, thresh, thresh * 2, 5);
    cv::findContours(canny_output, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_KCOS, cv::Point(0, 0));

    if(Config.bDrawContours)
    {
        cv::RNG rng(12345);
        cv::Mat drawing = cv::Mat::zeros(canny_output.size(), CV_8UC3);
        for (int i = 0; i < contours.size(); i++)
        {
            cv::Scalar colour = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
            cv::drawContours(frame, contours, i, colour, 2, 8, hierarchy, 0, cv::Point());
        }
    }
}

void Tracker::CheckForActivity(int& CurrentFrame)
{
    if (!contours.empty())
    {
        if(ActivityRange.find(ActivityRange.size()) == ActivityRange.end())
            ActivityRange.insert(std::make_pair(ActivityRange.size()+1, std::make_pair(CurrentFrame, -1)));
    }
    else 
    {
        if(ActivityRange.find(ActivityRange.size()) != ActivityRange.end())
        if (ActivityRange.find(ActivityRange.size())->first > -1)
            ActivityRange[ActivityRange.size()].second = CurrentFrame;
    }
}