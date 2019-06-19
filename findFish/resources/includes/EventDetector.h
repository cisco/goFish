#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgcodecs.hpp>

#include <map>
#include <string>

#include "JsonBuilder.h"

class EventBuilder
{
 public:
    EventBuilder() : start_frame{ -1 }, end_frame{ -1 }, json_object{JSON("")} {}
    EventBuilder(cv::Mat& frame);
    virtual ~EventBuilder() {}

    virtual void StartEvent(int&)  = 0;
    virtual void EndEvent(int&) = 0;

    const JSON GetAsJSON();

 protected:
    cv::Mat frame_;
    int start_frame, end_frame;
    JSON json_object;

};

class QREvent : public EventBuilder
{
 public:
    QREvent(cv::Mat& frame);
    virtual ~QREvent() {}

    virtual void StartEvent(int&) override;
    virtual void EndEvent(int&) override;

    const bool DetectedQR();
 
 private:
    std::map<std::string, std::string> GetGeoURIValues(std::string& uri);

};

class ActivityEvent : public EventBuilder
{
 public:
    ActivityEvent(int id, int&, int&);
    virtual ~ActivityEvent() {}

    virtual void StartEvent(int&) override;
    virtual void EndEvent(int&) override;

    void SetIsActive(bool);
    bool IsActive();

 private:
    int id_;
    bool active_;
};