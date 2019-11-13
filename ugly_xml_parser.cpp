#include "ugly_xml_parser.h"


std::string READER::XML_NODE::depth;


ANIMATION_DATA READER::read(const char path[], const char name[], bool first = true)
{
   std::string pth = path;
   std::ifstream file(pth + '/' + name);
   XML_NODE root;
   ugly_xml_parser(file, root);
   XML_NODE& dom = root[0];

   ANIMATION_DATA data;
   data.name = dom.get("name", data.name);
   if (first) {
      std::string def_fps = "24.0";
      data.fps = std::stod(dom.get("frameRate", def_fps));
   }

   auto& media = dom["media"];
   if (&media != &dom) {
      auto& bitmaps = media.childs;
      for (auto& it : bitmaps) {
         data.imp_textures.insert({
            (*it).get("name")
         ,  TEXTURE_DATA {
               (*it).get("sourceExternalFilepath")
            ,  nullptr
            ,  static_cast<unsigned>(std::stoul((*it).get("frameBottom")) / 20)
            ,  static_cast<unsigned>(std::stoul((*it).get("frameRight")) / 20)
            }
            });
      }
   }

   auto& sym = dom["symbols"];
   if (&sym != &dom) {
      auto& symbols = sym.childs;
      for (auto& it : symbols) {
         auto&& sub_root = read(pth.c_str(), ("./LIBRARY/" + (*it).get("href")).c_str(), false);
         data.imp_symbols.insert({
            sub_root.name
         ,  sub_root
            });
      }
   }

   auto* timel = &dom["timelines"];

   if (timel == &dom) timel = &dom["timeline"];

   if (timel != &dom) {
      auto& layers = (*timel)[0]["layers"].childs;
      static std::string zerof_str = "0.0";
      static std::string onef_str = "1.0";

      for (auto& it : layers) {
         data.layers.push_back(LAYER_DATA());
         auto& layer = data.layers.back();
         layer.name = it->get("name");
         auto& frames = (*it)["frames"].childs;
         for (auto& jt : frames) {
            layer.key_frames.push_back(KEY_FRAME_DATA());
            auto& key_frame = layer.key_frames.back();
            if ((*jt).fields.count("keyMode")) key_frame.animated = (*jt).get("keyMode") == "22017";
            if ((*jt).fields.count("index")) key_frame.idx = std::stoul((*jt).get("index"));
            if ((*jt).fields.count("duration")) key_frame.duration = std::stoul((*jt).get("duration"));
            auto& elements = (*jt)["elements"].childs;
            for (auto& kt : elements) {
               key_frame.elements.push_back(ELEMENT_DATA());
               auto& element = key_frame.elements.back();

               if (kt->id == "DOMSymbolInstance") element.type = ELEMENT_DATA::TYPE::SYMBOL;
               else if (kt->id == "DOMBitmapInstance") {
                  element.type = ELEMENT_DATA::TYPE::BITMAP;
               }

               element.name = kt->get("libraryItemName");
               {
                  auto* mx = &(*kt)["matrix"];
                  if (mx != kt.get()) {
                     auto& mtx = (*mx)[0];
                     element.matrix = glm::mat3{
                        std::stof(mtx.get("a" , onef_str)), std::stof(mtx.get("b" , zerof_str)), 0.f
                     ,  std::stof(mtx.get("c" , zerof_str)), std::stof(mtx.get("d" , onef_str)) , 0.f
                     ,  std::stof(mtx.get("tx", zerof_str)), std::stof(mtx.get("ty", zerof_str)), 1.f
                     };
                  }
               }

               {
                  auto* trpt = &(*kt)["transformationPoint"];
                  if (trpt != kt.get()) {
                     auto& p = (*trpt)[0];
                     element.translp = glm::vec2{
                        std::stof(p.get("x", zerof_str))
                     ,  std::stof(p.get("y", zerof_str))
                     };
                  }
               }
            }
         }
      }
   }
   file.close();

   return data;
}


void READER::ugly_xml_parser(std::ifstream& file, READER::XML_NODE& node)
{
   bool is_str = false, is_tag = false, is_clsr = false, is_frst = false;
   std::string id_buff, buff;
   std::vector<XML_NODE*> nodes;
   nodes.push_back(&node);

   while (!file.eof()) {
      char c;
      file.get(c);
      if (c == ' ' || c == '\t' || c == '\n') continue;
      if (is_clsr) { if (c == '>') { is_tag = is_frst = is_clsr = false; } continue; }
      if (is_str) {
         if (c == '"') { is_str = false; nodes.back()->add_field(id_buff, buff); id_buff.clear(); buff.clear(); continue; }
         buff.push_back(c);
      }
      else {
         if (c == '"') { buff.clear(); is_str = true;         continue; }
         if (c == '<') { is_frst = is_tag = true;             continue; }
         if (c == '>') { is_frst = is_tag = is_clsr = false;  continue; }
         if (is_tag) {
            if (c == '/') { is_clsr = true, nodes.pop_back(); continue; } // heh
            if (is_frst) {
               is_frst = false;
               std::string id;
               file >> id;
               id.insert(std::begin(id), c);
               if (id.back() == '>') id.pop_back();
               if (id.back() == '/') {
                  is_frst = is_tag = false;
                  is_clsr = true;
                  id.pop_back();
               }
               nodes.push_back(nodes.back()->add_child(id.c_str()));
               if (is_clsr) nodes.pop_back(), is_clsr = false;
               if (!is_tag) continue;
            }
            else {
               if (c == '=') id_buff = buff, buff.clear(); else buff.push_back(c);
               continue;
            }
         }
      }
   }
}


using namespace READER;

XML_NODE*  XML_NODE :: add_child(const char id[])
{
   XML_NODE* res;
   childs.push_back(std::unique_ptr<XML_NODE>(res = new XML_NODE()));
   res->id = id;
   return res;
}


void XML_NODE :: print()
{
   std::cout << depth << id << " {" << std::endl;
   depth += ' ';
   for (auto& it : fields) std::cout << depth << it.first << " = " << it.second << std::endl;
   depth.pop_back();
   std::cout << depth << "::" << std::endl;
   depth += ' ';
   for (auto& it : childs) it->print();
   depth.pop_back();
   std::cout << depth << '}' << std::endl;

}


XML_NODE& XML_NODE :: operator[](const char id[])
{
   XML_NODE* res = this;
   for (auto& c : childs) if (c->id == id) { res = c.get(); break; }
   return *res;
}
