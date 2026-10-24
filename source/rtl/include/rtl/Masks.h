// The System.Masks unit: TMask, Delphi's file mask matching.
//
// Case-insensitive; * matches any sequence, ? any character, [abc], [a-z] and
// [!a-z] a set of characters. Note that "*.*" requires a dot, unlike FindFirst.
//---------------------------------------------------------------------------
#ifndef RtlMasksH
#define RtlMasksH
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>

#include <vector>
//---------------------------------------------------------------------------
namespace System
{
namespace Masks
{
//---------------------------------------------------------------------------
RTL_DECLARE_EXCEPTION(EMaskException, Exception)
//---------------------------------------------------------------------------
class TMask : public TObject
{
public:
  explicit TMask(const UnicodeString MaskValue);
  bool Matches(const UnicodeString Filename);

private:
  enum TKind { mkLiteral, mkAnyChar, mkSet, mkAnyText };
  struct TState
  {
    TKind Kind;
    wchar_t Literal;
    bool Negate;
    // Pairs of inclusive ranges
    std::vector<std::pair<wchar_t, wchar_t>> Ranges;
  };

  bool MatchesFrom(size_t State, const wchar_t * P, const wchar_t * End) const;

  std::vector<TState> FStates;
};
//---------------------------------------------------------------------------
bool MatchesMask(const UnicodeString Filename, const UnicodeString Mask);
//---------------------------------------------------------------------------
} // namespace Masks
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Masks;
//---------------------------------------------------------------------------
#endif
