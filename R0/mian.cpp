

#include "mian.hpp"
#include "FindPath.hpp"
#include "Inject.h"
#include "dll.h"


EXTERN_C int _fltused = 1;

ULONG64 LdrInPebOffset = 0x018;
ULONG64 ModListInPebOffset = 0x010;
EXTERN_C
NTKERNELAPI UCHAR* PsGetProcessImageFileName(IN PEPROCESS Process);
EXTERN_C
NTKERNELAPI PPEB PsGetProcessPeb(PEPROCESS Process);

ULONG_PTR ModuleBase1 = 0;

HANDLE pid = NULL;



typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;// 按加载顺序链接的链表
	LIST_ENTRY InMemoryOrderLinks;// 按内存顺序链接的链表
	LIST_ENTRY InInitializationOrderLinks;// 按初始化顺序链接的链表
	PVOID DllBase;// DLL 的基地址
	PVOID EntryPoint;// DLL 的入口点地址  
	ULONG SizeOfImage;// DLL 映像的大小
	UNICODE_STRING FullDllName;// 完整的 DLL 名称
	UNICODE_STRING BaseDllName;// DLL 的基本名称
	ULONG Flags;// 标志
	USHORT LoadCount;// 装载计数器
	USHORT TlsIndex;// TLS 索引
	union {
		LIST_ENTRY HashLinks;// 哈希链接
		struct {
			PVOID SectionPointer;// 段指针
			ULONG CheckSum;// 校验和
		};
	};
	union {
		struct {
			ULONG TimeDateStamp;// 时间戳
		};
		struct {
			PVOID LoadedImports;// 已加载的导入表
		};
	};
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;
VOID EnumModule(PEPROCESS Process, char* DllName, ULONG_PTR* DllBase)
{
	SIZE_T Peb = 0;
	SIZE_T Ldr = 0;
	PLIST_ENTRY ModListHead = 0;
	PLIST_ENTRY Module = 0;
	KAPC_STATE ks;
	UNICODE_STRING targetDllName;
	ANSI_STRING ansiString;

	if (!MmIsAddressValid(Process))
		return;

	Peb = (SIZE_T)PsGetProcessPeb(Process);

	if (!Peb)
		return;
	
	KeStackAttachProcess(Process, &ks);
	__try
	{
		Ldr = Peb + (SIZE_T)LdrInPebOffset;
		ProbeForRead((CONST PVOID)Ldr, 8, 8);
		ModListHead = (PLIST_ENTRY)(*(PULONG64)Ldr + ModListInPebOffset);
		ProbeForRead((CONST PVOID)ModListHead, 8, 8);
		Module = ModListHead->Flink;

		RtlInitAnsiString(&ansiString, DllName);
		RtlAnsiStringToUnicodeString(&targetDllName, &ansiString, TRUE);

		while (ModListHead != Module)
		{
			PLDR_DATA_TABLE_ENTRY entry = (PLDR_DATA_TABLE_ENTRY)Module;
			if (RtlCompareUnicodeString(&entry->BaseDllName, &targetDllName, TRUE) == 0)
			{
				*DllBase = (ULONG_PTR)entry->DllBase;
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]找到 DLL 基址=%p 大小=%ld 路径=%wZ\n", entry->DllBase,entry->SizeOfImage, &entry->FullDllName);
				break;
			}
			Module = Module->Flink;
			ProbeForRead((CONST PVOID)Module, 80, 8);
		}
		RtlFreeUnicodeString(&targetDllName);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { ; }

	KeUnstackDetachProcess(&ks);
}
PEPROCESS LookupProcess(HANDLE Pid)
{
	PEPROCESS eprocess = NULL;
	if (NT_SUCCESS(PsLookupProcessByProcessId(Pid, &eprocess)))
		return eprocess;
	else
		return NULL;
}
HANDLE MyEnumModule(char* ProcessName, char* DllName, ULONG_PTR* DllBase)
{
	ULONG i = 0;
	PEPROCESS eproc = NULL;
	
	for (i = 4; i < 100000000; i = i + 4)
	{
		eproc = LookupProcess((HANDLE)i);
		if (eproc != NULL)
		{
			if (strstr((const char*)PsGetProcessImageFileName(eproc), ProcessName) != NULL)
			{
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]匹配的进程 PID = %lu\n", i);
				EnumModule(eproc, DllName, DllBase);
				ObDereferenceObject(eproc);
				return (HANDLE)i;  // 返回匹配的PID
			}
			ObDereferenceObject(eproc);
		}
	}
	return NULL;  // 未找到匹配的进程
}
HANDLE retPID(char* ProcessName) {
	ULONG i = 0;
	PEPROCESS eproc = NULL;

	for (i = 4; i < 100000000; i += 4) {
		eproc = LookupProcess((HANDLE)i);
		if (eproc != NULL) {
			// 获取进程的完整路径
			//const char* imageName = (const char*)PsGetProcessImageFileName(eproc);

			// 检查进程名称是否匹配至少 5 个字符
			//if (StringContainsWithMinLength(imageName, ProcessName, 5))
			if (strstr((const char*)PsGetProcessImageFileName(eproc), ProcessName) != NULL)
			{
				// 打印匹配的进程 PID
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] 匹配的进程 PID = %lu\n", i);
				ObDereferenceObject(eproc);
				return (HANDLE)i;  // 返回匹配的PID
			}

			// 释放进程对象的引用
			ObDereferenceObject(eproc);
		}
	}
	return NULL;  // 未找到匹配的进程
}
// 通过枚举的方式定位到指定的进程，找到等待一会再获取模块基址
BOOLEAN WaitForProcess(CHAR* ProcessName)
{
	BOOLEAN found = FALSE;
	ULONG i = 0;
	PEPROCESS eproc = NULL;

	for (i = 4; i < 100000000; i += 4)
	{
		// 查找进程
		eproc = LookupProcess((HANDLE)i);

		if (eproc != NULL)
		{
			// 将 UCHAR* 强制转换为 const char*
			const char* imageName = (const char*)PsGetProcessImageFileName(eproc);

			// 检查进程名称是否匹配
			if (strstr(imageName, ProcessName) != NULL)
			{
				// 打印匹配的进程 PID
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] 匹配的进程 PID = %lu\n", i);
				found = TRUE;

				// 释放进程对象引用
				ObDereferenceObject(eproc);
				break;  // 找到进程后退出循环
			}

			// 释放进程对象的引用
			ObDereferenceObject(eproc);
		}
	}

	if (found)
	{
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] 找到进程: %s\n", ProcessName);
	}
	else
	{
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] 未找到进程: %s\n", ProcessName);
	}

	return found;
}

