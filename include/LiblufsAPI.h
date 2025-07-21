#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #ifdef LIBLUFS_EXPORTS
    #define LIBLUFS_API __declspec(dllexport)
  #else
    #define LIBLUFS_API __declspec(dllimport)
  #endif
#else
  #define MYLIB_API
#endif