

#include "mian.hpp"
#include "Inject.h"
#include "dll.h"

PDEVICE_OBJECT g_device_object = NULL;


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



NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(pIrp);

	
	switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_IO_SetPROTECT:
	{	
		ULONG32* pInputData = (ULONG32*)pIrp->AssociatedIrp.SystemBuffer;

	if (*pInputData != 0) {

		HANDLE pid = (HANDLE)*(ULONG32*)(pIrp->AssociatedIrp.SystemBuffer);

		// 打印 pid 的值
		DbgPrintEx(77,0,"[hv] PID: 0x%d\n", pid); // 打印 pid 值（注意这里的 %p 用于打印指针值）

		
		SIZE_T dwImageSize = sizeof(sysData);
		unsigned char* pMemory  = (unsigned char*)ExAllocatePool(PagedPool, dwImageSize);
		memcpy(pMemory, sysData, dwImageSize);
		for (ULONG i = 0; i < dwImageSize; i++)
		{
			pMemory[i] ^= 0xd8;
			pMemory[i] ^= 0xcd;
		}
		InjectX64(pid,(char*) pMemory, dwImageSize);
		ExFreePool(pMemory);

		pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrp->IoStatus.Information = 0;
	}

	
	}break;

	default: 
	{
		pIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		pIrp->IoStatus.Information = 0;
	
	}break;

		
	}
	
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);  // 标记参数未使用

	if (g_device_object)
	{
	
		// 删除符号链接
		UNICODE_STRING uSymblicLinkname;
		RtlInitUnicodeString(&uSymblicLinkname, DEVICE_LINK_NAME);
		IoDeleteSymbolicLink(&uSymblicLinkname);

		// 删除设备对象
		if (DriverObject->DeviceObject) {
			IoDeleteDevice(DriverObject->DeviceObject);
		}
	
	}
	
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+]驱动卸载成功!\n");
}
EXTERN_C
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{

	DriverObject->DriverUnload = DriverUnload;


	UNICODE_STRING device_name;
	RtlInitUnicodeString(&device_name, L"\\Device\\NoScreen");
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &g_device_object);
	if (!NT_SUCCESS(status) || g_device_object == NULL) return STATUS_UNSUCCESSFUL;

	UNICODE_STRING symbolic_link;
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\NoScreen");
	status = IoCreateSymbolicLink(&symbolic_link, &device_name);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(g_device_object);
		return STATUS_UNSUCCESSFUL;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreate;


	g_device_object->Flags |= DO_DIRECT_IO;
	g_device_object->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}
