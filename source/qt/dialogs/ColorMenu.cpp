// The menu of the colors (upstream's CreateColorMenu of
// windows/UserInterface.cpp): the default color, the custom colors picked
// before, the standard palette and the color dialog.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <TextsWin.h>

#include <QApplication>
#include <QColorDialog>
#include <QGridLayout>
#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
const int ColorCols = 8;
const int UserColorRows = 1;
const int UserColorCount = UserColorRows * ColorCols;
const wchar_t ColorSeparator = L',';
//---------------------------------------------------------------------------
struct TNamedColor
{
  const char * Name;
  int Color;
};
//---------------------------------------------------------------------------
// The light colors of the sessions: the bottom row of TBX's default color
// set and the second row of the palette of Excel 2010 (with its second
// color skipped)
const TNamedColor SessionPalette[] =
{
  { "Rose", 0xCC99FF }, { "Tan", 0x99CCFF }, { "Light Yellow", 0x99FFFF }, { "Light Green", 0xCCFFCC },
  { "Light Turquoise", 0xFFFFCC }, { "Pale Blue", 0xFFCC99 }, { "Lavender", 0xFF99CC }, { "Light Orange", 0xB5D5FB },
  { "Darker White", 0xD8D8D8 }, { "Darker Tan", 0x97BDC4 }, { "Lighter Blue", 0xE2B38D }, { "Light Blue", 0xE4CCB8 },
  { "Lighter Red", 0xB7B9E5 }, { "Light Olive Green", 0xBCE3D7 }, { "Light Purple", 0xD9C1CC }, { "Light Aqua", 0xE8DDB7 },
};
//---------------------------------------------------------------------------
// TBX's default color set (packages/tbx/TBXToolPals.pas), for the fonts
const TNamedColor FontPalette[] =
{
  { "Black", 0x000000 }, { "Brown", 0x003399 }, { "Olive Green", 0x003333 }, { "Dark Green", 0x003300 },
  { "Dark Teal", 0x663300 }, { "Dark Blue", 0x800000 }, { "Indigo", 0x993333 }, { "Gray-80%", 0x333333 },
  { "Dark Red", 0x000080 }, { "Orange", 0x0066FF }, { "Dark Yellow", 0x008080 }, { "Green", 0x008000 },
  { "Teal", 0x808000 }, { "Blue", 0xFF0000 }, { "Blue-Gray", 0x996666 }, { "Gray-50%", 0x808080 },
  { "Red", 0x0000FF }, { "Light Orange", 0x0099FF }, { "Lime", 0x00CC99 }, { "Sea Green", 0x669933 },
  { "Aqua", 0xCCCC33 }, { "Light Blue", 0xFF6633 }, { "Violet", 0x800080 }, { "Gray-40%", 0x969696 },
  { "Pink", 0xFF00FF }, { "Gold", 0x00CCFF }, { "Yellow", 0x00FFFF }, { "Bright Green", 0x00FF00 },
  { "Turquoise", 0xFFFF00 }, { "Sky Blue", 0xFFCC00 }, { "Plum", 0x663399 }, { "Gray-25%", 0xC0C0C0 },
  { "Rose", 0xCC99FF }, { "Tan", 0x99CCFF }, { "Light Yellow", 0x99FFFF }, { "Light Green", 0xCCFFCC },
  { "Light Turquoise", 0xFFFFCC }, { "Pale Blue", 0xFFCC99 }, { "Lavender", 0xFF99CC }, { "White", 0xFFFFFF },
};
//---------------------------------------------------------------------------
TColor GetNonZeroColor(TColor Color)
{
  // 0,0,0 is "default color", near-black is used instead
  return (Color == TColor(0)) ? TColor(0x010101) : Color;
}
//---------------------------------------------------------------------------
std::vector<TNamedColor> StandardColors(bool SessionColors)
{
  return SessionColors ?
    std::vector<TNamedColor>(std::begin(SessionPalette), std::end(SessionPalette)) :
    std::vector<TNamedColor>(std::begin(FontPalette), std::end(FontPalette));
}
//---------------------------------------------------------------------------
bool IsStandardColor(bool SessionColors, TColor Color)
{
  for (const TNamedColor & StandardColor : StandardColors(SessionColors))
  {
    if (GetNonZeroColor(TColor(StandardColor.Color)) == Color)
    {
      return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
UnicodeString LoadCustomColors(bool SessionColors)
{
  return SessionColors ? AppConfiguration->SessionColors : AppConfiguration->FontColors;
}
//---------------------------------------------------------------------------
void SaveCustomColors(bool SessionColors, const UnicodeString & Colors)
{
  if (SessionColors)
  {
    AppConfiguration->SessionColors = Colors;
  }
  else
  {
    AppConfiguration->FontColors = Colors;
  }
}
//---------------------------------------------------------------------------
std::vector<TColor> CustomColors(bool SessionColors)
{
  std::vector<TColor> Result;
  UnicodeString Temp = LoadCustomColors(SessionColors);
  while (!Temp.IsEmpty())
  {
    Result.push_back(RestoreColor(CutToChar(Temp, ColorSeparator, true)));
  }
  return Result;
}
//---------------------------------------------------------------------------
// A color other than the standard ones is the first of the custom colors
void ColorChange(bool SessionColors, TColor Color)
{
  if ((Color != TColor(0)) && !IsStandardColor(SessionColors, Color))
  {
    UnicodeString Colors = StoreColor(Color);
    for (TColor CustomColor : CustomColors(SessionColors))
    {
      if (CustomColor != Color)
      {
        Colors += UnicodeString(ColorSeparator) + StoreColor(CustomColor);
      }
    }
    SaveCustomColors(SessionColors, Colors);
  }
}
//---------------------------------------------------------------------------
QIcon ColorIcon(TColor Color, int Size)
{
  QPixmap Pixmap(Size, Size);
  Pixmap.fill(TColorToQColor(Color));
  return QIcon(Pixmap);
}
//---------------------------------------------------------------------------
// The cells of the colors (upstream's TTBXColorPalette), the selected one
// checked; picking a color closes the menu
void AddColorPalette(QMenu * Menu, const QString & Name, const std::vector<TNamedColor> & Colors,
  TColor Selected, TColor & Picked)
{
  QWidgetAction * Action = new QWidgetAction(Menu);
  QWidget * Palette = new QWidget(Menu);
  Palette->setObjectName(Name);
  QGridLayout * Grid = new QGridLayout(Palette);
  Grid->setSpacing(2);
  Grid->setContentsMargins(4, 2, 4, 2);
  int Size = Menu->fontMetrics().height();
  for (size_t Index = 0; Index < Colors.size(); Index++)
  {
    TColor Color = TColor(Colors[Index].Color);
    QToolButton * Cell = new QToolButton(Palette);
    QString ColorName = QString::fromLatin1(Colors[Index].Name);
    Cell->setObjectName(QStringLiteral("Color%1").arg(ToQString(StoreColor(Color))));
    Cell->setToolTip(ColorName);
    Cell->setAccessibleName(ColorName.isEmpty() ? ToQString(StoreColor(Color)) : ColorName);
    Cell->setIcon(ColorIcon(Color, Size));
    Cell->setIconSize(QSize(Size, Size));
    Cell->setAutoRaise(true);
    Cell->setCheckable(true);
    Cell->setChecked((Selected != TColor(0)) && (GetNonZeroColor(Color) == Selected));
    QObject::connect(Cell, &QToolButton::clicked, Menu, [Menu, &Picked, Color]()
    {
      Picked = GetNonZeroColor(Color);
      Menu->close();
    });
    Grid->addWidget(Cell, static_cast<int>(Index) / ColorCols, static_cast<int>(Index) % ColorCols);
  }
  Action->setDefaultWidget(Palette);
  Menu->addAction(Action);
  Menu->addSeparator();
}
//---------------------------------------------------------------------------
// The color dialog with the custom colors (upstream's ColorPickClick)
bool ColorPick(TColor & Color, bool SessionColors)
{
  // The custom colors of the dialog are the user's ones, the light
  // standard ones for the sessions
  std::vector<TColor> Customs = CustomColors(SessionColors);
  int StandardIndex = 0;
  for (int Index = 0; Index < QColorDialog::customCount(); Index++)
  {
    QColor CustomColor = Qt::white;
    if (Index < static_cast<int>(Customs.size()))
    {
      CustomColor = TColorToQColor(Customs[Index]);
    }
    else if (SessionColors && (StandardIndex < static_cast<int>(std::size(SessionPalette))))
    {
      CustomColor = TColorToQColor(TColor(SessionPalette[StandardIndex].Color));
      StandardIndex++;
    }
    QColorDialog::setCustomColor(Index, CustomColor);
  }

  QColor Initial = TColorToQColor((Color != TColor(0)) ? Color : TColor(0xEBCE87)); // clSkyBlue
  QColorDialog Dialog(Initial, QApplication::activeWindow());
  Dialog.setObjectName(QStringLiteral("ColorDialog"));
  bool Result = (Dialog.exec() == QDialog::Accepted);
  if (Result)
  {
    UnicodeString Colors;
    for (int Index = 0; Index < QColorDialog::customCount(); Index++)
    {
      TColor CustomColor = QColorToTColor(QColorDialog::customColor(Index));
      // White is an empty cell of the dialog
      if ((CustomColor != QColorToTColor(Qt::white)) && !IsStandardColor(SessionColors, CustomColor))
      {
        AddToList(Colors, StoreColor(CustomColor), ColorSeparator);
      }
    }
    SaveCustomColors(SessionColors, Colors);
    Color = GetNonZeroColor(QColorToTColor(Dialog.selectedColor()));
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool ColorMenuPopup(const QPoint & Pos, TColor & Color, bool SessionColors)
{
  QMenu Menu(QApplication::activeWindow());
  Menu.setObjectName(QStringLiteral("ColorMenu"));
  QAction * DefaultItem = Menu.addAction(ToQString(LoadStr(COLOR_TRUE_DEFAULT_CAPTION)));
  DefaultItem->setObjectName(QStringLiteral("ColorDefaultItem"));
  DefaultItem->setCheckable(true);
  DefaultItem->setChecked(Color == TColor(0));
  Menu.addSeparator();

  TColor Picked = TColor(-1);
  std::vector<TColor> Customs = CustomColors(SessionColors);
  if (!Customs.empty())
  {
    std::vector<TNamedColor> UserColors;
    for (size_t Index = 0; (Index < Customs.size()) && (Index < static_cast<size_t>(UserColorCount)); Index++)
    {
      UserColors.push_back({ "", Customs[Index] });
    }
    AddColorPalette(&Menu, QStringLiteral("ColorUserPalette"), UserColors, Color, Picked);
  }
  AddColorPalette(&Menu, QStringLiteral("ColorPalette"), StandardColors(SessionColors), Color, Picked);

  QAction * PickItem = Menu.addAction(ToQString(LoadStr(COLOR_PICK_CAPTION)));
  PickItem->setObjectName(QStringLiteral("ColorPickItem"));

  QAction * Selected = Menu.exec(Pos);
  bool Result = false;
  TColor NewColor = Color;
  if (Selected == DefaultItem)
  {
    NewColor = TColor(0);
    Result = true;
  }
  else if (Selected == PickItem)
  {
    Result = ColorPick(NewColor, SessionColors);
  }
  else if (Picked != TColor(-1))
  {
    NewColor = Picked;
    Result = true;
  }

  if (Result)
  {
    ColorChange(SessionColors, NewColor);
    Color = NewColor;
  }
  return Result;
}
//---------------------------------------------------------------------------
