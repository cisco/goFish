#include "includes/Calibration.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/ximgproc.hpp>

#include <iostream>
#include <string>
#include <regex>
#include <algorithm>

std::vector<std::string> Split(std::string&, const char*);

Calibration::Calibration(Input& in, CalibrationType type, std::string outfile)
{
    for(int i = 0; i < 2; i++)
    {
        this->input.images[i] = in.images[i];
        this->input.image_points[i] = in.image_points[i];
    }

    this->input.image_size        = in.image_size;
    this->input.grid_size         = in.grid_size != cv::Size() ? in.grid_size : cv::Size(19, 11);
    this->input.grid_dot_size     = in.grid_dot_size >= 1.f ? in.grid_dot_size : 13.f; // mm

    this->type              = type;
    this->outfile_name      = outfile;
    this->out_dir           = "calib_config/";
}

void Calibration::ReadImages(std::string dir1, std::string dir2)
{
    cv::glob(dir1, input.images[0], false);
    cv::glob(dir2, input.images[1], false);

    {
        auto vec = Split(dir1, "/");
        this->input.camera_names[0] = vec[vec.size()-1];
    }
    {
        auto vec = Split(dir2, "/");
        this->input.camera_names[1] = vec[vec.size()-1];
    }
}

void Calibration::RunCalibration()
{
    if (type == CalibrationType::STEREO)
    {
        SingleCalibrate();
        StereoCalibrate();
        UndistortPoints();
    }
    else SingleCalibrate();
}

void Calibration::ReadCalibration()
{
    if(input.image_points[0].empty()) std::cout << "No left image points!\n";
    if(input.image_points[1].empty()) std::cout << "No right image points!\n";
    if(type == CalibrationType::STEREO)
    {
        if(!input.image_points[0].empty() &&!input.image_points[1].empty())
            result.n_image_pairs = (input.image_points[0].size() + input.image_points[1].size()) / 2;
        
        // Read in calibration data from file.
        {
            cv::FileStorage fs(out_dir + outfile_name, cv::FileStorage::READ);
            fs["K1"] >> result.CameraMatrix[0];
            fs["D1"] >> result.DistCoeffs[0];
            fs["K2"] >> result.CameraMatrix[1];
            fs["D2"] >> result.DistCoeffs[0];
            fs["K1"] >> result.CameraMatrix[0];
            fs["D1"] >> result.DistCoeffs[0];
            fs["K2"] >> result.CameraMatrix[1];
            fs["D2"] >> result.DistCoeffs[1];
            fs["E"] >> result.E;
            fs["F"] >> result.F;
            fs["R"] >> result.R;
            fs["T"] >> result.T;
            fs["P1"] >> result.P1;
            fs["R1"] >> result.R1;
            fs["P2"] >> result.P2;
            fs["R2"] >> result.R2;
        }
    }
}

