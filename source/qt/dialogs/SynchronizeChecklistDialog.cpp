// The synchronization checklist (upstream's TSynchronizeChecklistDialog,
// forms/SynchronizeChecklist.dfm): the differences found by the comparison,
// each with its action, to be checked, reversed and started, directly or in
// the queue. Upstream also moves files, finds move candidates, explores the
// directories and runs custom commands on the items; not yet.
//---------------------------------------------------------------------------
#include <QtInterface.h>
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Terminal.h>

#include "DirModel.h"
#include "SplitButton.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QStatusBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <map>
//---------------------------------------------------------------------------
const int ImageColumnIndex = 4;
//---------------------------------------------------------------------------
// The images of upstream's action image list, from the icon theme
static QIcon ActionImage(int Action)
{
  static const char * Names[] =
  {
    "dialog-ok", "go-up", "go-down", "go-top", "go-bottom", "edit-delete", "edit-delete"
  };
  return QIcon::fromTheme(QString::fromLatin1(Names[Action]));
}
//---------------------------------------------------------------------------
class TSynchronizeChecklistDialog : public QDialog
{
public:
  TSynchronizeChecklistDialog(TSynchronizeMode Mode, int Params,
    const UnicodeString & Directory1, const UnicodeString & Directory2,
    TFullSynchronizeEvent OnSynchronize, TQueueSynchronizeEvent OnQueueSynchronize,
    TSynchronizeChecklistCalculateSize OnSynchronizeChecklistCalculateSize, void * Token);

  bool Execute(TSynchronizeChecklist * Checklist);

protected:
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  TSynchronizeChecklist * FChecklist;
  TSynchronizeMode FMode;
  int FParams;
  UnicodeString FDirectory1;
  UnicodeString FDirectory2;
  int FTotals[1 + TSynchronizeChecklist::ActionCount];
  int FChecked[1 + TSynchronizeChecklist::ActionCount];
  __int64 FCheckedSize[1 + TSynchronizeChecklist::ActionCount];
  bool FChangingItemIgnore;
  bool FChangingItemMass;
  TSynchronizeChecklistCalculateSize FOnSynchronizeChecklistCalculateSize;
  typedef std::map<const TSynchronizeChecklist::TItem *, TSynchronizeChecklist::TAction> TActions;
  TActions FActions;
  std::map<const TSynchronizeChecklist::TItem *, bool> FCheckStates;
  TFullSynchronizeEvent FOnSynchronize;
  TQueueSynchronizeEvent FOnQueueSynchronize;
  void * FToken;
  bool FSynchronizing;
  std::unique_ptr<Exception> FException;
  std::map<const TSynchronizeChecklist::TItem *, QTreeWidgetItem *> FChecklistToListViewMap;
  int FDirectories;

  QTreeWidget * ListView2;
  QWidget * FStatusPanels[1 + TSynchronizeChecklist::ActionCount];
  QLabel * FStatusTexts[1 + TSynchronizeChecklist::ActionCount];
  TSplitButton * OkButton;

  QPushButton * CancelButton;
  QAction * CheckAction;
  QAction * UncheckAction;
  QAction * CheckAllAction;
  QAction * UncheckAllAction;
  QAction * SelectAllAction;
  QAction * ReverseAction;
  QAction * CalculateSizeAction;
  QAction * CalculateSizeAllAction;
  QAction * StartItem;
  QAction * StartQueueItem;

