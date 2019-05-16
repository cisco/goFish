#include "JsonBuilder.h"
#include <cstdarg>

JSON::JSON(std::string ObjectName)
: json_string_{ "{}" }, name_{ ObjectName } {}

JSON::JSON(const JSON& j)
{
    this->name_ = j.name_;
    this->json_string_ = j.json_string_;
    this->subobjects_ = j.subobjects_;
}

JSON::~JSON()
{
    this->json_string_ = "";
    this->name_ = "";
    this->subobjects_.clear();
}

JSON::JSON(std::string ObjectName, std::map<std::string, std::string> Object)
{
    name_ = ObjectName;
    json_string_ = "{";
    
    json_string_ += "\"" + name_ + "\" : {";
    auto it = Object.begin();
    for(auto e : Object)
    {
        ++it;
        json_string_ += "\"" + e.first + "\"";
        json_string_ += ":";
        if(std::atof(e.second.c_str())) json_string_ += e.second;
        else json_string_ += "\"" + e.second + "\"";
        if(it != Object.end()) json_string_ += ",";
    }
    json_string_ += "}";

    json_string_ += "}";
}

JSON::JSON(std::string ObjectName, std::vector<JSON> Object)
{
    name_ = ObjectName;
    json_string_ = "{";

    json_string_ += "\"" + name_ + "\" : [";
    auto it = Object.begin();
    for(auto e : Object)
    {
        ++it;
        json_string_ += e.GetJSON();
        if(it != Object.end()) json_string_ += ",";
    }
    json_string_ += "]";

    json_string_ += "}";
}

// Potentially interesting way to dynamically add numerous events. Might need some research.
JSON::JSON(std::string ObjectName, JSON Objects...)
{
    name_ = ObjectName;
    json_string_ = "{";
    json_string_ += "\"" + name_ + "\" : ";
    va_list args;
    va_start(args, Objects);

   // JSON e = va_arg(args, JSON);
   // json_string_ += e.GetJSON();

    va_end(args);
    json_string_ += "}";
}

void JSON::AddKeyValue(std::string Key, std::string Value)
{
    subobjects_.insert(std::make_pair(Key, Value));
}

void JSON::AddObject(JSON& Object)
{
    subobjects_.insert(make_pair(Object.GetName(), Object.GetJSON()));
}

void JSON::BuildJSONObjectArray()
{
    json_string_ = "{";

    json_string_ += "\"" + name_ + "\" : [";
    auto it = subobjects_.begin();
    for(auto e : subobjects_)
    {
        ++it;
        json_string_ += e.second;
        if(it != subobjects_.end()) json_string_ += ",";
    }
    json_string_ += "]";

    json_string_ += "}";
}

std::string JSON::GetJSON() const
{
    return this->json_string_;
}

std::string JSON::GetName() const
{
    return this->name_;
}

std::vector<std::string> JSON::GetSubobjectNames()
{
    std::vector<std::string> tempNames;
    for(auto o : subobjects_)
        if(o.first != "") tempNames.push_back(o.first);
    
    return tempNames;
}


