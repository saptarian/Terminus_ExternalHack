#include "util.hpp"

#include <tlhelp32.h>

namespace util {

HANDLE OpenGameProcess(uintptr_t processID) {
  HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION |
                                    PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
                                FALSE, processID);
  if (hProcess == NULL || hProcess == INVALID_HANDLE_VALUE) {
    return NULL;
  }
  return hProcess;
}

bool ValidateProcessHandle(HANDLE hProcess) {
  DWORD exitCode;
  if (GetExitCodeProcess(hProcess, &exitCode)) {
    return (exitCode == STILL_ACTIVE);
  } else {
    return false;
  }
}

uintptr_t GetModuleBaseAddress(uintptr_t processID, const char* moduleName) {
  uintptr_t baseAddress = 0;
  HANDLE snapshot = CreateToolhelp32Snapshot(
      TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
  if (snapshot != INVALID_HANDLE_VALUE) {
    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(snapshot, &moduleEntry)) {
      do {
        if (_stricmp(moduleEntry.szModule, moduleName) == 0) {
          baseAddress = (uintptr_t)moduleEntry.modBaseAddr;
          break;
        }
      } while (Module32Next(snapshot, &moduleEntry));
    }
    CloseHandle(snapshot);
  }
  return baseAddress;
}

uintptr_t GetProcessID(const char* processName) {
  uintptr_t processID = 0;
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot != INVALID_HANDLE_VALUE) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(snapshot, &processEntry)) {
      do {
        if (_stricmp(processEntry.szExeFile, processName) == 0) {
          processID = processEntry.th32ProcessID;
          break;
        }
      } while (Process32Next(snapshot, &processEntry));
    }
    CloseHandle(snapshot);
  }
  return processID;
}

bool EnableDebugPrivilege() {
  HANDLE hToken;
  TOKEN_PRIVILEGES tp;
  LUID luid;

  // Open the process token
  if (!OpenProcessToken(GetCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    return false;
  }

  // Lookup the LUID for the debug privilege
  if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
    CloseHandle(hToken);
    return false;
  }

  // Set up the TOKEN_PRIVILEGES structure
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  // Adjust the token privileges
  if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL,
                             NULL)) {
    CloseHandle(hToken);
    return false;
  }

  CloseHandle(hToken);
  return true;
}

bool IsRunningAsAdmin() {
  BOOL isAdmin = FALSE;
  PSID adminGroup = NULL;
  SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

  if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                               &adminGroup)) {
    CheckTokenMembership(NULL, adminGroup, &isAdmin);
    FreeSid(adminGroup);
  }
  return isAdmin;
}

void RequestElevation() {
  char szPath[MAX_PATH];
  if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) {
    SHELLEXECUTEINFO sei{};
    sei.lpVerb = "runas";
    sei.lpFile = szPath;
    sei.hwnd = NULL;
    sei.nShow = SW_NORMAL;

    ShellExecuteEx(&sei);
  }
}

// Debouncer

Debouncer::Debouncer(std::function<void()> func, int delay)
    : func(func), delay(delay), lastCall(std::chrono::steady_clock::now()) {}

void Debouncer::call() {
  auto now = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCall)
          .count() > delay) {
    func();
    lastCall = now;
  }
}

};  // namespace util
