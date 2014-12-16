#include <iostream>
#include <vector>
#include <chrono>
#include <map>
#include <Windows.h>

struct FileNode
{
  enum Type
  {
    FOLDER,
    FILE
  };

  Type FileType;
  std::string Name;
};

class FolderCrawler
{
public:
  void Crawl(std::string Path)
  {
    std::vector<FileNode> Files = ListCurrentFolder(Path);
    for (int i = 0; i < Files.size(); i++)
    {
      switch (Files[i].FileType)
      {
      case FileNode::FILE: ProcessFile(); break;
      case FileNode::FOLDER: Crawl(""); break;
      }     

    }
  }

private:
  std::vector<FileNode> ListCurrentFolder(std::string Path)
  {
    std::vector<FileNode> Files;

    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    
    hFind = FindFirstFile(Path.c_str(), &FindFileData);
    while (hFind != INVALID_HANDLE_VALUE)
    {
      std::cout << std::endl << FindFileData.cFileName;
      if (!FindNextFile(hFind, &FindFileData))
      {
        hFind = INVALID_HANDLE_VALUE;
      }
    }

    return Files;
  }
  void ProcessNode(FileNode& Node) {};

  virtual void ProcessFile() {};
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
  FolderCrawler fc = FolderCrawler();
  fc.Crawl("D:\\Games\\*");

  GlobalEventManager = new EventManager();
  Sender s = Sender();
  Receiver r = Receiver();
  s.DoStuff();

  getchar();
  return 0;
}