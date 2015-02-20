#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <Windows.h>
#include <regex>
#include <thread>
#include <stdarg.h>
#include <math.h>

#include "rgbe.h"

struct FileNode
{  
  enum Type
  {
    NONE,
    FOLDER,
    FILE
  };  

  FileNode(Type A_Type, std::string A_Name)
    :FileType(A_Type)
    ,Name(A_Name)
  {}

  Type FileType;
  std::string Name;
};

class FolderCrawler
{
public:
  void Crawl(std::string Path)
  {    
    CrawlFolder(Path, 0);
  }

private:

  void CrawlFolder(std::string Path, int Level)
  {    
    std::vector<FileNode> Files = ListCurrentFolder(Path + "\\*");
    for (int i = 0; i < Files.size(); i++)
    {
      switch (Files[i].FileType)
      {
      case FileNode::FILE: 
        ProcessFile(Path + "\\" + Files[i].Name, Level + 1);  
        break;
      case FileNode::FOLDER: 
        ProcessFile(Path + "\\" + Files[i].Name, Level + 1); CrawlFolder(Path + "\\" + Files[i].Name, Level + 1); break;
      }

    }
  }

  std::vector<FileNode> ListCurrentFolder(std::string Path)
  {
    std::vector<FileNode> Files;

    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    
    hFind = FindFirstFile(Path.c_str(), &FindFileData);
    while (hFind != INVALID_HANDLE_VALUE)
    {
      if (std::string(FindFileData.cFileName) != "." && std::string(FindFileData.cFileName) != "..")
      {
        FileNode Node = FileNode(
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? FileNode::FOLDER : FileNode::FILE,
          FindFileData.cFileName);
        Files.push_back(Node);
      }
        
      if (!FindNextFile(hFind, &FindFileData))
      {
        hFind = INVALID_HANDLE_VALUE;
      }
    }

    return Files;
  }

  virtual void ProcessFile(std::string Name, int Level) {};
  virtual void ProcessFolder(std::string Name, int Level) {};
};

class StringReplacer : public FolderCrawler
{
public:
  StringReplacer(std::string A_OldWord, std::string A_NewWord)
    :OldWord(A_OldWord)
    ,NewWord(A_NewWord)
  {
  }
  std::string OldWord;
  std::string NewWord;
protected:
  virtual void ProcessFile(std::string Name, int Level)
  {
    std::ifstream InFile(Name);
    std::ofstream OutFile(Name + ".temp");
    std::string Line;
    
    int WordLength = OldWord.size();

    while (getline(InFile, Line))
    {
      int Start = std::string::npos;
      do
      {
        Start = Line.find(OldWord);
        if (Start != std::string::npos)
          Line.replace(Start, WordLength, NewWord);
      } while (Start != std::string::npos);

      OutFile << Line << std::endl;
    }

    InFile.close();
    OutFile.close();

    remove(Name.c_str());
    rename((Name + ".temp").c_str(), Name.c_str());
  };

  virtual void ProcessFolder(std::string Name, int Level)
  {
    
  };
};

class FolderPrinter : public FolderCrawler
{
protected:
  void PrintName(std::string Name, int Level, bool bIsFile)
  {
    std::cout << std::endl;
    for (int i = 0; i < Level; i++)
    {
      std::cout << " ";
    }

    if (bIsFile)
      std::cout << "-" << Name.c_str();
    else
      std::cout << Name.c_str();
  }

  virtual void ProcessFile(std::string Name, int Level) 
  {
    PrintName(Name, Level, true);
  };

  virtual void ProcessFolder(std::string Name, int Level) 
  {
    PrintName(Name, Level, false);
  };
};

template<typename Ret, typename Param0>
class ICallback
{
public:
  virtual Ret Invoke(Param0 P0) = 0;
};

template<typename Ret, typename Param0>
class StaticCallback : public ICallback<Ret, Param0>
{
public:
  typedef Ret(*TFunc)(Param0);  

  StaticCallback(TFunc func) : _func(func) {};
  ~StaticCallback() { delete _func; }

  virtual Ret Invoke(Param0 P0)
  {
    return (*_func)(P0);
  }
private:
  TFunc _func;
};

