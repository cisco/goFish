#pragma once

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "Processor.h"

class ProcessorTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ProcessorTest);
    CPPUNIT_TEST(TestConstructor);
    CPPUNIT_TEST(TestProcessVideo);
    CPPUNIT_TEST(TestTriangulatePoints);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void TestConstructor();
    void TestProcessVideo();
    void TestTriangulatePoints();
    
private:
    std::unique_ptr<Processor> _proc;

};