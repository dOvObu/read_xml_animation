#include <fstream>
#include <iostream>
#include <utility>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <glm/glm.hpp>
#include <windows.h>

#include "animation_data.h"
#include "ugly_xml_parser.h"


std::map<std::string, std::unique_ptr<sf::Texture>> texturePool;


// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
int cyclic_bound(int x, int from, int to)
{
   int d = to - from;
   while (x >=  to) x -= d;
   while (x < from) x += d;
   return x;
}


void updateViewPosition(sf::View& v)
{
   float speed = 1000.f * get_dt();

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  v.move(-speed, 0.f);
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) v.move( speed, 0.f);
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  v.move(0.f,  speed);
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    v.move(0.f, -speed);

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
      auto size = v.getSize();
      v.setSize(size.x - size.x * get_dt(), size.y - size.y * get_dt());
   }
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
      auto size = v.getSize();
      v.setSize(size.x + size.x * get_dt(), size.y + size.y * get_dt());
   }
}


void initMarkerAndView(sf::View& v, sf::RectangleShape& r)
{
   v.setSize(800, 600);
   v.setCenter(0, 0);
   r.setPosition(0, 0);
   r.setSize({ 5, 5 });
   r.setFillColor(sf::Color::Yellow);
}

// -----------------------------------------------------------------------------
std::string current_path;
bool new_current_path = false;


void DropData(IDataObject* pDataObject)
{
   FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
   STGMEDIUM stgmed;
   
   // проверка содержит ли объект TEXT положеный как HGLOBAL
   if (pDataObject->QueryGetData(&fmtetc) == S_OK) {
      // ”ра! данные на месте, так что надо брать!
      if (pDataObject->GetData(&fmtetc, &stgmed) == S_OK) {
         // запрашиваем и блокируем данные как HGLOBAL
         HDROP hdrop = (HDROP)GlobalLock(stgmed.hGlobal);

         if (hdrop == NULL) {
            GlobalUnlock(stgmed.hGlobal);
            return;
         }

         UINT  uNumFiles;
         TCHAR szNextFile[MAX_PATH];
         // ”знаЄм число сброшенных файлов.
         uNumFiles = DragQueryFile(hdrop, -1, NULL, 0);

         if (uNumFiles == 1) {//  |  этому
         //  получили доступ к    v  файлу   сохранив результаты в szNextFile.
            DragQueryFile(hdrop,  0, szNextFile, MAX_PATH);
            current_path = szNextFile;
            
            if (current_path.substr(current_path.find('\\')).find('.') == std::string::npos) {
               new_current_path = true;
            }
         }
         // разблокировали глобальные данные
         GlobalUnlock(stgmed.hGlobal);
         // освободили пам€ть
         ReleaseStgMedium(&stgmed);
      }
   }
}


class DROP_ANIME_DIR_TARGET : public IDropTarget {
public:
   // IUnknown implementation
   HRESULT __stdcall QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
   {
      if (riid == IID_IDropTarget || riid == IID_IUnknown)
      {
         AddRef();
         *ppvObject = this;
         return S_OK;
      }
      else
      {
         *ppvObject = 0;
         return E_NOINTERFACE;
      }
   }
   ULONG STDMETHODCALLTYPE AddRef (void) override {return S_OK;}
   ULONG STDMETHODCALLTYPE Release(void) override {return S_OK;}

   // IDropTarget implementation
   HRESULT __stdcall DragEnter(__RPC__in_opt IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, __RPC__inout DWORD* pdwEffect) override {return S_OK;}
   HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, __RPC__inout DWORD* pdwEffect) override {return S_OK;}
   HRESULT __stdcall DragLeave(void) override {return S_OK;}
   HRESULT __stdcall Drop(__RPC__in_opt IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, __RPC__inout DWORD* pdwEffect) override
   {
      if (QueryDataObject(pDataObj)) {
         DropData(pDataObj);
      }
      *pdwEffect = DROPEFFECT_NONE;
      return S_OK;
   }
private:
   // internal helper function
   bool QueryDataObject(IDataObject* pDataObject)
   {
      FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
      // поддерживает ли объект CF_TEXT использу€ HGLOBAL?
      return pDataObject->QueryGetData(&fmtetc) == S_OK ? true : false;
   }

   // Private member variables
   HWND m_hWnd;
};


template<typename T> bool setAsDropTarget(sf::RenderWindow& w)
{
   static T cdr;
   return SUCCEEDED(OleInitialize(NULL)) && SUCCEEDED(RegisterDragDrop(w.getSystemHandle(), &cdr));
}


void revokeDropTarget(sf::RenderWindow& w)
{
   RevokeDragDrop(w.getSystemHandle());
   OleUninitialize();
}

// -----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
   ANIMATION_DATA data;

   sf::RenderWindow w({ 800u,600u }, "animation isn't loaded");
   if (!setAsDropTarget<DROP_ANIME_DIR_TARGET>(w)) return 0;

   sf::View v;
   sf::RectangleShape r;
   initMarkerAndView(v, r);

   while (w.isOpen())
   {
      auto start_time = std::chrono::system_clock::now();
      
      sf::Event e;
      while (w.pollEvent(e)) {
         if (e.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
            w.close();
         }
         if (e.type == sf::Event::KeyPressed) {
            if (e.key.control && !current_path.empty()) {
               if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                  data.write((current_path.substr(current_path.find_last_of('/') + 1) + ".an").c_str(), current_path);
               }
               if (sf::Keyboard::isKeyPressed(sf::Keyboard::U)) {
                  data = READER::read(current_path.c_str(), "DOMDocument.xml");
               }
            }
         }
      }

      if (new_current_path) {
         w.setTitle(current_path);
         data = READER::read(current_path.c_str(), "DOMDocument.xml");
         data.init_textures(current_path, texturePool);
         new_current_path = false;
      }
      updateViewPosition(v);

      w.setView(v);
      w.clear();
      w.draw(r);
      data.draw(w, texturePool);
      w.display();

      get_dt() = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - start_time).count()) / 1000000.0;
   }

   revokeDropTarget(w);
}

