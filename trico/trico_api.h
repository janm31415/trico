#pragma once

#ifdef _WIN32
#if defined(trico_EXPORTS)
#  define TRICO_API __declspec(dllexport)
#else
#  define TRICO_API __declspec(dllimport)
#endif
#else
#define TRICO_API
#endif