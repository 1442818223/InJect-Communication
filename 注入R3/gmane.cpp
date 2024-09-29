#include "gmane.h"
#include "LoadDriver.h"
#include "api.h"
#include <iostream>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 打开设备
HANDLE OpenDevice(LPCWSTR devicePath) {
	HANDLE hDevice = CreateFile(
		devicePath,                     // 设备路径
		GENERIC_WRITE | GENERIC_READ,    // 读写权限
		FILE_SHARE_READ | FILE_SHARE_WRITE, // 共享读写
		NULL,                         // 安全属性
		OPEN_EXISTING,                // 打开现有文件
		0,                            // 文件属性
		NULL                          // 模板文件
	);

	if (hDevice == INVALID_HANDLE_VALUE) {
		// 打开失败，输出错误信息
		printf("打开失败\n");
	}

	return hDevice;
}
// 检查设备是否已加载
BOOL IsDriverLoaded() {
	HANDLE hDevice = OpenDevice(L"\\\\.\\NoScreen");
	if (hDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(hDevice);
		return TRUE; // 设备已加载
	}
	return FALSE; // 设备未加载
}
// 发送 IOCTL 请求
BOOL SendIoctl(HANDLE hDevice, DWORD ioctlCode, DWORD* 数据) {
	DWORD returned = 0;

	BOOL success = DeviceIoControl(
		hDevice,
		ioctlCode,
		数据,             // 输入缓冲区
		sizeof(DWORD), // 输入缓冲区大小
		nullptr,
		0,
		&returned,        // 返回的字节数
		nullptr           // 重叠结构
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

	 // 检查驱动是否已加载
	 if (!IsDriverLoaded()) {
		 printf("驱动未加载，正在加载...\n");

		 // 如果驱动未加载，重新加载驱动
		 if (!SH_DriverLoad()) {
			 return FALSE; // 加载驱动失败
		 }
	 }


	 return  TRUE;
 }


// 启动函数
  BOOLEAN   LOADENTRY(DWORD* message) {

	
	HANDLE hDevice = OpenDevice(L"\\\\.\\NoScreen");

	BOOL ison = SendIoctl(hDevice, IOCTL_IO_SetPROTECT, message);


	CloseHandle(hDevice); // 确保在结束前关闭设备句柄

	return ison;
}
