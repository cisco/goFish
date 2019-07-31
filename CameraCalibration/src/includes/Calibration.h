/// \author Tomas Rigaux
/// \date June 13, 2019
///
/// Single ans stereo camera calibration. The class reads in two directories of
/// of images from different cameras, then calibrates each of them individually.
/// Once they are both calibrated, they are then passed together through stereo
/// calibration, resulting in the correct matrices that allow for the proper
/// undistortion. These undistorted points are then selected and triangulated
/// using epipolar geometry and SVD to output a real world coordinate.

#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

enum CalibrationType { SINGLE, STEREO };

/// \brief Calibrates single and/or stereo cameras.
class Calibration
{
public:
    /// \brief Input for grid dimensions, camera names, and image points.
    struct Input
    {
        cv::Size grid_size;
        float grid_dot_size;
        cv::Size image_size;
        std::string camera_names[2];
        std::vector<std::string> images[2];

        std::vector<std::vector<cv::Point3f>> object_points;
        std::vector<std::vector<cv::Point2f>> image_points[2];
    };

private:
    /// \brief The resultant matrices and undistorted points of the calibration.
    struct Result
    {
        // Intrinsic Matrices
        cv::Mat CameraMatrix[2];
        // Distortion coefficients
        cv::Mat DistCoeffs[2];
        std::vector<cv::Mat> rvecs[2], tvecs[2];
        int n_image_pairs;
        
        std::vector<std::string> good_images;
        std::vector<std::vector<cv::Point3f>> object_points;
        std::vector<std::vector<cv::Point2f>> undistorted_points[2];
        std::vector<std::vector<cv::Point2f>> non_stereo_points[2];

        // Stereo Matrices
        cv::Mat R, T;
        cv::Mat R1, R2, Q, P1, P2, E, F;
    };

public:
    /// Construct a calibration object with Input, Type, and specifies an out directory.
    /// \param[in, out] in The input object containing necessary information for obtaining points.
    /// \param[in] type The type of calibration to perform (Single or Stereo).
    /// \param[in] out_dir The directory to output calibration results.
    Calibration(Input& in, CalibrationType type, std::string out_dir);

    /// Gets image points, runs the calibration, and undistorts image points.
    void RunCalibration();

    /// Read in a lready calibrated data.
    void ReadCalibration();

    /// Read images from 2 different directories.
    void ReadImages(std::string, std::string);

    /// Undistorts and displays all obtained and valid calibration images.
    void GetUndistortedImage();

    /// Undistorts a given image using calibration results.
    /// \param[in, out] img The image to undistort.
    /// \param[in] index Which camera results to use.
    void UndistortImage(cv::Mat&, int);

    /// Triangulates undistorted image points into real world 3D coordinates.
    void TriangulatePoints();

private:
    /// Runs individual calibration for each camera.
    void SingleCalibrate();

    /// Runs stereo calibration on the two previously calibrated cameras.
    void StereoCalibrate();

    /// Finds key image points, such as a calibration grid.
    void GetImagePoints();

    /// Undistorts image points using stereo calibration results.
    void UndistortPoints();

public:
    bool bRunCalibration;
    Input input;

private:
    Result result;
    
    std::string outfile_name;
    std::string out_dir;
    CalibrationType type;
    int flags = 0;
};
