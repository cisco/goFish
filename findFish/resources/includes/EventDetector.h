/// \author Tomas Rigaux
/// \date June 18, 2019
///
/// All kinds of events are defined here, as they are small enough that they
/// can easily fit in here while not big enougbto justify having their own
/// respsective files. These events all assume that they happen within a range
/// of time in a video (as opposed to one-shot event), and thus exist between
/// a range of frames defined in a video.

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgcodecs.hpp>

#include <map>
#include <string>

#include "JsonBuilder.h"

/// \class EventBuilder
/// \brief Abstract Base class for defining an event.
class EventBuilder
{
 public:
   /// Default constructor.
   EventBuilder() : start_frame{ -1 }, end_frame{ -1 }, json_object{JSON("")} {}
   
   /// Constructs an event for a specific frame.
   EventBuilder(cv::Mat& frame);

   /// Default destructor.
   virtual ~EventBuilder() {}

   /// Defines the starting frame of the event.
   /// \param[in, out] frame The frame number that marks the start of the event.
   virtual void StartEvent(int& frame)  = 0;

   /// Defines the ending frame of the event.
   /// \param[in, out] frame The frame number that marks the end of the event.
   virtual void EndEvent(int& frame) = 0;

   /// Returns the event as a JSON object.
   /// \return The event formatted into a JSON object.
   const JSON GetAsJSON();

 protected:
   cv::Mat frame_;
   int start_frame, end_frame;
   JSON json_object;
};

/// \class QREvent
/// \brief Defines an event which attempts to detect a QR code from a frame.
class QREvent : public EventBuilder
{
 public:
   /// Constructs the event around a specific frame.
   /// \param[in, out] frame The frame in which to check for a QR code.
   QREvent(cv::Mat& frame);

   /// Default destructor.
   virtual ~QREvent() {}

   /// Denotes the start of the event, and begins checking for a QR code.
   /// param[in, out] frame The starting frame of the event.
   virtual void StartEvent(int&) override;
   
   /// Denotes the end of the event, and stops checking for a QR code.
   /// param[in, out] frame The ending frame of the event.
   virtual void EndEvent(int&) override;

   /// Returns whether or not a QR code was found.
   /// \return If the QR code was detected or not.
   const bool DetectedQR();
 
 private:
   /// Parses the QR code URL for a Geo URI.
   /// \return All the key-value pairs found in the URL.
   std::map<std::string, std::string> GetGeoURIValues(std::string& uri);

};

/// \class ActivityEvent
/// \brief Defines an event in which there was activity of some sort.
class ActivityEvent : public EventBuilder
{
 public:
   /// Constructs an event with a unique ID, which extends in a range from start
   /// to end.
   /// \param[in] id The unique ID of the event.
   /// \param[in, out] start The starting frame of the event.
   /// \param[in, out] end The ending frame of the event.
   ActivityEvent(int id, int& start, int& end);

   /// Default destructor for the class.
   virtual ~ActivityEvent() {}

   /// Denotes the start of the event
   /// param[in, out] frame The starting frame of the event.
   virtual void StartEvent(int&) override;

   /// Denotes the end of the event
   /// param[in, out] frame The ending frame of the event.
   virtual void EndEvent(int&) override;

   /// Checks whether or not the event is still happening.
   /// \param[in] active The activity of the event.
   void SetIsActive(bool active);

   /// Checks whether or not the event is still happening.
   /// \return The running state of the event.
   bool IsActive();

 private:
   int id_;
   bool active_;
};