#pragma once

#include <memory>
#include <vector>
#include <string>

class Camera
{
public:
    Camera() = delete;
    Camera(std::string);
    Camera(std::string, std::string);
    
    std::string GetName() const;
    std::vector<std::string> GetImages() const;

    void GetImagesFromDir(std::string&);
    void GetImagesFromDir(const std::string&);

    void ShowImages();

private:
    std::vector<std::string> _images;
    std::string _name;
    std::string _images_dir;

};