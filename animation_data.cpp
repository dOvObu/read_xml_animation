#include "animation_data.h"

double ANIMATION_DATA :: fps{ 24.0 };
std::map<std::string, TEXTURE_DATA>   ANIMATION_DATA :: imp_textures;
std::map<std::string, ANIMATION_DATA> ANIMATION_DATA :: imp_symbols;


void ANIMATION_DATA :: draw(sf::RenderWindow& w, std::map<std::string, std::unique_ptr<sf::Texture>>& texturePool, glm::mat3 general)
{
   if (max_key == 0) current_time = 0.0;
   else {
      current_time += get_dt();
      while (current_time > (double)max_key / fps) {
         current_time -= (double)max_key / fps;
      }
   }


   size_t current_frame = static_cast<size_t>((double)current_time * fps);

   for (auto& layer : layers) {
      KEY_FRAME_DATA* key_frame{ nullptr };
      KEY_FRAME_DATA* next_key_frame{ nullptr };

      auto& frames = layer.key_frames;

      for (size_t idx = 0; idx < frames.size(); ++idx) {
         if (current_frame < frames[idx].idx + frames[idx].duration) {
            key_frame = &frames[idx];
            ++idx;
            if (idx != frames.size()) next_key_frame = &frames[idx];
            break;
         }
      }

      if (key_frame == nullptr) continue;

      double progress = static_cast<double>(current_frame - key_frame->idx) / static_cast<double>(key_frame->duration);

      for (size_t idx = 0; idx < key_frame->elements.size(); ++idx) {
         auto& el = key_frame->elements[idx];

         glm::mat3 matrix = el.matrix;
         // промежуточный кадр
         if (key_frame->animated && next_key_frame != nullptr) {
            ELEMENT_DATA* p_el2{ nullptr };

            if (key_frame->same_idx_flag == 1) {
               p_el2 = &next_key_frame->elements[idx];
            }
            else {
               for (size_t jdx = 0; jdx < next_key_frame->elements.size(); ++jdx) {
                  auto& el2 = next_key_frame->elements[jdx];
                  if (el2.name == el.name) {
                     key_frame->same_idx_flag = jdx == idx;
                  }
               }
            }
            if (p_el2 != nullptr) {
               matrix *= glm::mat3(1.0 - progress) + p_el2->matrix * glm::mat3(progress);
            }
         }

         if (el.type == ELEMENT_DATA::TYPE::BITMAP) {
            sf::VertexArray qd(sf::Quads, 4);
            auto& txtr = imp_textures[el.name];

            qd[0].position = { 0.f, 0.f };
            qd[1].position = { (float)txtr.width, 0.f };
            qd[2].position = { (float)txtr.width, (float)txtr.height };
            qd[3].position = { 0.f, (float)txtr.height };

            qd[0].texCoords = { 0.f, 0.f };
            qd[1].texCoords = { (float)txtr.width, 0.f };
            qd[2].texCoords = { (float)txtr.width, (float)txtr.height };
            qd[3].texCoords = { 0.f, (float)txtr.height };

            // для каждой вершины
            for (int i = 0; i < 4; ++i) {

               glm::vec3 v{ qd[i].position.x, qd[i].position.y, 1.f };

               v = general * matrix * v;

               qd[i].position = sf::Vector2f(v.x, v.y);
            }

            w.draw(qd, texturePool[el.name].get());

         }
         else if (el.type == ELEMENT_DATA::TYPE::SYMBOL) {
            imp_symbols[el.name].draw(w, texturePool, general * matrix);
         }
      }
   }
}


void ANIMATION_DATA :: write(char const name[], std::string const& path)
{
   std::ofstream file(path + '/' + name, std::ios::binary);

   size_t size = imp_textures.size();
   file.write((char const*)&size, sizeof(size_t));

   for (auto& it : imp_textures) {
      size = it.first.size();
      file.write((char const*)&size, sizeof(size_t));
      file.write(it.first.c_str(), size);

      file.write((char const*)&it.second.width, sizeof(unsigned int));
      file.write((char const*)&it.second.height, sizeof(unsigned int));
      file.write(it.second.path.c_str(), it.second.path.size());
   }

   size = imp_symbols.size();
   file.write((char const*)&size, sizeof(size_t));

   for (auto& sym : imp_symbols) {
      size = sym.first.size();
      file.write((char const*)&size, sizeof(size_t));
      file.write(sym.first.c_str(), size);

      size = sym.second.name.size();
      file.write((char const*)&size, sizeof(size_t));
      file.write(sym.second.name.c_str(), size);

      file.write((char const*)&sym.second.max_key, sizeof(size_t));

      write_layer(file, sym.second.layers);
   }
   write_layer(file, layers);
}


void ANIMATION_DATA :: write_layer(std::ofstream& file, std::vector<LAYER_DATA>& layer)
{
   size_t size = layers.size();
   file.write((char const*)&size, sizeof(size_t));

   for (auto& layer : layers) {
      size = layer.name.size();
      file.write((char const*)&size, sizeof(size_t));
      file.write(layer.name.c_str(), size);

      size = layer.key_frames.size();
      file.write((char const*)&size, sizeof(size_t));

      for (auto& frame : layer.key_frames) {
         short flags{ 0 };
         if (frame.animated) flags |= 1;
         if (frame.same_idx_flag) flags |= 2;
         file.write((const char*)&flags, sizeof(short));

         file.write((const char*)&frame.idx, sizeof(size_t));
         file.write((const char*)&frame.duration, sizeof(size_t));

         size = frame.elements.size();
         file.write((const char*)&size, sizeof(size_t));

         for (auto& element : frame.elements) {
            flags = element.type == ELEMENT_DATA::TYPE::BITMAP ? 1 : 0;
            file.write((char const*)&flags, sizeof(short));

            size = element.name.size();
            file.write((char const*)&size, sizeof(size_t));
            file.write(element.name.c_str(), element.name.size());

            for (int i = 0; i < 3; ++i) {
               file.write((char const*)&element.matrix[i].x, sizeof(float));
               file.write((char const*)&element.matrix[i].y, sizeof(float));
            }

            file.write((char const*)&element.translp.x, sizeof(float));
            file.write((char const*)&element.translp.y, sizeof(float));
         }
      }
   }
}


void ANIMATION_DATA :: init_max_key()
{
   for (auto& sym : imp_symbols) {
      for (auto& layer : sym.second.layers) {
         size_t tmp_max_key = layer.key_frames.back().idx + layer.key_frames.back().duration;
         if (tmp_max_key > sym.second.max_key) {
            sym.second.max_key = tmp_max_key;
         }
      }
   }
   for (auto& layer : layers) {
      size_t tmp_max_key = layer.key_frames.back().idx + layer.key_frames.back().duration;
      if (tmp_max_key > max_key) {
         max_key = tmp_max_key;
      }
   }
}


void ANIMATION_DATA :: init_textures(std::string path, std::map<std::string, std::unique_ptr<sf::Texture>>& texturePool)
{
   for (auto& d_texture : imp_textures) {
      if (texturePool.count(d_texture.first) == 0) {
         texturePool[d_texture.first] = std::unique_ptr<sf::Texture>(d_texture.second.ptr = new sf::Texture());
         d_texture.second.ptr->loadFromFile(path + '/' + d_texture.second.path);
      }
   }
   init_max_key();
}



