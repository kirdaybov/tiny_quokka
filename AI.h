#pragma once 
#include <vector>

struct Actor
{

  std::vector<std::string> Labels;
};

class Behaviour
{
  bool Available()
  {

  }

  bool Do()
  {

  }
};

class AI
{
  void ChooseAction()
  {

  }

  std::vector<Behaviour*> Behaviours;
};