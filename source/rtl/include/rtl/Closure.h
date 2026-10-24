// System::TMethodPtr, the replacement for C++Builder's __closure.
//
// tools/bcb2clang converts `typedef R __fastcall (__closure * TEvent)(Args)` to
// `typedef TMethodPtr<R(Args)> TEvent`. Like a Delphi TMethod, a TMethodPtr is an
// object plus a method, can be NULL, is copyable and compares equal when both
// the object and the method are the same.
//---------------------------------------------------------------------------
#ifndef RtlClosureH
#define RtlClosureH
//---------------------------------------------------------------------------
#include <cstring>
#include <type_traits>
#include <utility>
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
struct TMethod
{
  void * Code;
  void * Data;
};
//---------------------------------------------------------------------------
namespace rtl_detail
{
struct TNullClosure;
struct TMethodPtrDummy;
typedef void (TMethodPtrDummy::*TAnyMemberFunction)();
}
//---------------------------------------------------------------------------
template <typename Signature> class TMethodPtr;
//---------------------------------------------------------------------------
template <typename R, typename... A>
class TMethodPtr<R(A...)>
{
public:
  TMethodPtr() noexcept = default;
  // Accepts NULL, 0 and nullptr
  TMethodPtr(rtl_detail::TNullClosure *) noexcept {}

  template <class O, class C>
  TMethodPtr(O * Object, R (C::*Method)(A...)) noexcept
  {
    static_assert(std::is_base_of_v<C, O>, "the method must belong to the object's class");
    Bind<C, R (C::*)(A...)>(static_cast<C *>(Object), Method, &InvokeMember<C, R (C::*)(A...)>);
  }

  template <class O, class C>
  TMethodPtr(O * Object, R (C::*Method)(A...) const) noexcept
  {
    static_assert(std::is_base_of_v<C, O>, "the method must belong to the object's class");
    Bind<C, R (C::*)(A...) const>(
      const_cast<C *>(static_cast<const C *>(Object)), Method, &InvokeMember<C, R (C::*)(A...) const>);
  }

  // A plain function receiving the data pointer as its first argument,
  // the equivalent of a TMethod whose Code is a function taking Self.
  TMethodPtr(void * Data, R (*Function)(void *, A...)) noexcept
  {
    if (Function != nullptr)
    {
      Bind<void, R (*)(void *, A...)>(Data, Function, &InvokeFunction);
    }
  }

  R operator ()(A... Args) const
  {
    return FInvoker(*this, std::forward<A>(Args)...);
  }

  explicit operator bool() const noexcept { return (FInvoker != nullptr); }
  bool operator !() const noexcept { return (FInvoker == nullptr); }

  bool operator ==(const TMethodPtr & Rhs) const noexcept
  {
    return
      (FData == Rhs.FData) && (FInvoker == Rhs.FInvoker) &&
      (std::memcmp(FMethod, Rhs.FMethod, sizeof(FMethod)) == 0);
  }
  bool operator !=(const TMethodPtr & Rhs) const noexcept { return !(*this == Rhs); }

  void * GetData() const noexcept { return FData; }

private:
  typedef R (*TInvoker)(const TMethodPtr & Self, A... Args);

  template <class C, typename M>
  void Bind(C * Object, M Method, TInvoker Invoker) noexcept
  {
    static_assert(sizeof(M) <= sizeof(FMethod));
    if (Method != nullptr)
    {
      FData = const_cast<void *>(static_cast<const void *>(Object));
      FInvoker = Invoker;
      std::memcpy(FMethod, &Method, sizeof(M));
    }
  }

  template <class C, typename M>
  static R InvokeMember(const TMethodPtr & Self, A... Args)
  {
    M Method;
    std::memcpy(&Method, Self.FMethod, sizeof(M));
    return (static_cast<C *>(Self.FData)->*Method)(std::forward<A>(Args)...);
  }

  static R InvokeFunction(const TMethodPtr & Self, A... Args)
  {
    R (*Function)(void *, A...);
    std::memcpy(&Function, Self.FMethod, sizeof(Function));
    return Function(Self.FData, std::forward<A>(Args)...);
  }

  void * FData = nullptr;
  TInvoker FInvoker = nullptr;
  alignas(rtl_detail::TAnyMemberFunction) unsigned char FMethod[sizeof(rtl_detail::TAnyMemberFunction)] = {};
};
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
namespace rtl
{
//---------------------------------------------------------------------------
// An object and one of its methods, convertible to the TMethodPtr of the
// method's signature. tools/bcb2clang converts C++Builder's references to
// methods (`OnEvent = Method;`, `&Method`, `&Object->Method`) to
// `::rtl::MethodPtr(Object, &TClass::Method)`.
template <class O, typename M>
class TMethodBinder
{
public:
  TMethodBinder(O * Object, M Method) noexcept :
    FObject(Object),
    FMethod(Method)
  {
  }

  template <typename S>
  operator System::TMethodPtr<S>() const noexcept
  {
    return System::TMethodPtr<S>(FObject, FMethod);
  }

private:
  O * FObject;
  M FMethod;
};
//---------------------------------------------------------------------------
template <class O, typename M>
TMethodBinder<O, M> MethodPtr(O * Object, M Method) noexcept
{
  return TMethodBinder<O, M>(Object, Method);
}
//---------------------------------------------------------------------------
// Delphi has no const objects; a method of a const object binds as well
template <class O, typename M>
TMethodBinder<O, M> MethodPtr(const O * Object, M Method) noexcept
{
  return TMethodBinder<O, M>(const_cast<O *>(Object), Method);
}
//---------------------------------------------------------------------------
} // namespace rtl
//---------------------------------------------------------------------------
#endif
