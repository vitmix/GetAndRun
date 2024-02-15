#include "Utils.h"

namespace
{
    static constexpr const auto ERROR_MESSAGE_LENGTH = 512;

    static void LogError(
        DWORD flags, 
        HMODULE module, 
        DWORD error, 
        const std::wstring& description)
    {
        PWSTR buffer = NULL;

        const auto result = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER  |
            FORMAT_MESSAGE_IGNORE_INSERTS |
            flags,
            module,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&buffer),
            ERROR_MESSAGE_LENGTH,
            NULL);

        if (result)
        {
            std::wcerr << description << " : " << buffer << std::endl;
            LocalFree(buffer);
        }
        else
        {
            std::wcerr << "Error " << GetLastError() 
                << " while formatting message for " << error 
                << " in " << description << std::endl;
        }

        return;
    }

    static constexpr const auto usageText = R"(
Usage: GetAndRun.exe url [file_path] [param]
Options:
    file_path   Directory and file name (if given) of downloaded file
    param       Parameters to run the downloaded file
)";
}

namespace utils
{
    void LogInetError(DWORD error, const std::wstring& description)
    {
        LogError(
            FORMAT_MESSAGE_FROM_HMODULE,
            GetModuleHandle(L"wininet.dll"),
            error,
            description
        );
    }

    void LogSystemError(DWORD error, const std::wstring& description)
    {
        LogError(
            FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,
            error,
            description
        );
    }

    void ShowUsage()
    {
        std::cout << usageText << std::endl;
    }

}
