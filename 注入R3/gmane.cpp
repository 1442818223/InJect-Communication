#include "gmane.h"
#include "LoadDriver.h"
#include "api.h"
#include <iostream>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ���豸
HANDLE OpenDevice(LPCWSTR devicePath) {
	HANDLE hDevice = CreateFile(
		devicePath,                     // �豸·��
		GENERIC_WRITE | GENERIC_READ,    // ��дȨ��
		FILE_SHARE_READ | FILE_SHARE_WRITE, // �����д
		NULL,                         // ��ȫ����
		OPEN_EXISTING,                // �������ļ�
		0,                            // �ļ�����
		NULL                          // ģ���ļ�
	);

	if (hDevice == INVALID_HANDLE_VALUE) {
		// ��ʧ�ܣ����������Ϣ
		printf("��ʧ��\n");
	}

	return hDevice;
}
// ����豸�Ƿ��Ѽ���
BOOL IsDriverLoaded() {
	HANDLE hDevice = OpenDevice(L"\\\\.\\NoScreen");
	if (hDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(hDevice);
		return TRUE; // �豸�Ѽ���
	}
	return FALSE; // �豸δ����
}
// ���� IOCTL ����
BOOL SendIoctl(HANDLE hDevice, DWORD ioctlCode, DWORD* ����) {
	DWORD returned = 0;

	BOOL success = DeviceIoControl(
		hDevice,
		ioctlCode,
		����,             // ���뻺����
		sizeof(DWORD), // ���뻺������С
		nullptr,
		0,
		&returned,        // ���ص��ֽ���
		nullptr           // �ص��ṹ
	);

	if (success) {
		std::cout << "Successfully sent IOCTL code: " << ioctlCode << std::endl;
		return 1;
	}
	else {
		std::cout << "Failed to send IOCTL code: " << ioctlCode << ". Error: " << GetLastError() << std::endl;
		return 0;
	}
}

 
 BOOL LOADN()
 {
	 HANDLE hDevice = NULL;

	 // ��������Ƿ��Ѽ���
	 if (!IsDriverLoaded()) {
		 printf("����δ���أ����ڼ���...\n");

		 // �������δ���أ����¼�������
		 if (!SH_DriverLoad()) {
			 return FALSE; // ��������ʧ��
		 }
	 }


	 return  TRUE;
 }


// ��������
  BOOLEAN   LOADENTRY(DWORD* message) {

	
	HANDLE hDevice = OpenDevice(L"\\\\.\\NoScreen");

	BOOL ison = SendIoctl(hDevice, IOCTL_IO_SetPROTECT, message);


	CloseHandle(hDevice); // ȷ���ڽ���ǰ�ر��豸���

	return ison;
}