template<typename Ret, typename Param0, typename T, typename Method>
class MethodCallback : public ICallback<Ret, Param0>
{
public:
  typedef Ret(T::*TFunc)(Param0);
  
  MethodCallback(T* object, Method method) : _object(object), _func(method) {}
  ~MethodCallback() { delete _func; }

  virtual Ret Invoke(Param0 P0)
  {
    return (_object->*_func)(P0);
  }
private:
  T* _object;
  TFunc _func;
};

template<typename Ret, typename Param0>
class Delegate
{
  ICallback<Ret, Param0>* _callback;
public:
  Delegate(Ret(*func)(Param0)) : _callback(new StaticCallback<Ret, Param0>(func)) {}

  template <typename T, typename Method>
  Delegate(T* obj, Method method) : _callback(new MethodCallback<Ret, Param0, T, Method>(obj, method)){}

  ~Delegate() { delete _callback; }

  Ret operator()(Param0 param)
  {
    return _callback->Invoke(param);
  }
};

enum Event
{
  EVENT_STUFF_IS_DONE
};

class EventManager
{
  typedef std::map<Event, Delegate<void, int>*> TListeners;
public:
  ~EventManager() 
  {
    for (TListeners::iterator i = Listeners.begin(); i != Listeners.end(); ++i)
      delete i->second;
  }

  void ReceiveMessage(Event A_Event)
  {
    if (Listeners[A_Event])
    {
      (*Listeners[A_Event])(1);
    }
  }

  void AddListener(Event A_Event, Delegate<void, int>* Delegate)
  {
    Listeners[A_Event] = Delegate;
  }

  TListeners Listeners;
};

static EventManager* GlobalEventManager;

class Sender
{
public:
  void DoStuff()
  {
    std::cout << std::endl << "Sender did stuff";
    GlobalEventManager->ReceiveMessage(EVENT_STUFF_IS_DONE);
  }
};

class Receiver
{
public:
  Receiver()
  {
    GlobalEventManager->AddListener(EVENT_STUFF_IS_DONE, new Delegate<void, int>(this, &Receiver::ReactOnDoStuff));
  }

  void ReactOnDoStuff(int SomeParam)
  {
    std::cout << std::endl << "Recevier has reacted";
  }
};

int CelsiusFahrenheitConverter()
{
  struct
  {
    int Lower;
    int High;
    int Step;
  } Temperature;

  std::cout << std::endl << "Lower limit: ";
  std::cin >> Temperature.Lower;

  std::cout << std::endl << "Higher limit: ";
  std::cin >> Temperature.High;

  std::cout << std::endl << "Step: ";
  std::cin >> Temperature.Step;

  for (int Cels = Temperature.Lower; Cels < Temperature.High; Cels += Temperature.Step)
  {
    float Fahrenheit = Cels * 9 / 5.0f + 32;

    printf("\n %4.2f  %4.2f", (float)Cels, Fahrenheit);
  }

  return 1;
}

