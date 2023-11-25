#pragma once
#include <bit>
#include <iostream>
template <typename... T>
inline auto& BMCWEB_LOG_DEBUG(const char* format, T... args)
{
    std::cout << format;
    (std::cout << ... << args);
    std::cout << "\n";
    return std::cout;
}
template <typename... T>
inline auto& BMCWEB_LOG_ERROR(const char* format, T... args)
{
    std::cout << format;
    (std::cout << ... << args);
    std::cout << "\n";
    return std::cout;
}
template <typename... T>
inline auto& BMCWEB_LOG_CRITICAL(const char* format, T... args)
{
    std::cout << format;
    (std::cout << ... << args);
    std::cout << "\n";
    return std::cout;
}
template <typename... T>
inline auto& BMCWEB_LOG_INFO(const char* format, T... args)
{
    std::cout << format;
    (std::cout << ... << args);
    std::cout << "\n";
    return std::cout;
}
template <typename... T>
inline auto& BMCWEB_LOG_WARNING(const char* format, T... args)
{
    std::cout << format;
    (std::cout << ... << args);
    std::cout << "\n";
    return std::cout;
}
template <typename T>
const void* logPtr(T p)
{
    static_assert(std::is_pointer<T>::value,
                  "Can't use logPtr without pointer");
    return std::bit_cast<const void*>(p);
}
inline void pritnFileds(auto& message)
{
    for (auto& p : message)
    {
        std::cout << p.name() << " : " << p.value() << "\n";
    }
}
