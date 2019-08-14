#include "includes/EventDetector.h"
#include "includes/JsonBuilder.h"

#include <vector>
#include <regex>

using namespace cv;

///////////////////////////////////////////////////////////////////////////////
// Forward Declarations
std::vector<std::string> SplitString(std::string& str, const char* delimiter);


///////////////////////////////////////////////////////////////////////////////
// Event Builder Base Class
EventBuilder::EventBuilder() 
    : _start_frame{ -1 }, _end_frame{ -1 }
{
    _json_object = std::make_unique<JSON>("");
}

EventBuilder::~EventBuilder() {}

const JSON EventBuilder::GetAsJSON()
{
    return *_json_object.get();
}

std::pair<int, int> EventBuilder::GetRange() const
{
    return std::make_pair(_start_frame, _end_frame);
}

///////////////////////////////////////////////////////////////////////////////
// QR Code Event
QREvent::QREvent() : EventBuilder() {}

void QREvent::CheckFrame(cv::Mat& frame, int& currFrame)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _frame = frame;
    if(!_frame.empty())
    {
        QRCodeDetector qrDetector;
        Mat boundBox;
        std::string url = qrDetector.detectAndDecode(_frame, boundBox);
        if (url.length() > 0 && !DetectedQR()) 
        {
            StartEvent(currFrame);
            auto info = GetGeoURIValues(url);
            info.insert(std::make_pair("frame", std::to_string(_start_frame)));
            _json_object = std::make_unique<JSON>("Event_QRCode", info);

            EndEvent(currFrame);
        }
    }  
}

void QREvent::StartEvent(int& currFrame)
{
    _start_frame = currFrame;
}

void QREvent::EndEvent(int& currFrame)
{
    _end_frame = currFrame;
}

const bool QREvent::DetectedQR() const
{
    return (_start_frame != -1 && _end_frame != -1);
}

std::map<std::string, std::string> QREvent::GetGeoURIValues(std::string& uri) const
{
    std::map<std::string, std::string> json;
    
    auto strings = SplitString(uri, ";");
    for(auto str : strings)
    {
        if(str.find("geo:") != std::string::npos)
        {
            str = str.substr(str.find("geo:")+4, str.length());
            auto values = SplitString(str, ",");
            
            json.insert(std::make_pair("lat", values[0]));
            json.insert(std::make_pair("long", values[1]));
        }
        auto values = SplitString(str, "=");
        for(size_t i = 0; i < values.size()-1; i+=2)
            json.insert(std::make_pair(values[i], values[i+1]));
    }
    
    return json;
}


/////////////////////////////////////////////////////////////////////////////////////
// Activity Event
ActivityEvent::ActivityEvent(int id, int start, int end) : EventBuilder(), id_{id}
{
    StartEvent(start);
    if(end > 0)
        EndEvent(end);
}

void ActivityEvent::CheckFrame(cv::Mat& frame, int& currFrame)
{
    std::lock_guard<std::mutex> lock(_mutex);
}

void ActivityEvent::StartEvent(int& currFrame)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if(_start_frame == -1) _start_frame = currFrame;
}

void ActivityEvent::EndEvent(int& currFrame)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if(IsActive()) _end_frame = currFrame;
    if(_start_frame != -1 && _end_frame != -1)
    {
        std::map<std::string, std::string> info;
        info.insert(std::make_pair("frame_start", std::to_string(_start_frame)));
        info.insert(std::make_pair("frame_end", std::to_string(_end_frame)));
        _json_object = std::make_unique<JSON>("Event_Activity_"+std::to_string(id_), info);
    }
}

bool ActivityEvent::IsActive() const
{
    return (_start_frame != -1 && _end_frame == -1);
}

/////////////////////////////////////////////////////////////////////////////////////
// Helper Functions
std::vector<std::string> SplitString(std::string& str, const char* delimiter)
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
    if(regex_replace(temp.substr(0, i), r, "") != "") 
        result.push_back(regex_replace(temp.substr(0, i), r, ""));

    return result;
}