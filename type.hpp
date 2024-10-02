#ifndef TYPE_HPP_
#define TYPE_HPP_

#include <string>
#include <vector>

#include "entry.hpp"

namespace nosj {

namespace ent {

struct Entry {
  std::string adr_alias{};
  std::string disable{};
  std::string enable{};
  std::string scan{};
  int offset{0};
};
}  // namespace ent
namespace it {
struct Item {
  uintptr_t id{};
  std::string name{};
  std::vector<ent::Entry> records{};
};
}  // namespace it
}  // namespace nosj

namespace bro {
struct Item {
  uintptr_t id{};
  std::string name{};
  std::vector<bro::Entry> records{};
};

}  // namespace bro

struct ScanPayload {
  bool isOk{false};
  unsigned row{0};
  std::string name{};
  double progress{0};
};

// Entry list columns.
enum { Col_Active, Col_Description, Col_Status };

// Menu items.
enum {
  Id_OpenConfig = 100,
  Id_SaveConfig,
  Id_SaveConfigAs,

  Id_EnableAll,
  Id_DisableAll,

  Id_Option_Start,
  Id_EnsureScanAll,
  Id_AddNewEntry,
  Id_Option_End,

  Id_CTRL = 1000,
  Id_Entry_Ctrl,
  Id_Pointer_Ctrl
};


#endif /*TYPE_HPP_*/