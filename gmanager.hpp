#ifndef GMANAGER_HPP
#define GMANAGER_HPP

#include <Python.h>
#include <windows.h>

#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#define MAX_STRING_BUFFER 32

struct DataResult;

// Helper typedef for simplicity
typedef std::vector<BYTE> ByteArray;

class GameManager {
 public:
  // Constructor to initialize with process ID
  GameManager(unsigned pid);

  // Destructor to clean up resources
  ~GameManager();

  DataResult ReadBytes(BYTE* lpBytes, SIZE_T size);
  std::string ReadString(const char* lpString, SIZE_T size = 0);

  // Compare two byte arrays
  bool CompareBytes(const ByteArray& bytes1, const ByteArray& bytes2);

  // Get base address of a module by its name
  uintptr_t GetModuleBaseAddress(const std::string& moduleName);

  // Allocate memory in the target process,  default size is 0x1000
  uintptr_t AllocateMemory(SIZE_T allocSize = 0x1000);
  void FreeMemory(LPVOID remoteBuffer);

  // Write the assembly code to the allocated memory
  void WriteBytes(uintptr_t address, const ByteArray& bytes);
  void WriteBuffer(LPVOID lpBaseAdr, LPCVOID lpBuffer, SIZE_T size);

  // Hook function that jumps to the new assembly code in another process
  void WriteHook(uintptr_t oriBlock, uintptr_t newBlock,
                 const unsigned char* asmSrc, SIZE_T asmSize, SIZE_T nopSize);

  // Function to write a byte pattern to a specific address in the memory of a
  // process
  void WriteBytes(uintptr_t address, const std::string& bytesStr);

  void WriteProtectedBytes(LPVOID lpAddress, const ByteArray& bytes);

  uintptr_t FindPattern(const std::string& patternStr, uintptr_t startAdr = 0,
                        uintptr_t endAdr = 0);
  uintptr_t FindPattern(const std::string& patternStr,
                        const std::string& moduleName);

  bool ReadBuffer(LPCVOID lpBaseAdr, LPVOID lpBuffer, SIZE_T size);

  // Function to read memory from another process
  template <typename T>
  T ReadMemory(LPCVOID address) {
    T buffer;
    if (RPM(address, &buffer, sizeof(T))) {
      return buffer;
    } else {
      throw std::runtime_error("Failed to read memory in target process!");
    }
  }

  // Function to write memory from another process
  template <typename T>
  void WriteMemory(T* address, LPCVOID buffer) {
    if (WPM(address, buffer, sizeof(T))) {
      return;
    } else {
      throw std::runtime_error("Failed to write memory in target process!");
    }
  }

 protected:
  HANDLE hProcess;

 private:
  std::vector<int> parsePattern(const std::string& pattern);
  bool comparePattern(const BYTE* data, const std::vector<int>& pattern);

  // Function to convert a string of hex bytes to a vector of bytes
  ByteArray bytesStrToBytes(const std::string& hex);

  // Helper method to open a process
  HANDLE openProcessHandle(unsigned pid);

  // Read bytes from the memory of another process
  bool RPM(LPCVOID lpBaseAdr, LPVOID lpBuffer, SIZE_T size,
           SIZE_T* lpBytesRread = nullptr);

  // Read bytes from the memory of another process
  bool WPM(LPVOID lpBaseAdr, LPCVOID lpBuffer, SIZE_T size,
           SIZE_T* lpBytesRread = nullptr);
};

// Helper struct to encapsulate reading results and comparison
struct DataResult {
  ByteArray data;
  GameManager* gameManager;

  DataResult(ByteArray d, GameManager* m) : data(d), gameManager(m) {}

  // Method to compare this result's data with another byte array
  bool Compare(const ByteArray& other) const {
    if (data.size() != other.size()) {
      return false;
    }
    for (size_t i = 0; i < data.size(); ++i) {
      if (data[i] != other[i]) {
        return false;
      }
    }
    return true;
  }

  ByteArray Copy() { return ByteArray(data); }
};

#define CHARA_DICT_SIZE 54
#define ANY_DICT_SIZE 6

namespace py {

struct PyObjectPlus;

struct CharObj {
  PyObject ob_base;
  PyDictObject* charVal;
};

struct PyIntObject {
  PyVarObject ob_base;
  int digit;
};

struct Array_ptrPyObject {
  PyObject* p_values[1];  // Array of PyObject*
};

struct PyStringObject {
  PyObject ob_base;
  unsigned size;
  char unknown[0xC];
  char str[MAX_STRING_BUFFER];
};

struct DictKey {
  unsigned unknown;
  PyStringObject* key;
  unsigned unknown2;
};

struct DictKeyVal {
  unsigned unknown;
  PyStringObject* key;
  PyObjectPlus* val;
};

struct DictKeys {
  DictKey keys[1];  // Array of struct DictKey
};

struct DictKeyValues {
  unsigned unknown3;
  DictKeyVal keyValues[1];
};

struct DictKeysHeads {
  unsigned ob_refcnt;
  unsigned size_junkBytes;
  char unknown[0x8];
  unsigned size_dict;
  // char junkBytes[];
};

struct PyObjectPlus {
  unsigned ob_refcnt;
  PyTypeObject* ob_type;
  unsigned size;
  char unknown[0xC];
  char str[MAX_STRING_BUFFER];
};

struct PyItem {
  std::string key;
  std::string val_type;
  std::variant<int, std::string, double> value;
  uintptr_t valueAddress{0};
};

std::vector<PyItem> LookupPyItem(GameManager& game, const PyDictObject& dict,
                                 const std::string& prefix = "");

void LookupInValues(std::vector<PyItem>& pyItem, GameManager& game,
                    const PyDictObject& dict, const std::string& prefix);
void LookupInKeys(std::vector<PyItem>& pyItem, GameManager& game,
                  const PyDictObject& dict, const std::string& prefix);

}  // namespace py

#endif /*GMANAGER_HPP*/