void Calibration::GetImagePoints()
{
    std::cout << "  > Finding image points..." << std::endl;
   
    for (size_t j = 0; j < (input.images[0].size() + input.images[1].size())/2; j++)
    {
        std::string imageL = input.images[0][j], imageR = input.images[1][j];
        cv::Mat imgL = cv::imread(imageL, cv::IMREAD_GRAYSCALE), imgR = cv::imread(imageR, cv::IMREAD_GRAYSCALE), frameL, frameR;

        if (input.image_size == cv::Size())
            input.image_size = imgL.size() != cv::Size() ? imgL.size() : imgR.size() != cv::Size() ? imgR.size() : cv::Size(1920, 1440);

        if(imgL.size() != cv::Size()) cv::resize(imgL, frameL, input.image_size);
        if(imgR.size() != cv::Size()) cv::resize(imgR, frameR, input.image_size);

        std::vector<cv::Point2f> bufferL, bufferR;
        bool found_left  = cv::findCirclesGrid(frameL, cv::Size(input.grid_size.width, input.grid_size.height), bufferL);
        bool found_right = cv::findCirclesGrid(frameR, cv::Size(input.grid_size.width, input.grid_size.height), bufferR);

        std::cout << "Left found=" << found_left << " |  Right found=" << found_right << "\n"; 
        std::cout << "STEREO both found: " << (type == CalibrationType::STEREO && (found_left && found_right)) <<std::endl;
        std::cout << "SINGLE found one:" << (type == CalibrationType::SINGLE && (found_left || found_right)) << std::endl;

        if ( (type == CalibrationType::STEREO && (found_left && found_right)) || (type == CalibrationType::SINGLE && (found_left || found_right)) )
        {
            //cv::drawChessboardCorners(frame, cv::Size(input.grid_size.width, input.grid_size.height), cv::Mat(buffer), true);
            
            if(found_left)  input.image_points[0].push_back(bufferL);
            if(found_right) input.image_points[1].push_back(bufferR);

            if(found_left)  result.good_images.push_back(imageL);
            if(found_right) result.good_images.push_back(imageR);
        }
    }

    std::vector<cv::Point3f> objs;
    for (int i = 0; i < input.grid_size.height; i++)
        for (int j = 0; j < input.grid_size.width; j++)
            objs.push_back(cv::Point3f((float)j * input.grid_dot_size, (float)i * input.grid_dot_size, 0));

    while (input.object_points.size() != input.image_points[0].size() && input.object_points.size() != input.image_points[1].size())
        input.object_points.push_back(objs);

    result.n_image_pairs = result.good_images.size() / 2;
}

void Calibration::SingleCalibrate()
{
    std::cout << "=== Starting Camera Calibration ===\n";

    GetImagePoints();
    
    int i = 0;
    for (auto image_points : input.image_points)
    {
        std::cout << "  > Calibrating Camera \"" << input.camera_names[i] << "\"...\n";
        
        if(input.image_points[i].size() < 2)
        {
            std::cout << " !> Not enough image points! Please try getting more image points\n";
            return;
        }

        if (image_points.empty())
        {
            std::cout << " !> No image points found for this camera!\n";
            i++;
            continue;
        }

        for(size_t i = 0; i < input.object_points.size(); i++)
        for(size_t j = 0; j < input.object_points[i].size(); j++)
                std::cout << "Object Points: " << input.object_points[i][j] <<std::endl;

for(size_t k = 0; k < input.image_points[i].size(); k++)
        for(size_t j = 0; j < input.image_points[i][k].size(); j++)
                std::cout << "Image Points: " << input.image_points[i][k][j] <<std::endl;

std::cout << "Image Size: " << input.image_size <<std::endl;

        //std::vector<cv::Mat> rvecs, tvecs;
        flags |= cv::CALIB_FIX_ASPECT_RATIO;
        flags |= cv::CALIB_FIX_PRINCIPAL_POINT; 
        flags |= cv::CALIB_ZERO_TANGENT_DIST;
        flags |= cv::CALIB_SAME_FOCAL_LENGTH;
        flags |= cv::CALIB_FIX_K3;
        flags |= cv::CALIB_FIX_K4;
        flags |= cv::CALIB_FIX_K5;

        double calib = calibrateCamera(input.object_points,
                                       image_points, input.image_size,
                                       result.CameraMatrix[i], result.DistCoeffs[i],
                                       result.rvecs[i], result.tvecs[i], flags);
        std::cout << "  > Calibration Result: " << calib << std::endl;


        // Save results to file.
        {
            cv::FileStorage fs(out_dir + "calib_camera_" + input.camera_names[i] + ".yaml", cv::FileStorage::WRITE);
            fs << "K" << result.CameraMatrix[i];
            fs << "D" << result.DistCoeffs[i];
            fs << "grid_size" << input.grid_size;
            fs << "grid_dot_size" << input.grid_dot_size;
            fs << "resolution" << input.image_size;
            std::cout << "  > Finished Camera " << std::to_string(i) << " calibration\n";
        }

        i++;
    }
    std::cout << "=== Finished Calibration ===" << std::endl;
}

