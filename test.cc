#include <iostream>
#include <vector>
#include <opencv2/calib3d.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

void GetImagePoints(vector<vector<Point2f>>&, vector<vector<Point3f>>&, vector<string>&);
void Calibrate(vector<vector<Point2f>>&, vector<vector<Point3f>>&);

int main(int argc, char** argv)
{
    vector<string> images;
    cv::glob(argv[1], images, false);
    
    vector<vector<Point2f>> image_points;
    vector<vector<Point3f>> object_points;
    GetImagePoints(image_points, object_points, images);
    Calibrate(image_points, object_points);
    
    return 0;
}

void GetImagePoints(vector<vector<Point2f>>& image_points, vector<vector<Point3f>>& object_points, vector<string>& images)
{
    for (size_t i = 0; i < (images.size() + images.size())/2; i++)
    {
        std::string image = images[i];
        cv::Mat img = cv::imread(image, IMREAD_GRAYSCALE);

        std::vector<cv::Point2f> buffer;
        bool found  = cv::findCirclesGrid(img, cv::Size(19, 11), buffer);

        if (found)
            image_points.push_back(buffer);
    }

    std::vector<cv::Point3f> objs;
    for (int i = 0; i < 11; i++)
        for (int i = 0; i < 19; i++)
            objs.push_back(cv::Point3f((float)i * 12.f, (float)i * 12.f, 0));

    while (object_points.size() != image_points.size())
        object_points.push_back(objs);
}

void Calibrate(vector<vector<Point2f>>& image_points, vector<vector<Point3f>>& object_points)
{
        std::vector<cv::Mat> rvecs, tvecs;
        int flags = 0;
        flags |= cv::CALIB_FIX_ASPECT_RATIO;
        flags |= cv::CALIB_FIX_PRINCIPAL_POINT; 
        flags |= cv::CALIB_ZERO_TANGENT_DIST;
        flags |= cv::CALIB_SAME_FOCAL_LENGTH;
        flags |= cv::CALIB_FIX_K3;
        flags |= cv::CALIB_FIX_K4;
        flags |= cv::CALIB_FIX_K5;

        cv::Mat CameraMatrix, DistCoeffs;
        double rms = calibrateCamera(object_points,
                                       image_points, cv::Size(19,11),
                                       CameraMatrix, DistCoeffs,
                                       rvecs, tvecs, flags);
        std::cout << "  > Calibration Result: " << rms << std::endl;
}