#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <opencv2/opencv.hpp>

class Camera;

class Calibrator
{
    struct Data
    {
        std::vector<std::vector<cv::Point2f>> image_points;
        std::vector<std::vector<cv::Point2f>> undistorted_points;
        std::unique_ptr<cv::Mat> K, D;
        std::unique_ptr<cv::Mat> R, P;
        std::vector<cv::Mat> rvecs, tvecs;

    };

public:
    Calibrator() = default;
    virtual ~Calibrator() = default;

    template<typename ...T>
    void SetCameras(std::shared_ptr<T>&&... cams)
    {
        const int size = sizeof...(cams);

        if(size != _data.size())
            throw std::runtime_error("Number of image directories does not "
                                    "match required amount of cameras "
                                    "(" + std::to_string(_data.size()) + ")");

        _cameras = {cams...};
    }

    template<typename ...T>
    void UndistortImages(T&&... imgs)
    {
        const int size = sizeof...(imgs);

        if(size > _data.size())
            throw std::runtime_error("Too many images for the amount of cameras");

        std::vector<cv::Mat> t = {imgs...};

        for(int i = 0; i < size; i++)
        {
            if(_data[i].K->empty() || _data[i].D->empty())
                throw std::runtime_error("Camera Matrix [" + std::to_string(i) +"] is empty");
                
            if(!t[i].empty())
            {
                cv::Mat uimg;
                cv::Mat frame;
                cv::resize(t[i], frame, *_resolution);
                cv::undistort(frame, uimg, *_data[i].K, *_data[i].D);
                t[i] = uimg;
            }
        }
    }

    virtual void RunCalibration() = 0;

private:
    virtual void FindImagePoints() = 0;
    virtual void UndistortPoints() = 0;

protected:
    std::vector<std::shared_ptr<Camera>> _cameras; 
    std::vector<std::vector<cv::Point3f>> object_points;
    std::unique_ptr<cv::Mat> R, T, E, F, Q;
    std::unique_ptr<cv::Size> _resolution;
    std::recursive_mutex _lock;
    std::vector<Data> _data;
    cv::Size* _grid_size;
    float _grid_spacing;
    int _flags;

};
