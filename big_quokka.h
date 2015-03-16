#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <map>

namespace quokka
{
  template <class T>
  class Singletone
  {
  public:
    ~Singletone()
    {
      delete Instance;
    }
    static T* GetInstance()
    {
      if (!Instance)
        Instance = new T();
      return Instance;
    }
  protected:
    static T* Instance;
  };
}

namespace quokka
{
  class Timer
  {
  public:
    virtual void CalculateFrameTime() = 0;
    virtual float GetFrameTime() = 0;
    virtual float GetGlobalTime() = 0;
  };

  class STDChronoTimer : public Timer, public Singletone<STDChronoTimer>
  {
  public:

    typedef std::chrono::nanoseconds Duration;

    STDChronoTimer()
      :Last(std::chrono::high_resolution_clock::now())
      , Start(Last)
    {}
    virtual ~STDChronoTimer(){}

    // Call only once per frame!
    virtual void CalculateFrameTime()
    {
      std::chrono::high_resolution_clock::time_point Now = std::chrono::high_resolution_clock::now();
      LastFrameDuration = std::chrono::duration_cast<Duration>(Now - Last).count();
      Last = Now;
    }

    virtual float GetFrameTime() { return LastFrameDuration / 1000.0f; }

    virtual float GetGlobalTime()
    {
      std::chrono::high_resolution_clock::time_point Now = std::chrono::high_resolution_clock::now();
      return std::chrono::duration_cast<Duration>(Now - Start).count();
    }

  private:
    long long LastFrameDuration;
    std::chrono::high_resolution_clock::time_point Last;
    std::chrono::high_resolution_clock::time_point Start;
  };

  STDChronoTimer* STDChronoTimer::Instance = NULL;
  static Timer* Timer() { return STDChronoTimer::GetInstance(); }
}

namespace quokka
{
  struct ProfilerStamp
  {    
    float Start = 0;
    float Stop = 0;
    float Sum = 0;
  };

  class Profiler : public Singletone<Profiler>
  {
  public:
    void Start(std::string Name)
    {
      ProfilerStamp ps;
      ps.Start = Timer()->GetGlobalTime();
      Stamps[Name] = ps;
    }

    void Stop(std::string Name)
    {
      Stamps[Name].Stop = Timer()->GetGlobalTime();
      Stamps[Name].Sum += Stamps[Name].Stop - Stamps[Name].Start;
    }

    void Print()
    {
      for (std::map<std::string, ProfilerStamp>::const_iterator it = Stamps.begin(); it != Stamps.end(); ++it)
      {
        printf("\n%20s : %15.2f", it->first.c_str(), it->second.Sum);
      }
    }

  private:
    std::map<std::string, ProfilerStamp> Stamps;
  };

  Profiler* GProfiler();
}