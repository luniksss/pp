#include <windows.h>
#include <string>
#include <iostream>

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
    int threadNumber = static_cast<int>(reinterpret_cast<INT_PTR>(lpParam));
    std::cout << "Поток №" << threadNumber << " выполняет свою работу" << std::endl;
    ExitThread(0);
}

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if (argc != 2)
    {
        std::cout << "Использование: " << argv[0] << " <количество потоков>" << std::endl;
        return 1;
    }

    const int threadCount = std::stoi(argv[1]);
    if (threadCount <= 0)
    {
        std::cout << "Количество потоков должно быть положительным числом" << std::endl;
        return 1;
    }

    auto* handles = new HANDLE[threadCount];

    for (int i = 0; i < threadCount; i++)
    {
        handles[i] = CreateThread(nullptr, 0, &ThreadProc, reinterpret_cast<LPVOID>(static_cast<INT_PTR>(i + 1)), CREATE_SUSPENDED, nullptr);
        if (handles[i] == nullptr)
        {
            std::cout << "Ошибка при создании потока " << i + 1 << std::endl;
            delete[] handles;
            return 1;
        }
    }

    for (int i = 0; i < threadCount; i++)
    {
        ResumeThread(handles[i]);
    }

    WaitForMultipleObjects(threadCount, handles, TRUE, INFINITE);

    for (int i = 0; i < threadCount; i++)
    {
        CloseHandle(handles[i]);
    }

    delete[] handles;

    return 0;
}