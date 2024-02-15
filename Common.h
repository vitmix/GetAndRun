#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include "Utils.h"

#include <wininet.h>

#include <cstdint>
#include <mutex>
#include <tuple>
#include <memory>
#include <vector>

namespace common
{
    static constexpr const auto BUFFER_LEN = 4096;
    
    using STATUS_CODE = DWORD;

    enum class States : std::uint8_t
    {
        RECV_DATA,
        WRITE_DATA,
        COMPLETE,
        UNDEFINED
    };

    class Configuration
    {
    public:

        bool init(const std::vector<std::wstring>& args);

        const std::wstring& getUrl() const;

        std::wstring getOutputFile() const;

        std::wstring getParamToRunWith() const;

    private:
        std::wstring url;
        std::wstring outputDirectory;
        std::wstring outputFileName;
        std::wstring paramToRunWith;
    };

    class Request
    {
    public:
        struct Context
        {
            Context() = default;
            ~Context();

            HINTERNET sessionHandle{ nullptr };
            HINTERNET requestHandle{ nullptr };
            DWORD downloadedBytes{ 0 };
            DWORD fileSize{ 0 };
            std::shared_ptr<char[]> outputBuffer{ nullptr }; // Maybe switch to some STL container
            States state{ States::RECV_DATA };
        };

    public:
        Request(const Configuration& c);

        ~Request();

        STATUS_CODE init();

        const States& getState() const;

        void setState(States nextState);

        STATUS_CODE recv();

        std::tuple<DWORD, LPSTR> getResponse() const;

    private:
        const Configuration& conf;
        std::unique_ptr<Context> ctxt{ nullptr };
    };

    class Downloader
    {
    public:

        Downloader(const Configuration& c)
            : conf{c}, req{conf}
        {}

        ~Downloader();

        bool init();

        void get();

        void run();

    private:
        STATUS_CODE writeDownloadedFile(bool& finishedWriting);

        const Configuration& conf;
        HANDLE downloadFile{ INVALID_HANDLE_VALUE };
        Request req;
    };
}
