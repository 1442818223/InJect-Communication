#pragma once
#include <ntifs.h>

EXTERN_C NTSTATUS InjectX64(HANDLE pid, char *shellcode, SIZE_T shellcodeSize);

NTSTATUS InjectX86(HANDLE pid, char *shellcode, SIZE_T shellcodeSize);