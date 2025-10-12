#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

HANDLE g_fileMutex;
std::ofstream g_outputFile;
DWORD g_startTime;

DWORD GetCurrentTimeMs()
{
    return GetTickCount() - g_startTime;
}

void WriteToFile(int threadNumber, DWORD operationTime)
{
    WaitForSingleObject(g_fileMutex, INFINITE);
    g_outputFile << threadNumber << "|" << operationTime << std::endl;
    ReleaseMutex(g_fileMutex);
}

void LongOperation()
{
    for (int i = 0; i < 1000000; ++i)
    {
        const auto k = i * i;
        i += k;
        for (int o = 0; o < k; ++o)
        {
            i -= k;
        }
    }
}

void AddThreadPriority(int threadCount, int index, HANDLE* handles)
{
    for (int i = 0; i < threadCount; i++)
    {
        if (i == index)
        {
            SetThreadPriority(handles[i], THREAD_PRIORITY_HIGHEST);
        }
        else
        {
            SetThreadPriority(handles[i], THREAD_PRIORITY_LOWEST);
        }
    }
}

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
    int threadNumber = static_cast<int>(reinterpret_cast<INT_PTR>(lpParam));
    const int operationCount = 10;

    for (int i = 0; i < operationCount; i++)
    {
        DWORD operationStart = GetCurrentTimeMs();
        LongOperation();
        DWORD operationEnd = GetCurrentTimeMs();

        WriteToFile(threadNumber, operationEnd);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << "=== ЭКСПЕРИМЕНТ С ПРИОРИТЕТАМИ ПОТОКОВ ===" << std::endl;
    std::cout << "ИНСТРУКЦИЯ:" << std::endl;
    std::cout << "1. Запустите Диспетчер задач (Ctrl+Shift+Esc)" << std::endl;
    std::cout << "2. Найдите процесс 'lab3.exe' или другое имя вашей программы" << std::endl;
    std::cout << "3. Щелкните правой кнопкой -> 'Перейти к деталям'" << std::endl;
    std::cout << "4. Щелкните правой кнопкой на процессе -> 'Задать сходство...'" << std::endl;
    std::cout << "5. Оставьте ТОЛЬКО ОДИН процессор выбранным" << std::endl;
    std::cout << "6. Нажмите OK" << std::endl;
    std::cout << std::endl;
    std::cout << "После настройки ограничения процессоров нажмите ENTER для запуска программы..." << std::endl;
    std::cin.get();

    g_startTime = GetTickCount();

    g_fileMutex = CreateMutex(nullptr, FALSE, nullptr);
    if (g_fileMutex == nullptr)
    {
        std::cout << "Ошибка при создании мьютекса" << std::endl;
        return 1;
    }

    // g_outputFile.open("./thread_operations.txt");
    g_outputFile.open("thread_operations_single_cpu.txt");
    if (!g_outputFile.is_open())
    {
        std::cout << "Ошибка при открытии файла для записи" << std::endl;
        CloseHandle(g_fileMutex);
        return 1;
    }

    const int threadCount = 2;
    HANDLE handles[threadCount];

    std::cout << "Создание " << threadCount << " потоков..." << std::endl;
    std::cout << "Время начала работы программы: " << g_startTime << " мс" << std::endl;

    for (int i = 0; i < threadCount; i++)
    {
        handles[i] = CreateThread(
            nullptr,
            0,
            &ThreadProc,
            reinterpret_cast<LPVOID>(static_cast<INT_PTR>(i + 1)),
            CREATE_SUSPENDED,
            nullptr
        );

        if (handles[i] == nullptr)
        {
            std::cout << "Ошибка при создании потока " << i + 1 << std::endl;

            for (int j = 0; j < i; j++)
            {
                CloseHandle(handles[j]);
            }

            g_outputFile.close();
            CloseHandle(g_fileMutex);
            return 1;
        }

        std::cout << "Поток " << i + 1 << " создан" << std::endl;
    }

    // AddThreadPriority(threadCount, 1, handles);
    std::cout << "Запуск потоков..." << std::endl;
    for (auto & handle : handles)
    {
        ResumeThread(handle);
    }

    std::cout << "Ожидание завершения потоков..." << std::endl;
    WaitForMultipleObjects(threadCount, handles, TRUE, INFINITE);

    for (auto & handle : handles)
    {
        CloseHandle(handle);
    }

    g_outputFile.close();
    CloseHandle(g_fileMutex);

    std::cout << "Все потоки завершены." << std::endl;
    std::cout << "Результаты записаны в файл: thread_operations.txt" << std::endl;
    std::cout << "Общее время работы: " << GetCurrentTimeMs() << " мс" << std::endl;

    return 0;
}