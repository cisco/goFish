#include <iostream>
#include "includes/Calibration.h"

int main(int argc, char** argv)
{
     Calibration::Input input;
     input.image_size = cv::Size(1920, 1440);

     Calibration* calib = new Calibration(input, CalibrationType::STEREO, "stereo_calibration.yaml");
     
     calib->ReadImages(argv[1], argv[2]);
     calib->RunCalibration();
     //calib->GetUndistortedImage();
     
     delete calib;
     return 0;
}
