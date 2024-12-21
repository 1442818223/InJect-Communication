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
    // ���·���Ƿ���Ч
    if (path == NULL || path->Buffer == NULL || path->Length == 0)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] �ṩ��·����Ч��\n");
        return STATUS_INVALID_PARAMETER;
    }

    // ��ӡԭʼ·��
   // DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ɾ���ļ���ԭʼ·��: %wZ\n", path);

    // ���� \\??\\ ǰ׺   ������exe��������ǰ׺
    UNICODE_STRING prefix;
    RtlInitUnicodeString(&prefix, L"\\??\\");

    // ������·������
    USHORT newLength = prefix.Length + path->Length;
    WCHAR* newBuffer = (WCHAR*)ExAllocatePoolWithTag(NonPagedPool, newLength + sizeof(WCHAR), 'path');

    if (newBuffer == NULL)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] �ڴ����ʧ�ܣ�\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // ����ǰ׺��ԭʼ·�����»�����
    RtlCopyMemory(newBuffer, prefix.Buffer, prefix.Length);
    RtlCopyMemory((PUCHAR)newBuffer + prefix.Length, path->Buffer, path->Length);

    // ȷ���ַ�����NULL��β
    newBuffer[newLength / sizeof(WCHAR)] = UNICODE_NULL;

    // ��ʼ���µ� UNICODE_STRING
    UNICODE_STRING newPath;
    newPath.Length = newLength;
    newPath.MaximumLength = newLength + sizeof(WCHAR);
    newPath.Buffer = newBuffer;

    // ��ӡ��·��
   // DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ɾ���ļ�������·��: %wZ\n", &newPath);

    // ��ʼ����������
    OBJECT_ATTRIBUTES objFile = { 0 };
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioBlock = { 0 };
    NTSTATUS status;

    InitializeObjectAttributes(&objFile, &newPath, OBJ_CASE_INSENSITIVE, NULL, NULL);

    // �����ļ����
    status = ZwCreateFile(
        &hFile,
        GENERIC_READ, // ȷ������ DELETE Ȩ��
        &objFile,
        &ioBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN_IF,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        NULL);

    // �ͷ��ڴ沢��������״̬��
    ExFreePool(newBuffer);

    if (!NT_SUCCESS(status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ZwCreateFile ʧ�ܣ�״̬��: 0x%X\n", status);
        return status;
    }

    PVOID Object = NULL;
    status = ObReferenceObjectByHandle(
        hFile,
        FILE_ALL_ACCESS, // ȷ�� DELETE Ȩ��
        *IoFileObjectType,
        KernelMode,
        &Object,
        NULL
    );
 
    if (!NT_SUCCESS(status))
    {
        ZwClose(hFile);
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ObReferenceObjectByHandle ʧ�ܣ�״̬��: 0x%X\n", status);
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
 
    // ɾ���ļ�
    status = ZwDeleteFile(&objFile);
 
   // DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[db+] ZwDeleteFile ״̬��: 0x%X\n", status);
 
    return status;
}