#pragma once

#include <chrono>



struct timer
  {
  void start()
    {
    tic = std::chrono::high_resolution_clock::now();
    }

  double time_elapsed()
    {
    toc = std::chrono::high_resolution_clock::now();
    diff = toc - tic;
    return (double)diff.count();
    }

  std::chrono::high_resolution_clock::time_point tic, toc;
  std::chrono::duration<double> diff;
  };
