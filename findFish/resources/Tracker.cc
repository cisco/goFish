#include "includes/Tracker.h"

#include <opencv2/imgcodecs.hpp>

#include <vector>

Tracker::Tracker(Tracker::Settings s)
{
    Config = s;
    bkgd_sub_ptr = cv::createBackgroundSubtractorKNN();
    bIsActive = false;
    GetCascades();
}

void Tracker::CreateMask(cv::Mat& frame)
{
    // Background subtraction method.
    {
        bkgd_sub_ptr->apply(frame, _mask);

        int sigmaX = 10, sigmaY = 10, ksize = 9;
        
        cv::Mat kernel = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * sigmaX + 1, 2 * sigmaY + 1), cv::Point(sigmaX, sigmaY));

        cv::GaussianBlur(_mask, _mask, cv::Size(ksize, ksize), sigmaX, sigmaY);
        cv::morphologyEx(_mask, _mask, cv::MORPH_CLOSE, cv::getGaussianKernel(ksize, sigmaX));
        
        cv::dilate(_mask, _mask, kernel, cv::Point(sigmaX, sigmaY));
        cv::erode(_mask, _mask, kernel, cv::Point(sigmaX, sigmaY));

        cv::threshold(_mask, _mask, Config.MinThreshold, Config.MaxThreshold, cv::THRESH_BINARY);

        //cv::imshow("mask", _mask);
    }

    // Haar Cascade method.
    for(auto cascade : cascades)
    {
        std::vector<cv::Rect> objects;
        cascade.second->detectMultiScale(frame, objects);
    }
    
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
    cv::findContours(canny_output, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

    std::vector<std::vector<cv::Point2f>> prec_conts(contours.size());

    if(Config.bDrawContours)
    {
        cv::RNG rng(12345);
        cv::Mat drawing = cv::Mat::zeros(canny_output.size(), CV_8UC3);
        for (size_t i = 0; i < contours.size(); i++)
        {
            cv::approxPolyDP(contours[i], prec_conts[i], 3, true);
            cv::Scalar colour = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
            //cv::drawContours(frame, contours, i, colour, -1, 8, hierarchy, 0, cv::Point());

            cv::Rect bound_rect = cv::boundingRect(prec_conts[i]);
            cv::rectangle(frame, bound_rect.tl(), bound_rect.br(), colour, 2);
        }
    }
}

void Tracker::CheckForActivity(int& CurrentFrame)
{
    if (!contours.empty())
    {
        if(!bIsActive)
        {
            bIsActive = true;
            std::cout << "*** Activity Event : " << ActivityRange.size() << std::endl;
            ActivityRange.insert(std::make_pair(ActivityRange.size()+1, std::make_pair(CurrentFrame, -1)));
        }
    }
    else 
    {
        // FIXME: Event ends are not being detected after the first ending. This needs to be fixed.
        if(ActivityRange[ActivityRange.size()].first != -1 && ActivityRange[ActivityRange.size()].second == -1 && bIsActive)
        {
            bIsActive = false;
            ActivityRange[ActivityRange.size()].second = CurrentFrame;
            std::cout << "*** End Activity!\n";
        }
    }
}

void Tracker::GetCascades()
{
    std::vector<std::string> names;
    for(auto name : names)
    {
        // Check if <name>.yaml exists.
        if(false/* Add file exist check here */) cascades.insert(std::make_pair(0, new cv::CascadeClassifier(name + ".yaml")));
    }
}