#pragma once

#include "ConstantVariables.h"

#define TOSTRING(x) std::to_string(x)
#define INT2ENUM(x, ENUM) static_cast<ENUM>(x)
#define ENUM2INT(ENUM) static_cast<int>(ENUM)

#define FORLOOP(x) for(unsigned int n = 0; n < x; ++n)

/**
 * @defgroup HelpingFunctions
 * @brief a group of inline functions, used for basic calculations, bounding value, etc.
 * @{
 */

/**
 * @brief checks if a float underflow has happened and overwrites the value if necessary
 * @param value the value that has to be checked
 */
inline void checkFloatUnderflow (float& value)
{
    bool retValue = false;
    if (value > 0.0 && value < SMALLEST_POSITIVE_FLOATVALUE)
    {
        value = 0.f;
        retValue = true;
    }
    else if (value < 0.0 && value > SMALLEST_NEGATIVE_FLOATVALUE)
    {
        value = 0.f;
        retValue = true;
    }
}

/**
 * @brief chekcs if a value has exceeded certain boundaries, if yes: sets value to the lower boundary
 *
 * @param value the value that has to be checked
 * @param min the lower bondary
 * @param max the upper boundary
 */
inline void checkBoundaries (float& value, const float min, const float max)
{
    if (value > max || value < min)
    {
        value = min;
    }
}

/**
 * @brief bounds a value to a certain range
 *
 * @param value the value that has to be checked
 * @param min the lower bondary
 * @param max the upper boundary
 */
template<typename T>
inline void boundValue(T& value, const T min, const T max)
{
    value = value > max ? max : value;
    value = value < min ? min : value;
}

/**
 * @brief approximates a sine output
 *
 * by René G. Ceballos <rene@rgcaudio.com>
 * input range: 0.0 to 2.0 * Pi
 * output range: -1.0 to 1.0
 *
 * @param angle the x position (0.0 to 2PI)
 *
 * @return the y position (-1.0 to 1.0)
 */
inline float approximateSine(float angle)
{
    float x,j;
    
    if (angle < PIo2){
        x = angle * TWOoPI - 0.5f;
        j = -(x * x) + 0.75f + x;
    }
    else if(angle < PI) {
        angle = PI - angle;
        x = angle * TWOoPI - 0.5f;
        j = -(x * x) + 0.75f + x;
    }
    else if (angle < PI3o2) {
        angle -= PI;
        x = angle * TWOoPI - 0.5f;
        j = x * x - 0.75f - x;
    }
    else {
        angle = TWOPI - angle;
        x = angle * TWOoPI - 0.5f;
        j = x * x - 0.75f - x;
    }
    
    return j;
}

/**
 * @brief checks is a value is close to another value
 *
 * @param x the value thas has to be checked
 * @param y the value, x should be close to
 * @param tolerance the one-sided distance to y, when x is in that tolerance, returns true
 *
 * @return true if @p x is in the range @code (y - tolerance) <= x <= (y + tolerance) @endcode
 */
template <class T>
inline bool isClose (const T& x, const T& y, const T tolerance)
{
    if (fabsf(x - y) < tolerance) return true;
    return false;
}

/**
 * @brief maps a value from one given input range to another output range
 *
 * @param x the value that has to be mapped
 * @param in_min input range minimum
 * @param in_max input range maximum
 * @param out_min output range minimum
 * @param out_max output range maximum
 *
 * @attention both input and output minima have to be smaller than their corresponding maxima
 *
 * @return the mapped value
 */
inline float mapValue(float x, float in_min, float in_max, float out_min, float out_max)
{
    float mapped = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    boundValue(mapped, out_min, out_max);
    return mapped;
}

