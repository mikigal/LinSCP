// Zero-filled exception objects.
//
// C++Builder allocates thrown Delphi-style exceptions on the heap, zero-filled
// like every Delphi object, and the core relies on it (ExtException does not
// initialize FMoreMessages in all constructors). The RTL links with
// --wrap=__cxa_allocate_exception, so that the storage of every thrown object
// comes from here; TObject::operator new does the same for objects allocated
// with new.
//---------------------------------------------------------------------------
#include <cstddef>
#include <cstring>
//---------------------------------------------------------------------------
extern "C" void * __real___cxa_allocate_exception(std::size_t Size) noexcept;
//---------------------------------------------------------------------------
extern "C" void * __wrap___cxa_allocate_exception(std::size_t Size) noexcept
{
  void * Result = __real___cxa_allocate_exception(Size);
  std::memset(Result, 0, Size);
  return Result;
}
//---------------------------------------------------------------------------
