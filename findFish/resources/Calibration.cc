#include "includes/Calibration.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/ximgproc.hpp>

#include <iostream>
#include <string>
#include <regex>
#include <algorithm>
#include <assert.h>
#include <stdexcept>

std::vector<std::string> Split(std::string&, const char*);

Calibration::Calibration(Input& in, CalibrationType type, std::string outfile)
{
    for(int i = 0; i < 2; i++)
    {
        this->_input.images[i] = in.images[i];
        this->_input.image_points[i] = in.image_points[i];
    }

    this->_input.image_size        = in.image_size;
    this->_input.grid_size         = in.grid_size != cv::Size() ? in.grid_size : cv::Size(19, 11);
    this->_input.grid_dot_size     = in.grid_dot_size >= 1.f ? in.grid_dot_size : 13.f; // mm

    this->_type              = type;
    this->_outfile_name      = outfile;
    this->_out_dir           = "calib_config/";
}

void Calibration::ReadImages(std::string dir1, std::string dir2)
{
    cv::glob(dir1, _input.images[0], false);
    cv::glob(dir2, _input.images[1], false);

    {
        auto vec = Split(dir1, "/");
        this->_input.camera_names[0] = vec[vec.size()-1];
    }
    {
        auto vec = Split(dir2, "/");
        this->_input.camera_names[1] = vec[vec.size()-1];
    }
}

void Calibration::RunCalibration()
{
    try 
    {
        if (_type == CalibrationType::STEREO)
        {
            SingleCalibrate();
            StereoCalibrate();
            UndistortPoints();
        }
        else SingleCalibrate();
    }
    catch(const std::exception& e) 
    {
        std::cerr << " !> " << e.what() << '\n';
        std::cerr << "=!= Aborted Calibration =!=\n";
    }
}

void Calibration::ReadCalibration()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if(_type == CalibrationType::STEREO)
    {
        if(!_input.image_points[0].empty() && !_input.image_points[1].empty())
            _result.n_image_pairs = (_input.image_points[0].size() + _input.image_points[1].size()) / 2;
        
        if(_input.image_points[0].size() != _input.image_points[1].size())
            throw std::runtime_error("Both sides do not have the same number of image points!");

        // Read in calibration data from file.
        cv::FileStorage fs(_out_dir + _outfile_name, cv::FileStorage::READ);
        fs["K1"] >> _result.CameraMatrix[0];
        fs["D1"] >> _result.DistCoeffs[0];
        fs["K2"] >> _result.CameraMatrix[1];
        fs["D2"] >> _result.DistCoeffs[1];
        fs["E"]  >> _result.E;
        fs["F"]  >> _result.F;
        fs["R"]  >> _result.R;
        fs["T"]  >> _result.T;
        fs["P1"] >> _result.P1;
        fs["R1"] >> _result.R1;
        fs["P2"] >> _result.P2;
        fs["R2"] >> _result.R2;
    }
}

void Calibration::GetImagePoints()
{
    std::cout << "=== Finding Image Points ===" << std::endl;
   
    for (size_t i = 0; i < (_input.images[0].size() + _input.images[1].size())/2; i++)
    {
        std::string imageL = _input.images[0][i], imageR = _input.images[1][i];
        cv::Mat imgL = cv::imread(imageL, cv::IMREAD_GRAYSCALE), imgR = cv::imread(imageR, cv::IMREAD_GRAYSCALE), frameL, frameR;

        if (_input.image_size == cv::Size())
            _input.image_size = imgL.size() != cv::Size() ? imgL.size() : imgR.size() != cv::Size() ? imgR.size() : cv::Size(1920, 1440);

        if(imgL.size() != cv::Size()) cv::resize(imgL, frameL, _input.image_size);
        if(imgR.size() != cv::Size()) cv::resize(imgR, frameR, _input.image_size);

        std::vector<cv::Point2f> bufferL, bufferR;
        bool found_left  = cv::findCirclesGrid(frameL, cv::Size(_input.grid_size.width, _input.grid_size.height), bufferL);
        bool found_right = cv::findCirclesGrid(frameR, cv::Size(_input.grid_size.width, _input.grid_size.height), bufferR);

        if ((_type == CalibrationType::STEREO && (found_left && found_right)) ||
            (_type == CalibrationType::SINGLE && (found_left || found_right)) )
        {
            if(found_left)  _input.image_points[0].push_back(bufferL);
            if(found_right) _input.image_points[1].push_back(bufferR);

            if(found_left)  _result.good_images.push_back(imageL);
            if(found_right) _result.good_images.push_back(imageR);
        }
    }

    std::vector<cv::Point3f> objs;
    for (int i = 0; i < _input.grid_size.height; i++)
        for (int j = 0; j < _input.grid_size.width; j++)
            objs.push_back(cv::Point3f((float)j * _input.grid_dot_size, (float)i * _input.grid_dot_size, 0));

    while (_input.object_points.size() != _input.image_points[0].size() &&
           _input.object_points.size() != _input.image_points[1].size())
        _input.object_points.push_back(objs);

    _result.n_image_pairs = _result.good_images.size() / 2;
}

