#ifndef UGLY_XML_PARSER_H
#define UGLY_XML_PARSER_H
#include <iostream>
#include <memory>
#include "animation_data.h"

namespace READER {

   struct XML_NODE {
      std::string id;
      std::map<std::string, std::string> fields;
      std::vector<std::unique_ptr<XML_NODE>> childs;

      XML_NODE* add_child(const char id[]);

      inline std::string& get(const char id[]) { return fields[id]; }
      inline std::string& get(const char id[], std::string& alter) { return fields.count(id) != 0 ? fields[id] : alter; }
      
      static std::string depth;

      void print();
      inline void add_field(const std::string& id, const std::string& content) { fields[id] = content; }
      inline XML_NODE& operator[](int idx) { return (*this)[(size_t)idx]; }
      inline XML_NODE& operator[](size_t idx) { return *(childs[idx].get()); }
      XML_NODE& operator[](const char id[]);
   };


   void ugly_xml_parser(std::ifstream& file, XML_NODE& node);


   ANIMATION_DATA read(const char path[], const char name[], bool first = true);

}

#endif // UGLY_XML_PARSER_H
