// rtl::TFinally, the replacement for C++Builder's try/__finally.
//
// tools/bcb2clang converts
//
//   try
//   {
//     A
//   }
//   __finally
//   {
//     B
//   }
//
// to
//
//   { ::rtl::TFinally Finally([&]()
//   {
//     B
//   }); try
//   {
//     A
//   }
//   catch (...) { Finally.Unwind(); } }
//
// B runs when A completes, including by return, break or continue (from the
// guard's destructor), and when A throws (from the catch handler). In the
// latter case B runs outside of stack unwinding, so an exception thrown by B
// replaces the original one, as in Delphi, instead of terminating the program.
//---------------------------------------------------------------------------
#ifndef RtlFinallyH
#define RtlFinallyH
//---------------------------------------------------------------------------
#include <utility>
//---------------------------------------------------------------------------
namespace rtl
{
//---------------------------------------------------------------------------
template <typename F>
class TFinally
{
public:
  explicit TFinally(F && Block) :
    FBlock(std::move(Block))
  {
  }

  TFinally(const TFinally &) = delete;
  TFinally & operator =(const TFinally &) = delete;

  ~TFinally() noexcept(false)
  {
    if (FArmed)
    {
      FArmed = false;
      FBlock();
    }
  }

  // To be called from a catch (...) handler only
  [[noreturn]] void Unwind()
  {
    FArmed = false;
    FBlock();
    throw;
  }

private:
  F FBlock;
  bool FArmed = true;
};
//---------------------------------------------------------------------------
} // namespace rtl
//---------------------------------------------------------------------------
#endif
