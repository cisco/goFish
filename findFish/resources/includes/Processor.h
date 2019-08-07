/// \author Tomas Rigaux
/// \date July 24, 2019
///
/// Handles the processing of stereo videos, analyzing the video frames for
/// detected motion, QR codes, etc.. Once a sync point for each video is 
/// found, the two videos are concatenated together, and written as one video.

#pragma once

#include <string>
#include <vector>
#include <mutex>

namespace cv {
  class Mat;
  class VideoCapture;
}
class Tracker;
class JSON;
class Video;
class Calibration;

/// \brief Goes through two videos to find events and concatenate them together.
///
/// Goes through stereo videos and finds events, and then writes a
/// concatenated video, as well as a file containing all detected
/// events from the video.
class Processor
{
public:
  Processor();
  Processor(std::string, std::string);
  ~Processor();

  /// Takes two videos and goes through each of them, finding activity events
  /// and a sync point, before concatenating them together and writing them as
  /// one video.
  void ProcessVideos();

  /// Reads stereo points from a file and triangulates a real world coordinate
  /// using stereo calibration data.
  /// \param[in] points_file The file which contains the left and right points.
  /// \param[in] calib_file The file which contains the stereo calibration data.
  void TriangulatePoints(std::string points_file, std::string calib_file);

private:
  /// Undistorts the given frame using calibration data for camera at index.
  /// \param[in, out] frame The frame to undistort.
  /// \param[in] index The camera index to get calibration from.
  void UndistortImage(cv::Mat&, int) const;

  /// Adds all activity events from the tracker into an array.
  /// \param[in, out] last_frame The last frame before quitting.
  void AssembleEvents(int&) const;

  /// Goes through each video and looks for a sync point.
  /// \returns True is both videos found a sync point point. False otherwise.
  bool SyncVideos() const;

public:
  bool Success;

private:
  std::unique_ptr<Video>        _videos[2];
  std::unique_ptr<Tracker>      _tracker;
  std::shared_ptr<JSON>         _detected_events;
  std::shared_ptr<Calibration>  _calib;

};

class Video
{
public:
  /// Constructs a video from a given file.
  /// \param[in] file THe files to read from.
  Video(std::string);

  /// Default destructor.
  ~Video();

  /// Reads in the next frame from the video. If the frame is empty, it recurses
  /// until it reads a non-empty frame.
  void Read();

  /// Returns a pointer to the current frame. If the frame is null, then the 
  /// video should be done.
  /// \returns Pointer to the current frame read from the video.
  std::shared_ptr<cv::Mat> Get() const;

  /// Checks whether the video has ended or not.
  /// \returns True if the video frames are equal to the total frames, false
  /// otherwise.
  bool Ended() const;

public:
  std::string FileName;
  int Frame;
  int TotalFrames;
  int Width;
  int Height;
  int FPS;
  int FOURCC;

private:
  std::string _filepath;
  std::shared_ptr<cv::Mat> _frame;
  std::unique_ptr<cv::VideoCapture> _vid_cap;
  mutable std::mutex _mutex;
};