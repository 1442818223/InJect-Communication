#pragma once
#include <ntifs.h>

#define DEVICE_NAME L"\\Device\\NoScreen"
#define DEVICE_LINK_NAME L"\\DosDevices\\NoScreen"


#define IOCTL_IO_SetPROTECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5021, METHOD_BUFFERED, FILE_ANY_ACCESS)




BOOLEAN WaitForProcess(CHAR* ProcessName);