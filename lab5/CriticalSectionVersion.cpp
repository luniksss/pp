#include <windows.h>
#include <string>
#include <fstream>

CRITICAL_SECTION csBalance;

int ReadFromFile()
{
    std::ifstream file("balance.txt");
    int result = 0;
    if (file)
    {
        file >> result;
    }
    return result;
}

void WriteToFile(int data)
{
    std::ofstream file("balance.txt");
    if (file)
    {
        file << data << std::endl;
    }
}

int GetBalance()
{
    EnterCriticalSection(&csBalance);
    int balance = ReadFromFile();
    LeaveCriticalSection(&csBalance);
    return balance;
}

void Deposit(int money)
{
    EnterCriticalSection(&csBalance);
    int balance = ReadFromFile();
    balance += money;
    WriteToFile(balance);
    LeaveCriticalSection(&csBalance);
    printf("Balance after deposit: %d\n", balance);
}

void Withdraw(int money)
{
    EnterCriticalSection(&csBalance);
    int balance = ReadFromFile();
    
    if (balance < money)
    {
        printf("Cannot withdraw money, balance lower than %d\n", money);
    }
    else
    {
        balance -= money;
        WriteToFile(balance);
        printf("Balance after withdraw: %d\n", balance);
    }
    LeaveCriticalSection(&csBalance);
}

DWORD WINAPI DoDeposit(CONST LPVOID lpParameter)
{
    Deposit(static_cast<int>(reinterpret_cast<intptr_t>(lpParameter)));
    ExitThread(0);
}

DWORD WINAPI DoWithdraw(CONST LPVOID lpParameter)
{
    Withdraw(static_cast<int>(reinterpret_cast<intptr_t>(lpParameter)));
    ExitThread(0);
}

int main()
{
    const int THREAD_COUNT = 50;
    HANDLE handles[THREAD_COUNT];
    
    InitializeCriticalSection(&csBalance);
    WriteToFile(0);

    SetProcessAffinityMask(GetCurrentProcess(), 1);
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (i % 2 == 0)
        {
            handles[i] = CreateThread(NULL, 0, DoDeposit, (LPVOID)230, CREATE_SUSPENDED, NULL);
        }
        else
        {
            handles[i] = CreateThread(NULL, 0, DoWithdraw, (LPVOID)1000, CREATE_SUSPENDED, NULL);
        }
        ResumeThread(handles[i]);
    }

    WaitForMultipleObjects(THREAD_COUNT, handles, TRUE, INFINITE);

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        CloseHandle(handles[i]);
    }
    
    printf("Final Balance: %d\n", GetBalance());
    getchar();
    DeleteCriticalSection(&csBalance);

    return 0;
}