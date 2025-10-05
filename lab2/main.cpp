#include <format>
#include <iostream>
#include <string>
#include <ctime>
#include "BMPUtils.h"

struct ParamsInfo
{
    std::string inputFilePath;
    std::string outputFilePath;
    unsigned threadCount;
    unsigned coreCount;
};

ParamsInfo ParseArgs(const int argc, char **argv)
{
    if (argc != 5)
    {
        throw std::invalid_argument(
            std::format("Usage: {} input.bmp output.bmp threads cores", argv[0])
        );
    }

    return {
        argv[1],
        argv[2],
        static_cast<unsigned>(std::stoi(argv[3])),
        static_cast<unsigned>(std::stoi(argv[4]))
    };
}

int main(const int argc, char **argv)
{
    try
    {
        const clock_t startTime = clock();
        constexpr auto count = 100;
        const auto [
            inputFilePath,
            outputFilePath,
            threadCount,
            coreCount
        ] = ParseArgs(argc, argv);

        auto originalBMP = ReadBMP(inputFilePath);
        SetCoreCount(coreCount);

        BMPImage blurredBMP;
        for (int i = 0; i < count; ++i)
        {
            blurredBMP = BlurBMP(originalBMP, threadCount);
        }

        WriteBMP(outputFilePath, blurredBMP);
        std::cout << coreCount << '\t' << threadCount << '\t' << clock() - startTime << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}