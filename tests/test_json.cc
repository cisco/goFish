#include "test_json.h"

void JSONTest::setUp()
{
    _json = std::make_unique<JSON>("");
}

void JSONTest::TestConstructors()
{
    {
        std::string name = "";
        auto j = new JSON(name);
        _json.reset(j);

        CPPUNIT_ASSERT_EQUAL(_json->GetName(), name);
        CPPUNIT_ASSERT_EQUAL(_json->GetJSON(), std::string("{}"));
    }

    {
        std::string name = "json";
        auto j = new JSON(name);
        _json.reset(j);

        CPPUNIT_ASSERT_EQUAL(_json->GetName(), name);
        CPPUNIT_ASSERT_EQUAL(_json->GetJSON(), "{\"" + name + "\":{}}");
    }

    {
        std::string name = "json";
        std::map<std::string, std::string> val;
        val.insert(std::make_pair("sub1", "val1"));
        val.insert(std::make_pair("sub2", "val2"));
        val.insert(std::make_pair("sub3", "val3"));

        auto j = new JSON(name, val);
        _json.reset(j);

        CPPUNIT_ASSERT_EQUAL(_json->GetName(), name);
        CPPUNIT_ASSERT_EQUAL(_json->GetJSON(), "{\"" + name + "\":{\"sub1\":\"val1\",\"sub2\":\"val2\",\"sub3\":\"val3\"}}");
    }
}

void JSONTest::TestAddKeyValue()
{
    std::string name = "json";
    auto j = new JSON(name);
    _json.reset(j);

    _json->AddKeyValue("sub1", "val1");
    
    _json->BuildJSONObject();
    CPPUNIT_ASSERT_EQUAL(_json->GetJSON(), "{\"" + name + "\":{\"sub1\":\"val1\"}}");

    _json->BuildJSONObjectArray();
    CPPUNIT_ASSERT_EQUAL(_json->GetJSON(), "{\"" + name + "\":[{\"sub1\":\"val1\"}]}");
}

void JSONTest::TestAddObject()
{
    std::string name = "json";
    auto j = new JSON(name);
    _json.reset(j);

    std::map<std::string, std::string> val;
    val.insert(std::make_pair("sub1", "val1"));

    auto j2 = new JSON("json2", val);
    _json->AddObject(*j2);

    // Test build normal object.
    _json->BuildJSONObject();
    CPPUNIT_ASSERT_EQUAL(_json->GetJSON(), "{\"" + name + "\":{\"json2\":{\"sub1\":\"val1\"}}}");

    // Test build array of objects.
    _json->BuildJSONObjectArray();
    CPPUNIT_ASSERT_EQUAL(_json->GetJSON(), "{\"" + name + "\":[{\"json2\":{\"sub1\":\"val1\"}}]}");

    // Test build array of key value pairs.
    _json.reset(j2);
    _json->BuildJSONObjectArray();
    CPPUNIT_ASSERT_EQUAL(_json->GetJSON(), std::string("{\"json2\":[{\"sub1\":\"val1\"}]}"));
}