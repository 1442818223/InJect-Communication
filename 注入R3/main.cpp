#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h> 
#include "gmane.h"

DWORD GetProcessIdByName(const wchar_t* processName) {
	PROCESSENTRY32 pe32;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe32)) {
		CloseHandle(hSnapshot);
		return 0;
	}

	do {
		if (_wcsicmp(pe32.szExeFile, processName) == 0) {
			CloseHandle(hSnapshot);
			return pe32.th32ProcessID;
		}
	} while (Process32Next(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return 0;
}
int main()
{

	if (!LOADN()) 
	{


		printf("驱动加载失败!\n");



		system("pause");

		return FALSE;
	
	}


	HWND window_handle = NULL;

	// 一直查找，直到找到窗口为止
	while (!(window_handle = FindWindow(L"CrossFire", L"穿越火线")))
	{
		// 每次未找到窗口时打印错误并继续尝试
		printf("正在等待cf加载...\n");
		Sleep(1000); // 等待1秒后重试，避免过多的资源消耗
	}


    printf("success: 发现目标,正在注入....\n");

	Sleep(4000);

	const wchar_t* targetProcessName = L"crossfire.exe";
	//const wchar_t* targetProcessName = L"procexp64.exe";
	DWORD pid = GetProcessIdByName(targetProcessName);


	while (1) 
	{
	
		NTSTATUS status = LOADENTRY(&pid);
		if (status == 1)
		{
			printf("success:注入成功\n");
		}
		else
		{
			printf("注入失败\n");
		}
	


		printf("要是注入没效果,进入人机房,按回车可以再次重新注入\n");

		system("pause");
	}


	system("pause");
}