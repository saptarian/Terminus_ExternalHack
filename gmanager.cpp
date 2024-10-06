#include "gmanager.hpp"

#include <memoryapi.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <algorithm>
#include <cstring>  // For std::strlen
#include <sstream>

GameManager::GameManager(unsigned pid) : hProcess(openProcessHandle(pid)) {}

GameManager::~GameManager() {
  if (hProcess) {
    CloseHandle(hProcess);
  }
}

HANDLE GameManager::openProcessHandle(unsigned pid) {
  HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION |
                                    PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
                                FALSE, pid);
  if (!hProcess) {
    throw std::runtime_error("Failed to open process!");
  }
  return hProcess;
}

uintptr_t GameManager::GetModuleBaseAddress(const std::string& moduleName) {
  HMODULE hMods[1024];  // Buffer to hold module handles
  DWORD cbNeeded;

  // Get a list of all modules in the target process
  if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
    for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
      char szModName[MAX_PATH];

      // Get the module name for each module
      if (GetModuleBaseNameA(hProcess, hMods[i], szModName,
                             sizeof(szModName) / sizeof(char))) {
        // std::cout << szModName << '\n';
        // If the module name matches the one provided, return the base address
        if (moduleName.compare(szModName) == 0) {
          return (uintptr_t)hMods[i];
        }
      }
    }
  }

  throw std::runtime_error("Module not found!");
}

bool GameManager::RPM(LPCVOID lpBaseAdr, LPVOID lpBuffer, SIZE_T size,
                      SIZE_T* lpBytesRread) {
  return ReadProcessMemory(hProcess, lpBaseAdr, lpBuffer, size, lpBytesRread);
}

bool GameManager::WPM(LPVOID lpBaseAdr, LPCVOID lpBuffer, SIZE_T size,
                      SIZE_T* lpBytesWritten) {
  return WriteProcessMemory(hProcess, lpBaseAdr, lpBuffer, size,
                            lpBytesWritten);
}

bool GameManager::ReadBuffer(LPCVOID lpBaseAdr, LPVOID lpBuffer, SIZE_T size) {
  if (!RPM(lpBaseAdr, lpBuffer, size)) {
    throw std::runtime_error("Failed to read memory in target process!");
  }
  return true;
}

DataResult GameManager::ReadBytes(BYTE* lpBytes, SIZE_T size) {
  ByteArray buffer(size);

  if (!RPM(lpBytes, buffer.data(), size)) {
    throw std::runtime_error("Failed to read memory in target process!");
  }

  // Return DataResult with data and this class
  return DataResult(buffer, this);
}

std::string GameManager::ReadString(const char* lpString, SIZE_T size) {
  char buffer[MAX_STRING_BUFFER]{0};

  if (!RPM(lpString, buffer, size ? size : sizeof(buffer))) {
    throw std::runtime_error("Failed to read memory in target process!");
  }

  // Calculate the length of the string using std::strlen
  size_t length = std::strlen(const_cast<char*>(buffer));

  // Construct a std::string using the pointer and length
  return std::string(buffer, length);
}

bool GameManager::CompareBytes(const ByteArray& bytes1,
                               const ByteArray& bytes2) {
  if (bytes1.size() != bytes2.size()) {
    return false;
  }

  // Compare byte by byte
  for (size_t i = 0; i < bytes1.size(); i++) {
    if (bytes1[i] != bytes2[i]) {
      return false;
    }
  }

  return true;
}

uintptr_t GameManager::AllocateMemory(SIZE_T allocSize) {
  void* remoteBuffer =
      VirtualAllocEx(hProcess, nullptr, allocSize, MEM_COMMIT | MEM_RESERVE,
                     PAGE_EXECUTE_READWRITE);
  if (!remoteBuffer) {
    throw std::runtime_error("Failed to allocate memory in target process.");
  }

  return (uintptr_t)remoteBuffer;
}

void GameManager::FreeMemory(LPVOID remoteBuffer) {
  VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
}

void GameManager::WriteBuffer(LPVOID lpBaseAdr, LPCVOID lpBuffer, SIZE_T size) {
  if (!WPM(lpBaseAdr, lpBuffer, size)) {
    throw std::runtime_error("Failed to write memory in target process!");
  }
}

void GameManager::WriteBytes(uintptr_t address, const ByteArray& bytes) {
  if (!WPM((LPVOID)address, bytes.data(), bytes.size())) {
    throw std::runtime_error("Failed to write memory in target process!");
  }
}

void GameManager::WriteBytes(uintptr_t address, const std::string& bytesStr) {
  ByteArray bytes = bytesStrToBytes(bytesStr);

  if (!WPM((LPVOID)address, bytes.data(), bytes.size())) {
    throw std::runtime_error("Failed to write memory in target process!");
  }
}

