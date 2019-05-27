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
    EventBuilder() : json_object{JSON("")} {}
    EventBuilder(cv::Mat& frame);
    virtual ~EventBuilder() {}

    virtual void StartEvent(int&);
    virtual void EndEvent(int&);

    const JSON GetAsJSON();


 protected:
    cv::Mat frame_;
    int start_frame, end_frame;
    JSON json_object;

};

class QREvent : public EventBuilder
{
 public:
    QREvent() : EventBuilder() {}
    QREvent(cv::Mat& frame) : EventBuilder(frame) {}
    virtual ~QREvent() {}

    virtual void StartEvent(int&);

    const bool DetectedQR();
 
 private:
    std::map<std::string, std::string> GetGeoURIValues(std::string& uri);

};

class ActivityEvent : public EventBuilder
{
 public:
    ActivityEvent(cv::Mat& frame, int id) : EventBuilder(frame), id_{id}, active_{true} {}
    virtual ~ActivityEvent() {}

    virtual void StartEvent(int&);
    virtual void EndEvent(int&);

    void SetIsActive(bool);
    bool IsActive();

 private:
    int id_;
    bool  active_;
};