/**
 * @brief Checks if a given pointer is of a specified base type.
 *
 * This template function determines if the type of a pointer `ptr` is derived from
 * or exactly of the type `Base`. It uses `dynamic_cast` to perform this check at runtime.
 *
 * @tparam Base The base type to check against.
 * @tparam T The type of the pointer `ptr`.
 * @param ptr A pointer to an object of type `T`.
 * @return true if `ptr` can be dynamically cast to a pointer of type `Base`, false otherwise.
 */
template<typename Base, typename T>
inline bool instanceof(const T *ptr)
{
   return dynamic_cast<const Base*>(ptr) != nullptr;
}

/**
 * @brief Calculates the logarithm of a number with a specified base.
 *
 * This function computes the logarithm of a given number `x` with respect to an arbitrary `base`.
 * It uses the formula \f$ \log_{\text{base}}(x) = \frac{\log(x)}{\log(\text{base})} \f$ to determine
 * the result, where `logf_neon` is assumed to be a function that computes the natural logarithm.
 *
 * @param x The number for which the logarithm is to be computed. Must be positive.
 * @param base The base of the logarithm. Must be positive and not equal to 1.
 * @return The logarithm of `x` to the given `base`.
 */
inline float logbase (float x, float base)
{
    if (x <= 0) {
        throw std::invalid_argument("The number x must be positive.");
    }
    if (base <= 0) {
        throw std::invalid_argument("The base must be positive.");
    }
    if (base == 1) {
        throw std::invalid_argument("The base must not be equal to 1.");
    }
    return logf_neon(x) / logf_neon(base);
}

/**
 * @brief Converts a linear value to a logarithmic scale.
 *
 * This function maps a linear value `x` in the range [0, 1] to a logarithmic scale
 * using a predefined slope and offset. The slope and an inverse logarithmic factor
 * are precalculated for efficiency.
 *
 * @param x A linear input value that should be in the range [0, 1].
 * @return A float representing the logarithmic equivalent of the input value `x`.
 * If `x` is outside the range [0, 1], it will be clamped to the nearest boundary.
 *
 * @note The function uses a fixed slope of 0.75 and precalculated constants for
 * conversion. The input `x` is clamped between 0 and 1 before computation.
 *
 * @code
 * float result = lin2log(0.5f); // result will be a logarithmic value corresponding to 0.5
 * @endcode
 */
inline float lin2log (float x) // fixed Slope 0.75, a & 1/log(b) precalculated
{
    static const float a = -1.125f;
    static const float b = -0.455119613313f;
    
    boundValue(x, 0.f, 1.0f);
    return logf_neon((x + a) / a) * b;
}

/**
 * @brief Rounds a floating-point number to one decimal place.
 *
 * This function rounds the input floating-point number `x` to the nearest tenth. The rounding
 * is done by first multiplying the number by 10, then adjusting it based on its sign and
 * the rounding rules, and finally dividing by 10 to shift the decimal place back.
 *
 * @param x The floating-point number to be rounded.
 * @return The input number rounded to one decimal place.
 *
 * @note The function handles both positive and negative values of `x`.
 *
 * @code
 * float result1 = round_float_1(3.456f); // result1 will be 3.5
 * float result2 = round_float_1(-2.344f); // result2 will be -2.3
 * @endcode
 */
inline float round_float_1 (float x)
{
    float value = (x >= 0.f) ? (int)(x * 10.f + 0.5f) : (int)(x * 10.f - 0.5f);
    return value * 0.1f;
}

/**
 * @brief Rounds a floating-point number to two decimal places.
 *
 * This function rounds the input floating-point number `x` to the nearest hundredth.
 * It multiplies the number by 100, adjusts it based on its sign and the rounding rules,
 * and then divides by 100 to shift the decimal place back.
 *
 * @param x The floating-point number to be rounded.
 * @return The input number rounded to two decimal places.
 *
 * @note The function handles both positive and negative values of `x`.
 *
 * @code
 * float result1 = round_float_2(3.4567f); // result1 will be 3.46
 * float result2 = round_float_2(-2.3445f); // result2 will be -2.34
 * @endcode
 */
