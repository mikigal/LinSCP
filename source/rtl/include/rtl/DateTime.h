// System::TDateTime, as in C++Builder's systdate.h. The value is the number of
// days since 1899-12-30; the fraction is the time of day (for negative values
// the fraction still counts forward from midnight, as in Delphi).
//---------------------------------------------------------------------------
#ifndef RtlDateTimeH
#define RtlDateTimeH
//---------------------------------------------------------------------------
#include <rtl/UnicodeString.h>
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
class TDateTimeBase
{
public:
  double Val;
};
//---------------------------------------------------------------------------
class TDateTime : public TDateTimeBase
{
public:
  // Used by TDateTime(const UnicodeString & Src)
  enum TDateTimeFlag { Date, Time, DateTime };

  static TDateTime CurrentDate();
  static TDateTime CurrentTime();
  static TDateTime CurrentDateTime();
  static TDateTime FileDateToDateTime(int FileDate);

  TDateTime() { Val = 0; }
  TDateTime(const TDateTimeBase & Src) { Val = Src.Val; }
  TDateTime(const TDateTime & Src) { Val = Src.Val; }
  TDateTime(const double Src) { Val = Src; }
  TDateTime(const int Src) { Val = Src; }
  TDateTime(const UnicodeString & Src, TDateTimeFlag Flag = DateTime);
  TDateTime(unsigned short Year, unsigned short Month, unsigned short Day);
  TDateTime(unsigned short Hour, unsigned short Min, unsigned short Sec, unsigned short MSec);
  TDateTime(unsigned short Year, unsigned short Month, unsigned short Day,
    unsigned short Hour, unsigned short Min, unsigned short Sec, unsigned short MSec);

  TDateTime & operator =(const TDateTimeBase & Rhs) { Val = Rhs.Val; return *this; }
  TDateTime & operator =(const TDateTime & Rhs) { Val = Rhs.Val; return *this; }
  TDateTime & operator =(const double Rhs) { Val = Rhs; return *this; }
  TDateTime & operator =(const int Rhs) { Val = Rhs; return *this; }

  TDateTime & operator +=(const TDateTimeBase & Rhs) { Val += Rhs.Val; return *this; }
  TDateTime & operator +=(const TDateTime & Rhs) { Val += Rhs.Val; return *this; }
  TDateTime & operator +=(const double Rhs) { Val += Rhs; return *this; }
  TDateTime & operator +=(const int Rhs) { Val += Rhs; return *this; }
  TDateTime & operator -=(const TDateTimeBase & Rhs) { Val -= Rhs.Val; return *this; }
  TDateTime & operator -=(const TDateTime & Rhs) { Val -= Rhs.Val; return *this; }
  TDateTime & operator -=(const double Rhs) { Val -= Rhs; return *this; }
  TDateTime & operator -=(const int Rhs) { Val -= Rhs; return *this; }
  TDateTime & operator ++() { Val++; return *this; }
  TDateTime operator ++(int) { TDateTime Tmp(*this); Val++; return Tmp; }
  TDateTime & operator --() { Val--; return *this; }
  TDateTime operator --(int) { TDateTime Tmp(*this); Val--; return Tmp; }

  TDateTime operator +(const TDateTimeBase & Rhs) const { return Val + Rhs.Val; }
  TDateTime operator +(const TDateTime & Rhs) const { return Val + Rhs.Val; }
  TDateTime operator +(const double Rhs) const { return Val + Rhs; }
  TDateTime operator +(const int Rhs) const { return Val + Rhs; }
  TDateTime operator -(const TDateTimeBase & Rhs) const { return Val - Rhs.Val; }
  TDateTime operator -(const TDateTime & Rhs) const { return Val - Rhs.Val; }
  TDateTime operator -(const double Rhs) const { return Val - Rhs; }
  TDateTime operator -(const int Rhs) const { return Val - Rhs; }

  bool operator ==(const TDateTime & Rhs) const { return Val == Rhs.Val; }
  bool operator !=(const TDateTime & Rhs) const { return Val != Rhs.Val; }
  bool operator >(const TDateTime & Rhs) const { return Val > Rhs.Val; }
  bool operator <(const TDateTime & Rhs) const { return Val < Rhs.Val; }
  bool operator >=(const TDateTime & Rhs) const { return Val >= Rhs.Val; }
  bool operator <=(const TDateTime & Rhs) const { return Val <= Rhs.Val; }

  operator UnicodeString() const;
  UnicodeString FormatString(const UnicodeString & Format) const;
  UnicodeString DateString() const;
  UnicodeString TimeString() const;
  UnicodeString DateTimeString() const;
  operator double() const { return Val; }
  operator int() const { return static_cast<int>(Val); }

  int DayOfWeek() const;
  int FileDate() const;
  void DecodeDate(unsigned short * Year, unsigned short * Month, unsigned short * Day) const;
  void DecodeTime(unsigned short * Hour, unsigned short * Min, unsigned short * Sec, unsigned short * MSec) const;
};
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
#endif
