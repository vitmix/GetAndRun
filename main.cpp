#include "Common.h"

int wmain(int argc, LPWSTR* argv)
{
    std::vector<std::wstring> args;
    args.reserve(argc);

    for (int idx = 0; idx < argc; idx++)
        args.emplace_back(argv[idx]);

    common::Configuration conf;
    if (!conf.init(args))
    {
        std::wcerr << "Failed to parse command line" << std::endl;
        return 1;
    }

    common::Downloader downloader(conf);
    if (!downloader.init())
    {
        std::wcerr << "Failed to init downloader" << std::endl;
        return 1;
    }

    downloader.get();
    downloader.run();

    return 0;
}