#include "test_calibration.h"

void CalibrationTest::setUp()
{
    Calibration::Input input;
    input.image_size = cv::Size(1920, 1440);
    _calib = std::make_unique<Calibration>(input, CalibrationType::SINGLE, "stereo_calibration.yaml");
}

void CalibrationTest::TestConstructor()
{
    Calibration::Input input;
    input.image_size = cv::Size(1920, 1440);
    Calibration* c = new Calibration(input, CalibrationType::SINGLE, "stereo_calibration.yaml");

    _calib.reset(c);
}

void CalibrationTest::TestRunCalibration()
{
    _calib->RunCalibration();
    // Add CPPUNIT ASSERTs here.
}

void CalibrationTest::TestReadCalibration()
{
    _calib->ReadCalibration();
    // Add CPPUNIT ASSERTs here.
}