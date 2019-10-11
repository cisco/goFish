#pragma once

#include "SingleCalibrator.h"

class StereoCalibrator : public SingleCalibrator
{
public:
    StereoCalibrator();
    StereoCalibrator(std::shared_ptr<Camera>, std::shared_ptr<Camera>);
    virtual ~StereoCalibrator() = default;

    virtual void RunCalibration() override;

    void Triangulate(std::vector<std::vector<cv::Point2f>>);

private:
    virtual void FindImagePoints() override;
    virtual void UndistortPoints() override;

private:
    std::vector<std::string> _good_stereo_images;
};