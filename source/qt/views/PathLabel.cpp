// The path above the files of a panel.
//---------------------------------------------------------------------------
#include "PathLabel.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// upstream's margins of the path labels of the panels
const int IndentHorizontal = 5;
const int IndentVertical = 3;
// upstream's HotTrackMask: the mask under the mouse
const QString HotTrackMask(QChar(1));
//---------------------------------------------------------------------------
QColor Mix(const QColor & A, const QColor & B, qreal Ratio)
{
  return QColor::fromRgbF(
    A.redF() * Ratio + B.redF() * (1 - Ratio), A.greenF() * Ratio + B.greenF() * (1 - Ratio),
    A.blueF() * Ratio + B.blueF() * (1 - Ratio));
}
//---------------------------------------------------------------------------
// The string cut with "..." to the width (upstream's MinimizeStr)
QString MinimizeStr(QString Buf, const QFontMetrics & Metrics, int Width)
{
  if (Metrics.horizontalAdvance(Buf) > Width)
  {
    Buf += QStringLiteral("...");
    do
    {
      Buf.remove(Buf.length() - 4, 1);
    }
    while ((Metrics.horizontalAdvance(Buf) > Width) && (Buf.length() > 4));
  }
  return Buf;
}
//---------------------------------------------------------------------------
// The path with its leading directories replaced by "..." to fit the width
// (the VCL's MinimizeName for the paths of Unix)
QString MinimizeName(const QString & Path, const QFontMetrics & Metrics, int Width)
{
  if (Metrics.horizontalAdvance(Path) <= Width)
  {
    return Path;
  }
  QStringList Parts = Path.split(QLatin1Char('/'));
  // The first part is the empty one before the root
  for (int Index = 2; Index < Parts.size(); Index++)
  {
    QString Result = Parts[0] + QStringLiteral("/.../") + Parts.mid(Index).join(QLatin1Char('/'));
    if (Metrics.horizontalAdvance(Result) <= Width)
    {
      return Result;
    }
  }
  return QStringLiteral(".../") + Parts.last();
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TPathLabel::TPathLabel(QWidget * Parent) :
  QLabel(Parent)
{
  FActive = false;
  FMouseInView = false;
  FMouseX = 0;
  FVirtualMaskHidden = false;
  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  setTextFormat(Qt::PlainText);
}
//---------------------------------------------------------------------------
void TPathLabel::SetPath(const QString & Path)
{
  if (text() != Path)
  {
    setText(Path);
    update();
  }
}
//---------------------------------------------------------------------------
void TPathLabel::SetMask(const QString & Mask)
{
  if (FMask != Mask)
  {
    FMask = Mask;
    update();
  }
}
//---------------------------------------------------------------------------
void TPathLabel::SetActive(bool Active)
{
  if (FActive != Active)
  {
    FActive = Active;
    update();
  }
}
//---------------------------------------------------------------------------
QSize TPathLabel::sizeHint() const
{
  return QSize(fontMetrics().horizontalAdvance(text()) + 2 * IndentHorizontal, fontMetrics().height() + 2 * IndentVertical);
}
//---------------------------------------------------------------------------
QSize TPathLabel::minimumSizeHint() const
{
  return QSize(0, fontMetrics().height() + 2 * IndentVertical);
}
//---------------------------------------------------------------------------
QString TPathLabel::GetDisplayPath() const
{
  const_cast<TPathLabel *>(this)->LayoutText(width() - 2 * IndentHorizontal);
  return FDisplayPath;
}
//---------------------------------------------------------------------------
void TPathLabel::LayoutText(int Width)
{
  // upstream's DoDrawTextIntern
  QFontMetrics Metrics = fontMetrics();
  QString S = text();

  // Without a mask, the mask under the mouse ("*.*") changes the filter
  FVirtualMask = FMask;
  FVirtualMaskHidden = false;
  if (FVirtualMask.isEmpty())
  {
    FVirtualMask = QStringLiteral("*.*");
    FVirtualMaskHidden = !FMouseInView;
  }

  FDisplayPath = S;
  if (!FDisplayPath.isEmpty() && !FDisplayPath.endsWith(QLatin1Char('/')))
  {
    FDisplayPath += QLatin1Char('/');
  }
  int WidthMask = Metrics.horizontalAdvance(FVirtualMask);
  int WidthPath = (WidthMask > Width / 3) ? (Width - (Width / 3)) : (Width - WidthMask);

  QString Buf = FDisplayPath;
  int WidthRemain = WidthPath;
  bool TrailingSeparator = (Buf.length() > 1) && Buf.endsWith(QLatin1Char('/'));
  if (TrailingSeparator)
  {
    WidthRemain -= Metrics.horizontalAdvance(QLatin1Char('/'));
    Buf.chop(1);
  }
  QString Buf2 = Buf.mid(Buf.lastIndexOf(QLatin1Char('/')) + 1);
  FShortenedDisplayPath = QStringLiteral(".../");
  if ((Buf.length() > 1) && (WidthRemain < Metrics.horizontalAdvance(FShortenedDisplayPath + Buf2)))
  {
    // Not even the last directory fits
    WidthRemain -= Metrics.horizontalAdvance(FShortenedDisplayPath);
    Buf2 = MinimizeStr(Buf2, Metrics, WidthRemain);
    FShortenedPath = Buf.left(Buf.lastIndexOf(QLatin1Char('/')) + 1);
    Buf = FShortenedDisplayPath + Buf2;
  }
  else
  {
    Buf2 = MinimizeName(Buf, Metrics, WidthRemain);
    if (Buf2 == Buf)
    {
      FShortenedPath.clear();
      FShortenedDisplayPath.clear();
    }
    else
    {
      // The beginning of the path that the "..." replaced: before the
      // separator I, from which the shown path is the original one
      int I = Buf.length() - 1;
      while (true)
      {
        int I2 = Buf.left(I).lastIndexOf(QLatin1Char('/'));
        if (I2 < 0)
        {
          break;
        }
        int L = Buf.length() - I2;
        if (Buf.mid(I2, L) == Buf2.right(L))
        {
          I = I2;
        }
        else
        {
          break;
        }
      }
      FShortenedPath = Buf.left(I + 1);
      FShortenedDisplayPath = Buf2.left(Buf2.length() - (Buf.length() - I) + 1);
      Buf = Buf2;
    }
  }
  if (TrailingSeparator)
  {
    Buf += QLatin1Char('/');
  }
  FDisplayPath = Buf;

  int DisplayWidth = Metrics.horizontalAdvance(FDisplayPath);
  FDisplayMask = MinimizeStr(FVirtualMask, Metrics, std::max(Width - DisplayWidth, 0));

  bool Shortened = (FDisplayPath != S) && (FDisplayPath != S + QLatin1Char('/'));
  bool MaskShortened = !FMask.isEmpty() && (FDisplayMask != FMask);
  setToolTip((Shortened || MaskShortened) ? (S + FMask) : QString());
}
//---------------------------------------------------------------------------
QString TPathLabel::HotTrackPath() const
{
  // upstream's HotTrackPath: the beginning of the displayed path up to the
  // directory under the mouse, or the mask
  QString Result;
  if (FMouseInView && isEnabled() && !FDisplayPath.isEmpty())
  {
    QFontMetrics Metrics = fontMetrics();
    int Len = FMouseX - IndentHorizontal;
    if (Len >= 0)
    {
      if (Len < Metrics.horizontalAdvance(FDisplayPath))
      {
        QString Path = FDisplayPath;
        do
        {
          int DelimPos = Path.indexOf(QLatin1Char('/'));
          if (DelimPos >= 0)
          {
            Result += Path.left(DelimPos + 1);
            Path.remove(0, DelimPos + 1);
          }
          else
          {
            Result += Path;
            Path.clear();
          }
        }
        while ((Metrics.horizontalAdvance(Result) < Len) && !Path.isEmpty());
      }
      else if (Len < Metrics.horizontalAdvance(FDisplayPath + FDisplayMask))
      {
        Result = HotTrackMask;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
QColor TPathLabel::BackgroundColor() const
{
  // The active panel's in the color of the selection (upstream's
  // clActiveCaption), the other one's of the window (clInactiveCaption)
  const QPalette & Palette = palette();
  return
    FActive ?
      Mix(Palette.color(QPalette::Active, QPalette::Highlight), Palette.color(QPalette::Active, QPalette::Window), 0.45) :
      Palette.color(QPalette::Active, QPalette::Window).darker(106);
}
//---------------------------------------------------------------------------
QColor TPathLabel::TextColor() const
{
  const QPalette & Palette = palette();
  if (!isEnabled())
  {
    return Palette.color(QPalette::Disabled, QPalette::WindowText);
  }
  return
    FActive ? Palette.color(QPalette::Active, QPalette::WindowText) :
      Mix(Palette.color(QPalette::Active, QPalette::WindowText), BackgroundColor(), 0.7);
}
//---------------------------------------------------------------------------
QColor TPathLabel::HotTrackColor() const
{
  // upstream's CalculateAutoHotTrackColor: the text lighter (on a light
  // background) or darker
  QColor Text = TextColor();
  bool Bright = (Text.red() + Text.green() + Text.blue()) > (256 / 2 * 3);
  auto Component = [Bright](int C)
  {
    int Delta = std::max(qRound(C * 0.3), 80);
    return Bright ? std::max(C - Delta, 0) : std::min(C + Delta, 255);
  };
  return QColor(Component(Text.red()), Component(Text.green()), Component(Text.blue()));
}
//---------------------------------------------------------------------------
void TPathLabel::paintEvent(QPaintEvent * /*Event*/)
{
  QPainter Painter(this);
  Painter.fillRect(rect(), BackgroundColor());
  QRect TextRect = rect().adjusted(IndentHorizontal, IndentVertical, -IndentHorizontal, -IndentVertical);
  LayoutText(TextRect.width());
  QFontMetrics Metrics = fontMetrics();
  int Baseline = TextRect.top() + (TextRect.height() - Metrics.height()) / 2 + Metrics.ascent();
  int X = TextRect.left();
  QString HotTrack = HotTrackPath();
  QColor Normal = TextColor();
  QColor Hot = HotTrackColor();

  QString Path = FDisplayPath;
  if (!HotTrack.isEmpty() && (HotTrack != HotTrackMask))
  {
    Painter.setPen(Hot);
    Painter.drawText(X, Baseline, HotTrack);
    X += Metrics.horizontalAdvance(HotTrack);
    Path.remove(0, HotTrack.length());
  }
  Painter.setPen(Normal);
  Painter.drawText(X, Baseline, Path);
  X += Metrics.horizontalAdvance(Path);
  if (HotTrack == HotTrackMask)
  {
    Painter.setPen(Hot);
    Painter.drawText(X, Baseline, FDisplayMask);
  }
  else if (!FVirtualMaskHidden)
  {
    Painter.drawText(X, Baseline, FDisplayMask);
  }
}
//---------------------------------------------------------------------------
void TPathLabel::mouseMoveEvent(QMouseEvent * Event)
{
  QString Before = HotTrackPath();
  FMouseX = Event->position().toPoint().x();
  if (HotTrackPath() != Before)
  {
    update();
  }
  setCursor(HotTrackPath().isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
  QLabel::mouseMoveEvent(Event);
}
//---------------------------------------------------------------------------
void TPathLabel::mouseReleaseEvent(QMouseEvent * Event)
{
  if ((Event->button() == Qt::LeftButton) && isEnabled())
  {
    // upstream's Click
    FMouseX = Event->position().toPoint().x();
    QString HotPath = HotTrackPath();
    if (HotPath == HotTrackMask)
    {
      Q_EMIT MaskClicked();
    }
    else if (!HotPath.isEmpty())
    {
      if (!FShortenedPath.isEmpty())
      {
        // The whole shown path is the original one
        if (HotPath == FDisplayPath)
        {
          HotPath = text();
        }
        else if (HotPath.length() >= FShortenedDisplayPath.length())
        {
          HotPath = FShortenedPath + HotPath.mid(FShortenedDisplayPath.length());
        }
      }
      if (HotPath.endsWith(QLatin1Char('/')) && (HotPath != QStringLiteral("/")))
      {
        HotPath.chop(1);
      }
      Q_EMIT PathClicked(HotPath);
    }
  }
  QLabel::mouseReleaseEvent(Event);
}
//---------------------------------------------------------------------------
void TPathLabel::mouseDoubleClickEvent(QMouseEvent * Event)
{
  if ((Event->button() == Qt::LeftButton) && isEnabled())
  {
    Q_EMIT DoubleClicked();
  }
  QLabel::mouseDoubleClickEvent(Event);
}
//---------------------------------------------------------------------------
void TPathLabel::enterEvent(QEnterEvent * Event)
{
  FMouseInView = true;
  FMouseX = Event->position().toPoint().x();
  update();
  QLabel::enterEvent(Event);
}
//---------------------------------------------------------------------------
void TPathLabel::leaveEvent(QEvent * Event)
{
  FMouseInView = false;
  update();
  QLabel::leaveEvent(Event);
}
//---------------------------------------------------------------------------
