#ifndef MACRO_H_
#define MACRO_H_

#include <cassert>

namespace util
{
//! if condition is false, execute the action
#define Require(condition, ret, action) \
    {                                   \
        if (!(condition))               \
        {                               \
            action;                     \
            return ret;                 \
        }                               \
    }
#define AssertRequire(condition, info) assert(condition && info)

//! time test
#define TIME_START() \
    do               \
    {                \
    auto time_test_start = std::chrono::system_clock::now()
#define TIME_END(time, action)                                                                           \
    if (util::Enable())                                                                                  \
    {                                                                                                    \
        auto time_test_end = std::chrono::system_clock::now();                                           \
        auto diff = time_test_end - time_test_start;                                                     \
        unsigned time = (unsigned)(std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()); \
        action;                                                                                          \
    }                                                                                                    \
    }                                                                                                    \
    while (0)
}  // namespace util
#endif
