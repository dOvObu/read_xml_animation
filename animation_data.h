#ifndef ANIMATION_DATA_H
#define ANIMATION_DATA_H
#include <string>
#include <fstream>
#include <glm/glm.hpp>
#include <SFML/Graphics.hpp>


static double& get_dt()
{
   static double dt{ 0.f };
   return dt;
}


static void print_mat(glm::mat3& m)
{
   //std::cout << "[ " << m[0][0] << ", " << m[0][1] << ", " << m[0][2] << std::endl;
   //std::cout << "  " << m[1][0] << ", " << m[1][1] << ", " << m[1][2] << std::endl;
   //std::cout << "  " << m[2][0] << ", " << m[2][1] << ", " << m[2][2] << " ]\n" << std::endl;
}


struct TEXTURE_DATA {
   std::string path;
   sf::Texture* ptr{ nullptr }; // pointer to element of texturePool
   unsigned height;
   unsigned width;
};


struct ELEMENT_DATA {
   enum class TYPE {
      BITMAP
   ,  SYMBOL
   } type;
   std::string name;
   glm::mat3 matrix{ 1.f, 0.f, 0.f,  0.f, 1.f, 0.f,  0.f, 0.f, 1.f };
   glm::vec2 translp{0.f, 0.f};
};


struct KEY_FRAME_DATA {
   size_t idx;
   size_t duration{ 1 };
   std::vector<ELEMENT_DATA> elements;
   bool animated{ false };
   bool same_idx_flag{ false };
};


struct LAYER_DATA {
   std::string name;
   std::vector<KEY_FRAME_DATA> key_frames;
};


struct ANIMATION_DATA {
   static double fps;
   static std::map<std::string, TEXTURE_DATA> imp_textures;
   static std::map<std::string, ANIMATION_DATA> imp_symbols;
   std::string name{ "" };

   std::vector<LAYER_DATA> layers;
   size_t max_key{ 0 };
   double current_time{ 0.0 };


   void init_max_key();
   void init_textures(std::string path, std::map<std::string, std::unique_ptr<sf::Texture>>& texturePool);
   void inline draw(sf::RenderWindow& w, std::map<std::string, std::unique_ptr<sf::Texture>>& texturePool)
   {
      draw(w, texturePool, {1.f,0.f,0.f, 0.f,1.f,0.f, 0.f,0.f,1.f});
   }

   void write(char const name[], std::string const& path);
private:
   void write_layer(std::ofstream& file, std::vector<LAYER_DATA>& layer);
   void draw(sf::RenderWindow& w, std::map<std::string, std::unique_ptr<sf::Texture>>& texturePool, glm::mat3 general);
};

#endif // ANIMATION_DATA_H
