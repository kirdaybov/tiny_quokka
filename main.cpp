#include <iostream>
#include <vector>
#include <chrono>
#include <map>
#include <Windows.h>

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
public:
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

  std::map<Event, Delegate<void, int>*> Listeners;
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

int main()
{
  FolderCrawler* fc = new FolderPrinter();
  fc->Crawl("D:\\Games");

  GlobalEventManager = new EventManager();
  Sender s = Sender();
  Receiver r = Receiver();
  s.DoStuff();

  getchar();
  return 0;
}