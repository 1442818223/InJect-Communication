

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
	LIST_ENTRY InLoadOrderLinks;// ������˳�����ӵ�����
	LIST_ENTRY InMemoryOrderLinks;// ���ڴ�˳�����ӵ�����
	LIST_ENTRY InInitializationOrderLinks;// ����ʼ��˳�����ӵ�����
	PVOID DllBase;// DLL �Ļ���ַ
	PVOID EntryPoint;// DLL ����ڵ��ַ  
	ULONG SizeOfImage;// DLL ӳ��Ĵ�С
	UNICODE_STRING FullDllName;// ������ DLL ����
	UNICODE_STRING BaseDllName;// DLL �Ļ�������
	ULONG Flags;// ��־
	USHORT LoadCount;// װ�ؼ�����
	USHORT TlsIndex;// TLS ����
	union {
		LIST_ENTRY HashLinks;// ��ϣ����
		struct {
			PVOID SectionPointer;// ��ָ��
			ULONG CheckSum;// У���
		};
	};
	union {
		struct {
			ULONG TimeDateStamp;// ʱ���
		};
		struct {
			PVOID LoadedImports;// �Ѽ��صĵ����
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
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]�ҵ� DLL ��ַ=%p ��С=%ld ·��=%wZ\n", entry->DllBase,entry->SizeOfImage, &entry->FullDllName);
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
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]ƥ��Ľ��� PID = %lu\n", i);
				EnumModule(eproc, DllName, DllBase);
				ObDereferenceObject(eproc);
				return (HANDLE)i;  // ����ƥ���PID
			}
			ObDereferenceObject(eproc);
		}
	}
	return NULL;  // δ�ҵ�ƥ��Ľ���
}
HANDLE retPID(char* ProcessName) {
	ULONG i = 0;
	PEPROCESS eproc = NULL;

	for (i = 4; i < 100000000; i += 4) {
		eproc = LookupProcess((HANDLE)i);
		if (eproc != NULL) {
			// ��ȡ���̵�����·��
			//const char* imageName = (const char*)PsGetProcessImageFileName(eproc);

			// �����������Ƿ�ƥ������ 5 ���ַ�
			//if (StringContainsWithMinLength(imageName, ProcessName, 5))
			if (strstr((const char*)PsGetProcessImageFileName(eproc), ProcessName) != NULL)
			{
				// ��ӡƥ��Ľ��� PID
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ƥ��Ľ��� PID = %lu\n", i);
				ObDereferenceObject(eproc);
				return (HANDLE)i;  // ����ƥ���PID
			}

			// �ͷŽ��̶��������
			ObDereferenceObject(eproc);
		}
	}
	return NULL;  // δ�ҵ�ƥ��Ľ���
}
// ͨ��ö�ٵķ�ʽ��λ��ָ���Ľ��̣��ҵ��ȴ�һ���ٻ�ȡģ���ַ
BOOLEAN WaitForProcess(CHAR* ProcessName)
{
	BOOLEAN found = FALSE;
	ULONG i = 0;
	PEPROCESS eproc = NULL;

	for (i = 4; i < 100000000; i += 4)
	{
		// ���ҽ���
		eproc = LookupProcess((HANDLE)i);

		if (eproc != NULL)
		{
			// �� UCHAR* ǿ��ת��Ϊ const char*
			const char* imageName = (const char*)PsGetProcessImageFileName(eproc);

			// �����������Ƿ�ƥ��
			if (strstr(imageName, ProcessName) != NULL)
			{
				// ��ӡƥ��Ľ��� PID
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ƥ��Ľ��� PID = %lu\n", i);
				found = TRUE;

				// �ͷŽ��̶�������
				ObDereferenceObject(eproc);
				break;  // �ҵ����̺��˳�ѭ��
			}

			// �ͷŽ��̶��������
			ObDereferenceObject(eproc);
		}
	}

	if (found)
	{
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] �ҵ�����: %s\n", ProcessName);
	}
	else
	{
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] δ�ҵ�����: %s\n", ProcessName);
	}

	return found;
}

