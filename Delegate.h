#pragma once

#include <iostream>
#include <map>

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
