#pragma once

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "Tracker.h"

class TrackerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TrackerTest);
    CPPUNIT_TEST(TestConstructor);
    CPPUNIT_TEST(TestCreateMask);
    CPPUNIT_TEST(TestGetObjectContours);
    CPPUNIT_TEST(TestCheckForActivity);
    CPPUNIT_TEST(TestGetCascades);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void TestConstructor();
    void TestCreateMask();
    void TestGetObjectContours();
    void TestCheckForActivity();
    void TestGetCascades();
    
private:
    std::unique_ptr<Tracker> _tracker;

};