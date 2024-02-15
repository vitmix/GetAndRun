#pragma once

#include <windows.h>

#include <iostream>
#include <string>
#include <variant>

namespace utils
{
    void LogInetError(DWORD error, const std::wstring& description);

    void LogSystemError(DWORD error, const std::wstring& description);

    void ShowUsage();

    template <typename T, typename Error>
    class Result : public std::variant<T, Error>
    {
    public:
        using std::variant<T, Error>::variant;
        using std::variant<T, Error>::operator=;

        Error* error() { return std::get_if<Error>(this); }
        const Error* error() const { return std::get_if<Error>(this); }

        T* value() { return std::get_if<T>(this); }
        const T* value() const { return std::get_if<T>(this); }

        T* operator->() { return value(); }
        const T* operator->() const { return value(); }

        operator bool() const { return error() == nullptr; }
    };
}