void Calibration::StereoCalibrate()
{
    if(result.CameraMatrix[0].empty() || result.CameraMatrix[1].empty())
        return;

    std::cout << "=== Starting Stereo Calibration ===" << std::endl;
    std::cout << "  > Calibrating stereo..." << std::endl;

    if(input.image_points[0].size() != input.image_points[1].size())
    {
        std::cout << " !> Bad stereo pair! Camera images are not fully paired up. Please try again or submit better stereo calibration images.\n";
        return;
    }
    

    flags = 0;
    //flags |= cv::CALIB_FIX_INTRINSIC;
    flags |= cv::CALIB_USE_INTRINSIC_GUESS;
    flags |= cv::CALIB_SAME_FOCAL_LENGTH;

    double rms = cv::stereoCalibrate(input.object_points,
                                     input.image_points[0],
                                     input.image_points[1],
                                     result.CameraMatrix[0],// Intrinsic Matrix Left
                                     result.DistCoeffs[0],
                                     result.CameraMatrix[1],// Intrinsic Matrix Right
                                     result.DistCoeffs[1],
                                     input.image_size,
                                     result.R,              // 3x3 Rotation Matrix
                                     result.T,              // 3x1 Translation Vector (Column Vector)
                                     result.E,              // Essential Matrix
                                     result.F,              // Fundamental Matrix
                                     flags,
                                     cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, 1e-5));

    std::cout << "  > Stereo Calibration Result: " << rms << std::endl;

    std::cout << "  > Rectifying stereo..." << std::endl;
    cv::stereoRectify(result.CameraMatrix[0],
                      result.DistCoeffs[0],
                      result.CameraMatrix[1],
                      result.DistCoeffs[1],
                      input.image_size,
                      result.R,
                      result.T,
                      result.R1,
                      result.R2,
                      result.P1,
                      result.P2,
                      result.Q,
                      cv::CALIB_ZERO_DISPARITY, -1, input.image_size);

    // Save calibration to yaml file to be used elsewhere.
    {
        cv::FileStorage fs(out_dir + outfile_name, cv::FileStorage::WRITE);
        fs << "K1" << result.CameraMatrix[0];
        fs << "D1" << result.DistCoeffs[0];
        fs << "K2" << result.CameraMatrix[1];
        fs << "D2" << result.DistCoeffs[1];
        fs << "E" << result.E;
        fs << "F" << result.F;
        fs << "R" << result.R;
        fs << "T" << result.T;
        fs << "P1" << result.P1;
        fs << "R1" << result.R1;
        fs << "P2" << result.P2;
        fs << "R2" << result.R2;
        fs << "grid_size" << input.grid_size;
        fs << "grid_dot_size" << input.grid_dot_size;
        fs << "image_size" << input.image_size;
        std::cout << "=== Finished Stereo Calibration ===" << std::endl;
    }
}

void Calibration::GetUndistortedImage()
{
    if(result.CameraMatrix[0].empty() || result.CameraMatrix[1].empty())
        return;

    int j = 0;
    for (size_t i = 0; i < result.good_images.size(); i++)
    {
        cv::Mat img, undist_img;
        img = cv::imread(result.good_images[i]);
        cv::Mat frame;
        cv::resize(img, frame, input.image_size);
    
        cv::Mat R, P;
        if(i%2 == 0)
        {
            R= result.R1;
            P = result.P1;
        }
        else
        {
            R= result.R2;
            P = result.P2;
        }

        cv::Mat remap[2][2];
        cv::initUndistortRectifyMap(result.CameraMatrix[i%2], result.DistCoeffs[i%2], R, P, input.image_size, CV_32FC1, remap[i%2][0], remap[i%2][1]);
        cv::remap(frame, undist_img, remap[i%2][0],remap[i%2][1], cv::INTER_LINEAR, cv::BORDER_CONSTANT);

        for(size_t l = 0; l < input.image_points[i%2][j].size(); l++)
        {
            //cv::circle(undist_img, input.image_points[i%2][j][l], 6, cv::Scalar(0, 0, 255), -1);
            //cv::circle(undist_img, result.non_stereo_points[i%2][j][l], 6, cv::Scalar(255, 100, 0), -1);
            cv::circle(undist_img, result.undistorted_points[i%2][j][l], 6, cv::Scalar(0, 255, 0), -1);
        }

        if(i % 2 != 0) j = (j+1) % result.undistorted_points[i%2].size();
        cv::imshow("Rectified (Undistorted) Image", undist_img); 
        char c = cv::waitKey(0);
        if(c==27) continue;
    }
}