void Calibration::SingleCalibrate()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    GetImagePoints();
    std::cout << "=== Starting Camera Calibration ===\n";
    int i = 0;
    for (auto image_points : _input.image_points)
    {
        std::cout << "  > Calibrating Camera \"" << _input.camera_names[i] << "\"...\n";

        if (image_points.empty()) 
        {
            if(_type == CalibrationType::SINGLE) 
                std::cout << " !> No image points found for this camera!\n";
            if(_type == CalibrationType::STEREO) 
                throw std::runtime_error("No image points found for camera \"" + _input.camera_names[i] + "\"!");
            i++;
            continue;
        }

        if (image_points.size() < 2)
            throw std::runtime_error("Not enough image points for camera.");

        _flags |= cv::CALIB_FIX_ASPECT_RATIO;
        _flags |= cv::CALIB_FIX_PRINCIPAL_POINT; 
        _flags |= cv::CALIB_ZERO_TANGENT_DIST;
        _flags |= cv::CALIB_SAME_FOCAL_LENGTH;
        _flags |= cv::CALIB_FIX_K3;
        _flags |= cv::CALIB_FIX_K4;
        _flags |= cv::CALIB_FIX_K5;

        double calib = calibrateCamera(_input.object_points,
                                       image_points, _input.image_size,
                                       _result.CameraMatrix[i], _result.DistCoeffs[i],
                                       _result.rvecs[i], _result.tvecs[i], _flags);
        std::cout << "  > Calibration Result: " << calib << std::endl;


        // Save results to file.
        cv::FileStorage fs(_out_dir + "calib_camera_" + _input.camera_names[i] + ".yaml", cv::FileStorage::WRITE);
        fs << "K" << _result.CameraMatrix[i];
        fs << "D" << _result.DistCoeffs[i];
        fs << "grid_size" << _input.grid_size;
        fs << "grid_dot_size" << _input.grid_dot_size;
        fs << "resolution" << _input.image_size;
        std::cout << "  > Finished Camera " << std::to_string(i) << " calibration\n";

        i++;
    }
    std::cout << "=== Finished Calibration ===" << std::endl;
}

void Calibration::StereoCalibrate()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if(_result.CameraMatrix[0].empty() || _result.CameraMatrix[1].empty())
        throw std::runtime_error("One or more Camera Matrix is empty!");

    std::cout << "=== Starting Stereo Calibration ===" << std::endl;
    std::cout << "  > Calibrating stereo..." << std::endl;

    if (_input.image_points[0].size() != _input.image_points[1].size())
        throw std::runtime_error("Bad stereo pair! Camera image points are not equal. Please"
                                 " try again or submit better stereo calibration images.");

    _flags = 0;
    _flags |= cv::CALIB_USE_INTRINSIC_GUESS;
    _flags |= cv::CALIB_SAME_FOCAL_LENGTH;

    double rms = cv::stereoCalibrate(_input.object_points,
                                     _input.image_points[0],
                                     _input.image_points[1],
                                     _result.CameraMatrix[0],// Intrinsic Matrix Left
                                     _result.DistCoeffs[0],
                                     _result.CameraMatrix[1],// Intrinsic Matrix Right
                                     _result.DistCoeffs[1],
                                     _input.image_size,
                                     _result.R,              // 3x3 Rotation Matrix
                                     _result.T,              // 3x1 Translation Vector (Column Vector)
                                     _result.E,              // Essential Matrix
                                     _result.F,              // Fundamental Matrix
                                     _flags,
                                     cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, 1e-5));

    std::cout << "  > Stereo Calibration Result: " << rms << std::endl;

    std::cout << "  > Rectifying stereo..." << std::endl;
    cv::stereoRectify(_result.CameraMatrix[0],
                      _result.DistCoeffs[0],
                      _result.CameraMatrix[1],
                      _result.DistCoeffs[1],
                      _input.image_size,
                      _result.R,
                      _result.T,
                      _result.R1,
                      _result.R2,
                      _result.P1,
                      _result.P2,
                      _result.Q,
                      cv::CALIB_ZERO_DISPARITY, -1, _input.image_size);

    // Save calibration to yaml file to be used elsewhere.
    cv::FileStorage fs(_out_dir + _outfile_name, cv::FileStorage::WRITE);
    fs << "K1" << _result.CameraMatrix[0];
    fs << "D1" << _result.DistCoeffs[0];
    fs << "K2" << _result.CameraMatrix[1];
    fs << "D2" << _result.DistCoeffs[1];
    fs << "E" << _result.E;
    fs << "F" << _result.F;
    fs << "R" << _result.R;
    fs << "T" << _result.T;
    fs << "P1" << _result.P1;
    fs << "R1" << _result.R1;
    fs << "P2" << _result.P2;
    fs << "R2" << _result.R2;
    fs << "grid_size" << _input.grid_size;
    fs << "grid_dot_size" << _input.grid_dot_size;
    fs << "image_size" << _input.image_size;
    std::cout << "=== Finished Stereo Calibration ===" << std::endl;
}

