// The toolbars as upstream's.
//---------------------------------------------------------------------------
#include "Toolbars.h"

#include <Common.h>

#include <QAction>
#include <QBoxLayout>
#include <QCursor>
#include <QApplication>
#include <QMenu>
#include <QPointer>
#include <QProxyStyle>
#include <QToolButton>
#include <QWidgetAction>

#include <algorithm>
#include <memory>
//---------------------------------------------------------------------------
namespace
{
// The desktop's style with the narrower drop down arrows and separators of
// upstream's toolbars
class TToolbarStyle : public QProxyStyle
{
public:
  virtual int pixelMetric(PixelMetric Metric, const QStyleOption * Option = nullptr,
    const QWidget * Widget = nullptr) const override
  {
    int Result = QProxyStyle::pixelMetric(Metric, Option, Widget);
    if (Metric == PM_MenuButtonIndicator)
    {
      Result = std::min(Result, 14);
    }
    else if (Metric == PM_ToolBarSeparatorExtent)
    {
      Result = std::min(Result, 6);
    }
    return Result;
  }
};
//---------------------------------------------------------------------------
QStyle * ToolbarStyle()
{
  static QPointer<QStyle> Style;
  if (Style.isNull())
  {
    Style = new TToolbarStyle();
    Style->setParent(qApp);
  }
  return Style;
}
}
//---------------------------------------------------------------------------
TToolbar::TToolbar(const QString & Name, const QString & Caption, QWidget * Parent) :
  QToolBar(Caption, Parent)
{
  setObjectName(Name);
  setStyle(ToolbarStyle());
  FSelectiveText = true;
  FBandAction = nullptr;
  // The style of each button is its own (explicit, the main window does
  // not reset it)
  setToolButtonStyle(Qt::ToolButtonIconOnly);
  SetLargerToolbar(1);
}
//---------------------------------------------------------------------------
QString TToolbar::GetKey() const
{
  return ToQString(RemoveSuffix(ToUnicodeString(objectName()), L"Toolbar", true));
}
//---------------------------------------------------------------------------
void TToolbar::AddEntry(const TEntry & Entry)
{
  FEntries.push_back(Entry);
  addAction(Entry.Action);
  SetupButton(Entry);
}
//---------------------------------------------------------------------------
void TToolbar::AddButton(QAction * Action, bool ImageAndText)
{
  TEntry Entry;
  Entry.Name = ToQString(RemoveSuffix(ToUnicodeString(Action->objectName()), L"Action", true));
  Entry.Action = Action;
  Entry.Menu = nullptr;
  Entry.MenuMode = tmmSplit;
  Entry.ImageAndText = ImageAndText;
  Entry.Visible = true;
  AddEntry(Entry);
}
//---------------------------------------------------------------------------
void TToolbar::AddMenuButton(QAction * Action, QMenu * Menu, TToolbarMenuMode Mode, bool ImageAndText)
{
  TEntry Entry;
  Entry.Name = ToQString(RemoveSuffix(ToUnicodeString(Action->objectName()), L"Action", true));
  Entry.Action = Action;
  Entry.Menu = Menu;
  Entry.MenuMode = Mode;
  Entry.ImageAndText = ImageAndText;
  Entry.Visible = true;
  AddEntry(Entry);
}
//---------------------------------------------------------------------------
void TToolbar::AddSubmenu(const QString & Name, QMenu * Menu, bool ImageAndText)
{
  // An action of the toolbar's own; the button pops up the menu (the menu
  // stays the menu of its own action, which other menus have)
  QAction * Action = new QAction(Menu->icon(), Menu->title(), this);
  Action->setObjectName(Name);
  Action->setToolTip(Menu->toolTip());
  // From the menu of the hidden buttons of a narrow toolbar
  connect(Action, &QAction::triggered, Menu, [Menu]() { Menu->exec(QCursor::pos()); });
  TEntry Entry;
  Entry.Name =
    ToQString(RemoveSuffix(RemoveSuffix(ToUnicodeString(Name), L"SubmenuItem", true), L"Item", true));
  Entry.Action = Action;
  Entry.Menu = Menu;
  Entry.MenuMode = tmmWhole;
  Entry.ImageAndText = ImageAndText;
  Entry.Visible = true;
  AddEntry(Entry);
}
//---------------------------------------------------------------------------
void TToolbar::AddControl(QWidget * Widget)
{
  QWidgetAction * Action = new QWidgetAction(this);
  Action->setDefaultWidget(Widget);
  TEntry Entry;
  Entry.Action = Action;
  Entry.Menu = nullptr;
  Entry.MenuMode = tmmSplit;
  Entry.ImageAndText = false;
  Entry.Visible = true;
  AddEntry(Entry);
}
//---------------------------------------------------------------------------
void TToolbar::AddSeparatorItem()
{
  QAction * Action = new QAction(this);
  Action->setSeparator(true);
  TEntry Entry;
  Entry.Action = Action;
  Entry.Menu = nullptr;
  Entry.MenuMode = tmmSplit;
  Entry.ImageAndText = false;
  Entry.Visible = true;
  AddEntry(Entry);
}
//---------------------------------------------------------------------------
void TToolbar::InsertEntry(QAction * Before, const TEntry & Entry)
{
  auto I = std::find_if(FEntries.begin(), FEntries.end(), [Before](const TEntry & E) { return (E.Action == Before); });
  FEntries.insert(I, Entry);
  Rebuild();
}
//---------------------------------------------------------------------------
void TToolbar::InsertButton(QAction * Before, QAction * Action, bool ImageAndText)
{
  TEntry Entry;
  Entry.Name = ToQString(RemoveSuffix(ToUnicodeString(Action->objectName()), L"Action", true));
  Entry.Action = Action;
  Entry.Menu = nullptr;
  Entry.MenuMode = tmmSplit;
  Entry.ImageAndText = ImageAndText;
  Entry.Visible = true;
  InsertEntry(Before, Entry);
}
//---------------------------------------------------------------------------
void TToolbar::InsertSeparatorItem(QAction * Before)
{
  QAction * Action = new QAction(this);
  Action->setSeparator(true);
  TEntry Entry;
  Entry.Action = Action;
  Entry.Menu = nullptr;
  Entry.MenuMode = tmmSplit;
  Entry.ImageAndText = false;
  Entry.Visible = true;
  InsertEntry(Before, Entry);
}
//---------------------------------------------------------------------------
void TToolbar::ClearItems()
{
  clear();
  FEntries.clear();
}
//---------------------------------------------------------------------------
QToolButton * TToolbar::GetButton(QAction * Action) const
{
  return qobject_cast<QToolButton *>(widgetForAction(Action));
}
//---------------------------------------------------------------------------
void TToolbar::SetupButton(const TEntry & Entry)
{
  QToolButton * Button = GetButton(Entry.Action);
  if (Button != nullptr)
  {
    // The tests find the buttons by the names of their actions in the
    // toolbar
    Button->setObjectName(Entry.Action->objectName());
    Button->setStyle(ToolbarStyle());
    Button->setToolButtonStyle(
      (Entry.ImageAndText && FSelectiveText) ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
    if (Entry.Menu != nullptr)
    {
      Button->setMenu(Entry.Menu);
      Button->setPopupMode((Entry.MenuMode == tmmSplit) ? QToolButton::MenuButtonPopup : QToolButton::InstantPopup);
    }
  }
}
//---------------------------------------------------------------------------
void TToolbar::Rebuild()
{
  clear();
  // The separators between the shown buttons only
  bool AnyBefore = false;
  QAction * PendingSeparator = nullptr;
  for (const TEntry & Entry : FEntries)
  {
    if (Entry.Action->isSeparator())
    {
      if (AnyBefore)
      {
        PendingSeparator = Entry.Action;
      }
    }
    else if (Entry.Visible)
    {
      if (PendingSeparator != nullptr)
      {
        addAction(PendingSeparator);
        PendingSeparator = nullptr;
      }
      addAction(Entry.Action);
      SetupButton(Entry);
      AnyBefore = true;
    }
  }
}
//---------------------------------------------------------------------------
QStringList TToolbar::GetItemNames() const
{
  QStringList Result;
  for (const TEntry & Entry : FEntries)
  {
    if (!Entry.Name.isEmpty())
    {
      Result.append(Entry.Name);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TToolbar::IsItemVisible(const QString & Name) const
{
  for (const TEntry & Entry : FEntries)
  {
    if (Entry.Name.compare(Name, Qt::CaseInsensitive) == 0)
    {
      return Entry.Visible;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
void TToolbar::SetItemVisible(const QString & Name, bool Visible)
{
  for (TEntry & Entry : FEntries)
  {
    if ((Entry.Name.compare(Name, Qt::CaseInsensitive) == 0) && (Entry.Visible != Visible))
    {
      Entry.Visible = Visible;
      Rebuild();
    }
  }
}
//---------------------------------------------------------------------------
void TToolbar::FillCustomizeMenu(QMenu * Menu)
{
  for (const TEntry & Entry : FEntries)
  {
    if (Entry.Action->isSeparator())
    {
      Menu->addSeparator();
    }
    else if (!Entry.Name.isEmpty())
    {
      QAction * Item = Menu->addAction(Entry.Action->icon(), Entry.Action->iconText());
      Item->setObjectName(Entry.Name + QStringLiteral("CustomizeItem"));
      Item->setCheckable(true);
      Item->setChecked(Entry.Visible);
      QString Name = Entry.Name;
      connect(Item, &QAction::toggled, this, [this, Name](bool Checked) { SetItemVisible(Name, Checked); });
    }
  }
}
//---------------------------------------------------------------------------
void TToolbar::SetSelectiveText(bool Value)
{
  if (FSelectiveText != Value)
  {
    FSelectiveText = Value;
    for (const TEntry & Entry : FEntries)
    {
      SetupButton(Entry);
    }
  }
}
//---------------------------------------------------------------------------
int TToolbar::IconSize(int LargerToolbar)
{
  // The sizes of upstream's glyphs at 96, 120 and 144 DPI
  return (LargerToolbar <= 0) ? 16 : ((LargerToolbar == 1) ? 20 : 24);
}
//---------------------------------------------------------------------------
void TToolbar::SetLargerToolbar(int Value)
{
  int Size = IconSize(Value);
  setIconSize(QSize(Size, Size));
}
//---------------------------------------------------------------------------
TToolbarDock::TToolbarDock(const QString & Name, QWidget * Parent) :
  QWidget(Parent)
{
  setObjectName(Name);
  FLayout = new QVBoxLayout(this);
  FLayout->setContentsMargins(0, 0, 0, 0);
  FLayout->setSpacing(0);
}
//---------------------------------------------------------------------------
void TToolbarDock::AddToolbar(TToolbar * Toolbar, bool Stretch)
{
  Toolbar->setParent(this);
  Toolbar->setMovable(false);
  Toolbar->setFloatable(false);
  // Compact, as upstream's toolbars of the panels
  Toolbar->layout()->setContentsMargins(0, 0, 0, 0);
  TDocked Docked;
  Docked.Toolbar = Toolbar;
  Docked.Row = static_cast<int>(FToolbars.size());
  Docked.Pos = 0;
  Docked.Stretch = Stretch;
  FToolbars.push_back(Docked);
}
//---------------------------------------------------------------------------
void TToolbarDock::SetPosition(TToolbar * Toolbar, int Row, int Pos)
{
  for (TDocked & Docked : FToolbars)
  {
    if (Docked.Toolbar == Toolbar)
    {
      Docked.Row = Row;
      Docked.Pos = Pos;
    }
  }
}
//---------------------------------------------------------------------------
void TToolbarDock::GetPosition(TToolbar * Toolbar, int & Row, int & Pos) const
{
  for (const TDocked & Docked : FToolbars)
  {
    if (Docked.Toolbar == Toolbar)
    {
      Row = Docked.Row;
      Pos = Docked.Pos;
    }
  }
}
//---------------------------------------------------------------------------
std::vector<TToolbar *> TToolbarDock::GetToolbars() const
{
  std::vector<TToolbar *> Result;
  for (const TDocked & Docked : FToolbars)
  {
    Result.push_back(Docked.Toolbar);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TToolbarDock::Arrange()
{
  // The rows of before, the toolbars stay
  while (QLayoutItem * Item = FLayout->takeAt(0))
  {
    if (QLayout * Row = Item->layout())
    {
      while (QLayoutItem * RowItem = Row->takeAt(0))
      {
        delete RowItem;
      }
    }
    delete Item;
  }
  std::vector<TDocked> Sorted = FToolbars;
  std::stable_sort(Sorted.begin(), Sorted.end(), [](const TDocked & A, const TDocked & B)
  {
    return (A.Row < B.Row) || ((A.Row == B.Row) && (A.Pos < B.Pos));
  });
  QHBoxLayout * Row = nullptr;
  int RowIndex = 0;
  bool Stretched = false;
  for (const TDocked & Docked : Sorted)
  {
    if ((Row == nullptr) || (Docked.Row != RowIndex))
    {
      if ((Row != nullptr) && !Stretched)
      {
        Row->addStretch(1);
      }
      Row = new QHBoxLayout();
      Row->setContentsMargins(0, 0, 0, 0);
      Row->setSpacing(0);
      FLayout->addLayout(Row);
      RowIndex = Docked.Row;
      Stretched = false;
    }
    Docked.Toolbar->setSizePolicy(
      Docked.Stretch ? QSizePolicy::Expanding : QSizePolicy::Maximum, QSizePolicy::Preferred);
    Row->addWidget(Docked.Toolbar, Docked.Stretch ? 1 : 0);
    Stretched = Stretched || Docked.Stretch;
  }
  if ((Row != nullptr) && !Stretched)
  {
    Row->addStretch(1);
  }
}
//---------------------------------------------------------------------------
TToolbarsLayout::TToolbarsLayout(const UnicodeString & Value)
{
  std::unique_ptr<TStringList> List(CommaTextToStringList(Value));
  for (int Index = 0; Index < List->Count; Index++)
  {
    FValues.emplace_back(ToQString(List->Names[Index]), ToQString(List->ValueFromIndex[Index]));
  }
}
//---------------------------------------------------------------------------
bool TToolbarsLayout::Get(const QString & Key, TToolbarPosition & Position) const
{
  for (const auto & Value : FValues)
  {
    if (Value.first.compare(Key, Qt::CaseInsensitive) == 0)
    {
      // Visible:Dock:Row+Pos
      QStringList Parts = Value.second.split(QLatin1Char(':'));
      if (Parts.size() < 3)
      {
        return false;
      }
      Position.Visible = (Parts[0].toInt() != 0);
      Position.Dock = Parts[1];
      QStringList RowPos = Parts[2].split(QLatin1Char('+'));
      Position.Row = RowPos[0].toInt();
      Position.Pos = (RowPos.size() > 1) ? RowPos[1].toInt() : 0;
      return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
void TToolbarsLayout::Set(const QString & Key, const TToolbarPosition & Position)
{
  QString Str = QStringLiteral("%1:%2:%3+%4").arg(Position.Visible ? 1 : 0).arg(Position.Dock).arg(Position.Row).arg(Position.Pos);
  for (auto & Value : FValues)
  {
    if (Value.first.compare(Key, Qt::CaseInsensitive) == 0)
    {
      Value.second = Str;
      return;
    }
  }
  FValues.emplace_back(Key, Str);
}
//---------------------------------------------------------------------------
UnicodeString TToolbarsLayout::ToString() const
{
  std::unique_ptr<TStringList> List(new TStringList());
  for (const auto & Value : FValues)
  {
    List->Add(ToUnicodeString(Value.first + QLatin1Char('=') + Value.second));
  }
  return List->CommaText;
}
//---------------------------------------------------------------------------
void LoadToolbarsButtons(const std::vector<TToolbar *> & Toolbars, const UnicodeString & Value)
{
  UnicodeString Buttons = Value;
  while (!Buttons.IsEmpty())
  {
    UnicodeString S = CutToChar(Buttons, L';', true);
    QString Key = ToQString(CutToChar(S, L'=', true));
    for (TToolbar * Toolbar : Toolbars)
    {
      if (Toolbar->GetKey().compare(Key, Qt::CaseInsensitive) == 0)
      {
        UnicodeString Items = S;
        while (!Items.IsEmpty())
        {
          UnicodeString Item = CutToChar(Items, L',', true);
          QString Name = ToQString(CutToChar(Item, L':', true));
          bool Visible = (StrToIntDef(CutToChar(Item, L':', true), 0) != 0);
          Toolbar->SetItemVisible(Name, Visible);
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString SaveToolbarsButtons(const std::vector<TToolbar *> & Toolbars)
{
  // All the buttons are shown by default, only the hidden ones are kept (as
  // upstream does)
  UnicodeString Result;
  for (TToolbar * Toolbar : Toolbars)
  {
    UnicodeString ToolbarStr;
    for (const QString & Name : Toolbar->GetItemNames())
    {
      if (!Toolbar->IsItemVisible(Name))
      {
        AddToList(ToolbarStr, FORMAT(L"%s:0", (ToUnicodeString(Name))), L",");
      }
    }
    if (!ToolbarStr.IsEmpty())
    {
      AddToList(Result, FORMAT(L"%s=%s", (ToUnicodeString(Toolbar->GetKey()), ToolbarStr)), L";");
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