void Calibration::UndistortImage(cv::Mat& img, int index)
{
    if(index < 2 && !img.empty())
    {
        cv::Mat uimg;
        cv::Mat frame;
        cv::resize(img, frame, input.image_size);
        cv::undistort(frame, uimg, result.CameraMatrix[index], result.DistCoeffs[index]);
        img = uimg;
    }
}

void Calibration::UndistortPoints()
{
    if(result.CameraMatrix[0].empty() || result.CameraMatrix[1].empty())
        return;

    result.undistorted_points[0].resize(input.image_points[0].size());
    result.undistorted_points[1].resize(input.image_points[1].size());
    result.non_stereo_points[0].resize(input.image_points[0].size());
    result.non_stereo_points[1].resize(input.image_points[1].size());

    for (int i = 0; i < result.n_image_pairs; i++)
    {
        // TODO: Get better stereo images so that R and P can be used.
        cv::undistortPoints(input.image_points[0][i],
                            result.undistorted_points[0][i],
                            result.CameraMatrix[0],
                            result.DistCoeffs[0],
                            result.R1,
                            result.P1);

        cv::undistortPoints(input.image_points[1][i],
                            result.undistorted_points[1][i],
                            result.CameraMatrix[1],
                            result.DistCoeffs[1],
                            result.R2,
                            result.P2);
    }
}

void Calibration::TriangulatePoints()
{
    cv::Mat P1_32FC1, P2_32FC1;
    result.P1.convertTo(P1_32FC1, CV_32FC1);
    result.P2.convertTo(P2_32FC1, CV_32FC1);

    if(result.undistorted_points[0].empty() || result.undistorted_points[1].empty())
    {
        result.undistorted_points[0] = input.image_points[0];
        result.undistorted_points[1] = input.image_points[1];
    }

    if(result.undistorted_points[0].empty() || result.undistorted_points[1].empty())
        return;


    std::cout << "=== Starting Triangulation ===" << std::endl;
    std::cout << "  > Triangulating points..." << std::endl;

    cv::Mat homogeneous_points[result.n_image_pairs];
    for (int i = 0; i < result.n_image_pairs; i++)
        cv::triangulatePoints(P1_32FC1, P2_32FC1,
                              result.undistorted_points[0][i], // Left
                              result.undistorted_points[1][i], // Right
                              homogeneous_points[i]);

    result.object_points.resize(result.n_image_pairs);
    for(int i = 0; i < result.n_image_pairs; i++)
        cv::convertPointsFromHomogeneous(homogeneous_points[i].t(), result.object_points[i]);

    {
        cv::FileStorage fs(out_dir + "ObjectPoints.yaml", cv::FileStorage::WRITE);
        fs << "object_points" << result.object_points;
    }

    std::cout << "=== Finished Triangulation ===" << std::endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions
/////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> Split(std::string& str, const char* delimiter)
{
    std::vector<std::string> result;
    size_t i = 0;
    std::string temp = str;

    std::regex r("\\s+");
    while ((i = temp.find(delimiter)) != std::string::npos)
    {
        if (i > str.length())
            i = str.length() - 1;
        result.push_back(regex_replace(temp.substr(0, i), r, ""));
        temp.erase(0, i + 1);
    }
    if(regex_replace(temp.substr(0, i), r, "") != "") 
        result.push_back(regex_replace(temp.substr(0, i), r, ""));

    return result;
}