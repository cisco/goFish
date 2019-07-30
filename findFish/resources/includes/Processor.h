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
   Processor();
   ~Processor();

   /// Takes two videos and goes through each of them, finding activity events
   /// and a sync point, before concatenating them together and writing them as
   /// one video.
   /// \param[in] left_file The video from the left camera.
   /// \param[in] right_file The video from the right camera.
   void ProcessVideos(std::string left_file, std::string right_file);

   /// Reads stereo points from a file and triangulates a real world coordinate
   /// using stereo calibration data.
   /// \param[in] points_file The file which contains the left and right points.
   /// \param[in] calib_file The file which contains the stereo calibration data.
   void TriangulatePoints(std::string points_file, std::string calib_file);

 private:
   /// Undistorts the given frame using calibration data for camera at index.
   void UndistortImage(cv::Mat&, int);
   
   /// Adds all activity events from the tracker into an array.
   void AssembleEvents(std::unique_ptr<class Tracker>&, std::unique_ptr<class JSON>&);

 private:
    int frame_num_;
    int total_frames_;

};