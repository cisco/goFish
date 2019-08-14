#pragma once

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "EventDetector.h"

class EventTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(EventTest);
    CPPUNIT_TEST(TestConstructors);
    CPPUNIT_TEST(TestStartEvent);
    CPPUNIT_TEST(TestCheckFrame);
    CPPUNIT_TEST(TestEndEvent);
    CPPUNIT_TEST(TestGetAsJSON);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void TestConstructors();
    void TestStartEvent();
    void TestCheckFrame();
    void TestEndEvent();
    void TestGetAsJSON();
    
private:
    std::unique_ptr<EventBuilder> _event;

};