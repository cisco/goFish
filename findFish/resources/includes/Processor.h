/// \author Tomas Rigaux
/// \date July 24, 2019
///
/// Handles the processing of stereo videos, analyzing the video frames for
/// detected motion, QR codes, etc.. Once a sync point for each video is 
/// found, the two videos are concatenated together, and written as one video.

#pragma once

#include <string>
#include <vector>
#include "Tracker.h"

/// \class Processor
/// \brief Goes through stereo videos and finds events, and then writes a
///        concatenated video, as well as a file containg all events.
class Processor
{
 public:
    Processor(std::string, std::string);
    ~Processor();

    void ProcessVideos(std::string left_file, std::string right_file);

 private:
    /// Undistorts the given frame using calibration data for camera at index.
    void UndistortImage(cv::Mat&, int);
    
    /// Adds all activity events from the tracker into an array.
    void AssembleEvents(std::unique_ptr<class Tracker>&, std::unique_ptr<class JSON>&);

 private:
    int frame_num_;
    int total_frames_;

};