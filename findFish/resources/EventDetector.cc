#include "includes/EventDetector.h"
#include "includes/JsonBuilder.h"

#include <vector>
#include <regex>

using namespace cv;

std::vector<std::string> GetSplitString(std::string& str, const char* delimiter);

EventBuilder::EventBuilder(Mat& frame)
    : frame_{ frame }, start_frame{ -1 }, end_frame{ -1 }, json_object{JSON("")}
{

}

void EventBuilder::StartEvent(int& currFrame)
{
    start_frame = currFrame;
    // Add code to begin the event.
}

void EventBuilder::EndEvent(int& currFrame)
{
    end_frame = currFrame;
    // Add code to end the event.
}

const JSON EventBuilder::GetAsJSON()
{
    return json_object;
}


void QREvent::StartEvent(int& currFrame)
{
    QRCodeDetector qrDetector;
    Mat boundBox;
    std::string url = qrDetector.detectAndDecode(frame_, boundBox);

    if (url.length() > 0 && (start_frame == -1 && end_frame == -1)) 
    {
        start_frame = currFrame;

        std::map<std::string, std::string> info = GetGeoURIValues(url);
        info.insert(std::make_pair("frame", std::to_string(start_frame)));
        json_object = JSON("Event_QRCode", info);

        EndEvent(currFrame);
    }
}

const bool QREvent::DetectedQR()
{
    return (start_frame != -1 && end_frame != -1);
}

std::map<std::string, std::string> QREvent::GetGeoURIValues(std::string& uri)
{
    std::map<std::string, std::string> json;
    
    auto strings = GetSplitString(uri, ";");
    for(auto str : strings)
    {
        if(str.find("geo:") != std::string::npos)
        {
            str = str.substr(str.find("geo:")+4, str.length());
            auto values = GetSplitString(str, ",");
            
            json.insert(std::make_pair("lat", values[0]));
            json.insert(std::make_pair("long", values[1]));
        }
        auto values = GetSplitString(str, "=");
        for(int i = 0; i < values.size()-1; i+=2)
            json.insert(std::make_pair(values[i], values[i+1]));
    }
    
    return json;
}

std::vector<std::string> GetSplitString(std::string& str, const char* delimiter)
{
    std::vector<std::string> result;
    size_t i = 0;
    std::string temp = str;

    std::regex r("\\s+");
    while ((i = temp.find(delimiter)) != std::string::npos)
    {
        if (i > str.length())
            i = str.length() - 1;
            result.push_back(regex_replace(temp.substr(0, i), r, ""));
            temp.erase(0, i + 1);
    }
    result.push_back(regex_replace(temp.substr(0, i), r, ""));

    return result;
}


void ActivityEvent::StartEvent(int& currFrame)
{
    if(start_frame == -1) start_frame = currFrame;
}

void ActivityEvent::EndEvent(int& currFrame)
{
    if(start_frame != -1 && end_frame == -1 && !IsActive()) end_frame = currFrame;
    if(start_frame != -1 && end_frame != -1)
    {
        std::map<std::string, std::string> info;
        info.insert(std::make_pair("frame_start", std::to_string(start_frame)));
        info.insert(std::make_pair("frame_end", std::to_string(start_frame)));
        json_object = JSON("Event_Activity_"+std::to_string(id_), info);
    }
}

void ActivityEvent::SetIsActive(bool active)
{
    active_ = active;
}

bool ActivityEvent::IsActive()
{
    return active_;
}
