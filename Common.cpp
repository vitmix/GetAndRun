#include "Common.h"
#include <filesystem>

namespace common
{
    Downloader::~Downloader()
    {
        if (downloadFile != INVALID_HANDLE_VALUE)
            CloseHandle(downloadFile);
    }

    bool Downloader::init()
    {
        if (req.init() != ERROR_SUCCESS)
        {
            std::wcerr << "Failed to init Request" << std::endl;
            return false;
        }

        // try to open file to write down downloaded .exe
        //std::wcout << "CreateFile for file: " << conf.getOutputFile() << std::endl;

        downloadFile = CreateFile(
            conf.getOutputFile().c_str(),
            GENERIC_WRITE, // Do we need the GENERIC_EXECUTE ?..
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (downloadFile == INVALID_HANDLE_VALUE)
        {
            utils::LogSystemError(GetLastError(), L"CreateFile for downloaded file has failed");
            return false;
        }

        return true;
    }

    Request::Request(const Configuration& c)
        : conf{c}
    {}

    Request::~Request()
    {}

    STATUS_CODE Request::init()
    {
        ctxt = std::make_unique<Context>();

        if (!ctxt)
            return ERROR_NOT_ENOUGH_MEMORY;

        ctxt->sessionHandle = InternetOpen(
            L"GetAndRun",
            INTERNET_OPEN_TYPE_DIRECT,
            nullptr, // no proxy name
            nullptr, // proxy bypass, do not bypass any address
            0
        );

        if (!ctxt->sessionHandle)
        {
            utils::LogInetError(GetLastError(), L"InternetOpen");
            return false;
        }

        ctxt->requestHandle = InternetOpenUrl(
            ctxt->sessionHandle,
            conf.getUrl().c_str(),
            nullptr,
            0,
            INTERNET_FLAG_RELOAD,
            0
        );

        if (!ctxt->requestHandle)
        {
            auto status = GetLastError();
            utils::LogInetError(status, L"InternetOpenUrl");
            return status;
        }

        ctxt->outputBuffer = std::make_unique<char[]>(BUFFER_LEN);

        if (!ctxt->outputBuffer)
            return ERROR_NOT_ENOUGH_MEMORY;

        return ERROR_SUCCESS;
    }

    const States& Request::getState() const
    {
        return ctxt ? ctxt->state : States::UNDEFINED;
    }

    void Request::setState(States nextState)
    {
        if (ctxt)
            ctxt->state = nextState;
    }

    STATUS_CODE Request::recv()
    {
        if (!ctxt)
            return ERROR_OPERATION_ABORTED;

        const auto result = InternetReadFile(
            ctxt->requestHandle,
            ctxt->outputBuffer.get(),
            BUFFER_LEN,
            &ctxt->downloadedBytes
        );

        STATUS_CODE returnCode{ ERROR_SUCCESS };
        
        if (!result)
        {
            returnCode = GetLastError();
            if (returnCode == ERROR_IO_PENDING)
                std::wcerr << "Waiting for InternetReadFile to complete" << std::endl;
            else
                utils::LogInetError(returnCode, L"InternetReadFile");
        }

        return returnCode;
    }

    std::tuple<DWORD, LPSTR> Request::getResponse() const
    {
        if (!ctxt)
            return { 0, nullptr };

        return { ctxt->downloadedBytes, ctxt->outputBuffer.get() };
    }

    void Downloader::get()
    {
        STATUS_CODE code{ ERROR_SUCCESS };
        bool finishedWriting{ false };

        while (code == ERROR_SUCCESS && req.getState() != States::COMPLETE)
        {
            switch (req.getState())
            {
            case States::RECV_DATA:
            {
                req.setState(States::WRITE_DATA);
                code = req.recv();
                break;
            }
            case States::WRITE_DATA:
            {
                req.setState(States::RECV_DATA);
                code = writeDownloadedFile(finishedWriting);
                
                if (finishedWriting)
                    req.setState(States::COMPLETE);

                break;
            }
            case States::UNDEFINED:
            default:
            {
                break;
            }
            }
        }
    }

    void Downloader::run()
    {
        const auto appName = conf.getOutputFile();
        auto params = conf.getParamToRunWith();

        if (appName.empty())
            return;

        if (downloadFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(downloadFile);
            downloadFile = INVALID_HANDLE_VALUE;
        }

        STARTUPINFOW startupInfo = { sizeof(STARTUPINFOW) };
        PROCESS_INFORMATION processInfo = {};

        const auto success = CreateProcess(
            appName.c_str(),
            params.data(),
            nullptr,
            nullptr,
            false,
            DETACHED_PROCESS,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

        if (success)
        {
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
        }
        else
        {
            utils::LogSystemError(GetLastError(), L"CreateProcess");
        }
    }

    STATUS_CODE Downloader::writeDownloadedFile(bool& finishedWriting)
    {
        DWORD bytesWritten{ 0 };
        finishedWriting = false;

        auto [respSize, respBuffer] = req.getResponse();
        
        if (respSize == 0)
        {
            finishedWriting = true;
            return ERROR_SUCCESS;
        }

        if (!WriteFile(
            downloadFile,
            respBuffer,
            respSize,
            &bytesWritten,
            nullptr) || bytesWritten != respSize)
        {
            auto status = GetLastError();
            utils::LogSystemError(status, L"WriteFile");
            return status;
        }

        return ERROR_SUCCESS;
    }

    bool Configuration::init(const std::vector<std::wstring>& args)
    {
        if (args.size() < 2)
        {
            utils::ShowUsage();
            return false;
        }

        url.assign(args.at(1));
        std::wcout << "URL to get file from: " << url << std::endl;

        if (args.size() >= 3)
        {
            namespace fs = std::filesystem;
            
            fs::path filePath(args.at(2)), correctPath;
            
            if (fs::exists(filePath))
            {
                outputDirectory.assign(args.at(2));
            }
            else if (fs::exists(filePath.parent_path())) // get file name from provided path
            {
                outputDirectory.assign(filePath.parent_path().wstring());
                outputFileName.assign(filePath.filename().wstring());
            }
            else
            {
                std::wcerr << "Invalid file path: " << filePath.wstring() << std::endl;
                return false;
            }

            if (!outputDirectory.empty())
            {
                if (outputDirectory.back() != L'/' && outputDirectory.back() != L'\\')
                    outputDirectory.append(L"\\");
            }
        }
        
        if (outputDirectory.empty())
            outputDirectory.assign(L"./");

        // try to get file name from URL or just use default name
        if (outputFileName.empty())
        {
            const auto pos = url.rfind(L'/');
            if (pos != std::wstring::npos && url.at(pos) != url.back())
                outputFileName.assign(url.substr(pos + 1));
            else
                outputFileName.assign(L"downloadedFile.exe");
        }

        std::wcout << "Directory to save: " << outputDirectory << std::endl;
        std::wcout << "File name: " << outputFileName << std::endl;

        if (args.size() >= 4)
        {
            for (size_t idx = 3; idx < args.size(); idx++)
                paramToRunWith.append(args.at(idx));

            std::wcout << "Params to run with: " << paramToRunWith << std::endl;
        }

        return true;
    }

    const std::wstring& Configuration::getUrl() const
    {
        return url;
    }

    std::wstring Configuration::getOutputFile() const
    {
        return outputDirectory + outputFileName;
    }

    std::wstring Configuration::getParamToRunWith() const
    {
        return paramToRunWith;
    }

    Request::Context::~Context()
    {
        if (sessionHandle)
            InternetCloseHandle(sessionHandle);

        if (requestHandle)
            InternetCloseHandle(requestHandle);
    }
}
