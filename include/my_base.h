//

#ifndef _MY_BASE
#define _MY_BASE 1

#include <type_traits>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <string.h>
namespace mystd
{

    
    void Print() { std::cout << std::endl; }

    template <typename T, typename... TS> void Print(T value, TS... values) {
    std::cout << value << "   ";
    Print(values...);
    }

    std::string GetSysteamErrorMessage(int code)
    {
        char buffer[1024];
        auto res = strerror_r(code, buffer, sizeof(buffer));
        return std::string(buffer);
    }

    template <typename T, typename... TS>
    void Exit(int code, T value, TS... values){
        Print("system message: ", GetSysteamErrorMessage(code),"    ", value, values...);
        exit(EXIT_FAILURE);
    }

    void Exit(const std::string &str)
    {
        std::cerr << "exit: "
                  << "user message: "
                  << str
                  << "    "
                  << "system message: "
                  << GetSysteamErrorMessage(errno)
                  << std::endl;

        exit(EXIT_FAILURE);
    }

    template <typename T>
    inline auto make_lambda_unique(T &&func)
    {
        return std::make_unique<T>(std::forward<T>(func));
    }

    class SysteamException : public std::exception
    {

        std::string m_message;
        int m_code;

    public:
        SysteamException(int errorCode)
        {
            m_code = errorCode;

            m_message = mystd::GetSysteamErrorMessage(errorCode);
        }

        SysteamException(std::string message) : m_message(std::move(message))
        {
        }

        int GetErrorCode()
        {
            return m_code;
        }

        const char *what() const noexcept override
        {
            return m_message.c_str();
        }
    };

    template <typename T>
    concept EnumType = std::is_enum_v<T>;

    template <EnumType T>
    inline T operator|(T left, T right) noexcept
    {
        using TN = typename std::underlying_type_t<T>;

        return static_cast<T>(static_cast<TN>(left) | static_cast<TN>(right));
    }

    template <EnumType T>
    inline T operator&(T left, T right) noexcept
    {
        using TN = typename std::underlying_type_t<T>;

        return static_cast<T>(static_cast<TN>(left) & static_cast<TN>(right));
    }

    class Delete_Base
    {

    public:
        Delete_Base() noexcept
        {
        }

        Delete_Base(const Delete_Base &) = delete;

        Delete_Base(Delete_Base &&) = delete;

        Delete_Base &operator=(Delete_Base &&) = delete;

        Delete_Base &operator=(const Delete_Base &) = delete;

        ~Delete_Base()
        {
        }
    };

} // namespace mystd
#endif