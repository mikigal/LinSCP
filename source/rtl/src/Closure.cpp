// Interned closure codes, see rtl/Closure.h.
//---------------------------------------------------------------------------
#include <rtl/Closure.h>

#include <cstring>
#include <mutex>
#include <set>
//---------------------------------------------------------------------------
namespace System
{
namespace rtl_detail
{
//---------------------------------------------------------------------------
namespace
{
struct TClosureCodeLess
{
  bool operator ()(const TClosureCode & Code1, const TClosureCode & Code2) const
  {
    if (Code1.Invoker != Code2.Invoker)
    {
      return std::less<TAnyFunction>()(Code1.Invoker, Code2.Invoker);
    }
    return std::memcmp(Code1.Method, Code2.Method, sizeof(Code1.Method)) < 0;
  }
};
} // namespace
//---------------------------------------------------------------------------
const TClosureCode * InternClosureCode(TAnyFunction Invoker, const void * Method, size_t Size)
{
  // Constructed on first use, never destroyed: closures may be called from
  // static destructors
  static std::mutex * Mutex = new std::mutex();
  static std::set<TClosureCode, TClosureCodeLess> * Codes = new std::set<TClosureCode, TClosureCodeLess>();
  TClosureCode Code;
  Code.Invoker = Invoker;
  std::memset(Code.Method, 0, sizeof(Code.Method));
  std::memcpy(Code.Method, Method, Size);
  std::lock_guard<std::mutex> Lock(*Mutex);
  // Elements of a set do not move
  return &*Codes->insert(Code).first;
}
//---------------------------------------------------------------------------
} // namespace rtl_detail
} // namespace System
//---------------------------------------------------------------------------
