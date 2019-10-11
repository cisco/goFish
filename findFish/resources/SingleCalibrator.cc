#include <Camera/SingleCalibrator.h>
#include <Camera/Camera.h>
#include <cassert>

#include <opencv2/calib3d.hpp>

SingleCalibrator::SingleCalibrator() {}

SingleCalibrator::SingleCalibrator(std::shared_ptr<Camera> c)
{
    _data.resize(1);
    _cameras = {c};

    assert(_cameras.size() == 1);
    assert(_cameras.size() == _data.size());
}

void SingleCalibrator::FindImagePoints()
{
    for (size_t i = 0; i < _cameras[0]->GetImages().size(); i++)
    {
        cv::Mat img = cv::imread(_cameras[0]->GetImages()[i], cv::IMREAD_GRAYSCALE), frame;

        if (*_resolution == cv::Size())
            _resolution = std::make_unique<cv::Size>(img.size());

        if(img.size() != cv::Size()) cv::resize(img, frame, *_resolution);

        std::vector<cv::Point2f> buffer;
        bool found  = cv::findCirclesGrid(frame, *_grid_size, buffer);

        if(found) _data[0].image_points.push_back(buffer);
    }

    std::vector<cv::Point3f> objs;
    for (int i = 0; i < _grid_size->height; i++)
        for (int j = 0; j < _grid_size->width; j++)
            objs.push_back(cv::Point3f((float)j * _grid_spacing,
                                       (float)i * _grid_spacing, 0));

    while (object_points.size() != _data[0].image_points.size())
        object_points.push_back(objs);
}

void SingleCalibrator::UndistortPoints()
{
    if(_data[0].K->empty())
        throw std::runtime_error("Camera Matrix is empty");

    _data[0].undistorted_points.resize(_data[0].image_points.size());

    for (int i = 0; i < _data[0].image_points.size(); i++)
        cv::undistortPoints(_data[0].image_points[i],
                            _data[0].undistorted_points[i],
                            *_data[0].K, *_data[0].D,
                            *_data[0].R, *_data[0].P);
}

void SingleCalibrator::RunCalibration()
{
    std::lock_guard<std::recursive_mutex> lock(_lock);

    FindImagePoints();

    if (_data[0].image_points.empty()) 
        throw std::runtime_error("No image points found for camera");

    if (_data[0].image_points.size() < 2)
        throw std::runtime_error("Not enough image points for camera");

    _flags = 0;
    _flags |= cv::CALIB_FIX_ASPECT_RATIO;
    _flags |= cv::CALIB_FIX_PRINCIPAL_POINT; 
    _flags |= cv::CALIB_ZERO_TANGENT_DIST;
    _flags |= cv::CALIB_SAME_FOCAL_LENGTH;
    _flags |= cv::CALIB_FIX_K3;
    _flags |= cv::CALIB_FIX_K4;
    _flags |= cv::CALIB_FIX_K5;

    float rms = cv::calibrateCamera(object_points, _data[0].image_points,
                                     *_resolution, *_data[0].K, *_data[0].D,
                                     _data[0].rvecs, _data[0].tvecs, _flags);

    if(rms > 5.f)
        throw std::runtime_error("RMS = " + std::to_string(rms) + ". Please use better calibration images");
}
