#include <windows.h>
#include <stdio.h>
#include "pc.h"

/* Initialize job object once */
static void init_job_object() {
    if (g_job != NULL)
        return;

    g_job = CreateJobObject(NULL, NULL);

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = { 0 };
    jobInfo.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    SetInformationJobObject(
        g_job,
        JobObjectExtendedLimitInformation,
        &jobInfo,
        sizeof(jobInfo)
    );
}

/* system-like but controlled */
PROCESS_INFORMATION run_process(const char* program, const char* args)
{
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    char cmdLine[1024];

    init_job_object();

    si.cb = sizeof(si);

    // Build command line (like system)
    if (args)
        snprintf(cmdLine, sizeof(cmdLine), "%s %s", program, args);
    else
        snprintf(cmdLine, sizeof(cmdLine), "%s", program);

    if (!CreateProcess(
        NULL,
        cmdLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi))
    {
        printf("CreateProcess failed (%lu)\n", GetLastError());
    }
    else
    {
        AssignProcessToJobObject(g_job, pi.hProcess);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    return pi;
}

void kill_process(PROCESS_INFORMATION pi)
{
    if (pi.dwProcessId == 0)
        return;

    HANDLE hProcess = OpenProcess(
        PROCESS_TERMINATE,
        FALSE,
        pi.dwProcessId
    );

    if (hProcess == NULL)
    {
        printf("OpenProcess failed (%lu)\n", GetLastError());
        return;
    }

    if (!TerminateProcess(hProcess, 1))
    {
        printf("TerminateProcess failed (%lu)\n", GetLastError());
    }

    CloseHandle(hProcess);
}