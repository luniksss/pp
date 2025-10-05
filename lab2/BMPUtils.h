#ifndef BMPUTILS_H
#define BMPUTILS_H
#pragma once
#include <cmath>
#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <windows.h>

//отключает выравнивание структур в памяти. структуры будут занимать ровно столько байт, сколько нужно, без дополнения.
#pragma pack(push, 1)
struct BMPFileHeader
{
    uint16_t fileType;
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offsetData;
};

struct BMPInfoHeader
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t sizeImage;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

struct BMPImage
{
    BMPFileHeader fileHeader{};
    BMPInfoHeader infoHeader{};
    std::vector<uint8_t> pixelData;
    std::vector<uint8_t> additionalData;
};

inline BMPImage ReadBMP(const std::string &filePath)
{
    BMPImage bmpImage;

    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    file.read(reinterpret_cast<char *>(&bmpImage.fileHeader), sizeof(bmpImage.fileHeader));
    if (bmpImage.fileHeader.fileType != 0x4D42)
    {
        throw std::runtime_error("Not a valid BMP file.");
    }

    file.read(reinterpret_cast<char *>(&bmpImage.infoHeader), sizeof(bmpImage.infoHeader));
    if (bmpImage.infoHeader.bitCount != 24)
    {
        throw std::runtime_error("Only 24-bit BMP files are supported.");
    }

    const size_t additionalDataSize = bmpImage.fileHeader.offsetData -
                                     sizeof(BMPFileHeader) -
                                     sizeof(BMPInfoHeader);

    if (additionalDataSize > 0)
    {
        bmpImage.additionalData.resize(additionalDataSize);
        file.read(reinterpret_cast<char *>(bmpImage.additionalData.data()), additionalDataSize);
    }

    const int width = bmpImage.infoHeader.width;
    const int height = std::abs(bmpImage.infoHeader.height);
    constexpr int bytesPerPixel = 3;
    const int rowSize = (width * bytesPerPixel + 3) & ~3;
    if (bmpImage.infoHeader.sizeImage == 0)
    {
        const int dataSize = rowSize * height;
        bmpImage.infoHeader.sizeImage = dataSize;
    }

    bmpImage.pixelData.resize(bmpImage.infoHeader.sizeImage);
    file.read(reinterpret_cast<char *>(bmpImage.pixelData.data()), bmpImage.infoHeader.sizeImage);

    return bmpImage;
}

inline void WriteBMP(const std::string &filePath, const BMPImage &bmpImage)
{
    std::ofstream file(filePath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    file.write(reinterpret_cast<const char *>(&bmpImage.fileHeader), sizeof(bmpImage.fileHeader));
    file.write(reinterpret_cast<const char *>(&bmpImage.infoHeader), sizeof(bmpImage.infoHeader));

    if (!bmpImage.additionalData.empty())
    {
        file.write(reinterpret_cast<const char *>(bmpImage.additionalData.data()),
                   bmpImage.additionalData.size());
    }

    file.write(reinterpret_cast<const char *>(bmpImage.pixelData.data()), bmpImage.pixelData.size());
    if (!file.good())
    {
        throw std::runtime_error("Error writing to file: " + filePath);
    }
}

inline void SetCoreCount(unsigned count)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    const auto maxCoreCount = info.dwNumberOfProcessors;

    if (count > maxCoreCount)
    {
        count = maxCoreCount;
    }

    const auto hProcess = GetCurrentProcess();
    const auto mask = static_cast<DWORD_PTR>(pow(2, count) - 1);

    SetThreadAffinityMask(hProcess, mask);
}

struct ThreadData
{
    BMPImage *bmpImage;
    BMPImage *blurredImage;
    int startCol;
    int endCol;
};

inline DWORD WINAPI BlurSection(const LPVOID param)
{
    const ThreadData *data = static_cast<ThreadData *>(param);
    const BMPImage *sourceImage = data->bmpImage;
    BMPImage *blurredImage = data->blurredImage;
    const int startCol = data->startCol;
    const int endCol = data->endCol;

    const int width = sourceImage->infoHeader.width;
    const int height = std::abs(sourceImage->infoHeader.height);
    constexpr int bytesPerPixel = 3;
    const int rowSize = (width * bytesPerPixel + 3) & ~3;

    for (int y = 0; y < height; ++y)
    {
        for (int x = startCol; x < endCol; ++x)
        {
            int redSum = 0, greenSum = 0, blueSum = 0, pixelCount = 0;

            for (int offsetY = -1; offsetY <= 1; ++offsetY)
            {
                for (int offsetX = -1; offsetX <= 1; ++offsetX)
                {
                    const int neighborY = y + offsetY;
                    const int neighborX = x + offsetX;

                    if (neighborX >= 0 && neighborX < width && neighborY >= 0 && neighborY < height)
                    {
                        const int index = neighborY * rowSize + neighborX * bytesPerPixel;
                        blueSum += sourceImage->pixelData[index];
                        greenSum += sourceImage->pixelData[index + 1];
                        redSum += sourceImage->pixelData[index + 2];
                        pixelCount++;
                    }
                }
            }

            const int currentPixelIndex = y * rowSize + x * bytesPerPixel;
            blurredImage->pixelData[currentPixelIndex] = static_cast<uint8_t>(blueSum / pixelCount);
            blurredImage->pixelData[currentPixelIndex + 1] = static_cast<uint8_t>(greenSum / pixelCount);
            blurredImage->pixelData[currentPixelIndex + 2] = static_cast<uint8_t>(redSum / pixelCount);
        }
    }

    return 0;
}

inline BMPImage BlurBMP(BMPImage &bmpImage, const unsigned threadCount)
{
    BMPImage blurredImage;

    blurredImage.fileHeader = bmpImage.fileHeader;
    blurredImage.infoHeader = bmpImage.infoHeader;
    blurredImage.additionalData = bmpImage.additionalData;

    const int width = bmpImage.infoHeader.width;
    const int height = std::abs(bmpImage.infoHeader.height);
    const int bytesPerPixel = 3;
    const int rowSize = (width * bytesPerPixel + 3) & ~3;
    const int dataSize = rowSize * height;

    blurredImage.infoHeader.sizeImage = dataSize;
    blurredImage.fileHeader.fileSize = sizeof(BMPFileHeader) +
                                      sizeof(BMPInfoHeader) +
                                      blurredImage.additionalData.size() +
                                      dataSize;

    blurredImage.pixelData.resize(dataSize);
    std::vector<HANDLE> threads(threadCount);
    std::vector<ThreadData> threadData(threadCount);

    const int totalCols = width;
    const int colsPerThread = totalCols / threadCount;

    for (unsigned i = 0; i < threadCount; ++i)
    {
        const int startCol = i * colsPerThread;
        const int endCol = (i == threadCount - 1) ? totalCols : startCol + colsPerThread;
        threadData[i] = {&bmpImage, &blurredImage, startCol, endCol};
        threads[i] = CreateThread(nullptr, 0, BlurSection, &threadData[i], 0, nullptr);

        if (threads[i] == nullptr)
        {
            throw std::runtime_error("Failed to create thread");
        }
    }

    WaitForMultipleObjects(threadCount, threads.data(), TRUE, INFINITE);

    for (const HANDLE thread : threads)
    {
        CloseHandle(thread);
    }

    return blurredImage;
}

#endif //BMPUTILS_H