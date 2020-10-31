#ifndef _TRACE_H_
#define _TRACE_H_

#include <iostream>
#include <mutex>
#include <sstream>

#include "macro.h"

namespace util
{
struct Output
{
    //! @brief print bool
    static void Print(std::stringstream& data, const bool& value) { data << ((value == true) ? "true" : "false"); }

    //! @brief print normal data
    template <typename T>
    static void Print(std::stringstream& data, const T& value)
    {
        data << value;
    }

    //! @brief print pair
    template <typename T1, typename T2>
    static void Print(std::stringstream& data, const std::pair<T1, T2>& value)
    {
        Print(data, value.first);
        data << "=";
        Print(data, value.second);
    }
};

// whether is container type(has begin() & end())

//! @brief isn't container type
template <typename T, typename = void>
struct HasIterator : std::false_type
{
};

//! @brief is container type
template <typename T>
struct HasIterator<T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>> : std::true_type
{
};

//! @brief Print normal type
template <typename T, bool IsContainer, bool IsString>
struct Printer
{
    static void Out(std::stringstream& data, const T& t) { Output::Print(data, t); }
};

//!@brief Print the container
template <typename T>
struct Printer<T, true, false>
{
    static void PrintContainer(std::stringstream& data, const T& t)
    {
        auto begin = t.begin();
        auto end = t.end();
        data << "size=" << t.size() << ", value=[";
        while (begin != end)
        {
            Output::Print(data, *begin);
            begin++;
            data << (begin != end ? ", " : "]");
        }
    }

    static void Out(std::stringstream& data, const T& t) { PrintContainer(data, t); }
};

class Logger
{
public:
    static Logger* GetInstance()
    {
        static Logger logger;
        return &logger;
    }

    template <typename T1, typename... T2>
    void Trace(T1&& t1, T2&&... t2)
    {
        std::stringstream ss;
        Log(ss, t1, t2...);
        std::lock_guard<std::mutex> lk(m_lock);
        std::cout << ss.str() << std::endl;
    }

private:
    template <typename T1, typename... T2>
    void Log(std::stringstream& data, T1&& t1, T2&&... t2)
    {
        Printer<T1, HasIterator<T1>::value, std::is_same<T1, std::string&>::value>::Out(data, t1);
        if constexpr (sizeof...(t2) > 0)
        {
            Log(data, t2...);
        }
    }

    Logger() = default;
    ~Logger() = default;
    std::mutex m_lock;
};

}  // namespace util

#define Trace(...)                                       \
    do                                                   \
    {                                                    \
        util::Logger::GetInstance()->Trace(__VA_ARGS__); \
    } while (0)

#endif
