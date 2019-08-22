#include "utils.h"

#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winnt.h>
#include <winternl.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <process.h>

typedef struct _CLIENT_ID {
    PVOID UniqueProcess;
    PVOID UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _SECTION_IMAGE_INFORMATION {
    PVOID EntryPoint;
    ULONG StackZeroBits;
    ULONG StackReserved;
    ULONG StackCommit;
    ULONG ImageSubsystem;
    WORD SubSystemVersionLow;
    WORD SubSystemVersionHigh;
    ULONG Unknown1;
    ULONG ImageCharacteristics;
    ULONG ImageMachineType;
    ULONG Unknown2[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef struct _RTL_USER_PROCESS_INFORMATION {
    ULONG Size;
    HANDLE Process;
    HANDLE Thread;
    CLIENT_ID ClientId;
    SECTION_IMAGE_INFORMATION ImageInformation;
} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION;

#define RTL_CLONE_PROCESS_FLAGS_CREATE_SUSPENDED 0x00000001
#define RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES  0x00000002
#define RTL_CLONE_PROCESS_FLAGS_NO_SYNCHRONIZE   0x00000004

#define RTL_CLONE_PARENT 0
#define RTL_CLONE_CHILD 297

typedef NTSTATUS(*RtlCloneUserProcess_f)(ULONG ProcessFlags,
        PSECURITY_DESCRIPTOR ProcessSecurityDescriptor,
        PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
        HANDLE DebugPort,
        PRTL_USER_PROCESS_INFORMATION ProcessInformation);

pid_t fork(void) {
    HMODULE mod;
    RtlCloneUserProcess_f clone_p;
    RTL_USER_PROCESS_INFORMATION process_info;
    NTSTATUS result;

    mod = GetModuleHandle(LPCWSTR("ntdll.dll"));
    if (!mod) {
        return -ENOSYS;
    }

    clone_p = (RtlCloneUserProcess_f)GetProcAddress(mod, "RtlCloneUserProcess");
    if (clone_p == NULL) {
        return -ENOSYS;
    }

    /* lets do this */
    result = clone_p(RTL_CLONE_PROCESS_FLAGS_CREATE_SUSPENDED | RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES, NULL, NULL, NULL, &process_info);
    if (result == RTL_CLONE_PARENT) {
        HANDLE me, hp, ht, hcp = 0;
        DWORD pi, ti, mi;
        me = GetCurrentProcess();
        pi = (DWORD)process_info.ClientId.UniqueProcess;
        ti = (DWORD)process_info.ClientId.UniqueThread;

        assert(hp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi));
        assert(ht = OpenThread(THREAD_ALL_ACCESS, FALSE, ti));

        ResumeThread(ht);
        CloseHandle(ht);
        CloseHandle(hp);
        return (pid_t)pi;
    } else if (result == RTL_CLONE_CHILD) {
        /* fix stdio */
        AllocConsole();
        return 0;
    } else {
        return -1;
    }
    return -1;
}

bool kill(pid_t dwPid, int code) {
    if (0 == dwPid) {
        return false;
    }
    auto hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
    if (!TerminateProcess(hPrc, code)) { // Terminates a process.
        CloseHandle(hPrc);
        return false;
    } else {
        WaitForSingleObject(hPrc, 200); // At most ,waite 2000  millisecond.
    }
    CloseHandle(hPrc);
    return true;
}