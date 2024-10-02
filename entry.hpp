#ifndef ENTRY_HPP
#define ENTRY_HPP

#include <string>
#include <unordered_map>

namespace bro {

// Entry

class Entry {
 public:
  Entry(const std::string adr_alias, const std::string disable,
        const std::string enable, const std::string signature,
        uintptr_t scanOffset = 0);
  ~Entry();

  inline static std::unordered_map<std::string, uintptr_t> AddressPool;

  std::string AdrAlias();
  bool Enabled();
  void SetAddress(uintptr_t address);
  uintptr_t Address();

  uintptr_t Scan(unsigned pid);
  void EnsureScan(unsigned pid);
  void Disable(unsigned pid);
  void Enable(unsigned pid, bool enable = true);

 protected:
  uintptr_t scanOffset;
  std::string signature;
  std::string enable;
  std::string disable;

  size_t sigSize;
  size_t writeSize;

 private:
  std::string adr_alias;
  bool enabled;
};

}  // namespace bro

#endif /* ENTRY_HPP */