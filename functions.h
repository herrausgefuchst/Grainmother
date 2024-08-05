#ifndef functions_h
#define functions_h

//#define BELA_CONNECTED
//#define CONSOLE_PRINT_ALLOWED
//#define JSON_USED

#include <iostream>
#include <vector>
#include <math.h>
#include <ctime>
#include <array>
#include <functional>

#ifdef BELA_CONNECTED
#include <libraries/math_neon/math_neon.h>
#else
#define sqrtf_neon(float) sqrtf(float)
#define powf_neon(float1, float2) powf(float1, float2)
#define cosf_neon(float) cosf(float)
#define sinf_neon(float) sinf(float)
#define floorf_neon(float) floorf(float)
#define logf_neon(float) logf(float)
#define fabsf_neon(float) fabsf(float)
#endif

#define TOSTRING(x) std::to_string(x)
#define INT2ENUM(x, ENUM) static_cast<ENUM>(x)
#define ENUM2INT(ENUM) static_cast<int>(ENUM)

#define FORLOOP(x) for(unsigned int n = 0; n < x; n++)

using String = std::string;
using FloatPair = std::pair<float,float>;

template <class T>
void boundValue (T &x, T min, T max)
{
    if (x < min) x = min;
    else if (x > max) x = max;
}

template <class T>
inline bool isClose (const T& x, const T& y, const T tolerance)
{
    if (fabsf(x - y) < tolerance) return true;
    return false;
}

inline float mapValue(float x, float in_min, float in_max, float out_min, float out_max)
{
    float mapped = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    boundValue(mapped, out_min, out_max);
    return mapped;
}

template<typename Base, typename T>
inline bool instanceof(const T *ptr)
{
   return dynamic_cast<const Base*>(ptr) != nullptr;
}

inline float logbase (float x, float base)
{
    return logf_neon(x) / logf_neon(base);
}

inline float lin2log (float x) // fixed Slope 0.75, a & 1/log(b) precalculated
{
    boundValue(x, 0.f, 1.0f);
    float a = -1.125f;
    float b = -0.455119613313f;
    return logf_neon((x + a) / a) * b;
}

inline float round_float_1 (float x)
{
    float value = (x >= 0.f) ? (int)(x * 10.f + 0.5f) : (int)(x * 10.f - 0.5f);
    return value * 0.1f;
}

inline float round_float_2 (float x)
{
    float value = (x >= 0.f) ? (int)(x * 100.f + 0.5f) : (int)(x * 100.f - 0.5f);
    return value * 0.01f;
}

inline float round_float_3 (float x)
{
    float value = (x >= 0.f) ? (int)(x * 1000.f + 0.5f) : (int)(x * 1000.f - 0.5f);
    return value * 0.001f;
}

inline void engine_error (bool _condition, String _message, String _file, int _line, bool _exit)
{
    if (_condition)
    {
#ifndef BELA_CONNECTED
        std::cout << "------------------------------------" << std::endl;
        std::cout << "ERROR: @" << _file << " // Line: " << _line << std::endl;
        std::cout << _message << std::endl;
        if (_exit) std::cout << "Program stopped" << std::endl;
        std::cout << "------------------------------------" << std::endl;
        if (_exit) exit(0);
#else
        // Bela is connected
        rt_printf("------------------------------------ \n");
        rt_printf("ERROR: @ %s // Line: %i \n", _file.c_str(), _line);
        rt_printf("%s \n", _message.c_str());
        if (_exit) printf("PROGRAMM STOPPED \n");
        rt_printf("------------------------------------ \n");
        if (_exit) exit(0);
#endif
    }
}

// same as engine error, only that the condition must be previously cehcked
inline void engine_rt_error (String _message, String _file, int _line, bool _exit)
{
#ifndef BELA_CONNECTED
        std::cout << "------------------------------------" << std::endl;
        std::cout << "ERROR: @" << _file << " // Line: " << _line << std::endl;
        std::cout << _message << std::endl;
        if (_exit) std::cout << "Program stopped" << std::endl;
        std::cout << "------------------------------------" << std::endl;
        if (_exit) exit(0);
#else
        // Bela is connected
        rt_printf("------------------------------------ \n");
        rt_printf("ERROR: @ %s // Line: %i \n", _file.c_str(), _line);
        rt_printf("%s \n", _message.c_str());
        if (_exit) printf("PROGRAMM STOPPED \n");
        rt_printf("------------------------------------ \n");
        if (_exit) exit(0);
#endif
}

inline void consoleprint (String _message, String _file, int _line)
{
#ifdef CONSOLE_PRINT_ALLOWED
#ifndef BELA_CONNECTED
    std::cout << ">> CONSOLE: " << _file << " // Line: " << _line << " <<" << std::endl;
    std::cout << ">> " << _message << " <<" << std::endl << std::endl;
#else
    printf(">> %s // Line: %i << \n", _file.c_str(), _line);
    printf(">> %s << \n\n", _message.c_str());
#endif
#endif
}

inline String getDateAsString()
{
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
     
    String day = std::to_string(now->tm_mday);
    String month = std::to_string(now->tm_mon + 1);
    String year = std::to_string(now->tm_year + 1900);
    
    return day + "/" + month + "/" + year;
}

#endif /* functions_h */