#define m_print_out(str, ...) \
  {     \
  char output[256]; \
  sprintf_s(output, str, ##__VA_ARGS__); \
  std::cout << output; \
  OutputDebugString(output); \
  }

void print_out(const char* format, ...)
{
  char output[256];
  va_list args;
  va_start(args, format);
  vsprintf_s(output, format, args);
  va_end(args);
  std::cout << output;
  OutputDebugString(output);
}

const int MEMORY_COUNTER_SIZE = 512;

struct s_memory
{
  ~s_memory() { print_out("Memory left: %d\n", in_use); }

  int counter = 0;
  int in_use = 0;
  struct
  {
    void* ptr;
    int size;
  } arr[MEMORY_COUNTER_SIZE];
} memory;

inline void* operator new(std::size_t sz)
{  
  void* ptr = malloc(sz);
  //print_out("\nNew called: %10d byte allocated at line %d, ", sz, __LINE__);
  memory.in_use += sz;
  if (memory.counter < MEMORY_COUNTER_SIZE)
  {    
    memory.arr[memory.counter] = { ptr, sz };
    memory.counter++;
  }
  return ptr;
}

inline void operator delete(void* ptr)
{
  int i = 0;
  for (i = 0; i < MEMORY_COUNTER_SIZE; i++)
    if (memory.arr[i].ptr == ptr) break;
  //if (i < MEMORY_COUNTER_SIZE-1)
  //  print_out("\nDel called: %10d byte released at line %d", memory.arr[i].size, __LINE__);
  memory.in_use -= memory.arr[i].size;
  free(ptr);
}

struct pixel
{
  float r, g, b;
};

void make_cube_image(pixel* pixels, int width, int height);

void open_hdri(std::string filename)
{
  int width = 2400;
  int height = 1200;

  FILE* f;

  errno_t err = fopen_s(&f, filename.c_str(), "rb");
  rgbe_header_info header;

  RGBE_ReadHeader(f, &width, &height, NULL);

  // read pixels
  float* data = new float[width*height*3];  
  RGBE_ReadPixels_RLE(f, data, width, height);
  fclose(f);

  // convert for convenience  
  pixel* pixels = new pixel[width*height];
  memcpy(pixels, data, sizeof(float) * 3 * width*height);
  
  make_cube_image(pixels, width, height);
  return;
}

void make_cube_image(pixel* pixels, int width, int height)
{
  float r = height / 2.f;

  float cube_edge = 2*r / sqrtf(3);  
  int cube_edge_i = round(cube_edge);

  int new_width = cube_edge * 4 + 1;
  int new_height = cube_edge*3;
  
  pixel* out_pixels = new pixel[new_width*new_height];
  
  float c_x = -cube_edge / 2;
  float c_y = -cube_edge / 2;
  float c_z = -cube_edge / 2;

  float M_PI = 3.1415;
  
  pixel* edges[6];

  for (int i = 0; i < 6; i++)
  {
    edges[i] = new pixel[cube_edge_i*cube_edge_i];
  }

  for (int x = c_x; x < cube_edge / 2; x += 1)
  {
    for (int z = c_z; z < cube_edge / 2; z += 1)
    {
      float y = cube_edge/2;
	  float xyz_sqrt = sqrtf(x*x + y*y + z*z);
	  float s_x = -r*x / xyz_sqrt;
	  float s_y = r*y / xyz_sqrt;
	  float s_z = r*z / xyz_sqrt;
      float inclination = atan2f(sqrtf(s_x*s_x + s_y*s_y), s_z);
      float azimuth = atan2f(s_y, s_x);
      if (inclination < 0) inclination += 2 * M_PI;
      if (azimuth     < 0) azimuth += 2 * M_PI;

      int r_x = round(azimuth / (2 * M_PI)*width);
      int r_y = round(inclination / M_PI*height);

      if (r_x < width && r_x >= 0 && r_y < height && r_y >= 0)
      {
        pixel p = pixels[r_x + r_y*width];
        int index = int(x - c_x) + cube_edge_i*int(z - c_z);
		if (index < cube_edge_i*cube_edge_i && index >=0) edges[0][index] = p;
      }
    }
  }

  for (int y = c_y; y < cube_edge / 2; y += 1)
  {
    for (int z = c_z; z < cube_edge / 2; z += 1)
    {
      float x = cube_edge/2;
	  float xyz_sqrt = sqrtf(x*x + y*y + z*z);
	  float s_x = -r*x / xyz_sqrt;
	  float s_y = -r*y / xyz_sqrt;
	  float s_z = r*z / xyz_sqrt;
      float inclination = atan2f(sqrtf(s_x*s_x + s_y*s_y), s_z);
      float azimuth = atan2f(s_y, s_x);
      if (inclination < 0) inclination += 2 * M_PI;
      if (azimuth     < 0) azimuth += 2 * M_PI;

      int r_x = round(azimuth / (2 * M_PI)*width);
      int r_y = round(inclination / M_PI*height);

      if (r_x < width && r_x >= 0 && r_y < height && r_y >= 0)
      {
        pixel p = pixels[r_x + r_y*width];
        int index = int(y - c_y) + cube_edge_i*int(z - c_z);
        if (index < cube_edge_i*cube_edge_i && index >= 0) edges[1][index] = p;
      }
    }
  }

  for (int x = c_x; x < cube_edge / 2; x += 1)
  {
	  for (int z = c_z; z < cube_edge / 2; z += 1)
	  {
		  float y = cube_edge / 2;
		  float xyz_sqrt = sqrtf(x*x + y*y + z*z);
		  float s_x = r*x / xyz_sqrt;
		  float s_y = -r*y / xyz_sqrt;
		  float s_z = r*z / xyz_sqrt;
		  float inclination = atan2f(sqrtf(s_x*s_x + s_y*s_y), s_z);
		  float azimuth = atan2f(s_y, s_x);
		  if (inclination < 0) inclination += 2 * M_PI;
		  if (azimuth     < 0) azimuth += 2 * M_PI;

		  int r_x = round(azimuth / (2 * M_PI)*width);
		  int r_y = round(inclination / M_PI*height);

		  if (r_x < width && r_x >= 0 && r_y < height && r_y >= 0)
		  {
			  pixel p = pixels[r_x + r_y*width];
              int index = int(x - c_x) + cube_edge_i*int(z - c_z);
              if (index < cube_edge_i*cube_edge_i && index >= 0) edges[2][index] = p;
		  }
	  }
  }

  for (int y = c_y; y < cube_edge / 2; y += 1)
  {
	  for (int z = c_z; z < cube_edge / 2; z += 1)
	  {
		  float x = cube_edge / 2;
		  float xyz_sqrt = sqrtf(x*x + y*y + z*z);
		  float s_x = r*x / xyz_sqrt;
		  float s_y = r*y / xyz_sqrt;
		  float s_z = r*z / xyz_sqrt;
		  float inclination = atan2f(sqrtf(s_x*s_x + s_y*s_y), s_z);
		  float azimuth = atan2f(s_y, s_x);
		  if (inclination < 0) inclination += 2 * M_PI;
		  if (azimuth     < 0) azimuth += 2 * M_PI;

		  int r_x = round(azimuth / (2 * M_PI)*width);
		  int r_y = round(inclination / M_PI*height);

		  if (r_x < width && r_x >= 0 && r_y < height && r_y >= 0)
		  {
			  pixel p = pixels[r_x + r_y*width];
              int index = int(y - c_y) + cube_edge_i*int(z - c_z);
              if (index < cube_edge_i*cube_edge_i && index >= 0) edges[3][index] = p;
		  }
	  }
  }

  for (int x = c_x; x < cube_edge / 2; x += 1)
  {
    for (int y = c_y; y < cube_edge / 2; y += 1)
    {
  	  float z = cube_edge / 2;
  	  float xyz_sqrt = sqrtf(x*x + y*y + z*z);
  	  float s_x = r*x / xyz_sqrt;
  	  float s_y = -r*y / xyz_sqrt;
  	  float s_z = r*z / xyz_sqrt;
  	  float inclination = atan2f(sqrtf(s_x*s_x + s_y*s_y), s_z);
  	  float azimuth = atan2f(s_y, s_x);
  	  if (inclination < 0) inclination += 2 * M_PI;
  	  if (azimuth     < 0) azimuth += 2 * M_PI;
  
  	  int r_x = round(azimuth / (2 * M_PI)*width);
  	  int r_y = round(inclination / M_PI*height);
  
  	  if (r_x < width && r_x >= 0 && r_y < height && r_y >= 0)
  	  {
  		  pixel p = pixels[r_x + r_y*width];
          int index = int(y - c_y) + cube_edge_i*int(x - c_x);
          if (index < cube_edge_i*cube_edge_i && index >= 0) edges[4][index] = p;
  	  }
    }
  }
  
  for (int x = c_x; x < cube_edge / 2; x += 1)
  {
    for (int y = c_y; y < cube_edge / 2; y += 1)
    {
  	  float z = cube_edge / 2;
  	  float xyz_sqrt = sqrtf(x*x + y*y + z*z);
  	  float s_x = -r*x / xyz_sqrt;
  	  float s_y = -r*y / xyz_sqrt;
  	  float s_z = -r*z / xyz_sqrt;
  	  float inclination = atan2f(sqrtf(s_x*s_x + s_y*s_y), s_z);
  	  float azimuth = atan2f(s_y, s_x);
  	  if (inclination < 0) inclination += 2 * M_PI;
  	  if (azimuth     < 0) azimuth += 2 * M_PI;
  
  	  int r_x = round(azimuth / (2 * M_PI)*width);
  	  int r_y = round(inclination / M_PI*height);
  
  	  if (r_x < width && r_x >= 0 && r_y < height && r_y >= 0)
  	  {
  		  pixel p = pixels[r_x + r_y*width];
          int index = int(y - c_y) + cube_edge_i*int(x - c_x);
          if (index < cube_edge_i*cube_edge_i && index >= 0) edges[5][index] = p;
  	  }
    }
  }
  
  for (int i = 0; i < 6; i++)
  {
    int dx, dy;
    switch (i)
    {
    case 0: dx = 0            ; dy = cube_edge_i  ; break;
    case 1: dx = cube_edge_i  ; dy = cube_edge_i  ; break;
    case 2: dx = cube_edge_i*2; dy = cube_edge_i  ; break;
    case 3: dx = cube_edge_i*3; dy = cube_edge_i  ; break;
    case 4: dx = cube_edge_i  ; dy = cube_edge_i*2; break;
    case 5: dx = cube_edge_i  ; dy = 0            ; break;
    }
    for (int x = 0; x < cube_edge_i; x++)
    {
      for (int y = 0; y < cube_edge_i; y++)
      {
        int index1 = x + dx + new_width*(new_height - y - dy - 1);
        int index2 = x + cube_edge_i*y;
        if(index1 >= 0) out_pixels[index1] = edges[i][index2];
      }      
    }
  }

  FILE* f;

  std::string filename = "D:\\Stuff\\hdri_cubemap_converter\\output.hdr";
  //std::string filename = "E:\\Work\\hdr_cubemap\\images\\output.hdr";
  errno_t err = fopen_s(&f, filename.c_str(), "wb");

  rgbe_header_info header;
  header.exposure = 1.0f;
  strcpy_s<16>(header.programtype, "RADIANCE");
  header.valid = RGBE_VALID_PROGRAMTYPE | RGBE_VALID_EXPOSURE;
  RGBE_WriteHeader(f, new_width, new_height, &header);

  float* data = new float[new_width*new_height * 3];
  memcpy(data, out_pixels, sizeof(float) * 3 * new_width*new_height);
  RGBE_WritePixels_RLE(f, data, new_width, new_height);

  fclose(f);

  //for (int i = 0; i < width; i++)
  //{
  //  for (int j = 0; j < height; j++)
  //  {
  //    pixel p = pixels[i + j*width];
  //    Colour c = { p.r * 255, p.g * 255, p.b * 255, 255 };
  //    image->setPixel(c, j, i);
  //  }
  //}

  //image->WriteImage(filename);
}

int main()
{ 
  //FolderCrawler* fc = new StringReplacer("RangeAttackAction", "CloseCombatAction");
  //fc->Crawl("D:\\Replacer");
  
  //GlobalEventManager = new EventManager();
  //Sender s = Sender();
  //Receiver r = Receiver();
  //s.DoStuff();
  ////std::regex Exp;
  //
  //delete GlobalEventManager;
  //Exp.assign("..[a-z][a-z][a-z][a-z]");
  //std::cout << std::endl << std::regex_match("* zxcv", Exp);
  //std::cout << std::endl << std::regex_match("", Exp);

  //open_hdri("E:\\Work\\hdr_cubemap\\images\\uffizi-large.hdr");
  //open_hdri("E:\\Work\\hdr_cubemap\\images\\grace-new.hdr");
  //open_hdri("E:\\Work\\hdr_cubemap\\images\\glacier.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\HDR_110_Tunnel_Ref.hdr");
  open_hdri("D:\\Stuff\\hdri_cubemap_converter\\uffizi-large.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\glacier.hdr");
  //open_hdri("D:\\Stuff\\hdri_cubemap_converter\\output.hdr");

  system("PAUSE");
  //int exit;
  //std::cin >> exit;
  return 0;
}