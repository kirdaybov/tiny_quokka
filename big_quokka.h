#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <map>

#include "print.h"

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
    virtual long long GetGlobalTime() = 0;
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

    virtual long long GetGlobalTime()
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

  typedef unsigned long long uint64;
  typedef unsigned long uint32;
  
  class HighResolutionTimer : public Timer, public Singletone<HighResolutionTimer>
  {
  public:
    
    HighResolutionTimer()
      :Last(rdtsc())
      , Start(Last)
    {}
    virtual ~HighResolutionTimer(){}

    uint64 rdtsc()
    {
      uint32 cur_time;
      __asm rdtsc;
      __asm mov [cur_time], eax;
      return cur_time;
    }

    // Call only once per frame!
    virtual void CalculateFrameTime()
    {
      uint64 Now = rdtsc();
      LastFrameDuration = Now - Last;
      Last = Now;
    }

    virtual float GetFrameTime() { return LastFrameDuration / 1000.0f; }

    virtual long long GetGlobalTime()
    {
      uint64 Now = rdtsc();
      return Now - Start;
    }

  private:
    uint64 LastFrameDuration;
    uint64 Last;
    uint64 Start;
  };

  HighResolutionTimer* HighResolutionTimer::Instance = NULL;
  static Timer* Timer() { return HighResolutionTimer::GetInstance(); }
}

namespace quokka
{
  struct ProfilerStamp
  {    
    uint64 Start = 0;
    uint64 Stop = 0;
    uint64 Sum = 0;
  };

  class Profiler : public Singletone<Profiler>
  {
  public:
    void Start(std::string Name)
    {
      ProfilerStamp ps;
      if (Stamps.find(Name) != Stamps.end()) ps = Stamps[Name];
      ps.Start = Timer()->GetGlobalTime();
      Stamps[Name] = ps;
    }

    void Stop(std::string Name)
    {
      Stamps[Name].Stop = Timer()->GetGlobalTime();
      uint64 Elapsed = Stamps[Name].Stop - Stamps[Name].Start;
      //print_out("\n%20s : %15u", Name.c_str(), Elapsed);
      Stamps[Name].Sum = Stamps[Name].Sum + Elapsed;
    }

    void Print()
    {
      for (std::map<std::string, ProfilerStamp>::const_iterator it = Stamps.begin(); it != Stamps.end(); ++it)
      {
        print_out("\n%20s : %15u", it->first.c_str(), it->second.Sum);
        //print_out("\n%20s : %15.5f", it->first.c_str(), it->second.Sum/1.0e9f);
      }
    }

  private:
    std::map<std::string, ProfilerStamp> Stamps;
  };

  extern Profiler* GProfiler();
}