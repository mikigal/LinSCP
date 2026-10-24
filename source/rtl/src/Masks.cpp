// System.Masks
//---------------------------------------------------------------------------
#include <rtl/Masks.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Masks
{
//---------------------------------------------------------------------------
TMask::TMask(const UnicodeString MaskValue)
{
  UnicodeString Mask = AnsiUpperCase(MaskValue);
  int Len = Mask.Length();
  int I = 1;
  auto InvalidMask = [&](int Pos)
    {
      throw EMaskException(L"'%s' is an invalid mask at (%d)", ARRAYOFCONST((MaskValue, Pos)));
    };
  while (I <= Len)
  {
    TState State;
    State.Negate = false;
    State.Literal = L'\0';
    wchar_t Ch = Mask[I];
    switch (Ch)
    {
      case L'*':
        State.Kind = mkAnyText;
        // Consecutive stars are one
        if (!FStates.empty() && (FStates.back().Kind == mkAnyText))
        {
          I++;
          continue;
        }
        I++;
        break;

      case L'?':
        State.Kind = mkAnyChar;
        I++;
        break;

      case L'[':
        {
          State.Kind = mkSet;
          I++;
          if ((I <= Len) && (Mask[I] == L'!'))
          {
            State.Negate = true;
            I++;
          }
          bool Closed = false;
          while (I <= Len)
          {
            wchar_t First = Mask[I];
            if (First == L']')
            {
              Closed = true;
              I++;
              break;
            }
            wchar_t Last = First;
            if ((I + 2 <= Len) && (Mask[I + 1] == L'-') && (Mask[I + 2] != L']'))
            {
              Last = Mask[I + 2];
              if (Last < First)
              {
                InvalidMask(I);
              }
              I += 3;
            }
            else
            {
              I++;
            }
            State.Ranges.push_back(std::make_pair(First, Last));
          }
          if (!Closed || State.Ranges.empty())
          {
            InvalidMask(I - 1);
          }
        }
        break;

      default:
        State.Kind = mkLiteral;
        State.Literal = Ch;
        I++;
        break;
    }
    FStates.push_back(State);
  }
}
//---------------------------------------------------------------------------
bool TMask::MatchesFrom(size_t StateIndex, const wchar_t * P, const wchar_t * End) const
{
  while (StateIndex < FStates.size())
  {
    const TState & State = FStates[StateIndex];
    switch (State.Kind)
    {
      case mkAnyText:
        if (StateIndex + 1 == FStates.size())
        {
          return true;
        }
        for (const wchar_t * Q = P; Q <= End; Q++)
        {
          if (MatchesFrom(StateIndex + 1, Q, End))
          {
            return true;
          }
        }
        return false;

      case mkAnyChar:
        if (P >= End)
        {
          return false;
        }
        break;

      case mkSet:
        {
          if (P >= End)
          {
            return false;
          }
          bool InSet = false;
          for (const auto & Range : State.Ranges)
          {
            if ((*P >= Range.first) && (*P <= Range.second))
            {
              InSet = true;
              break;
            }
          }
          if (InSet == State.Negate)
          {
            return false;
          }
        }
        break;

      case mkLiteral:
        if ((P >= End) || (*P != State.Literal))
        {
          return false;
        }
        break;
    }
    P++;
    StateIndex++;
  }
  return (P == End);
}
//---------------------------------------------------------------------------
bool TMask::Matches(const UnicodeString Filename)
{
  UnicodeString Name = AnsiUpperCase(Filename);
  return MatchesFrom(0, Name.c_str(), Name.c_str() + Name.Length());
}
//---------------------------------------------------------------------------
bool MatchesMask(const UnicodeString Filename, const UnicodeString Mask)
{
  TMask MaskObj(Mask);
  return MaskObj.Matches(Filename);
}
//---------------------------------------------------------------------------
} // namespace Masks
} // namespace System
