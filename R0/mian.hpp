#pragma once
#include <ntifs.h>

#define DEVICE_NAME L"\\device\\Feng2022"
#define DEVICE_LINK_NAME L"\\??\\tearWrite_driver"


#define IOCTL_IO_SetPROTECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2021, METHOD_BUFFERED, FILE_ANY_ACCESS)//ÄÚÍ¸¿ª¹Ø




BOOLEAN WaitForProcess(CHAR* ProcessName);