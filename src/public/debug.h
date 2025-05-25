#pragma once
#include <Windows.h>
#include <cstdio>

#ifdef _DEBUG
    #define DEBUG_BREAK() __debugbreak()
    #define ASSERT(condition) \
        do { \
            if (!(condition)) { \
                char buffer[512]; \
                sprintf_s(buffer, "Assertion failed: %s\nFile: %s\nLine: %d", \
                         #condition, __FILE__, __LINE__); \
                OutputDebugStringA(buffer); \
                MessageBoxA(nullptr, buffer, "Assertion Failed", MB_OK | MB_ICONERROR); \
                DEBUG_BREAK(); \
            } \
        } while(0)
#else
    #define DEBUG_BREAK()
    #define ASSERT(condition)
#endif