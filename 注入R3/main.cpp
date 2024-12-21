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


		printf("��������ʧ��!\n");



		system("pause");

		return FALSE;
	
	}


	HWND window_handle = NULL;

	// һֱ���ң�ֱ���ҵ�����Ϊֹ
	while (!(window_handle = FindWindow(L"CrossFire", L"��Խ����")))
	{
		// ÿ��δ�ҵ�����ʱ��ӡ���󲢼�������
		printf("���ڵȴ�cf����...\n");
		Sleep(1000); // �ȴ�1������ԣ�����������Դ����
	}


    printf("success: ����Ŀ��,����ע��....\n");

	Sleep(4000);

	const wchar_t* targetProcessName = L"crossfire.exe";
	//const wchar_t* targetProcessName = L"procexp64.exe";
	DWORD pid = GetProcessIdByName(targetProcessName);


	while (1) 
	{
	
		NTSTATUS status = LOADENTRY(&pid);
		if (status == 1)
		{
			printf("success:ע��ɹ�\n");
		}
		else
		{
			printf("ע��ʧ��\n");
		}
	


		printf("Ҫ��ע��ûЧ��,�����˻���,���س������ٴ�����ע��\n");

		system("pause");
	}


	system("pause");
}