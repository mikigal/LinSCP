// Dialect support and basic Delphi types for code written for C++Builder.
// See docs/port/PLAN.md, section 3.2. The C++Builder keywords that have a direct
// equivalent are plain macros; constructs that need restructuring (__property,
// __closure, try/__finally) are converted by tools/bcb2clang instead.
//---------------------------------------------------------------------------
#ifndef RtlDefsH
#define RtlDefsH
//---------------------------------------------------------------------------
#include <cstddef>
#include <cstdint>
//---------------------------------------------------------------------------
#define __fastcall
// Borland accepts one underscore too
#define _fastcall
#define __published public
#define PACKAGE
#define DELPHI_PACKAGE
#define PASCALIMPLEMENTATION
#define DELPHICLASS class
#define DYNAMIC virtual
#define HIDESBASE
#define RTL_DELPHIRETURN
//---------------------------------------------------------------------------
// A typedef rather than a macro, so that functional casts like __int64(X) work.
typedef long long __int64;
//---------------------------------------------------------------------------
namespace rtl
{
// Used by the accessors tools/bcb2clang generates for __property getters,
// which may or may not be const
template <class T>
inline T * Mutable(const T * Object)
{
  return const_cast<T *>(Object);
}
} // namespace rtl
//---------------------------------------------------------------------------
// Accessors of the RTL's properties. Clang looks the accessors of a
// __declspec(property) up where the property is used, so a method of a
// derived class with the name of an accessor would replace it (in Delphi,
// the accessors are bound where the property is declared). The properties
// therefore name forwarders with unique names, which call the real accessors
// from the declaring class.
#define RTL_PROPERTY_GET(Name, Getter) \
  template <typename... A> \
  decltype(auto) __property_get_##Name(A &&... Args) const \
  { return ::rtl::Mutable(this)->Getter(static_cast<A &&>(Args)...); }
// Setters take the property's type (not forwarded arguments), so that the
// value converts as in an assignment (NULL to a pointer, for instance)
#define RTL_PROPERTY_PUT(Name, Setter, Type) \
  void __property_set_##Name(Type Value) { Setter(Value); }
#define RTL_PROPERTY_PUT_INDEXED(Name, Setter, IndexType, Type) \
  void __property_set_##Name(IndexType Index, Type Value) { Setter(Index, Value); }
//---------------------------------------------------------------------------
namespace System
{
typedef int Integer;
typedef unsigned int Cardinal;
typedef signed char ShortInt;
typedef short SmallInt;
typedef unsigned char Byte;
typedef unsigned short Word;
typedef int LongInt;
typedef unsigned int LongWord;
typedef unsigned int FixedUInt;
typedef __int64 Int64;
typedef unsigned long long UInt64;
// Pointer-sized integers follow the platform (long on LP64 Linux), like size_t.
typedef intptr_t NativeInt;
typedef uintptr_t NativeUInt;
typedef bool Boolean;
typedef int LongBool;
typedef char AnsiChar;
// wchar_t is UTF-32 on Linux; see PLAN.md 3.2 for why UnicodeString is built on it.
typedef wchar_t WideChar;
typedef wchar_t Char;
typedef AnsiChar * PAnsiChar;
typedef WideChar * PWideChar;
typedef WideChar * PChar;
typedef void * Pointer;
typedef float Single;
typedef double Double;
typedef long double Extended;
// Delphi's 64-bit integer held in a floating point type
typedef double Comp;
typedef NativeUInt THandle;
typedef unsigned int TThreadID;

const int MaxInt = 2147483647;
const int MaxLongint = 2147483647;
} // namespace System
//---------------------------------------------------------------------------
#endif
