#include "Camera/StereoCalibrator.h"
#include <Camera/Camera.h>

#include <opencv2/calib3d.hpp>

StereoCalibrator::StereoCalibrator()
    : SingleCalibrator{}
{
    _data.resize(2);
}

StereoCalibrator::StereoCalibrator(std::shared_ptr<Camera> c1, std::shared_ptr<Camera> c2)
    : SingleCalibrator{}
{
    _data.resize(2);
    _cameras = {c1, c2};
    assert(_cameras.size() == 2);
    assert(_cameras.size() == _data.size());
}

void StereoCalibrator::FindImagePoints()
{
    std::lock_guard<std::recursive_mutex> lock(_lock);

    for (size_t i = 0; i < (_cameras[0]->GetImages().size() + _cameras[1]->GetImages().size()) / 2; i++) 
    {
        std::string imageL = _cameras[0]->GetImages()[i], imageR = _cameras[1]->GetImages()[i];
        cv::Mat imgL = cv::imread(imageL, cv::IMREAD_GRAYSCALE);
        cv::Mat imgR = cv::imread(imageR, cv::IMREAD_GRAYSCALE);
        cv::Mat frameL, frameR;

        if (*_resolution == cv::Size())
            _resolution = std::make_unique<cv::Size>(imgL.size() != cv::Size() ? 
                                                     imgL.size() : imgR.size() != cv::Size() ?
                                                     imgR.size() : cv::Size(1920, 1440));

        if (imgL.size() != cv::Size() && imgR.size() != cv::Size())
            if(imgL.size() == imgR.size())
            {
                cv::resize(imgL, frameL, *_resolution);
                cv::resize(imgR, frameR, *_resolution);
            }
            else throw std::runtime_error("Cameras do not have the same resolution");
        else throw std::runtime_error("Images have no resolution");

        std::vector<cv::Point2f> bufferL, bufferR;
        bool found_left = cv::findCirclesGrid(frameL, *_grid_size, bufferL);
        bool found_right = cv::findCirclesGrid(frameR, *_grid_size, bufferR);

        if (found_left && found_right)
        {
            _data[0].image_points.push_back(bufferL);
            _data[1].image_points.push_back(bufferR);
            _good_stereo_images.push_back(imageL);
            _good_stereo_images.push_back(imageR);
        }
    }

    std::vector<cv::Point3f> objs;
    for (int i = 0; i < _grid_size->height; i++)
        for (int j = 0; j < _grid_size->width; j++)
            objs.push_back(cv::Point3f((float)j * _grid_spacing,
                                       (float)i * _grid_spacing, 0));

    while (object_points.size() != _data[0].image_points.size())
        object_points.push_back(objs);
}

void StereoCalibrator::UndistortPoints()
{
    if(_data[0].K->empty() || _data[1].K->empty())
    throw std::runtime_error("One or more Camera Matrix is empty");

    for(int k = 0; k < 2; k++)
    {
        _data[k].undistorted_points.resize(_data[k].image_points.size());
        for (int i = 0; i < int(_good_stereo_images.size() / 2); i++)
            cv::undistortPoints(_data[k].image_points[i],
                                _data[k].undistorted_points[i],
                                *_data[k].K, *_data[k].D,
                                *_data[k].R, *_data[k].P);
    }
}

template<typename T>
void swap(T& a, T& b)
{
    T temp = std::move(a);
    a = std::move(b);
    b = std::move(temp);
}

void StereoCalibrator::RunCalibration()
{
    std::lock_guard<std::recursive_mutex> lock(_lock);

    // Calibrate each camera individually.
    {
        SingleCalibrator::RunCalibration();
        swap(_cameras[0], _cameras[1]);
        swap(_data[0], _data[1]);
        SingleCalibrator::RunCalibration();
        swap(_cameras[0], _cameras[1]);
        swap(_data[0], _data[1]);
    }

    if(_data[0].K->empty() || _data[1].K->empty())
        throw std::runtime_error("One or more Camera Matrix is empty");

    if (_data[0].image_points.size() != _data[1].image_points.size())
        throw std::runtime_error("Image points do not match (bad stereo pair");

    _flags = 0;
    _flags |= cv::CALIB_USE_INTRINSIC_GUESS;
    _flags |= cv::CALIB_SAME_FOCAL_LENGTH;

    float rms = cv::stereoCalibrate(object_points,
                                    _data[0].image_points, _data[1].image_points,
                                     *_data[0].K, *_data[0].D,
                                     *_data[1].K, *_data[1].D,
                                     *_resolution,
                                     *R, *T, *E, *F,
                                     _flags);

    if(rms > 10.f)
        throw std::runtime_error("RMS = " + std::to_string(rms) + ". Please use better calibration images");

    cv::stereoRectify(*_data[0].K, *_data[0].D,
                      *_data[1].K, *_data[1].D,
                      *_resolution, *R, *T,
                      *_data[0].R, *_data[1].R,
                      *_data[0].P, *_data[1].P,
                      *Q, cv::CALIB_ZERO_DISPARITY, -1, *_resolution);

}