// ������ǲ����
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
		////DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]����\n");
		//////�ڲ�͸��               crossfire.exe+5E36F7(����)              6.6.6 0x5CB3D7
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
	//	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]�ر�\n");
	//	////�ڲ�͸��
	//	*Buffer = 0x1;
	//	 PhysicalPageWrite(pid, (ULONG64)(ModuleBase1+0x5E36F7), Buffer, sizeof(BYTE));
	//	 // PhysicalPageWrite(pid, (ULONG64)(ModuleBase1 + 0x2), Buffer, sizeof(BYTE));
	//	 ExFreePoolWithTag(Buffer, 'tag3');
	//}
	//else if (*pInputData == 3)
	//{//cshell_x64.dll+2BD6913 - 3E 9A 99193E0A D723   - call (invalid) 23D7:      ���
	//	BYTE* Buffer2 = (BYTE*)ExAllocatePoolWithTag(NonPagedPool, 8 * sizeof(BYTE), 'tag6');
	//	//�����������Ϊ0x90
	//	RtlFillMemory(Buffer2, 8, 0x90);
	//	// ִ��д���������6���ֽڵ�0x90д��Ŀ���ַ
	//	PhysicalPageWrite(pid, (ULONG64)(ModuleBase2 + 0x2BD6913), Buffer2, 8 * sizeof(BYTE));
	//	ExFreePoolWithTag(Buffer2, 'tag6');
	//}
	//else if (*pInputData == 4)
	//{
	//	// ԭʼ�ֽ�����
	//	BYTE originalBytes[] = { 0x3E, 0x9A, 0x99, 0x19, 0x3E, 0x0A, 0xD7, 0x23 };

	//	// �����ڴ��Դ洢ԭʼ�ֽ�
	//	BYTE* buffer2 = (BYTE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(originalBytes), 'tag6');

	//	// ��ԭʼ�ֽ����ݸ��Ƶ�������
	//	RtlCopyMemory(buffer2, originalBytes, sizeof(originalBytes));

	//	// ִ��д���������ԭʼ�ֽ�����д��Ŀ���ַ
	//	PhysicalPageWrite(pid, (ULONG64)(ModuleBase2 + 0x2BD6913), buffer2, sizeof(originalBytes));

	//	// �ͷŷ�����ڴ�
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
	UNREFERENCED_PARAMETER(DriverObject);  // ��ǲ���δʹ��


	// ɾ����������
	UNICODE_STRING uSymblicLinkname;
	RtlInitUnicodeString(&uSymblicLinkname, DEVICE_LINK_NAME);
	IoDeleteSymbolicLink(&uSymblicLinkname);

	// ɾ���豸����
	if (DriverObject->DeviceObject) {
		IoDeleteDevice(DriverObject->DeviceObject);
	}
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]����ж�سɹ�!\n");
}
EXTERN_C
NTSTATUS DriverEntry( PDRIVER_OBJECT DriverObject,  PUNICODE_STRING RegistryPath)
{

	UNREFERENCED_PARAMETER(RegistryPath);  // ��ǲ���δʹ��
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

		// ���û���ҵ����̣�����ѡ��ȴ�һ��ʱ�������
		LARGE_INTEGER shortInterval;
		shortInterval.QuadPart = -10000000LL;  // 1����ӳ٣���λΪ100����
		KeDelayExecutionThread(KernelMode, FALSE, &shortInterval);
	}

	//// �ڼ�⵽���̺�ȴ�15��
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] Waiting for 15 seconds...\n");
	LARGE_INTEGER interval;
	interval.QuadPart = -100000000LL;  // 10����ӳ٣���λΪ100����
	KeDelayExecutionThread(KernelMode, FALSE, &interval);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] Wait completed, continuing...\n");


	do {

		pid = retPID("crossfire.exe");
		//LARGE_INTEGER interval;
		//interval.QuadPart = -30000000LL;  // 3����ӳ٣���λΪ100����  ���ȡ��̫��ȡ���������ַ
		//KeDelayExecutionThread(KernelMode, FALSE, &interval);
		//pid = MyEnumModule("crossfire.exe", "cshell_x64.dll", &ModuleBase2);
		
		//pid = MyEnumModule("Bandizip.exe", "ark.x64.dll", &ModuleBase1);

	} while (pid == NULL);

	// ��ӡ�ҵ���ģ���ַ
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]�ҵ���ģ���ַ: 0x%llx\n", ModuleBase1);

	//hide();


	UNICODE_STRING uDeviceName;
	UNICODE_STRING uSymbliclinkname;
	PDEVICE_OBJECT pDevice;

	// ��ʼ���豸���ƺͷ�����������
	RtlInitUnicodeString(&uDeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&uSymbliclinkname, DEVICE_LINK_NAME);

	// �����豸����                                                ��ʾ���豸֧�ְ�ȫ��
	IoCreateDevice(DriverObject, 0, &uDeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDevice);
	// ������������
	IoCreateSymbolicLink(&uSymbliclinkname, &uDeviceName);

	return STATUS_SUCCESS;
}
