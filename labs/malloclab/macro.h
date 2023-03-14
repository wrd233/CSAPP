#ifndef LOG_H
#define LOG_H

// #include <cstdio>
#include <stdio.h>

#define INFO_OUT
#define DEBUG_OUT

#ifdef INFO_OUT
#define INFO(...) \
printf("[INFO ] %s  %s:%d\n\t", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
printf(__VA_ARGS__); \
printf("\n");
#else
#define INFO(...)
#endif

#ifdef DEBUG_OUT
#define DEBUG(...) \
printf("[DEBUG] %s  %s:%d\n\t", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
printf(__VA_ARGS__); \
printf("\n");
#else
#define DEBUG(...)
#endif

#define ERROR(...) \
printf("[ERROR] %s  %s:%d\n\t", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
printf(__VA_ARGS__); \
printf("\n");

#endif // LOG_H