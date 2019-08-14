#pragma once

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "Calibration.h"

class CalibrationTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CalibrationTest);
    CPPUNIT_TEST(TestConstructor);
    CPPUNIT_TEST(TestRunCalibration);
    CPPUNIT_TEST(TestReadCalibration);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void TestConstructor();
    void TestRunCalibration();
    void TestReadCalibration();
    
private:
    std::unique_ptr<Calibration> _calib;

};