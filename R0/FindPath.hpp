#pragma once

#include "struct.hpp"

typedef struct _PEB_LDR_DATA64 {
    ULONG Length;
    UCHAR Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID EntryInProgress;
    UCHAR ShutdownInProgress;
    PVOID ShutdownThreadId;
} PEB_LDR_DATA64, * PPEB_LDR_DATA64;

//typedef struct _LDR_DATA_TABLE_ENTRY {
//    LIST_ENTRY InLoadOrderLinks;
//    LIST_ENTRY InMemoryOrderLinks;
//    LIST_ENTRY InInitializationOrderLinks;
//    PVOID DllBase;
//    PVOID EntryPoint;
//    ULONG SizeOfImage;
//    UNICODE_STRING FullDllName;
//    UNICODE_STRING BaseDllName;
//    ULONG Flags;
//    unsigned short LoadCount;
//    unsigned short TlsIndex;
//    union {
//        LIST_ENTRY HashLinks;
//        struct {
//            PVOID SectionPointer;
//            ULONG CheckSum;
//        };
//    };
//    union {
//        ULONG TimeDateStamp;
//        PVOID LoadedImports;
//    };
//    PVOID EntryPointActivationContext;
//    PVOID PatchInformation;
//    LIST_ENTRY ForwarderLinks;
//    LIST_ENTRY ServiceTagLinks;
//    LIST_ENTRY StaticLinks;
//} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

//typedef struct _PEB64 {
//    UCHAR InheritedAddressSpace;
//    UCHAR ReadImageFileExecOptions;
//    UCHAR BeingDebugged;
//    union {
//        UCHAR BitField;
//        struct {
//            UCHAR ImageUsesLargePages : 1;
//            UCHAR IsProtectedProcess : 1;
//            UCHAR IsImageDynamicallyRelocated : 1;
//            UCHAR SkipPatchingUser32Forwarders : 1;
//            UCHAR IsPackagedProcess : 1;
//            UCHAR IsAppContainer : 1;
//            UCHAR IsProtectedProcessLight : 1;
//            UCHAR IsLongPathAwareProcess : 1;
//        };
//    };
//    UCHAR Padding0[4];
//    ULONGLONG Mutant;
//    ULONGLONG ImageBaseAddress;
//    PPEB_LDR_DATA64 Ldr;
//    ULONGLONG ProcessParameters;
//    ULONGLONG SubSystemData;
//    ULONGLONG ProcessHeap;
//    ULONGLONG FastPebLock;
//    ULONGLONG AtlThunkSListPtr;
//    ULONGLONG IFEOKey;
//} PEB64, * PPEB64;

//typedef struct _CURDIR {
//    UNICODE_STRING DosPath;
//    VOID* Handle;
//} CURDIR, * PCURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG MaximumLength;
    ULONG Length;
    ULONG Flags;
    ULONG DebugFlags;
    VOID* ConsoleHandle;
    ULONG ConsoleFlags;
    VOID* StandardInput;
    VOID* StandardOutput;
    VOID* StandardError;
    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

EXTERN_C
NTKERNELAPI PPEB PsGetProcessPeb(PEPROCESS Process);

UNICODE_STRING GetProcessFullImagePath(HANDLE pid);

NTSTATUS SelfDeleteFile(UNICODE_STRING* path);