void GameManager::WriteProtectedBytes(LPVOID lpAddress,
                                      const ByteArray& bytes) {
  DWORD oldProtect;
  // Change memory protection to allow writing to the target address
  VirtualProtectEx(hProcess, lpAddress, bytes.size(), PAGE_EXECUTE_READWRITE,
                   &oldProtect);
  // Write the jump instruction into the target process
  WriteBytes((uintptr_t)lpAddress, bytes);
  // Restore original protection
  VirtualProtectEx(hProcess, lpAddress, bytes.size(), oldProtect, &oldProtect);
}

// Hook function that jumps to the new assembly code in another process
void GameManager::WriteHook(uintptr_t oriBlock, uintptr_t newBlock,
                            const unsigned char* asmSrc, SIZE_T asmSize,
                            SIZE_T nopSize) {
  const unsigned jmpSize = 5;
  ByteArray jmpBytes(jmpSize + nopSize, 0x90);

  // Calculate relative jump offset
  DWORD relativeOffset = (DWORD)newBlock - ((DWORD)oriBlock + jmpSize);

  // x86 jump opcode (0xE9)
  jmpBytes[0] = 0xE9;
  memcpy(jmpBytes.data() + 1, &relativeOffset, sizeof(DWORD));

  // Save original instructions
  ByteArray oriInstructions =
      ReadBytes((BYTE*)oriBlock, jmpSize + nopSize).Copy();

  WriteProtectedBytes((LPVOID)oriBlock, jmpBytes);

  // Create a trampoline to jump back to the original function
  uintptr_t returnOriBlock = oriBlock + jmpBytes.size();
  ByteArray trampoline(asmSize + oriInstructions.size() + jmpSize, 0);
  // structuring format:
  // new ASM bytes
  // original instruction
  // jump to return to the original block
  memcpy(trampoline.data(), asmSrc, asmSize);
  std::copy(oriInstructions.begin(), oriInstructions.end(),
            trampoline.begin() + asmSize);
  trampoline[asmSize + oriInstructions.size()] = 0xE9;  // JMP opcode
  DWORD relativeAddress =
      (DWORD)returnOriBlock -
      ((DWORD)newBlock + asmSize + oriInstructions.size() + jmpSize);
  memcpy(trampoline.data() + 1 + asmSize + oriInstructions.size(),
         &relativeAddress, sizeof(DWORD));

  // Write new assembly plus the trampoline to the allocated memory
  WriteBytes(newBlock, trampoline);
}

uintptr_t GameManager::FindPattern(const std::string& patternStr,
                                   uintptr_t startAdr, uintptr_t endAdr) {
  std::vector<int> pattern(parsePattern(patternStr));

  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);

  uintptr_t startAddress =
      startAdr > 0 ? startAdr : (uintptr_t)sysInfo.lpMinimumApplicationAddress;
  uintptr_t endAddress =
      endAdr > 0 ? endAdr : (uintptr_t)sysInfo.lpMaximumApplicationAddress;

  MEMORY_BASIC_INFORMATION mbi;

  std::vector<BYTE> buffer;
  for (uintptr_t address = startAddress; address < endAddress;
       address += mbi.RegionSize) {
    if (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi))) {
      if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_READWRITE) &&
          !(mbi.Protect & PAGE_GUARD)) {
        buffer.resize(mbi.RegionSize);
        SIZE_T bytesRead;
        if (RPM((LPCVOID)address, buffer.data(), mbi.RegionSize, &bytesRead)) {
          for (SIZE_T i = 0; i < bytesRead - pattern.size(); ++i) {
            if (comparePattern(buffer.data() + i, pattern)) {
              return address + i;
            }
          }
        }
      }
    }
  }
  return 0;
}

uintptr_t GameManager::FindPattern(const std::string& patternStr,
                                   const std::string& moduleName) {
  uintptr_t startAdr = 0;
  startAdr = GetModuleBaseAddress(moduleName);
  return FindPattern(patternStr, startAdr);
}

std::vector<int> GameManager::parsePattern(const std::string& pattern) {
  std::vector<int> parsedPattern;
  for (size_t i = 0; i < pattern.length(); i += 3) {
    if (pattern[i] == '?') {
      parsedPattern.push_back(-1);  // Use -1 to represent a wildcard
    } else {
      parsedPattern.push_back(std::stoi(pattern.substr(i, 2), nullptr, 16));
    }
  }
  return parsedPattern;
}

bool GameManager::comparePattern(const BYTE* data,
                                 const std::vector<int>& pattern) {
  for (size_t i = 0; i < pattern.size(); ++i) {
    if (pattern[i] != -1 && data[i] != pattern[i]) {
      return false;
    }
  }
  return true;
}

ByteArray GameManager::bytesStrToBytes(const std::string& hex) {
  std::vector<BYTE> bytes;
  std::istringstream hex_stream(hex);
  std::string byte_str;
  while (std::getline(hex_stream, byte_str, ' ')) {
    bytes.push_back(static_cast<BYTE>(std::stoi(byte_str, nullptr, 16)));
  }
  return bytes;
}