void Calibration::GetUndistortedImage() const
{
    if(_result.CameraMatrix[0].empty() || _result.CameraMatrix[1].empty())
        throw std::runtime_error("One or more Camera Matrix is empty!");

    int j = 0;
    for (size_t i = 0; i < _result.good_images.size(); i++)
    {
        cv::Mat img, undist_img;
        img = cv::imread(_result.good_images[i]);
        cv::Mat frame;
        cv::resize(img, frame, _input.image_size);
    
        cv::Mat R, P;
        if(i%2 == 0)
        {
            R= _result.R1;
            P = _result.P1;
        }
        else
        {
            R= _result.R2;
            P = _result.P2;
        }

        cv::Mat remap[2][2];
        cv::initUndistortRectifyMap(_result.CameraMatrix[i%2], _result.DistCoeffs[i%2], R, P, _input.image_size, CV_32FC1, remap[i%2][0], remap[i%2][1]);
        cv::remap(frame, undist_img, remap[i%2][0],remap[i%2][1], cv::INTER_LINEAR, cv::BORDER_CONSTANT);

        if(i % 2 != 0) j = (j+1) % _result.undistorted_points[i%2].size();
        
        cv::imshow("Rectified (Undistorted) Image", undist_img); 
        char c = cv::waitKey(0);
        if(c==27) continue;
    }
}

void Calibration::UndistortImage(cv::Mat& img, int index) const
{
    if(_result.CameraMatrix[index].empty() || _result.DistCoeffs[index].empty())
        throw std::runtime_error("Camera Matrix [" + std::to_string(index) +"] is empty!");
        
    if(index < 2 && !img.empty())
    {
        cv::Mat uimg;
        cv::Mat frame;
        cv::resize(img, frame, _input.image_size);
        cv::undistort(frame, uimg, _result.CameraMatrix[index], _result.DistCoeffs[index]);
        img = uimg;
    }
}

void Calibration::UndistortPoints()
{
    if(_result.CameraMatrix[0].empty() || _result.CameraMatrix[1].empty())
        throw std::runtime_error("One or more Camera Matrix is empty!");

    _result.undistorted_points[0].resize(_input.image_points[0].size());
    _result.undistorted_points[1].resize(_input.image_points[1].size());

    for (int i = 0; i < _result.n_image_pairs; i++)
    {
        cv::undistortPoints(_input.image_points[0][i],
                            _result.undistorted_points[0][i],
                            _result.CameraMatrix[0],
                            _result.DistCoeffs[0],
                            _result.R1,
                            _result.P1);

        cv::undistortPoints(_input.image_points[1][i],
                            _result.undistorted_points[1][i],
                            _result.CameraMatrix[1],
                            _result.DistCoeffs[1],
                            _result.R2,
                            _result.P2);
    }
}

void Calibration::TriangulatePoints()
{
    cv::Mat P1_32FC1, P2_32FC1;
    _result.P1.convertTo(P1_32FC1, CV_32FC1);
    _result.P2.convertTo(P2_32FC1, CV_32FC1);

    if(_result.undistorted_points[0].empty() || _result.undistorted_points[1].empty())
    {
        if(_input.image_points[0].empty() || _input.image_points[1].empty())
            throw std::runtime_error("No points to triangulate!");

        _result.undistorted_points[0] = _input.image_points[0];
        _result.undistorted_points[1] = _input.image_points[1];
    }

    std::cout << "=== Starting Triangulation ===\n";
    std::cout << "  > Triangulating points...\n";

    cv::Mat homogeneous_points[_result.n_image_pairs];
    for (int i = 0; i < _result.n_image_pairs; i++)
        cv::triangulatePoints(P1_32FC1, P2_32FC1,
                              _result.undistorted_points[0][i], // Left
                              _result.undistorted_points[1][i], // Right
                              homogeneous_points[i]);

    _result.object_points.resize(_result.n_image_pairs);
    for(int i = 0; i < _result.n_image_pairs; i++)
        cv::convertPointsFromHomogeneous(homogeneous_points[i].t(), _result.object_points[i]);

    cv::FileStorage fs(_out_dir + "object_points.yaml", cv::FileStorage::WRITE);
    fs << "object_points" << _result.object_points;

    std::cout << "=== Finished Triangulation ===" << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

std::vector<std::string> Split(std::string& str, const char* delimiter)
{
    std::vector<std::string> _result;
    size_t i = 0;
    std::string temp = str;

    std::regex r("\\s+");
    while ((i = temp.find(delimiter)) != std::string::npos)
    {
        if (i > str.length())
            i = str.length() - 1;
        _result.push_back(regex_replace(temp.substr(0, i), r, ""));
        temp.erase(0, i + 1);
    }
    if(regex_replace(temp.substr(0, i), r, "") != "") 
        _result.push_back(regex_replace(temp.substr(0, i), r, ""));

    return _result;
}