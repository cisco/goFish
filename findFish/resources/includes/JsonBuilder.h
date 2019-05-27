#pragma once

#include <string>
#include <map>
#include <vector>

class JSON
{
 public:
    JSON(std::string ObjectName);
    ~JSON();
    JSON(const JSON&);
    JSON(std::string ObjectName, std::map<std::string, std::string> Object);
    JSON(std::string ObjectName, JSON Objects...);

    void AddKeyValue(std::string Key, std::string Value);
    void AddObject(JSON);
    void AddObject(JSON&);

    void BuildJSONObject();
    void BuildJSONObjectArray();

    std::string GetJSON() const;
    std::string GetName() const;
    std::vector<std::string> GetSubobjectNames();
    

 private:
    std::string json_string_;
    std::string name_;
    std::map<std::string, std::string> subobjects_;
};

