#include"FindPath.hpp"



UNICODE_STRING GetProcessFullImagePath(HANDLE pid)
{
    UNICODE_STRING failureResult;
    RtlInitUnicodeString(&failureResult, L""); // Initialize with empty string to indicate failure

    PEPROCESS eProcess = NULL;
    PMPEB64 peb = NULL;
    PRTL_USER_PROCESS_PARAMETERS processParameters = NULL;
    KAPC_STATE ks;

    // Get the EPROCESS structure for the given PID
    if (NT_SUCCESS(PsLookupProcessByProcessId(pid, &eProcess)))
    {
        // Attach to the target process
        KeStackAttachProcess(eProcess, &ks);

        __try
        {
            // Access PEB
            peb = (PMPEB64)PsGetProcessPeb(eProcess);
            if (peb && peb->ProcessParameters)
            {
                // Check and cast ProcessParameters
                processParameters = (PRTL_USER_PROCESS_PARAMETERS)peb->ProcessParameters;
                if (processParameters)
                {
                    // Ensure ImagePathName is valid before using
                    UNICODE_STRING processImagePath = processParameters->ImagePathName;

                    // Allocate memory for the path string
                    WCHAR* pathBuffer = (WCHAR*)ExAllocatePoolWithTag(NonPagedPool, processImagePath.Length + sizeof(WCHAR), 'Path');
                    if (pathBuffer)
                    {
                        RtlCopyMemory(pathBuffer, processImagePath.Buffer, processImagePath.Length);
                        processImagePath.Buffer = pathBuffer;
                        processImagePath.Buffer[processImagePath.Length / sizeof(WCHAR)] = L'\0';

                        // Use the pathBuffer for your operations
                        //DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] Process Full Path: %wZ\n", &processImagePath);

                        // Clean up
                        KeUnstackDetachProcess(&ks);
                        ObDereferenceObject(eProcess);

                        // Return the copied ImagePathName
                        return processImagePath;
                    }
                    else
                    {
                        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] Failed to allocate memory for path buffer\n");
                    }
                }
                else
                {
                    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ProcessParameters is NULL\n");
                }
            }
            else
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] PEB or ProcessParameters is NULL\n");
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] Exception occurred while accessing process memory\n");
        }

        // Detach from the target process
        KeUnstackDetachProcess(&ks);

        // Release reference count
        ObDereferenceObject(eProcess);
    }
    else
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] Failed to get process for PID: %u\n", pid);
    }

    // Return the failure value
    return failureResult;
}


NTSTATUS SelfDeleteFile(UNICODE_STRING* path)
{
    // 检查路径是否有效
    if (path == NULL || path->Buffer == NULL || path->Length == 0)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] 提供的路径无效！\n");
        return STATUS_INVALID_PARAMETER;
    }

    // 打印原始路径
   // DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] 删除文件的原始路径: %wZ\n", path);

    // 定义 \\??\\ 前缀   三环的exe必须加这个前缀
    UNICODE_STRING prefix;
    RtlInitUnicodeString(&prefix, L"\\??\\");

    // 计算新路径长度
    USHORT newLength = prefix.Length + path->Length;
    WCHAR* newBuffer = (WCHAR*)ExAllocatePoolWithTag(NonPagedPool, newLength + sizeof(WCHAR), 'path');

    if (newBuffer == NULL)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] 内存分配失败！\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // 复制前缀和原始路径到新缓冲区
    RtlCopyMemory(newBuffer, prefix.Buffer, prefix.Length);
    RtlCopyMemory((PUCHAR)newBuffer + prefix.Length, path->Buffer, path->Length);

    // 确保字符串以NULL结尾
    newBuffer[newLength / sizeof(WCHAR)] = UNICODE_NULL;

    // 初始化新的 UNICODE_STRING
    UNICODE_STRING newPath;
    newPath.Length = newLength;
    newPath.MaximumLength = newLength + sizeof(WCHAR);
    newPath.Buffer = newBuffer;

    // 打印新路径
   // DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] 删除文件的完整路径: %wZ\n", &newPath);

    // 初始化对象属性
    OBJECT_ATTRIBUTES objFile = { 0 };
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioBlock = { 0 };
    NTSTATUS status;

    InitializeObjectAttributes(&objFile, &newPath, OBJ_CASE_INSENSITIVE, NULL, NULL);

    // 创建文件句柄
    status = ZwCreateFile(
        &hFile,
        GENERIC_READ, // 确保请求 DELETE 权限
        &objFile,
        &ioBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN_IF,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        NULL);

    // 释放内存并处理句柄和状态码
    ExFreePool(newBuffer);

    if (!NT_SUCCESS(status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ZwCreateFile 失败，状态码: 0x%X\n", status);
        return status;
    }

    PVOID Object = NULL;
    status = ObReferenceObjectByHandle(
        hFile,
        FILE_ALL_ACCESS, // 确保 DELETE 权限
        *IoFileObjectType,
        KernelMode,
        &Object,
        NULL
    );
 
    if (!NT_SUCCESS(status))
    {
        ZwClose(hFile);
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ObReferenceObjectByHandle 失败，状态码: 0x%X\n", status);
        return status;
    }
 
    PFILE_OBJECT pFile = (PFILE_OBJECT)Object;
 
    pFile->DeleteAccess = TRUE;
    pFile->DeletePending = FALSE;
 
    pFile->SectionObjectPointer->DataSectionObject = NULL;
    pFile->SectionObjectPointer->ImageSectionObject = NULL;
    //pFile->SectionObjectPointer->SharedCacheMap = NULL;
 
    ObDereferenceObject(pFile);
    ZwClose(hFile);
 
    // 删除文件
    status = ZwDeleteFile(&objFile);
 
   // DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ZwDeleteFile 状态码: 0x%X\n", status);
 
    return status;
}