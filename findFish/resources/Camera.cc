#include <Camera/Camera.h>
#include <Camera/Calibrator.h>

Camera::Camera(std::string name)
    : _name{name}
{
}

Camera::Camera(std::string name, std::string dir)
    : _name{name}
{
    GetImagesFromDir(dir);
}

std::string Camera::GetName() const
{
    return _name;
}

std::vector<std::string> Camera::GetImages() const
{
    return _images;
}

void Camera::GetImagesFromDir(std::string& dir)
{
    cv::glob(dir, _images, false);
}

void Camera::GetImagesFromDir(const std::string& dir)
{
    cv::glob(dir, _images, false);
}

void Camera::ShowImages()
{
    
}