inline float round_float_2 (float x)
{
    float value = (x >= 0.f) ? (int)(x * 100.f + 0.5f) : (int)(x * 100.f - 0.5f);
    return value * 0.01f;
}

/**
 * @brief Rounds a floating-point number to three decimal places.
 *
 * This function rounds the input floating-point number `x` to the nearest thousandth.
 * The process involves multiplying the number by 1000, adjusting it based on its sign
 * and the rounding rules, and then dividing by 1000 to shift the decimal place back.
 *
 * @param x The floating-point number to be rounded.
 * @return The input number rounded to three decimal places.
 *
 * @note The function handles both positive and negative values of `x`.
 *
 * @code
 * float result1 = round_float_3(3.45678f); // result1 will be 3.457
 * float result2 = round_float_3(-2.34456f); // result2 will be -2.345
 * @endcode
 */
inline float round_float_3 (float x)
{
    float value = (x >= 0.f) ? (int)(x * 1000.f + 0.5f) : (int)(x * 1000.f - 0.5f);
    return value * 0.001f;
}

/**
 * @brief Handles error reporting and optionally stops the program execution.
 *
 * This function checks a condition and, if true, prints an error message along with the
 * file name and line number where the error occurred. It can optionally stop the program
 * execution if the error is critical.
 *
 * @param _condition A boolean value indicating whether the error condition is met.
 * @param _message A string containing the error message to be displayed.
 * @param _file A string indicating the name of the file where the error occurred.
 * @param _line An integer specifying the line number in the file where the error occurred.
 * @param _exit A boolean flag that, if true, will cause the program to terminate after
 * reporting the error.
 *
 * @note This function uses `rt_printf` for output, which is assumed to be a real-time safe
 * print function. The `_exit` parameter should be used with caution, as it will terminate
 * the program execution if set to true.
 */
inline void engine_error (bool _condition, String _message, String _file, int _line, bool _exit)
{
    if (_condition)
    {
        rt_printf("------------------------------------ \n");
        rt_printf("ERROR: @ %s // Line: %i \n", _file.c_str(), _line);
        rt_printf("%s \n", _message.c_str());
        if (_exit) printf("PROGRAMM STOPPED \n");
        rt_printf("------------------------------------ \n");
        if (_exit) exit(0);
    }
}

/**
 * @brief Reports a real-time error and optionally stops the program execution.
 *
 * This function reports an error with a specified message, file name, and line number.
 * Unlike `engine_error`, it assumes the error condition has already been checked.
 * It can optionally stop the program execution if the error is critical.
 *
 * @param _message A string containing the error message to be displayed.
 * @param _file A string indicating the name of the file where the error occurred.
 * @param _line An integer specifying the line number in the file where the error occurred.
 * @param _exit A boolean flag that, if true, will cause the program to terminate after
 * reporting the error.
 *
 * @note This function uses `rt_printf` for output, which is assumed to be a real-time safe
 * print function. The `_exit` parameter should be used with caution, as it will terminate
 * the program execution if set to true.
 */
inline void engine_rt_error (String _message, String _file, int _line, bool _exit)
{
    rt_printf("------------------------------------ \n");
    rt_printf("ERROR: @ %s // Line: %i \n", _file.c_str(), _line);
    rt_printf("%s \n", _message.c_str());
    if (_exit) rt_printf("PROGRAMM STOPPED \n");
    rt_printf("------------------------------------ \n");
    if (_exit) exit(0);
}

/**
 * @brief Prints a message to the console along with the file and line number.
 *
 * This function outputs a message, along with the file name and line number, to the console.
 * It is useful for logging purposes and for providing context about where the message
 * originates from in the codebase.
 *
 * @param _message A string containing the message to be displayed.
 * @param _file A string indicating the name of the file where the message is logged.
 * @param _line An integer specifying the line number in the file where the message is logged.
 *
 * @note This function uses `rt_printf` for output, which is assumed to be a real-time safe
 * print function.
 */
