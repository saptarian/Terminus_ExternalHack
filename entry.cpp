#include "Entry.hpp"

#include <exception>

#include "gmanager.hpp"

namespace bro {

// Entry

Entry::Entry(const std::string adr_alias, const std::string disable,
             const std::string enable, const std::string signature,
             uintptr_t scanOffset) {
  this->adr_alias = adr_alias;
  this->disable = disable;
  this->enable = enable;
  this->signature = signature;
  this->scanOffset = scanOffset;

  this->sigSize = (signature.length() + 1) / 3;
  this->writeSize = (disable.length() + 1) / 3;

  this->enabled = false;
}

std::string Entry::AdrAlias() { return this->adr_alias; }

bool Entry::Enabled() { return this->enabled; }

uintptr_t Entry::Address() { return this->AddressPool[this->adr_alias]; }

void Entry::SetAddress(uintptr_t address) {
  this->AddressPool[this->adr_alias] = address;
}

Entry::~Entry() {}

void Entry::Disable(unsigned pid) { return Enable(pid, false); }

void Entry::Enable(unsigned pid, bool enable) {
  try {
    if (uintptr_t address = this->Address()) {
      GameManager(pid).WriteBytes(address + this->scanOffset,
                                  enable ? this->enable : this->disable);
      this->enabled = enable;
    }
  } catch (const std::exception& ex) {
    this->enabled = !enable;
  }
}

uintptr_t Entry::Scan(unsigned pid) {
  if (this->sigSize < 8) return 0;
  return GameManager(pid).FindPattern(this->signature);
}

void Entry::EnsureScan(unsigned pid) {
  uintptr_t address = this->Scan(pid);

  // no need to save when saved address just same
  if (!address || address == this->Address()) return;

  if (this->Enabled()) {
    this->Disable(pid);
    this->SetAddress(address);
    this->Enable(pid);
  } else {
    this->SetAddress(address);
  }
}

}  // namespace bro
