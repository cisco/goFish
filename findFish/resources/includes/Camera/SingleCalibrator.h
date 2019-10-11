#pragma once

#include "Calibrator.h"

class SingleCalibrator : public Calibrator
{
public:
    SingleCalibrator();
    SingleCalibrator(std::shared_ptr<Camera>);
    virtual ~SingleCalibrator() = default;

    virtual void RunCalibration() override;

private:
    virtual void FindImagePoints() override;
    virtual void UndistortPoints() override;

};