inline void consoleprint (String _message, String _file, int _line)
{
    rt_printf(">> %s // Line: %i << \n", _file.c_str(), _line);
    rt_printf(">> %s << \n\n", _message.c_str());
}

/**
 * @brief Gets the current date as a string.
 *
 * This function returns the current date formatted as "day/month/year". It fetches the
 * current local time and formats it accordingly.
 *
 * @return A string representing the current date in the format "day/month/year".
 *
 * @note The returned date string uses the local time settings of the system.
 */
inline String getDateAsString()
{
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
     
    String day = std::to_string(now->tm_mday);
    String month = std::to_string(now->tm_mon + 1);
    String year = std::to_string(now->tm_year + 1900);
    
    return day + "/" + month + "/" + year;
}

/** @} */

/**
 * @defgroup HelpingClasses
 * @brief a group of classes and structs, supporting the interface
 * @{
 */

/**
 * @struct StereoFloat
 * @brief a container for a stereo pair of floats
 */
struct StereoFloat
{
    float leftSample;
    float rightSample;
    
    StereoFloat operator+(const StereoFloat& other_) const
    {
        return StereoFloat{leftSample + other_.leftSample, rightSample + other_.rightSample};
    }
    
    StereoFloat operator-(const StereoFloat& other_) const
    {
        return StereoFloat{leftSample - other_.leftSample, rightSample - other_.rightSample};
    }
    
    StereoFloat& operator-() {
        leftSample = -leftSample;
        rightSample = -rightSample;
        return *this;
    }
    
    StereoFloat operator*(float value_) const
    {
        return StereoFloat{leftSample * value_, rightSample * value_};
    }
    
    StereoFloat& operator+=(const StereoFloat& other_)
    {
        leftSample += other_.leftSample;
        rightSample += other_.rightSample;
        return *this;
    }
    
    StereoFloat& operator*=(const float& value_)
    {
        leftSample *= value_;
        rightSample *= value_;
        return *this;
    }
    
    float& operator[](const size_t& index_)
    {
        if (index_ == 0)
            return leftSample;
        else if (index_ == 1)
            return rightSample;
        else
            throw std::out_of_range("Index out of range");
    }

    const float& operator[](const size_t& index_) const
    {
        if (index_ == 0)
            return leftSample;
        else if (index_ == 1)
            return rightSample;
        else
            throw std::out_of_range("Index out of range");
    }
};

/**
 * @brief A deleter struct for aligned single objects.
 *
 * This struct provides a custom deleter for use with smart pointers, specifically designed
 * for handling aligned memory allocations of single objects. It ensures that the object's
 * destructor is called and the aligned memory is properly deallocated.
 *
 * @tparam T The type of the object to be deleted.
 *
 * @note This deleter assumes that the object was allocated with aligned memory.
 */
template<typename T>
struct AlignedDeleter {
    void operator()(T* ptr) const {
        if (ptr) {
            // Ruft den Destruktor des Objekts manuell auf
            ptr->~T();
            // Gibt den Speicher frei
            std::free(ptr);
        }
    }
};

/**
 * @brief A deleter struct for aligned type arrays.
 *
 * This struct provides a custom deleter for use with smart pointers, specifically designed
 * for handling aligned memory allocations of arrays of objects. It ensures that the destructor
 * of each object in the array is called and the aligned memory is properly deallocated.
 *
 * @tparam T The type of the objects in the array to be deleted.
 *
 * @note This deleter assumes that the array was allocated with aligned memory and that the
 * `count` field accurately reflects the number of elements in the array.
 */
template<typename T>
struct AlignedDeleterArray {
    void operator()(T* ptr) const {
        if (ptr) {
            for (size_t i = 0; i < count; ++i) {
                ptr[i].~T();
            }
            std::free(ptr);
        }
    }
    
    size_t count;  // Number of elements in the array
};

/** @} */
