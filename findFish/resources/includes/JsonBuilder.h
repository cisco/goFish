/// \author Tomas Rigaux
/// \date May 15, 2019
///
/// A helper class for dynamically building valid JSON strings out of objects
/// passed in. The class currently does not use templates, and instead relies
/// on the user to convert their objects to strings first before passing them
/// into the object.

#pragma once

#include <string>
#include <map>
#include <vector>

/// A JSON builder which takes in values as strings an formats them into
/// valid JSON.
class JSON
{
 public:
   /// Constructor which sets the name of the object.
   /// \param[in] ObjectName The name of the JSON object.
   JSON(std::string ObjectName);

   /// Default destructor.
   ~JSON();

   /// Copy constructor.
   /// \param[in, out] obj The JSON object to copy from.
   JSON(const JSON& obj);

   /// Constructs a JSON object with a name and populates it with values.
   /// \param[in] ObjectName The name of the object.
   /// \param[in] Object The default values to populate the object.
   JSON(std::string ObjectName, std::map<std::string, std::string> Object);

   /// Constructor which creates a JSON object of JSON objects.
   /// \param[in] ObjectName The name of the object.
   /// \param[in] Objects... A variadic amount of JSON objects to populate the 
   ///                       object.
   JSON(std::string ObjectName, JSON Objects...);

   /// Creates a new key-value pair and appends it to the object.
   /// \param[in] Key The value of the key.
   /// \parampin] Value The value associated with the key.
   void AddKeyValue(std::string Key, std::string Value);

   /// Adds a new JSON Object to this JSON.
   /// \param[in] obj The JSON object to be appended.
   void AddObject(JSON obj);

   /// Adds a premade JSON Object to this JSON.
   /// \param[in, out] obj The JSON object to be appended.
   void AddObject(JSON& obj);

   /// Builds the formatted JSON object as valid JSON.
   void BuildJSONObject();

   /// Builds a formatted JSON object array as valid JSON.
   void BuildJSONObjectArray();

   /// Get the actual valid JSON string.
   /// \return Formatted JSON string.
   std::string GetJSON() const;

   /// Get the JSON object's name.
   /// \return The name of the object.
   std::string GetName() const;

   /// Get the names of child objects.
   /// \return A vector of all subobject names.
   std::vector<std::string> GetSubobjectNames();
    

 private:
    std::string json_string_;
    std::string name_;
    std::map<std::string, std::string> subobjects_;
};

