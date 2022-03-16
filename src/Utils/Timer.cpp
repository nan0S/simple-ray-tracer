#include "Timer.h"

#include <iostream>
#include <cmath>

#include "Utils/Log.h"

Timer::Timer(const char* label /* = nullptr */)
   : m_Label(label), m_Start(clock_t::now()) {}

Timer::~Timer()
{
   stop();
}

void Timer::stop()
{
   if (m_Stopped)
      return;
   m_Stopped = true;

   float ms = elapsed();
   if (m_Label)
      print("[", m_Label, "] ", ms, " ms");
   else
      print(ms, " ms");
}

float Timer::elapsed()
{
   clock_t::time_point end = clock_t::now();
   return std::chrono::duration<float>(end - m_Start).count() * 1000;
}


AggregateTimer::AggregateTimer(const char* label /* = nullptr */)
   : m_Label(label), m_Index(0) {}

void AggregateTimer::start()
{
   m_Start = clock_t::now();
}

void AggregateTimer::stop()
{
   clock_t::time_point stop = clock_t::now();
   m_Measures[m_Index++] = std::chrono::duration<float>(stop - m_Start).count() * 1000.0f;
   if (m_Index == REPEAT)
   {
      m_Index = 0;
      float mean = 0;
      for (int i = 0; i < REPEAT; ++i)
         mean += m_Measures[i];
      mean /= REPEAT;
      float stddev = 0;
      for (int i = 0; i < REPEAT; ++i)
      {
         float diff = m_Measures[i] - mean;
         stddev += diff * diff;
      }
      stddev = std::sqrt(stddev / REPEAT);
      if (m_Label)
         print("[", m_Label, "] ", mean, " +/- ", stddev, " ms");
      else
         print(mean, " +/- ", stddev, " ms");
   }
}