  QAction * CreateAction(const char * Name, const QString & Caption, const QKeySequence & ShortCut,
    std::function<void()> Execute);
  QPushButton * CreateButton(QLayout * Layout, const char * Name, QAction * Action);
  void UpdateControls();
  void UpdateCaption();
  void UpdateStatusBar();
  void LoadItem(QTreeWidgetItem * Item);
  void LoadList();
  void CheckAll(bool Check);
  void Check(bool Check);
  QTreeWidgetItem * SelectAll(bool Select, int Action = 0, bool OnlyTheAction = true);
  static const TSynchronizeChecklist::TItem * GetChecklistItem(QTreeWidgetItem * Item);
  TSynchronizeChecklist::TAction & GetChecklistItemAction(const TSynchronizeChecklist::TItem * ChecklistItem);
  bool IsPossible(TSynchronizeChecklist::TAction Action);
  void ItemChanged(QTreeWidgetItem * Item);
  void ProcessedItem(void * Token, const TSynchronizeChecklist::TItem * ChecklistItem);
  void UpdatedSynchronizationChecklistItems(const TSynchronizeChecklist::TItemList & Items);
  void CountItemSize(const TSynchronizeChecklist::TItem * ChecklistItem, int Factor);
  void CountItem(const TSynchronizeChecklist::TItem * ChecklistItem, int Factor);
  void CountItemTotal(const TSynchronizeChecklist::TItem * ChecklistItem, int Factor);
  void ReverseActionExecute();
  void CalculateSize(bool All);
  void DoSynchronize(bool Queue);
};
//---------------------------------------------------------------------------
TSynchronizeChecklistDialog::TSynchronizeChecklistDialog(TSynchronizeMode Mode, int Params,
  const UnicodeString & Directory1, const UnicodeString & Directory2,
  TFullSynchronizeEvent OnSynchronize, TQueueSynchronizeEvent OnQueueSynchronize,
  TSynchronizeChecklistCalculateSize OnSynchronizeChecklistCalculateSize, void * Token) :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("SynchronizeChecklistDialog"));
  setSizeGripEnabled(true);
  FMode = Mode;
  FParams = Params;
  FDirectory1 = ExcludeTrailingBackslash(Directory1);
  FDirectory2 = UniversalExcludeTrailingBackslash(FLAGCLEAR(Params, TTerminal::spLocalLocal), Directory2);
  FOnSynchronizeChecklistCalculateSize = OnSynchronizeChecklistCalculateSize;
  DebugAssert(OnSynchronize);
  FOnSynchronize = OnSynchronize;
  FOnQueueSynchronize = OnQueueSynchronize;
  FToken = Token;
  FChecklist = NULL;
  FChangingItemIgnore = false;
  FChangingItemMass = false;
  FSynchronizing = false;
  FDirectories = 0;

  CheckAction = CreateAction("CheckAction", tr("&Check"), QKeySequence(), [this]() { Check(true); });
  UncheckAction = CreateAction("UncheckAction", tr("&Uncheck"), QKeySequence(), [this]() { Check(false); });
  CheckAllAction = CreateAction("CheckAllAction", tr("Check &All"), QKeySequence(), [this]() { CheckAll(true); });
  UncheckAllAction = CreateAction("UncheckAllAction", tr("Uncheck A&ll"), QKeySequence(), [this]() { CheckAll(false); });
  SelectAllAction = CreateAction("SelectAllAction", tr("Select &All"), QKeySequence(Qt::CTRL | Qt::Key_A), [this]() { SelectAll(true); });
  ReverseAction = CreateAction("ReverseAction", tr("&Reverse"), QKeySequence(), [this]() { ReverseActionExecute(); });
  CalculateSizeAction = CreateAction("CalculateSizeAction", tr("C&alculate"), QKeySequence(Qt::ALT | Qt::Key_Return),
    [this]() { CalculateSize(false); });
  CalculateSizeAllAction = CreateAction("CalculateSizeAllAction", tr("Calc&ulate All"),
    QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Return), [this]() { CalculateSize(true); });
  StartItem = CreateAction("StartItem", tr("&Start"), QKeySequence(), [this]() { DoSynchronize(false); });
  StartQueueItem = CreateAction("StartQueueItem", tr("Start on &Background"), QKeySequence(), [this]() { DoSynchronize(true); });

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QHBoxLayout * ContentLayout = new QHBoxLayout();
  ListView2 = new QTreeWidget(this);
  ListView2->setObjectName(QStringLiteral("ListView2"));
  ListView2->setRootIsDecorated(false);
  ListView2->setUniformRowHeights(true);
  ListView2->setSelectionMode(QAbstractItemView::ExtendedSelection);
  QStringList Columns = { tr("Name"), tr("Local directory"), tr("Size"), tr("Date modified"), QString(),
    tr("Remote directory"), tr("Size"), tr("Date modified") };
  if (FLAGSET(Params, TTerminal::spLocalLocal))
  {
    Columns[1] = ToQString(LoadStr(SYNCHRONIZE_CHECKLIST_LEFT_DIR));
    Columns[5] = ToQString(LoadStr(SYNCHRONIZE_CHECKLIST_RIGHT_DIR));
  }
  ListView2->setHeaderLabels(Columns);
  const int Widths[] = { 150, 140, 70, 130, 26, 140, 70, 130 };
  for (int Column = 0; Column < static_cast<int>(std::size(Widths)); Column++)
  {
    ListView2->setColumnWidth(Column, Widths[Column]);
  }
  ListView2->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ListView2, &QWidget::customContextMenuRequested, this, [this](const QPoint & Pos)
  {
    UpdateControls();
    QMenu Menu(this);
    Menu.setObjectName(QStringLiteral("ListViewPopupMenu"));
    Menu.addAction(CheckAction);
    Menu.addAction(UncheckAction);
    Menu.addSeparator();
    Menu.addAction(ReverseAction);
    QMenu * CalculateMenu = Menu.addMenu(tr("C&alculate"));
    CalculateMenu->addAction(CalculateSizeAction);
    CalculateMenu->addAction(CalculateSizeAllAction);
    Menu.addSeparator();
    Menu.addAction(SelectAllAction);
    Menu.exec(ListView2->viewport()->mapToGlobal(Pos));
  });
  connect(ListView2, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem * Item) { ItemChanged(Item); });
  connect(ListView2, &QTreeWidget::itemSelectionChanged, this, [this]() { UpdateControls(); });
  ListView2->viewport()->installEventFilter(this);
  ContentLayout->addWidget(ListView2, 1);

  QVBoxLayout * ButtonsLayout = new QVBoxLayout();
  // upstream's split button with OkPopupMenu (in the queue)
  OkButton = new TSplitButton(this);
  OkButton->setText(tr("OK"));
  OkButton->setObjectName(QStringLiteral("OkButton"));
  OkButton->setDefault(true);
  connect(OkButton, &QPushButton::clicked, StartItem, &QAction::trigger);
  QMenu * OkPopupMenu = new QMenu(OkButton);
  OkPopupMenu->setObjectName(QStringLiteral("OkPopupMenu"));
  OkPopupMenu->addAction(StartItem);
  OkPopupMenu->addAction(StartQueueItem);
  OkButton->SetDropDownMenu(OkPopupMenu);
  OkButton->SetDropDownVisible(static_cast<bool>(FOnQueueSynchronize));
  ButtonsLayout->addWidget(OkButton);
  CancelButton = new QPushButton(tr("Cancel"), this);
  CancelButton->setObjectName(QStringLiteral("CancelButton"));
  CancelButton->setAutoDefault(false);
  connect(CancelButton, &QPushButton::clicked, this, &QDialog::reject);
  ButtonsLayout->addWidget(CancelButton);
  // upstream's order of the buttons
  QPushButton * HelpButton = new QPushButton(tr("&Help"), this);
  HelpButton->setObjectName(QStringLiteral("HelpButton"));
  HelpButton->setAutoDefault(false);
  connect(HelpButton, &QPushButton::clicked, this, []() { ShowHelp(FormHelpKeyword(L"SynchronizeChecklist.dfm")); });
  ButtonsLayout->addWidget(HelpButton);
  ButtonsLayout->addSpacing(fontMetrics().lineSpacing());
  CreateButton(ButtonsLayout, "CheckButton", CheckAction);
  CreateButton(ButtonsLayout, "UncheckButton", UncheckAction);
  CreateButton(ButtonsLayout, "CheckAllButton", CheckAllAction);
  CreateButton(ButtonsLayout, "UncheckAllButton", UncheckAllAction);
  ButtonsLayout->addSpacing(fontMetrics().lineSpacing());
  CreateButton(ButtonsLayout, "ReverseButton", ReverseAction);
  QPushButton * ToolsMenuButton = new QPushButton(tr("&Tools"), this);
  ToolsMenuButton->setObjectName(QStringLiteral("ToolsMenuButton"));
  ToolsMenuButton->setAutoDefault(false);
  QMenu * ToolsPopupMenu = new QMenu(ToolsMenuButton);
  ToolsPopupMenu->setObjectName(QStringLiteral("ToolsPopupMenu"));
  ToolsPopupMenu->addAction(CalculateSizeAction);
  ToolsPopupMenu->addAction(CalculateSizeAllAction);
  ToolsMenuButton->setMenu(ToolsPopupMenu);
  ButtonsLayout->addWidget(ToolsMenuButton);
  ButtonsLayout->addStretch(1);
  ContentLayout->addLayout(ButtonsLayout);
  MainLayout->addLayout(ContentLayout, 1);

  // A panel per action (upstream draws them on the status bar), a click
  // selects the items of the action
  QStatusBar * StatusBar2 = new QStatusBar(this);
  StatusBar2->setObjectName(QStringLiteral("StatusBar2"));
  StatusBar2->setSizeGripEnabled(false);
  for (int Action = 0; Action <= TSynchronizeChecklist::ActionCount; Action++)
  {
    QWidget * Panel = new QWidget(StatusBar2);
    Panel->setObjectName(QStringLiteral("StatusPanel%1").arg(Action));
    QHBoxLayout * PanelLayout = new QHBoxLayout(Panel);
    PanelLayout->setContentsMargins(2, 0, 2, 0);
    QLabel * Image = new QLabel(Panel);
    Image->setObjectName(QStringLiteral("StatusImage%1").arg(Action));
    int IconSize = fontMetrics().height();
    Image->setPixmap(ActionImage(Action).pixmap(IconSize, IconSize));
    PanelLayout->addWidget(Image);
    QLabel * Text = new QLabel(Panel);
    Text->setObjectName(QStringLiteral("StatusText%1").arg(Action));
    Text->setTextFormat(Qt::PlainText);
    PanelLayout->addWidget(Text, 1);
    Panel->installEventFilter(this);
    StatusBar2->addWidget(Panel, 1);
    FStatusPanels[Action] = Panel;
    FStatusTexts[Action] = Text;
  }
  MainLayout->addWidget(StatusBar2);
  resize(fontMetrics().averageCharWidth() * 150, fontMetrics().lineSpacing() * 30);
}
//---------------------------------------------------------------------------
QAction * TSynchronizeChecklistDialog::CreateAction(const char * Name, const QString & Caption, const QKeySequence & ShortCut,
  std::function<void()> Execute)
{
  QAction * Action = new QAction(Caption, this);
  Action->setObjectName(QString::fromLatin1(Name));
  Action->setShortcut(ShortCut);
  addAction(Action);
  connect(Action, &QAction::triggered, this, Execute);
  return Action;
}
//---------------------------------------------------------------------------
QPushButton * TSynchronizeChecklistDialog::CreateButton(QLayout * Layout, const char * Name, QAction * Action)
{
  QPushButton * Button = new QPushButton(Action->text(), this);
  Button->setObjectName(QString::fromLatin1(Name));
  Button->setAutoDefault(false);
  connect(Button, &QPushButton::clicked, Action, &QAction::trigger);
  connect(Action, &QAction::enabledChanged, Button, &QPushButton::setEnabled);
  Layout->addWidget(Button);
  return Button;
}
//---------------------------------------------------------------------------
bool TSynchronizeChecklistDialog::Execute(TSynchronizeChecklist * Checklist)
{
  FChecklist = Checklist;
  LoadList();

  bool Result = (exec() == QDialog::Accepted);

  if (FException.get() != NULL)
  {
    RethrowException(FException.get());
  }

  return Result;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::UpdateCaption()
{
  UnicodeString Title = LoadStr(FSynchronizing ? SYNCHRONIZE_PROGRESS_SYNCHRONIZE2 : SYNCHRONIZE_CHECKLIST_CAPTION);
  setWindowTitle(ToQString(FormatMainFormCaption(Title)));
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::UpdateControls()
{
  UpdateCaption();
  UpdateStatusBar();

  bool AllChecked = true;
  bool AllUnchecked = true;
  bool AnyDirectory = false;
  QList<QTreeWidgetItem *> Selected = ListView2->selectedItems();
  for (QTreeWidgetItem * Item : Selected)
  {
    const TSynchronizeChecklist::TItem * ChecklistItem = GetChecklistItem(Item);
    if (Item->checkState(0) == Qt::Checked)
    {
      AllUnchecked = false;
    }
    else
    {
      AllChecked = false;
    }
    if (ChecklistItem->IsDirectory)
    {
      AnyDirectory = true;
    }
  }

  int SelCount = Selected.count();
  OkButton->setEnabled((FChecked[0] > 0) && !FSynchronizing);
  StartItem->setEnabled(OkButton->isEnabled());
  StartQueueItem->setEnabled(OkButton->isEnabled() && static_cast<bool>(FOnQueueSynchronize));
  CancelButton->setEnabled(!FSynchronizing);
  CheckAction->setEnabled(!AllChecked && !FSynchronizing);
  UncheckAction->setEnabled(!AllUnchecked && !FSynchronizing);
  CheckAllAction->setEnabled((FChecked[0] < FTotals[0]) && !FSynchronizing);
  UncheckAllAction->setEnabled((FChecked[0] > 0) && !FSynchronizing);
  ReverseAction->setEnabled((SelCount > 0) && !FSynchronizing);
  CalculateSizeAction->setEnabled((SelCount > 0) && AnyDirectory && !FSynchronizing);
  CalculateSizeAllAction->setEnabled((FDirectories > 0) && !FSynchronizing);

  int Count = ListView2->topLevelItemCount();
  DebugAssert(FTotals[0] == Count);
  SelectAllAction->setEnabled((SelCount < Count) && !FSynchronizing);
}
//---------------------------------------------------------------------------
bool TSynchronizeChecklistDialog::IsPossible(TSynchronizeChecklist::TAction Action)
{
  bool Possible;
  if (FTotals[Action] > 0)
  {
    // if direction is overriden to an action that is otherwise not possible
    // in given synchronization mode, we still want to show the stats
    Possible = true;
  }
  else
  {
    switch (Action)
    {
      case TSynchronizeChecklist::saNone:
        Possible = true;
        break;

      case TSynchronizeChecklist::saUploadNew:
        Possible = ((FMode == smRemote) || (FMode == smBoth)) &&
          FLAGCLEAR(FParams, TTerminal::spTimestamp);
        break;

      case TSynchronizeChecklist::saDownloadNew:
        Possible = ((FMode == smLocal) || (FMode == smBoth)) &&
          FLAGCLEAR(FParams, TTerminal::spTimestamp);
        break;

      case TSynchronizeChecklist::saUploadUpdate:
        Possible =
          ((FMode == smRemote) || (FMode == smBoth)) &&
          (FLAGCLEAR(FParams, TTerminal::spNotByTime) || FLAGSET(FParams, TTerminal::spBySize) ||
           FLAGSET(FParams, TTerminal::spByChecksum));
        break;

      case TSynchronizeChecklist::saDownloadUpdate:
        Possible =
          ((FMode == smLocal) || (FMode == smBoth)) &&
          (FLAGCLEAR(FParams, TTerminal::spNotByTime) || FLAGSET(FParams, TTerminal::spBySize) ||
           FLAGSET(FParams, TTerminal::spByChecksum));
        break;

      case TSynchronizeChecklist::saDeleteRemote:
        Possible = (FMode == smRemote) &&
          FLAGCLEAR(FParams, TTerminal::spTimestamp);
        break;

      case TSynchronizeChecklist::saDeleteLocal:
        Possible = (FMode == smLocal) &&
          FLAGCLEAR(FParams, TTerminal::spTimestamp);
        break;

      default:
        DebugFail();
        Possible = false;
        break;
    }
  }
  return Possible;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::UpdateStatusBar()
{
  static const int Hints[] = {
    0, SYNCHRONIZE_CHECKLIST_UPLOAD_NEW, SYNCHRONIZE_CHECKLIST_DOWNLOAD_NEW, SYNCHRONIZE_CHECKLIST_UPLOAD_UPDATE,
    SYNCHRONIZE_CHECKLIST_DOWNLOAD_UPDATE, SYNCHRONIZE_CHECKLIST_DELETE_REMOTE, SYNCHRONIZE_CHECKLIST_DELETE_LOCAL };
  for (int ActionIndex = 0; ActionIndex <= TSynchronizeChecklist::ActionCount; ActionIndex++)
  {
    TSynchronizeChecklist::TAction Action = TSynchronizeChecklist::TAction(ActionIndex);
    bool Possible = IsPossible(Action);
    UnicodeString PanelText;
    if (Possible)
    {
      PanelText = FORMAT(LoadStrPart(SYNCHRONIZE_SELECTED_ACTIONS, 1),
        (FormatNumber(FChecked[ActionIndex]),
         FormatNumber(FTotals[ActionIndex])));
      if ((FChecked[ActionIndex] > 0) &&
          ((ActionIndex == 0) || !TSynchronizeChecklist::IsItemSizeIrrelevant(Action)))
      {
        PanelText += FORMAT(L" (%s)", (FormatBytes(FCheckedSize[ActionIndex])));
      }
    }
    else
    {
      PanelText = LoadStrPart(SYNCHRONIZE_SELECTED_ACTIONS, 2);
    }
    FStatusTexts[ActionIndex]->setText(ToQString(PanelText));
    QWidget * Panel = FStatusPanels[ActionIndex];
    Panel->setToolTip((Hints[ActionIndex] != 0) ? ToQString(LoadStr(Hints[ActionIndex])) : QString());
    Panel->setEnabled(Possible);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::LoadItem(QTreeWidgetItem * Item)
{
  UnicodeString S;
  const TSynchronizeChecklist::TItem * ChecklistItem = GetChecklistItem(Item);
  QString Type;
  QIcon Icon;
  FileTypeAndIcon(ToQString(ChecklistItem->GetFileName()), ChecklistItem->IsDirectory, Type, Icon);
  Item->setIcon(0, Icon);

  S = ChecklistItem->GetFileName();
  if (ChecklistItem->IsDirectory)
  {
    S = IncludeTrailingBackslash(S);
  }
  Item->setText(0, ToQString(S));

  int Index = 1;
  auto AddSubItem = [&](const UnicodeString & Value) { Item->setText(Index++, ToQString(Value)); };

  TSynchronizeChecklist::TAction Action = GetChecklistItemAction(ChecklistItem);

  if (Action == TSynchronizeChecklist::saDeleteRemote)
  {
    AddSubItem(L"");
    AddSubItem(L"");
    AddSubItem(L"");
  }
  else
  {
    S = ChecklistItem->Info1.Directory;
    if (AnsiSameText(FDirectory1, S.SubString(1, FDirectory1.Length())))
    {
      S[1] = L'.';
      S.Delete(2, FDirectory1.Length() - 1);
    }
    else
    {
      DebugFail();
    }
    AddSubItem(S);
    if (Action == TSynchronizeChecklist::saDownloadNew)
    {
      AddSubItem(L"");
      AddSubItem(L"");
    }
    else
    {
      if (!ChecklistItem->HasSize())
      {
        AddSubItem(L"");
      }
      else
      {
        AddSubItem(FormatPanelBytes(ChecklistItem->Info1.Size, fbKilobytes));
      }
      AddSubItem(UserModificationStr(ChecklistItem->Info1.Modification, ChecklistItem->Info1.ModificationFmt));
    }
  }

  DebugAssert(Index == ImageColumnIndex);
  static const int Hints[] = {
    0, SYNCHRONIZE_CHECKLIST_UPLOAD_NEW, SYNCHRONIZE_CHECKLIST_DOWNLOAD_NEW, SYNCHRONIZE_CHECKLIST_UPLOAD_UPDATE,
    SYNCHRONIZE_CHECKLIST_DOWNLOAD_UPDATE, SYNCHRONIZE_CHECKLIST_DELETE_REMOTE, SYNCHRONIZE_CHECKLIST_DELETE_LOCAL };
  Item->setIcon(ImageColumnIndex, ActionImage(Action));
  Item->setToolTip(ImageColumnIndex,
    ToQString(FORMAT(L"%s\n%s", (LoadStr(Hints[Action]), LoadStr(SYNCHRONIZE_CHECKLIST_REVERSE)))));
  // The action as data, for the automation of the tests
  Item->setData(ImageColumnIndex, Qt::AccessibleTextRole, ToQString(LoadStr(Hints[Action])));
  Index++;

  if (Action == TSynchronizeChecklist::saDeleteLocal)
  {
    AddSubItem(L"");
    AddSubItem(L"");
    AddSubItem(L"");
  }
  else
  {
    S = ChecklistItem->Info2.Directory;
    if (AnsiSameText(FDirectory2, S.SubString(1, FDirectory2.Length())))
    {
      S[1] = L'.';
      S.Delete(2, FDirectory2.Length() - 1);
    }
    else
    {
      DebugFail();
    }
    AddSubItem(S);
    if (Action == TSynchronizeChecklist::saUploadNew)
    {
      AddSubItem(L"");
      AddSubItem(L"");
    }
    else
    {
      if (!ChecklistItem->HasSize())
      {
        AddSubItem(L"");
      }
      else
      {
        AddSubItem(FormatPanelBytes(ChecklistItem->Info2.Size, fbKilobytes));
      }
      AddSubItem(UserModificationStr(ChecklistItem->Info2.Modification, ChecklistItem->Info2.ModificationFmt));
    }
  }
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::CountItemSize(const TSynchronizeChecklist::TItem * ChecklistItem, int Factor)
{
  TSynchronizeChecklist::TAction Action = GetChecklistItemAction(ChecklistItem);
  int ActionIndex = int(Action);
  __int64 ItemSize = ChecklistItem->GetSize(Action);
  FCheckedSize[ActionIndex] += Factor * ItemSize;
  FCheckedSize[0] += Factor * ItemSize;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::CountItem(const TSynchronizeChecklist::TItem * ChecklistItem, int Factor)
{
  int ActionIndex = int(GetChecklistItemAction(ChecklistItem));
  FChecked[ActionIndex] += Factor;
  FChecked[0] += Factor;

  CountItemSize(ChecklistItem, Factor);
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::CountItemTotal(const TSynchronizeChecklist::TItem * ChecklistItem, int Factor)
{
  FTotals[0] += Factor;
  int ActionIndex = int(GetChecklistItemAction(ChecklistItem));
  FTotals[ActionIndex] += Factor;
  if (ChecklistItem->IsDirectory)
  {
    FDirectories += Factor;
  }
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::LoadList()
{
  memset(&FTotals, 0, sizeof(FTotals));
  memset(&FChecked, 0, sizeof(FChecked));
  memset(&FCheckedSize, 0, sizeof(FCheckedSize));
  FDirectories = 0;

  ListView2->clear();
  for (int Index = 0; Index < FChecklist->Count; Index++)
  {
    const TSynchronizeChecklist::TItem * ChecklistItem = FChecklist->Item[Index];
    {
      TAutoFlag IgnoreFlag(FChangingItemIgnore);
      QTreeWidgetItem * Item = new QTreeWidgetItem();
      Item->setFlags(Item->flags() | Qt::ItemIsUserCheckable);
      TSynchronizeChecklist::TAction Action = ChecklistItem->Action;
      FActions.insert(std::make_pair(ChecklistItem, Action));
      FChecklistToListViewMap.insert(std::make_pair(ChecklistItem, Item));
      Item->setData(0, Qt::UserRole, QVariant::fromValue(const_cast<void *>(static_cast<const void *>(ChecklistItem))));
      Item->setCheckState(0, ChecklistItem->Checked ? Qt::Checked : Qt::Unchecked);
      FCheckStates[ChecklistItem] = ChecklistItem->Checked;
      LoadItem(Item);
      ListView2->addTopLevelItem(Item);
    }

    CountItemTotal(ChecklistItem, 1);
    if (ChecklistItem->Checked)
    {
      CountItem(ChecklistItem, 1);
    }
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::ItemChanged(QTreeWidgetItem * Item)
{
  if (!FChangingItemIgnore)
  {
    const TSynchronizeChecklist::TItem * ChecklistItem = GetChecklistItem(Item);
    bool Checked = (Item->checkState(0) == Qt::Checked);
    bool & WasChecked = FCheckStates[ChecklistItem];
    if (WasChecked != Checked)
    {
      WasChecked = Checked;
      CountItem(ChecklistItem, Checked ? 1 : -1);

      if (!FChangingItemMass)
      {
        UpdateControls();
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::CheckAll(bool Check)
{
  {
    TAutoFlag MassFlag(FChangingItemMass);
    for (int Index = 0; Index < ListView2->topLevelItemCount(); Index++)
    {
      ListView2->topLevelItem(Index)->setCheckState(0, Check ? Qt::Checked : Qt::Unchecked);
    }
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::Check(bool Check)
{
  {
    TAutoFlag MassFlag(FChangingItemMass);
    for (QTreeWidgetItem * Item : ListView2->selectedItems())
    {
      Item->setCheckState(0, Check ? Qt::Checked : Qt::Unchecked);
    }
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TSynchronizeChecklistDialog::SelectAll(bool Select, int Action, bool OnlyTheAction)
{
  QTreeWidgetItem * Result = NULL;
  for (int Index = 0; Index < ListView2->topLevelItemCount(); Index++)
  {
    QTreeWidgetItem * Item = ListView2->topLevelItem(Index);
    if (Action == 0)
    {
      Item->setSelected(Select);
      if (Result == NULL)
      {
        Result = Item;
      }
    }
    else
    {
      const TSynchronizeChecklist::TItem * ChecklistItem = GetChecklistItem(Item);
      bool WantedAction = (int(GetChecklistItemAction(ChecklistItem)) == Action);
      if (WantedAction || !OnlyTheAction)
      {
        Item->setSelected(Select && WantedAction);
        if (WantedAction && (Result == NULL))
        {
          Result = Item;
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TSynchronizeChecklistDialog::eventFilter(QObject * Object, QEvent * Event)
{
  if (Event->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent * MouseEvent = static_cast<QMouseEvent *>(Event);
    if (Object == ListView2->viewport())
    {
      // A click on the action reverses it (when not selecting)
      Qt::KeyboardModifiers Modifiers = MouseEvent->modifiers();
      if (!Modifiers.testFlag(Qt::ShiftModifier) && !Modifiers.testFlag(Qt::ControlModifier))
      {
        QModelIndex Index = ListView2->indexAt(MouseEvent->position().toPoint());
        if (Index.isValid() && (Index.column() == ImageColumnIndex))
        {
          UpdateControls();
          if (ReverseAction->isEnabled())
          {
            ReverseAction->trigger();
          }
        }
      }
    }
    else
    {
      for (int Action = 0; Action <= TSynchronizeChecklist::ActionCount; Action++)
      {
        if (Object == FStatusPanels[Action])
        {
          QTreeWidgetItem * Item = SelectAll(true, Action, MouseEvent->modifiers().testFlag(Qt::ControlModifier));
          if (Item != NULL)
          {
            ListView2->scrollToItem(Item);
            ListView2->setCurrentItem(Item, 0, QItemSelectionModel::NoUpdate);
            ListView2->setFocus();
          }
        }
      }
    }
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
const TSynchronizeChecklist::TItem * TSynchronizeChecklistDialog::GetChecklistItem(QTreeWidgetItem * Item)
{
  return static_cast<const TSynchronizeChecklist::TItem *>(Item->data(0, Qt::UserRole).value<void *>());
}
//---------------------------------------------------------------------------
TSynchronizeChecklist::TAction & TSynchronizeChecklistDialog::GetChecklistItemAction(
  const TSynchronizeChecklist::TItem * ChecklistItem)
{
  TActions::iterator i = FActions.find(ChecklistItem);
  if (i == FActions.end())
  {
    throw EInvalidOperation(L"");
  }
  return i->second;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::ReverseActionExecute()
{
  {
    TAutoFlag MassFlag(FChangingItemMass);
    for (QTreeWidgetItem * Item : ListView2->selectedItems())
    {
      const TSynchronizeChecklist::TItem * ChecklistItem = GetChecklistItem(Item);
      TSynchronizeChecklist::TAction & Action = GetChecklistItemAction(ChecklistItem);
      TSynchronizeChecklist::TAction NewAction = TSynchronizeChecklist::Reverse(Action);

      if (DebugAlwaysTrue(Action != NewAction))
      {
        bool Checked = (Item->checkState(0) == Qt::Checked);
        CountItemTotal(ChecklistItem, -1);
        if (Checked)
        {
          CountItem(ChecklistItem, -1);
        }

        Action = NewAction;

        CountItemTotal(ChecklistItem, 1);
        if (Checked)
        {
          // item size may differ with action (0 for delete, but non-0 for new file transfer)
          CountItem(ChecklistItem, 1);
        }

        TAutoFlag IgnoreFlag(FChangingItemIgnore);
        LoadItem(Item);
      }
    }
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::ProcessedItem(void * /*Token*/, const TSynchronizeChecklist::TItem * ChecklistItem)
{
  QTreeWidgetItem * Item = FChecklistToListViewMap[ChecklistItem];
  // We can get multiple triggeres on IFileOperationProgressSink for the same file, particularly multiple errors.
  // Do not know yet how (if possible at all) to tell what trigger is the last one, so we have to uncheck on the first trigger,
  // and ignore the others.
  Item->setCheckState(0, Qt::Unchecked);
  ListView2->scrollToItem(Item);
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::UpdatedSynchronizationChecklistItems(
  const TSynchronizeChecklist::TItemList & Items)
{
  for (const TSynchronizeChecklist::TItem * ChecklistItem : Items)
  {
    QTreeWidgetItem * Item = FChecklistToListViewMap[ChecklistItem];
    {
      TAutoFlag IgnoreFlag(FChangingItemIgnore);
      LoadItem(Item);
    }
    // When called from FOnSynchronize, we rely on a caller never to update size of an item that had size already,
    // otherwise we get it counted twice here.
    if (Item->checkState(0) == Qt::Checked)
    {
      CountItemSize(ChecklistItem, 1);
    }
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::DoSynchronize(bool Queue)
{
  ListView2->clearSelection();
  for (int Index = 0; Index < ListView2->topLevelItemCount(); Index++)
  {
    QTreeWidgetItem * Item = ListView2->topLevelItem(Index);
    const TSynchronizeChecklist::TItem * ChecklistItem = GetChecklistItem(Item);
    FChecklist->Update(ChecklistItem, (Item->checkState(0) == Qt::Checked), GetChecklistItemAction(ChecklistItem));
  }

  {
    TAutoFlag Flag(FSynchronizing);
    UpdateControls();
    try
    {
      if (Queue)
      {
        FOnQueueSynchronize(FToken);
      }
      else
      {
        FOnSynchronize(FToken,
          ::rtl::MethodPtr(this, &TSynchronizeChecklistDialog::ProcessedItem),
          ::rtl::MethodPtr(this, &TSynchronizeChecklistDialog::UpdatedSynchronizationChecklistItems));
      }
    }
    catch (Exception & E)
    {
      FException.reset(CloneException(&E));
    }
  }

  accept();
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::CalculateSize(bool All)
{
  TSynchronizeChecklist::TItemList Items;
  for (int Index = 0; Index < ListView2->topLevelItemCount(); Index++)
  {
    QTreeWidgetItem * Item = ListView2->topLevelItem(Index);
    if (All || Item->isSelected())
    {
      const TSynchronizeChecklist::TItem * ChecklistItem = GetChecklistItem(Item);
      Items.push_back(ChecklistItem);
      if (Item->checkState(0) == Qt::Checked)
      {
        CountItemSize(ChecklistItem, -1);
      }
    }
  }

  ::rtl::TFinally Finally([&]() { UpdatedSynchronizationChecklistItems(Items); });
  FOnSynchronizeChecklistCalculateSize(FChecklist, Items, FToken);
}
//---------------------------------------------------------------------------
bool DoSynchronizeChecklistDialog(TSynchronizeChecklist * Checklist, TSynchronizeMode Mode, int Params,
  const UnicodeString & Directory1, const UnicodeString & Directory2,
  TFullSynchronizeEvent OnSynchronize, TQueueSynchronizeEvent OnQueueSynchronize,
  TSynchronizeChecklistCalculateSize OnSynchronizeChecklistCalculateSize, void * Token)
{
  TSynchronizeChecklistDialog Dialog(Mode, Params, Directory1, Directory2,
    OnSynchronize, OnQueueSynchronize, OnSynchronizeChecklistCalculateSize, Token);
  return Dialog.Execute(Checklist);
}
//---------------------------------------------------------------------------
