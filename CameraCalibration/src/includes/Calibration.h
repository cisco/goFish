#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

enum CalibrationType { SINGLE, STEREO };

class Calibration
{
public:
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
    struct Result
    {
        // Intrinsic Matrices and distortion coefficients.
        cv::Mat CameraMatrix[2];
        cv::Mat DistCoeffs[2];
        cv::Mat OptimalMatrix[2];
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
    Calibration(Input&, CalibrationType, std::string);

    void RunCalibration();
    void ReadCalibration();
    void ReadImages(std::string, std::string);
    void GetUndistortedImage();
    void UndistortImage(cv::Mat&, int);
    void TriangulatePoints();

private:
    void SingleCalibrate();
    void StereoCalibrate();
    void GetImagePoints();
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
