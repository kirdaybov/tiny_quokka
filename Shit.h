#pragma once

#define log(str) (std::cout << std::endl << (str))
//#define log(str, param) (std::cout << std::endl << (str) << (param))
#define log_error(str) (std::cout << __FILE__ << " " << __LINE__ << ":" << (str))
//#include "Architecture.h";

namespace kdlib
{
  template<class T>
  class Ptr
  {
    T* obj;
    int* counter;
  public:
    Ptr(T* obj)
    {
      this->obj = obj;
      counter = new int;
      *counter = 1;
    }
    Ptr(const Ptr<T> &other_pointer)
    {
      this->obj = other_pointer.obj;

      this->counter = other_pointer.counter;
      (*counter)++;
    }
    ~Ptr()
    {
      std::cout << "\n" << *counter;
      if (*counter == 1)
      {
        delete obj;
        delete counter;
      }
      else
      {
        (*counter)--;
      }
    }
    T operator*()
    {
      return *obj;
    }
    T* operator->()
    {
      return obj;
    }
  };
}

template<class T>
class ListNode
{
  T data;
  ListNode* next;

public:
  ListNode()
    :next(NULL)
  {};
  ListNode(T a_data)
    :next(NULL)
    , data(a_data)
  {
  }
};

template<class T>
class List
{
  ListNode<T>* first;
  ListNode<T>* current;
  ListNode<T>* last;
  int count;

public:
  List()
    :first(NULL)
    , current(NULL)
    , last(NULL)
    , count(0)
  {
  }
  ~List()
  {
  }

  void Add(T data)
  {
    ListNode<T>* newNode = new ListNode(data);
    if (!first)
      first = newNode;
    else
      last->next = newNode;

    last = newNode;
  }

  T& Get(int index)
  {
    ListNode<T>* current = first;
    for (int i = 0; i < index; i++)
    {
      if (current->next)
        current = current->next;
      else
        break;
    }
    return current->data;
  }

  class Iterator
  {

  };

  T& Current()
  {
    return current->data;
  }

private:

  ListNode<T>* next()
  {
    current = current->next;
    return current;
  }
};

template<class T>
class TreeNode
{
public:
  T data;
  List<TreeNode*> neighbours;
};

class UI
{
public:
  virtual void Render() = 0;
  virtual void ProcessMessages() = 0;
};

class TreeUI : public UI
{
public:
  void Render()
  {

  }
  void ProcessMessages()
  {

  }
protected:
  List<TreeUI> elements;
};



