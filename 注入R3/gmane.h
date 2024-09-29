



#ifndef GMANE_H
#define GMANE_H
#include <windows.h>
#include <winioctl.h>
#include <Winsvc.h>
#include <string>
#include <stdbool.h>
#include <numeric>
#include <ntstatus.h>


 // ÉùÃ÷ hDevice

#endif // GMANE_H


// IOCTL ¿ØÖÆ´úÂë
#define IOCTL_IO_SetPROTECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5021, METHOD_BUFFERED, FILE_ANY_ACCESS)


 BOOL LOADN();

 BOOLEAN   LOADENTRY(DWORD* message);

