#pragma once

#include <chrono>

struct Timer
{
   using clock_t = std::chrono::high_resolution_clock;

   Timer(const char* label = nullptr);
   ~Timer();

   void stop();
   float elapsed();

private:
   bool m_Stopped = false;
   const char* m_Label;
   clock_t::time_point m_Start;
};

struct AggregateTimer
{
   using clock_t = std::chrono::high_resolution_clock;

   AggregateTimer(const char* label = nullptr);

   void start();
   void stop();

private:
   static constexpr int REPEAT = 100;
   const char* m_Label;
   clock_t::time_point m_Start;
   float m_Measures[REPEAT];
   int m_Index;
};
