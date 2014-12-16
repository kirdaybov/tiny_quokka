
#pragma once

// INTERFACES
class IGame
{
public:
  virtual void Run() = 0;
};

// IMPLEMENTATIONS

class Input
{
};

class Engine
{
  void* Input;
};

class InsomniaGame : public IGame
{
public:

  IController* controller;

  InsomniaGame():IGame()
  {
  }
  virtual ~InsomniaGame(){}
  
  void Run()
  {
    while (true)
    {
      ProcessInput();
      DoLogic();
    }
  }

  void ProcessInput()
  {

  }

  void DoLogic() 
  {
    if (rand() % 1000 == 5)
    {
      controller->PutCommand(5);
    }
  }
};

// CONTROLLER

class IController
{
public:
  virtual void PutCommand(int cmd) = 0;
};

class PlayerController : public IController
{
public:
  Avatar* avatar;
  void PutCommand(int cmd)
  {
    avatar->ExecuteCommand(cmd);
  }
};

// CHARACTER

class Avatar
{
public:
  void ExecuteCommand(int command)
  {
    std::cout << "\n" << command;
  }
};

class State
{

};

class Character
{
public:
  Avatar* avatar;
  State* state;
};
