// The System.TimeSpan unit: TTimeSpan, a duration in 100 ns ticks.
//---------------------------------------------------------------------------
#ifndef RtlTimeSpanH
#define RtlTimeSpanH
//---------------------------------------------------------------------------
#include <rtl/System.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Timespan
{
//---------------------------------------------------------------------------
struct TTimeSpan
{
public:
  static const __int64 TicksPerMillisecond = 10000;
  static const __int64 TicksPerSecond = 1000 * TicksPerMillisecond;
  static const __int64 TicksPerMinute = 60 * TicksPerSecond;
  static const __int64 TicksPerHour = 60 * TicksPerMinute;
  static const __int64 TicksPerDay = 24 * TicksPerHour;

  static const TTimeSpan Zero;
  static const TTimeSpan MaxValue;
  static const TTimeSpan MinValue;

  TTimeSpan() = default;
  explicit TTimeSpan(__int64 ATicks) : FTicks(ATicks) {}
  TTimeSpan(int Hours, int Minutes, int Seconds);
  TTimeSpan(int Days, int Hours, int Minutes, int Seconds);
  TTimeSpan(int Days, int Hours, int Minutes, int Seconds, int Milliseconds);

  static TTimeSpan FromDays(double Value);
  static TTimeSpan FromHours(double Value);
  static TTimeSpan FromMinutes(double Value);
  static TTimeSpan FromSeconds(double Value);
  static TTimeSpan FromMilliseconds(double Value);
  static TTimeSpan FromTicks(__int64 Value) { return TTimeSpan(Value); }
  static TTimeSpan Subtract(const TDateTime D1, const TDateTime D2);

  __int64 GetTicks() const { return FTicks; }
  int GetDays() const { return static_cast<int>(FTicks / TicksPerDay); }
  int GetHours() const { return static_cast<int>((FTicks / TicksPerHour) % 24); }
  int GetMinutes() const { return static_cast<int>((FTicks / TicksPerMinute) % 60); }
  int GetSeconds() const { return static_cast<int>((FTicks / TicksPerSecond) % 60); }
  int GetMilliseconds() const { return static_cast<int>((FTicks / TicksPerMillisecond) % 1000); }
  double GetTotalDays() const { return static_cast<double>(FTicks) / TicksPerDay; }
  double GetTotalHours() const { return static_cast<double>(FTicks) / TicksPerHour; }
  double GetTotalMinutes() const { return static_cast<double>(FTicks) / TicksPerMinute; }
  double GetTotalSeconds() const { return static_cast<double>(FTicks) / TicksPerSecond; }
  double GetTotalMilliseconds() const { return static_cast<double>(FTicks) / TicksPerMillisecond; }

  RTL_PROPERTY_GET(Ticks, GetTicks)
  __declspec(property(get = __property_get_Ticks)) __int64 Ticks;
  RTL_PROPERTY_GET(Days, GetDays)
  __declspec(property(get = __property_get_Days)) int Days;
  RTL_PROPERTY_GET(Hours, GetHours)
  __declspec(property(get = __property_get_Hours)) int Hours;
  RTL_PROPERTY_GET(Minutes, GetMinutes)
  __declspec(property(get = __property_get_Minutes)) int Minutes;
  RTL_PROPERTY_GET(Seconds, GetSeconds)
  __declspec(property(get = __property_get_Seconds)) int Seconds;
  RTL_PROPERTY_GET(Milliseconds, GetMilliseconds)
  __declspec(property(get = __property_get_Milliseconds)) int Milliseconds;
  RTL_PROPERTY_GET(TotalDays, GetTotalDays)
  __declspec(property(get = __property_get_TotalDays)) double TotalDays;
  RTL_PROPERTY_GET(TotalHours, GetTotalHours)
  __declspec(property(get = __property_get_TotalHours)) double TotalHours;
  RTL_PROPERTY_GET(TotalMinutes, GetTotalMinutes)
  __declspec(property(get = __property_get_TotalMinutes)) double TotalMinutes;
  RTL_PROPERTY_GET(TotalSeconds, GetTotalSeconds)
  __declspec(property(get = __property_get_TotalSeconds)) double TotalSeconds;
  RTL_PROPERTY_GET(TotalMilliseconds, GetTotalMilliseconds)
  __declspec(property(get = __property_get_TotalMilliseconds)) double TotalMilliseconds;

  TTimeSpan Add(const TTimeSpan & TS) const { return TTimeSpan(FTicks + TS.FTicks); }
  TTimeSpan Duration() const { return TTimeSpan((FTicks < 0) ? -FTicks : FTicks); }
  TTimeSpan Negate() const { return TTimeSpan(-FTicks); }
  UnicodeString ToString() const;

  TTimeSpan operator +(const TTimeSpan & Rhs) const { return Add(Rhs); }
  TTimeSpan operator -(const TTimeSpan & Rhs) const { return TTimeSpan(FTicks - Rhs.FTicks); }
  TTimeSpan operator -() const { return Negate(); }
  bool operator ==(const TTimeSpan & Rhs) const { return FTicks == Rhs.FTicks; }
  bool operator !=(const TTimeSpan & Rhs) const { return FTicks != Rhs.FTicks; }
  bool operator <(const TTimeSpan & Rhs) const { return FTicks < Rhs.FTicks; }
  bool operator <=(const TTimeSpan & Rhs) const { return FTicks <= Rhs.FTicks; }
  bool operator >(const TTimeSpan & Rhs) const { return FTicks > Rhs.FTicks; }
  bool operator >=(const TTimeSpan & Rhs) const { return FTicks >= Rhs.FTicks; }

private:
  __int64 FTicks = 0;
};
//---------------------------------------------------------------------------
} // namespace Timespan
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Timespan;
//---------------------------------------------------------------------------
#endif
