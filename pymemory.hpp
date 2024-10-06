#ifndef PYMEMORY_HPP
#define PYMEMORY_HPP

#include <Python.h>

#include <string>
#include <variant>

#include "gmanager.hpp"

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

#endif /*PYMEMORY_HPP*/