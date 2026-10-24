//---------------------------------------------------------------------------
#include <rtl/Menus.h>
//---------------------------------------------------------------------------
namespace Vcl
{
namespace Menus
{
//---------------------------------------------------------------------------
System::UnicodeString StripHotkey(const System::UnicodeString Text)
{
  System::UnicodeString Result = Text;
  int Index = 1;
  while (Index <= Result.Length())
  {
    if (Result[Index] == L'&')
    {
      // The character after the marker stays, even if it is another '&'
      Result.Delete(Index, 1);
    }
    Index++;
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace Menus
} // namespace Vcl
//---------------------------------------------------------------------------
