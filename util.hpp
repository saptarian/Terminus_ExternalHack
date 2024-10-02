#ifndef UTIL_HPP
#define UTIL_HPP

#include <windows.h>

#include <chrono>
#include <functional>

namespace util {
HANDLE OpenGameProcess(uintptr_t pid);
bool ValidateProcessHandle(HANDLE hProcess);
uintptr_t GetModuleBaseAddress(uintptr_t processID, const char* moduleName);
uintptr_t GetProcessID(const char* processName);
bool EnableDebugPrivilege();
bool IsRunningAsAdmin();
void RequestElevation();

class Debouncer {
 public:
  Debouncer(std::function<void()> func, int delay);

  void call();

 private:
  std::function<void()> func;
  int delay;  // delay in milliseconds
  std::chrono::steady_clock::time_point lastCall;
};

};  // namespace util


#endif /*UTIL_HPP*/