// printf-style formatting with Windows C runtime conventions, see rtl/Printf.h
//---------------------------------------------------------------------------
#include <rtl/Printf.h>
#include <rtl/Conversion.h>

#include <cstdio>
#include <cstring>
#include <cwchar>
//---------------------------------------------------------------------------
namespace rtl
{
//---------------------------------------------------------------------------
locale_t CLocale()
{
  static locale_t Locale = newlocale(LC_ALL_MASK, "C", static_cast<locale_t>(0));
  return Locale;
}
//---------------------------------------------------------------------------
namespace
{
enum TLength { lnNone, lnHH, lnH, lnL, lnLL, lnBigL, lnZ, lnJ, lnT, lnW };

template <typename... T>
std::string FormatNumber(const std::string & Spec, T... Value)
{
  return WithCLocale([&]()
    {
      char Buffer[512];
      int Len = snprintf(Buffer, sizeof(Buffer), Spec.c_str(), Value...);
      if (Len < 0)
      {
        return std::string();
      }
      if (static_cast<size_t>(Len) < sizeof(Buffer))
      {
        return std::string(Buffer, Len);
      }
      std::string Result(Len + 1, '\0');
      snprintf(Result.data(), Result.size(), Spec.c_str(), Value...);
      Result.resize(Len);
      return Result;
    });
}

void AppendPadded(std::wstring & Result, const std::wstring & Value, int Width, bool LeftJustify)
{
  int Padding = (Width > static_cast<int>(Value.size())) ? (Width - static_cast<int>(Value.size())) : 0;
  if (!LeftJustify)
  {
    Result.append(Padding, L' ');
  }
  Result += Value;
  if (LeftJustify)
  {
    Result.append(Padding, L' ');
  }
}

void AppendAscii(std::wstring & Result, const std::string & Value)
{
  for (char Ch : Value)
  {
    Result.push_back(static_cast<wchar_t>(static_cast<unsigned char>(Ch)));
  }
}

template <typename CharT>
bool IsDigit(CharT Ch)
{
  return (Ch >= '0') && (Ch <= '9');
}

template <typename CharT>
std::wstring DoPrintf(const CharT * Format, va_list Args, bool Wide)
{
  std::wstring Result;
  if (Format == nullptr)
  {
    return Result;
  }
  const CharT * P = Format;
  while (*P != 0)
  {
    if (*P != '%')
    {
      const CharT * Start = P;
      while ((*P != 0) && (*P != '%'))
      {
        P++;
      }
      if constexpr (sizeof(CharT) == 1)
      {
        Result += DecodeUtf8(reinterpret_cast<const char *>(Start), P - Start);
      }
      else
      {
        Result.append(reinterpret_cast<const wchar_t *>(Start), P - Start);
      }
      continue;
    }

    const CharT * SpecStart = P;
    P++;
    if (*P == '%')
    {
      Result.push_back(L'%');
      P++;
      continue;
    }

    std::string Flags;
    while ((*P != 0) && (strchr("-+ #0'", static_cast<char>(*P)) != nullptr))
    {
      Flags.push_back(static_cast<char>(*P));
      P++;
    }
    int Width = -1;
    if (*P == '*')
    {
      Width = va_arg(Args, int);
      if (Width < 0)
      {
        Flags.push_back('-');
        Width = -Width;
      }
      P++;
    }
    else
    {
      while (IsDigit(*P))
      {
        Width = ((Width < 0) ? 0 : Width * 10) + (*P - '0');
        P++;
      }
    }
    int Precision = -1;
    if (*P == '.')
    {
      P++;
      Precision = 0;
      if (*P == '*')
      {
        Precision = va_arg(Args, int);
        if (Precision < 0)
        {
          Precision = -1;
        }
        P++;
      }
      else
      {
        while (IsDigit(*P))
        {
          Precision = Precision * 10 + (*P - '0');
          P++;
        }
      }
    }

    TLength Length = lnNone;
    if ((P[0] == 'h') && (P[1] == 'h'))
    {
      Length = lnHH;
      P += 2;
    }
    else if ((P[0] == 'l') && (P[1] == 'l'))
    {
      Length = lnLL;
      P += 2;
    }
    else if ((P[0] == 'I') && (P[1] == '6') && (P[2] == '4'))
    {
      Length = lnLL;
      P += 3;
    }
    else if ((P[0] == 'I') && (P[1] == '3') && (P[2] == '2'))
    {
      P += 3;
    }
    else
    {
      switch (*P)
      {
        case 'h': Length = lnH; P++; break;
        case 'l': Length = lnL; P++; break;
        case 'L': Length = lnBigL; P++; break;
        case 'q': Length = lnLL; P++; break;
        case 'I':
        case 'z': Length = lnZ; P++; break;
        case 'j': Length = lnJ; P++; break;
        case 't': Length = lnT; P++; break;
        case 'w': Length = lnW; P++; break;
      }
    }

    CharT Conversion = *P;
    if (Conversion == 0)
    {
      break;
    }
    P++;

    std::string Spec = "%" + Flags;
    if (Width >= 0)
    {
      Spec += std::to_string(Width);
    }
    if (Precision >= 0)
    {
      Spec += "." + std::to_string(Precision);
    }
    bool LeftJustify = (Flags.find('-') != std::string::npos);

    switch (Conversion)
    {
      case 'd':
      case 'i':
        {
          long long Value;
          switch (Length)
          {
            case lnLL:
            case lnBigL:
            case lnJ: Value = va_arg(Args, long long); break;
            case lnL:
            case lnZ:
            case lnT: Value = va_arg(Args, long); break;
            case lnH: Value = static_cast<short>(va_arg(Args, int)); break;
            case lnHH: Value = static_cast<signed char>(va_arg(Args, int)); break;
            default: Value = va_arg(Args, int); break;
          }
          AppendAscii(Result, FormatNumber(Spec + "lld", Value));
        }
        break;

      case 'u':
      case 'o':
      case 'x':
      case 'X':
        {
          unsigned long long Value;
          switch (Length)
          {
            case lnLL:
            case lnBigL:
            case lnJ: Value = va_arg(Args, unsigned long long); break;
            case lnL:
            case lnZ:
            case lnT: Value = va_arg(Args, unsigned long); break;
            case lnH: Value = static_cast<unsigned short>(va_arg(Args, unsigned int)); break;
            case lnHH: Value = static_cast<unsigned char>(va_arg(Args, unsigned int)); break;
            default: Value = va_arg(Args, unsigned int); break;
          }
          AppendAscii(Result, FormatNumber(Spec + "ll" + static_cast<char>(Conversion), Value));
        }
        break;

      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
      case 'a':
      case 'A':
        if (Length == lnBigL)
        {
          long double Value = va_arg(Args, long double);
          AppendAscii(Result, FormatNumber(Spec + "L" + static_cast<char>(Conversion), Value));
        }
        else
        {
          double Value = va_arg(Args, double);
          AppendAscii(Result, FormatNumber(Spec + static_cast<char>(Conversion), Value));
        }
        break;

      case 'c':
      case 'C':
        {
          bool NarrowArg =
            Wide ? ((Length == lnH) || (Conversion == 'C')) : ((Length != lnL) && (Length != lnW) && (Conversion != 'C'));
          int Value = va_arg(Args, int);
          std::wstring Str;
          if (NarrowArg)
          {
            char Ch = static_cast<char>(Value);
            Str = DecodeUtf8(&Ch, 1);
          }
          else
          {
            Str.assign(1, static_cast<wchar_t>(Value));
          }
          AppendPadded(Result, Str, Width, LeftJustify);
        }
        break;

      case 's':
      case 'S':
        {
          bool NarrowArg =
            Wide ? ((Length == lnH) || (Conversion == 'S')) : ((Length != lnL) && (Length != lnW) && (Conversion != 'S'));
          std::wstring Str;
          if (NarrowArg)
          {
            const char * Value = va_arg(Args, const char *);
            if (Value == nullptr)
            {
              Value = "(null)";
            }
            size_t Len = strlen(Value);
            if ((Precision >= 0) && (static_cast<size_t>(Precision) < Len))
            {
              Len = Precision;
            }
            Str = DecodeUtf8(Value, Len);
          }
          else
          {
            const wchar_t * Value = va_arg(Args, const wchar_t *);
            if (Value == nullptr)
            {
              Value = L"(null)";
            }
            size_t Len = wcslen(Value);
            if ((Precision >= 0) && (static_cast<size_t>(Precision) < Len))
            {
              Len = Precision;
            }
            Str.assign(Value, Len);
          }
          AppendPadded(Result, Str, Width, LeftJustify);
        }
        break;

      case 'p':
        {
          void * Value = va_arg(Args, void *);
          AppendAscii(Result, FormatNumber(Spec + "p", Value));
        }
        break;

      case 'n':
        {
          long long Count = static_cast<long long>(Result.size());
          switch (Length)
          {
            case lnHH: *va_arg(Args, signed char *) = static_cast<signed char>(Count); break;
            case lnH: *va_arg(Args, short *) = static_cast<short>(Count); break;
            case lnL:
            case lnZ:
            case lnT: *va_arg(Args, long *) = static_cast<long>(Count); break;
            case lnLL:
            case lnJ: *va_arg(Args, long long *) = Count; break;
            default: *va_arg(Args, int *) = static_cast<int>(Count); break;
          }
        }
        break;

      default:
        // Unknown conversion, output verbatim
        for (const CharT * Q = SpecStart; Q < P; Q++)
        {
          Result.push_back(static_cast<wchar_t>(*Q));
        }
        break;
    }
  }
  return Result;
}
} // namespace
//---------------------------------------------------------------------------
std::wstring PrintfV(const wchar_t * Format, va_list Args)
{
  va_list ArgsCopy;
  va_copy(ArgsCopy, Args);
  std::wstring Result = DoPrintf(Format, ArgsCopy, true);
  va_end(ArgsCopy);
  return Result;
}
//---------------------------------------------------------------------------
std::string AnsiFormatV(const char * Format, va_list Args)
{
  va_list ArgsCopy;
  va_copy(ArgsCopy, Args);
  std::wstring Result = DoPrintf(Format, ArgsCopy, false);
  va_end(ArgsCopy);
  return EncodeUtf8(Result.data(), Result.size());
}
//---------------------------------------------------------------------------
} // namespace rtl
