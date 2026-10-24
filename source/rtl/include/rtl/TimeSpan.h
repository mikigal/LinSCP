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

  __declspec(property(get = GetTicks)) __int64 Ticks;
  __declspec(property(get = GetDays)) int Days;
  __declspec(property(get = GetHours)) int Hours;
  __declspec(property(get = GetMinutes)) int Minutes;
  __declspec(property(get = GetSeconds)) int Seconds;
  __declspec(property(get = GetMilliseconds)) int Milliseconds;
  __declspec(property(get = GetTotalDays)) double TotalDays;
  __declspec(property(get = GetTotalHours)) double TotalHours;
  __declspec(property(get = GetTotalMinutes)) double TotalMinutes;
  __declspec(property(get = GetTotalSeconds)) double TotalSeconds;
  __declspec(property(get = GetTotalMilliseconds)) double TotalMilliseconds;

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