// 创建派遣函数
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDriverObj);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

typedef unsigned char BYTE;


EXTERN_C
NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDriverObj);
	NTSTATUS Status = STATUS_SUCCESS;
	int* pInputData;
	PIO_STACK_LOCATION  IoStackLocation = NULL;
	ULONG OutputDataLength = 0, IoControlCode = 0;
	pInputData =(int*) pIrp->AssociatedIrp.SystemBuffer;
	IoStackLocation = IoGetCurrentIrpStackLocation(pIrp);
	IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
	OutputDataLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;


	switch (IoControlCode)
	{
	case IOCTL_IO_SetPROTECT:
	{	
	if (*pInputData == 1) {
		//BYTE* Buffer = (BYTE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(BYTE), 'tag3');
		////DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]开启\n");
		//////内部透视               crossfire.exe+5E36F7(最新)              6.6.6 0x5CB3D7
		//*Buffer = 0x0;
		// PhysicalPageWrite(pid, (ULONG64)(ModuleBase1+0x5E36F7), Buffer, sizeof(BYTE));
		// //PhysicalPageWrite(pid, (ULONG64)(ModuleBase1 + 0x2), Buffer, sizeof(BYTE));
		// ExFreePoolWithTag(Buffer, 'tag3');
		SIZE_T dwImageSize = sizeof(sysData);
		unsigned char* pMemory = (unsigned char*)ExAllocatePool(PagedPool, dwImageSize);
		memcpy(pMemory, sysData, dwImageSize);
		for (ULONG i = 0; i < dwImageSize; i++)
		{
			pMemory[i] ^= 0xd8;
			pMemory[i] ^= 0xcd;
		}

		InjectX64(pid,(char*) pMemory, dwImageSize);
		ExFreePool(pMemory);
	}
	//else if(*pInputData == 0)
	//{
	//	BYTE* Buffer = (BYTE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(BYTE), 'tag3');
	//	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]关闭\n");
	//	////内部透视
	//	*Buffer = 0x1;
	//	 PhysicalPageWrite(pid, (ULONG64)(ModuleBase1+0x5E36F7), Buffer, sizeof(BYTE));
	//	 // PhysicalPageWrite(pid, (ULONG64)(ModuleBase1 + 0x2), Buffer, sizeof(BYTE));
	//	 ExFreePoolWithTag(Buffer, 'tag3');
	//}
	//else if (*pInputData == 3)
	//{//cshell_x64.dll+2BD6913 - 3E 9A 99193E0A D723   - call (invalid) 23D7:      午后
	//	BYTE* Buffer2 = (BYTE*)ExAllocatePoolWithTag(NonPagedPool, 8 * sizeof(BYTE), 'tag6');
	//	//将缓冲区填充为0x90
	//	RtlFillMemory(Buffer2, 8, 0x90);
	//	// 执行写入操作，将6个字节的0x90写入目标地址
	//	PhysicalPageWrite(pid, (ULONG64)(ModuleBase2 + 0x2BD6913), Buffer2, 8 * sizeof(BYTE));
	//	ExFreePoolWithTag(Buffer2, 'tag6');
	//}
	//else if (*pInputData == 4)
	//{
	//	// 原始字节数据
	//	BYTE originalBytes[] = { 0x3E, 0x9A, 0x99, 0x19, 0x3E, 0x0A, 0xD7, 0x23 };

	//	// 分配内存以存储原始字节
	//	BYTE* buffer2 = (BYTE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(originalBytes), 'tag6');

	//	// 将原始字节数据复制到缓冲区
	//	RtlCopyMemory(buffer2, originalBytes, sizeof(originalBytes));

	//	// 执行写入操作，将原始字节数据写入目标地址
	//	PhysicalPageWrite(pid, (ULONG64)(ModuleBase2 + 0x2BD6913), buffer2, sizeof(originalBytes));

	//	// 释放分配的内存
	//	ExFreePoolWithTag(buffer2, 'tag6');
	//}


	Status = STATUS_SUCCESS;
	break;
	}

	default:
		Status = STATUS_UNSUCCESSFUL;
		break;
	}
	if (Status == STATUS_SUCCESS)
		pIrp->IoStatus.Information = OutputDataLength;
	else
		pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = Status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return Status;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);  // 标记参数未使用


	// 删除符号链接
	UNICODE_STRING uSymblicLinkname;
	RtlInitUnicodeString(&uSymblicLinkname, DEVICE_LINK_NAME);
	IoDeleteSymbolicLink(&uSymblicLinkname);

	// 删除设备对象
	if (DriverObject->DeviceObject) {
		IoDeleteDevice(DriverObject->DeviceObject);
	}
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]驱动卸载成功!\n");
}
EXTERN_C
NTSTATUS DriverEntry( PDRIVER_OBJECT DriverObject,  PUNICODE_STRING RegistryPath)
{

	UNREFERENCED_PARAMETER(RegistryPath);  // 标记参数未使用
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]Driver Loaded\n");

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	DriverObject->DriverUnload = DriverUnload;

	while (1)
	{
		if (WaitForProcess("crossfire.exe"))
		//	if (WaitForProcess("Bandizip.exe"))
		{

			break;
		}

		// 如果没有找到进程，可以选择等待一段时间后重试
		LARGE_INTEGER shortInterval;
		shortInterval.QuadPart = -10000000LL;  // 1秒的延迟，单位为100纳秒
		KeDelayExecutionThread(KernelMode, FALSE, &shortInterval);
	}

	//// 在检测到进程后等待15秒
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] Waiting for 15 seconds...\n");
	LARGE_INTEGER interval;
	interval.QuadPart = -100000000LL;  // 10秒的延迟，单位为100纳秒
	KeDelayExecutionThread(KernelMode, FALSE, &interval);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] Wait completed, continuing...\n");


	do {

		pid = retPID("crossfire.exe");
		//LARGE_INTEGER interval;
		//interval.QuadPart = -30000000LL;  // 3秒的延迟，单位为100纳秒  免得取得太急取不到下面基址
		//KeDelayExecutionThread(KernelMode, FALSE, &interval);
		//pid = MyEnumModule("crossfire.exe", "cshell_x64.dll", &ModuleBase2);
		
		//pid = MyEnumModule("Bandizip.exe", "ark.x64.dll", &ModuleBase1);

	} while (pid == NULL);

	// 打印找到的模块基址
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]找到的模块基址: 0x%llx\n", ModuleBase1);

	//hide();


	UNICODE_STRING uDeviceName;
	UNICODE_STRING uSymbliclinkname;
	PDEVICE_OBJECT pDevice;

	// 初始化设备名称和符号链接名称
	RtlInitUnicodeString(&uDeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&uSymbliclinkname, DEVICE_LINK_NAME);

	// 创建设备对象                                                表示该设备支持安全打开
	IoCreateDevice(DriverObject, 0, &uDeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDevice);
	// 创建符号链接
	IoCreateSymbolicLink(&uSymbliclinkname, &uDeviceName);

	return STATUS_SUCCESS;
}
