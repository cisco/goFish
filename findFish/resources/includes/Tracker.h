/// \author Tomas Rigaux
/// \date June 18, 2019
///
/// A tracker for detecting object movement and applying contours around them.
/// Once there is enough training data, then the tracker will also use a vector
/// of Haar Cascade Classifiers for object identification, to attempt to detect
/// multiple different kinds of objects in a frame.

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <map>

/// \class Tracker
/// \brief Uses background subtraction and thresholding to detect motion in an
///        image.
class Tracker
{
public:
    /// \struct Settings
    /// \brief Nested wrapper class for settings pertaining to motion detection
    ///        and edge detection.
    struct Settings
    {
        // Contour Settings
        bool bDrawContours = false;
        
        // Threshold Settings
        int MaxThreshold = 255;
        int MinThreshold = 250;
    };

public:
    /// Constructor which takes in some settings object.
    /// \param[in] settings The settings for the tracker.
    Tracker(Settings settings);

    /// Creates the background subtracted masked image.
    /// \param[in, out] img The image/frame to be masked.
    void CreateMask(cv::Mat& img);

    /// Finds the contours of all detected objects in a frame.
    /// \param[in, out] img The image/frame for which to detect contours.
    void GetObjectContours(cv::Mat&);

    /// Checks to see if there are objects found in  the frame.
    /// \param[in, out] currentFrame The current frame number.
    void CheckForActivity(int&);

    /// Gets all cascade classifiers.
    void GetCascades();

public:
    /// Settings for the Tracker.
    Settings Config;

    /// Container for all activity events detected.
    std::map<int, std::pair<int, int>> ActivityRange;

private:
    cv::Mat _mask;
    cv::Ptr<cv::BackgroundSubtractor> bkgd_sub_ptr;
    std::map<int, cv::Ptr<cv::CascadeClassifier>> cascades;
    std::vector<std::vector<cv::Point>> contours;
    bool bIsActive;
};