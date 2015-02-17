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

#include "Image.h"

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

class A
{
  int Data = 5;
};

struct TreeNode
{
  std::vector<TreeNode*> Children;
};

class Tree
{
  //Tree()
  //{
  //  Root = new TreeNode();
  //  Root->Children.push_back()
  //}
  //
  //TreeNode* NewNode()
  //{
  //  TreeNode* NewNode = new TreeNode();
  //  return NewNode;
  //}
  //TreeNode* Root;
};

//#define print_out(str, ...) \
//  {     \
//  char output[256]; \
//  sprintf_s(output, str, ##__VA_ARGS__); \
//  std::cout << output; \
//  OutputDebugString(output); \
//  }

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

void open_hdri(std::string filename)
{
  int width = 2400;
  int height = 1200;

  TGAImage* image = new TGAImage(width, height);
  
  //

  std::ifstream file;

  file.open(filename);
  std::string line;
  int k = 0;
  //while (std::getline(file, line) && k < 6)
  //{
  //  //uint8_t RGBEData[4];
  //  //file >> RGBEData;
  //  std::cout << std::endl << line;
  //  k++;
  //}
  int count = 0;
  while (!file.eof())
  {
    char Byte;
    file.read(&Byte, 1);
    count++;
  }
  print_out("\n %d ", count);
  for (int i = 0; i < width; i++)
  {
    for (int j = 0; j < height; j++)
    {
      if (file.eof())
        break;

      char RGBEData[4];
      file.read(RGBEData, 4);      
      unsigned char RGBEData_c[4];
      memcpy(RGBEData_c, RGBEData, 4);
      float scale = 1.f;// ldexp(1.0, RGBEData_c[3] - 136);
      //print_out("\n%d %d %d %d", RGBEData_c[0], RGBEData_c[1], RGBEData_c[2], RGBEData_c[3]);
      Colour c = { RGBEData_c[0] * scale, RGBEData_c[1] * scale, RGBEData_c[2] * scale, 255 };
      image->setPixel(c, i, j);
    }
  }
  
  file.close();

  image->WriteImage("D:\\Stuff\\hdri_cubemap_converter\\output.tga");
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


  open_hdri("D:\\Stuff\\hdri_cubemap_converter\\uffizi-large.hdr");

  system("PAUSE");
  //int exit;
  //std::cin >> exit;
  return 0;
}