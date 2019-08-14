#include "includes/JsonBuilder.h"
#include <cstdarg>
#include <cstdlib>

JSON::JSON(std::string ObjectName)
: _json_string{ "{}" }, _name{ ObjectName } 
{
}

JSON::JSON(const JSON& j)
{
    this->_name = j._name;
    this->_json_string = j._json_string;
    this->_key_val_pairs = j._key_val_pairs;
}

JSON::~JSON()
{
    this->_json_string = "";
    this->_name = "";
    this->_key_val_pairs.clear();
}

JSON::JSON(std::string ObjectName, std::map<std::string, std::string> Object)
{
    _name = ObjectName;
    _key_val_pairs = Object;
    BuildJSONObject();
}

// Potentially interesting way to dynamically add numerous events. Might need some research.
JSON::JSON(std::string ObjectName, JSON Objects...)
{
    _name = ObjectName;
    _json_string += "\"" + _name + "\" : ";
    va_list args;
    va_start(args, Objects);

   // JSON e = va_arg(args, JSON);
   // _json_string += e.GetJSON();

    va_end(args);
}

void JSON::AddKeyValue(std::string Key, std::string Value)
{
    _key_val_pairs.insert(std::make_pair(Key, Value));
}

void JSON::AddObject(JSON& Object)
{
    if(Object.GetJSON() != "{}")
        _subobjects.push_back(Object);
}

void JSON::AddObject(const JSON& Object)
{
    if(Object.GetJSON() != "{}")
        _subobjects.push_back(Object);
}

void JSON::BuildJSONObject()
{
    _json_string = "{";
    auto it = _key_val_pairs.begin();
    for(auto e : _key_val_pairs)
    {
        ++it;
        _json_string += "\"" + e.first + "\":";
        
        char* err;
        std::strtod(e.second.c_str(), &err);
        _json_string += *err == '\0' ? e.second : "\"" + e.second + "\"";

        if(it != _key_val_pairs.end()) _json_string += ",";
    }

    auto jt = _subobjects.begin();
    if(!_key_val_pairs.empty() && it != _key_val_pairs.end()) _json_string += ",";
    for(auto e : _subobjects)
    {
        ++jt;
        _json_string += e.GetJSON().substr(1, e.GetJSON().size() - 2);
        if(jt != _subobjects.end()) _json_string += ",";
    }
    
    _json_string += "}";
}

void JSON::BuildJSONObjectArray()
{
    _json_string = "[";
    auto it = _key_val_pairs.begin();
    for(auto e : _key_val_pairs)
    {
        ++it;
        _json_string += "{\"" + e.first + "\":";
        
        char* err;
        std::strtod(e.second.c_str(), &err);
        _json_string += *err == '\0' ? e.second : "\"" + e.second + "\"";
        _json_string += "}";
        if(it != _key_val_pairs.end()) _json_string += ",";
    }

    auto jt = _subobjects.begin();
    if(!_key_val_pairs.empty() && it != _key_val_pairs.end()) _json_string += ",";
    for(auto e : _subobjects)
    {
        ++jt;
        _json_string += e.GetJSON();
        if(jt != _subobjects.end()) _json_string += ",";
    }
    _json_string += "]";
}

std::string JSON::GetJSON() const
{
    
    return this->_name != "" ? "{\"" + this->_name + "\":" + this->_json_string + "}" : "{}";
}

std::string JSON::GetName() const
{
    return this->_name;
}

std::vector<std::string> JSON::GetSubobjectNames()
{
    std::vector<std::string> tempNames;
    for(auto o : _key_val_pairs)
        if(o.first != "") tempNames.push_back(o.first);
    for(auto o : _subobjects)
        if(o.GetName() != "") tempNames.push_back(o.GetName());
    
    return tempNames;
}
