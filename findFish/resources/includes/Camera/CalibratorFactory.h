#include <memory>
#include <Camera/Camera.h>
#include <Camera/SingleCalibrator.h>
#include <Camera/StereoCalibrator.h>

enum CalibratorType { C_SINGLE, C_STEREO, C_MULTI };

class CalibratorFactory
{
public:
    template<typename ...T>
    static std::shared_ptr<Calibrator> MakeCalibrator(std::shared_ptr<T>&... cams);
    template<typename ...T>
    static std::shared_ptr<Calibrator> MakeCalibrator(CalibratorType type, std::shared_ptr<T>&... cams);
};

template<typename ...T>
std::shared_ptr<Calibrator> CalibratorFactory::MakeCalibrator(std::shared_ptr<T>&... cams)
{
    const int size = sizeof...(cams);
    
    assert(size > 0);

    std::vector<std::shared_ptr<Camera>> t = {cams...};
    if(size == 1)
        return std::make_shared<SingleCalibrator>(t[0]);
    else if(size == 2) 
        return std::make_shared<StereoCalibrator>(t[0], t[1]);
    else
        return nullptr;
}

template<typename ...T>
std::shared_ptr<Calibrator> CalibratorFactory::MakeCalibrator(CalibratorType type, std::shared_ptr<T>&... cams)
{
    std::vector<std::shared_ptr<Camera>> t = {cams...};
    switch(type)
    {
        case CalibratorType::C_SINGLE:
        return std::make_shared<SingleCalibrator>(t[0]);
        break;
        case CalibratorType::C_STEREO:
        return std::make_shared<StereoCalibrator>(t[0], t[1]);
        break;
        case CalibratorType::C_MULTI:
        return nullptr;
        break;
    }
    return nullptr;
}

