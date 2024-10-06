#include "pymemory.hpp"

// py

namespace py {

std::vector<PyItem> LookupPyItem(GameManager& game, const PyDictObject& dict,
                                 const std::string& prefix) {
  std::vector<PyItem> pyItems;

  if (dict.ma_values) {
    LookupInValues(pyItems, game, dict, prefix);
  } else {
    LookupInKeys(pyItems, game, dict, prefix);
  }

  return pyItems;
}

void LookupInKeys(std::vector<PyItem>& pyItem, GameManager& game,
                  const PyDictObject& dict, const std::string& prefix) {
  // Prepare for array of PyObject*
  DictKeysHeads heads = game.ReadMemory<DictKeysHeads>(dict.ma_keys);
  unsigned totalSize = sizeof(DictKeysHeads) + heads.size_junkBytes +
                       sizeof(int) + (sizeof(DictKeyVal) * dict.ma_used);
  DictKeysHeads* p_heads = (DictKeysHeads*)malloc(totalSize);
  game.ReadBuffer(dict.ma_keys, p_heads, totalSize);

  uintptr_t heads_end =
      (uintptr_t)p_heads + sizeof(DictKeysHeads) + heads.size_junkBytes;

  for (int i = 0; i < dict.ma_used; ++i) {
    DictKeyVal* p_dictKeyValue =
        (DictKeyVal*)(heads_end + (i * sizeof(DictKeyVal)));
    // keys processing
    PyStringObject dictKey =
        game.ReadMemory<PyStringObject>(p_dictKeyValue->key);

    // values processing
    PyObjectPlus dictValObj =
        game.ReadMemory<PyObjectPlus>(p_dictKeyValue->val);

    PyTypeObject valType = game.ReadMemory<PyTypeObject>(dictValObj.ob_type);
    std::string valTypeName(game.ReadString(valType.tp_name));

    if (valTypeName == "dict") {
      PyDictObject dict_obj =
          game.ReadMemory<PyDictObject>(p_dictKeyValue->val);

      if (dict_obj.ma_used <= 10) {
        std::string _prefix(dictKey.str);
        _prefix.push_back('.');

        if (dict_obj.ma_values) {
          LookupInValues(pyItem, game, dict_obj, _prefix);
        } else {
          LookupInKeys(pyItem, game, dict_obj, _prefix);
        }
      }
      continue;
    }

    if (valTypeName != "str" && valTypeName != "int" && valTypeName != "float")
      continue;

    PyItem item;
    item.key = prefix;
    item.key.append(dictKey.str);
    item.val_type = valTypeName;

    if (valTypeName == "str") {
      PyStringObject* str_obj = reinterpret_cast<PyStringObject*>(&dictValObj);

      item.value = str_obj->str;

    } else if (valTypeName == "int") {
      PyIntObject* int_obj = reinterpret_cast<PyIntObject*>(&dictValObj);
      uintptr_t addr = (uintptr_t)dict.ma_keys + sizeof(DictKeysHeads) +
                       heads.size_junkBytes + offsetof(DictKeyVal, val) +
                       (i * sizeof(DictKeyVal));

      item.value = int_obj->digit;
      item.valueAddress = addr;
    } else if (valTypeName == "float") {
      PyFloatObject* int_obj = reinterpret_cast<PyFloatObject*>(&dictValObj);

      item.value = int_obj->ob_fval;
      item.valueAddress =
          (uintptr_t)p_dictKeyValue->val + offsetof(PyFloatObject, ob_fval);
    }
    pyItem.push_back(item);
  }

  free(p_heads);
}

void LookupInValues(std::vector<PyItem>& pyItem, GameManager& game,
                    const PyDictObject& dict, const std::string& prefix) {
  // Prepare for array of PyObject*
  unsigned readSize = sizeof(Array_ptrPyObject) * dict.ma_used;
  Array_ptrPyObject* p_dictValues = (Array_ptrPyObject*)malloc(readSize);
  game.ReadBuffer(dict.ma_values, p_dictValues, readSize);

  // Prepare space for array of DictKey
  DictKeysHeads heads = game.ReadMemory<DictKeysHeads>(dict.ma_keys);
  unsigned totalSize = sizeof(DictKeysHeads) + heads.size_junkBytes +
                       sizeof(int) + (sizeof(DictKey) * dict.ma_used);
  DictKeysHeads* p_heads = (DictKeysHeads*)malloc(totalSize);
  game.ReadBuffer(dict.ma_keys, p_heads, totalSize);

  uintptr_t heads_end =
      (uintptr_t)p_heads + sizeof(DictKeysHeads) + heads.size_junkBytes;

  for (int i = 0; i < dict.ma_used; ++i) {
    // keys processing
    DictKey* p_dictKey = (DictKey*)(heads_end + (i * sizeof(DictKey)));
    PyStringObject dictKey = game.ReadMemory<PyStringObject>(p_dictKey->key);

    // values processing
    PyObjectPlus dictValObj =
        game.ReadMemory<PyObjectPlus>(p_dictValues->p_values[i]);

    PyTypeObject valType = game.ReadMemory<PyTypeObject>(dictValObj.ob_type);
    std::string valTypeName(game.ReadString(valType.tp_name));

    if (valTypeName == "dict") {
      PyDictObject dict_obj =
          game.ReadMemory<PyDictObject>(p_dictValues->p_values[i]);

      if (dict_obj.ma_used <= 10) {
        std::string _prefix(dictKey.str);
        _prefix.push_back('.');

        if (dict_obj.ma_values) {
          LookupInValues(pyItem, game, dict_obj, _prefix);
        } else {
          LookupInKeys(pyItem, game, dict_obj, _prefix);
        }
      }
      continue;
    }

    if (valTypeName != "str" && valTypeName != "int" && valTypeName != "float")
      continue;

    PyItem item;
    item.key = prefix;
    item.key.append(dictKey.str);
    item.val_type = valTypeName;

    if (valTypeName == "str") {
      PyStringObject* str_obj = reinterpret_cast<PyStringObject*>(&dictValObj);

      item.value = str_obj->str;

    } else if (valTypeName == "int") {
      PyIntObject* int_obj = reinterpret_cast<PyIntObject*>(&dictValObj);
      uintptr_t addr = (uintptr_t)dict.ma_values + (i * sizeof(PyObject*));

      item.value = int_obj->digit;
      item.valueAddress = addr;
    } else if (valTypeName == "float") {
      PyFloatObject* float_obj = reinterpret_cast<PyFloatObject*>(&dictValObj);

      item.value = float_obj->ob_fval;
      item.valueAddress = (uintptr_t)p_dictValues->p_values[i] +
                          offsetof(PyFloatObject, ob_fval);
    }

    pyItem.push_back(item);
  }

  free(p_heads);
  free(p_dictValues);
}

}  // namespace py