#pragma once

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "JsonBuilder.h"

class JSONTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(JSONTest);
    CPPUNIT_TEST(TestConstructors);
    CPPUNIT_TEST(TestAddKeyValue);
    CPPUNIT_TEST(TestAddObject);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void TestConstructors();
    void TestAddKeyValue();
    void TestAddObject();

private:
    std::unique_ptr<JSON> _json;

};