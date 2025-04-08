#ifndef OPENCV2_HPP
#define OPENCV2_HPP

/**
 * @brief This header exists because opencv2 is a pain to handle and throws a bunch of warnings that can be ignored
 * 
 * Remember kids: Compile with all warnings are errors enabled.
 */

// Disable warnings
#ifdef _MSC_VER // For Microsoft Visual C++
    #pragma warning(push)
    #pragma warning(disable : 4996) 
#elif defined(__GNUC__) || defined(__clang__) // For GCC and Clang
    #pragma GCC diagnostic push   
    #pragma GCC diagnostic ignored "-Wall"
    #pragma GCC diagnostic ignored "-Wextra"
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif

// Include OpenCV headers
#undef EPS
#include <opencv2/opencv.hpp>
#define EPS 192

// Restore warnings
#ifdef _MSC_VER
    #pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif

#endif // OPENCV2_HPP
