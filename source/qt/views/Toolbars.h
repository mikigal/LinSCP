// The toolbars as upstream's (the TTBXToolbar of its forms with the layout
// of Toolbar2000): buttons of actions, the text shown on the important ones
// (upstream's selective text labels), drop-down menus, buttons hidden by the
// user; the layout of the toolbars and their hidden buttons in upstream's
// format (ToolbarsLayout2 and ToolbarsButtons, windows/UserInterface.cpp and
// forms/CustomScpExplorer.cpp).
//---------------------------------------------------------------------------
#ifndef ToolbarsH
#define ToolbarsH
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <QToolBar>

#include <vector>
//---------------------------------------------------------------------------
class QMenu;
class QToolButton;
class QVBoxLayout;
//---------------------------------------------------------------------------
// How a button opens its drop-down menu
enum TToolbarMenuMode
{
  // The arrow next to the button, the button runs its action (upstream's
  // DropdownCombo)
  tmmSplit,
  // The whole button (upstream's tboDropdownArrow)
  tmmWhole,
};
//---------------------------------------------------------------------------
class TToolbar : public QToolBar
{
  Q_OBJECT

public:
  // The name is upstream's (CommandsToolbar, RemoteFileToolbar, ...)
  TToolbar(const QString & Name, const QString & Caption, QWidget * Parent);

  // The key of the toolbar in the layout: the name without "Toolbar"
  // (upstream's GetToolbarKey)
  QString GetKey() const;
  // A button of the action; upstream shows the text of the important ones
  // (nbdmImageAndText) with its selective text labels
  void AddButton(QAction * Action, bool ImageAndText = false);
  // A button with a drop-down menu
  void AddMenuButton(QAction * Action, QMenu * Menu, TToolbarMenuMode Mode, bool ImageAndText = false);
  // A button of a submenu (New): with the title and the icon of the menu,
  // its name is upstream's of the item
  void AddSubmenu(const QString & Name, QMenu * Menu, bool ImageAndText = false);
  // A label or a combo box (not hidden by the user); its action hides it
  QAction * AddControl(QWidget * Widget);
  void AddSeparatorItem();
  // A button, a separator before the button of another action
  void InsertButton(QAction * Before, QAction * Action, bool ImageAndText = false);
  void InsertSeparatorItem(QAction * Before);
  // Without any buttons
  void ClearItems();
  // The button of the action, NULL when hidden
  QToolButton * GetButton(QAction * Action) const;

  // The buttons the user can hide (upstream's Customize Toolbar), by the
  // names of upstream (GetToolbarItemName: the action's without "Action")
  QStringList GetItemNames() const;
  bool IsItemVisible(const QString & Name) const;
  void SetItemVisible(const QString & Name, bool Visible);
  // The items of the buttons, checked when shown, in the menu
  void FillCustomizeMenu(QMenu * Menu);

  // The action that shows and hides the toolbar (upstream's *BandAction)
  void SetBandAction(QAction * Value) { FBandAction = Value; }
  QAction * GetBandAction() const { return FBandAction; }

  void SetSelectiveText(bool Value);
  // upstream's LargerToolbar: 0 normal, 1 large, 2 very large
  void SetLargerToolbar(int Value);
  static int IconSize(int LargerToolbar);

private:
  struct TEntry
  {
    // Empty for the separators and the controls
    QString Name;
    // Of the button, the control (QWidgetAction) or the separator
    QAction * Action;
    QMenu * Menu;
    TToolbarMenuMode MenuMode;
    bool ImageAndText;
    bool Visible;
  };
  std::vector<TEntry> FEntries;
  bool FSelectiveText;
  QAction * FBandAction;

  void AddEntry(const TEntry & Entry);
  void InsertEntry(QAction * Before, const TEntry & Entry);
  // The buttons again, without the hidden ones (and the separators they
  // leave alone)
  void Rebuild();
  void SetupButton(const TEntry & Entry);
};
//---------------------------------------------------------------------------
// The toolbars in rows above a panel (upstream's TTBXDock of the panels,
// LocalTopDock and RemoteTopDock): in the order of their positions, the
// stretched one takes the rest of its row
class TToolbarDock : public QWidget
{
  Q_OBJECT

public:
  // The name is upstream's (LocalTopDock, ...)
  TToolbarDock(const QString & Name, QWidget * Parent);

  void AddToolbar(TToolbar * Toolbar, bool Stretch = false);
  void SetPosition(TToolbar * Toolbar, int Row, int Pos);
  // The toolbar is elsewhere for a while (the Explorer's address bar is the
  // remote panel's path toolbar), not arranged
  void SetUndocked(TToolbar * Toolbar, bool Undocked);
  void GetPosition(TToolbar * Toolbar, int & Row, int & Pos) const;
  std::vector<TToolbar *> GetToolbars() const;
  // The toolbars in their rows, in the order of their positions
  void Arrange();

private:
  struct TDocked
  {
    TToolbar * Toolbar;
    int Row;
    int Pos;
    bool Stretch;
    bool Undocked;
  };
  std::vector<TDocked> FToolbars;
  QVBoxLayout * FLayout;
};
//---------------------------------------------------------------------------
// The position of a toolbar in upstream's layout (Toolbar2000's
// Visible:Dock:Row+Pos)
struct TToolbarPosition
{
  bool Visible = true;
  QString Dock;
  int Row = 0;
  int Pos = 0;
};
//---------------------------------------------------------------------------
// The layout of the toolbars of a window (upstream's ToolbarsLayout2): the
// positions by the keys of the toolbars; the other values (the menu, the
// toolbars not ported, the resolution) are kept
class TToolbarsLayout
{
public:
  explicit TToolbarsLayout(const UnicodeString & Value);

  bool Get(const QString & Key, TToolbarPosition & Position) const;
  void Set(const QString & Key, const TToolbarPosition & Position);
  UnicodeString ToString() const;

private:
  std::vector<std::pair<QString, QString>> FValues;
};
//---------------------------------------------------------------------------
// The hidden buttons of the toolbars (upstream's ToolbarsButtons:
// Key=Button:0,Button:0;Key=...)
void LoadToolbarsButtons(const std::vector<TToolbar *> & Toolbars, const UnicodeString & Value);
UnicodeString SaveToolbarsButtons(const std::vector<TToolbar *> & Toolbars);
//---------------------------------------------------------------------------
#endif
