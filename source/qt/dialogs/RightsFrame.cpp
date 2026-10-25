// The permissions editor (upstream's forms/Rights.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include "RightsFrame.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QScreen>
#include <QToolButton>
//---------------------------------------------------------------------------
static const wchar_t * AddXToDirectoriesSuffix = L"+x";
//---------------------------------------------------------------------------
TGrayedCheckBox::TGrayedCheckBox(const QString & Text, QWidget * Parent) :
  QCheckBox(Text, Parent)
{
}
//---------------------------------------------------------------------------
void TGrayedCheckBox::nextCheckState()
{
  switch (checkState())
  {
    case Qt::Unchecked:
      setCheckState(Qt::Checked);
      break;
    case Qt::Checked:
      setCheckState(isTristate() ? Qt::PartiallyChecked : Qt::Unchecked);
      break;
    case Qt::PartiallyChecked:
      setCheckState(Qt::Unchecked);
      break;
  }
}
//---------------------------------------------------------------------------
TRightsFrame::TRightsFrame(QWidget * Parent) :
  QFrame(Parent)
{
  setObjectName(QStringLiteral("RightsFrame"));
  FAllowAddXToDirectories = true;
  FInitialized = false;
  FAcl = false;
  FPopup = false;
  FOctalModified = false;
  FUpdating = 0;
  FSettingOctal = 0;

  QGridLayout * Layout = new QGridLayout(this);
  Layout->setContentsMargins(0, 0, 0, 0);
  Layout->setHorizontalSpacing(fontMetrics().averageCharWidth() * 2);

  struct TGroupControls
  {
    const char * ButtonName;
    QString ButtonCaption;
    TRights::TRightGroup Group;
    const char * Prefix;
  };
  const TGroupControls Groups[] =
  {
    { "OwnerButton", tr("&Owner"), TRights::rgUser, "Owner" },
    { "GroupButton", tr("&Group"), TRights::rgGroup, "Group" },
    { "OthersButton", tr("Ot&hers"), TRights::rgOther, "Others" },
  };
  const char * LevelNames[] = { "Read", "Write", "Execute" };
  const QString LevelCaptions[] = { QStringLiteral("R"), QStringLiteral("W"), QStringLiteral("X") };
  const QString LevelHints[] = { tr("Read"), tr("Write"), tr("Execute/Enter") };
  for (TRights::TRight Right = TRights::rrFirst; Right <= TRights::rrLast; Right = static_cast<TRights::TRight>(Right + 1))
  {
    FChecks[Right] = nullptr;
  }

  for (int GroupIndex = 0; GroupIndex < 3; GroupIndex++)
  {
    const TGroupControls & Group = Groups[GroupIndex];
    QToolButton * Button = new QToolButton(this);
    Button->setObjectName(QString::fromLatin1(Group.ButtonName));
    Button->setText(Group.ButtonCaption);
    Button->setAutoRaise(true);
    Button->setFocusPolicy(Qt::NoFocus);
    TRights::TRightGroup RightGroup = Group.Group;
    connect(Button, &QToolButton::clicked, this, [this, RightGroup]() { CycleRights(RightGroup); });
    Layout->addWidget(Button, GroupIndex, 0);
    for (int Level = TRights::rlRead; Level <= TRights::rlExec; Level++)
    {
      TRights::TRight Right = TRights::CalculateRight(RightGroup, static_cast<TRights::TRightLevel>(Level));
      TGrayedCheckBox * Check = new TGrayedCheckBox(LevelCaptions[Level], this);
      Check->setObjectName(QString::fromLatin1(Group.Prefix) + QString::fromLatin1(LevelNames[Level]) + QStringLiteral("Check"));
      Check->setToolTip(LevelHints[Level]);
      FChecks[Right] = Check;
      Layout->addWidget(Check, GroupIndex, 1 + Level);
    }
  }
  OwnerButton = findChild<QToolButton *>(QStringLiteral("OwnerButton"));
  GroupButton = findChild<QToolButton *>(QStringLiteral("GroupButton"));
  OthersButton = findChild<QToolButton *>(QStringLiteral("OthersButton"));

  struct TSpecialControls
  {
    const char * Name;
    QString Caption;
    TRights::TRight Right;
  };
  const TSpecialControls Specials[] =
  {
    { "SetUidCheck", tr("Set UID"), TRights::rrUserIDExec },
    { "SetGIDCheck", tr("Set GID"), TRights::rrGroupIDExec },
    { "StickyBitCheck", tr("Sticky bit"), TRights::rrStickyBit },
  };
  for (int Index = 0; Index < 3; Index++)
  {
    TGrayedCheckBox * Check = new TGrayedCheckBox(Specials[Index].Caption, this);
    Check->setObjectName(QString::fromLatin1(Specials[Index].Name));
    FChecks[Specials[Index].Right] = Check;
    Layout->addWidget(Check, Index, 4);
  }

  // Changes of the checks by the frame itself update the controls
  // explicitly
  for (QCheckBox * Check : FChecks)
  {
    connect(Check, &QCheckBox::clicked, this, [this]() { UpdateControls(); });
  }

  OctalLabel = new QLabel(tr("O&ctal:"), this);
  OctalLabel->setObjectName(QStringLiteral("OctalLabel"));
  Layout->addWidget(OctalLabel, 3, 0);
  OctalEdit = new QLineEdit(this);
  OctalEdit->setObjectName(QStringLiteral("OctalEdit"));
  OctalEdit->setMaxLength(4);
  OctalEdit->setMaximumWidth(fontMetrics().averageCharWidth() * 10);
  OctalLabel->setBuddy(OctalEdit);
  Layout->addWidget(OctalEdit, 3, 1, 1, 3);
  // Any change but the frame's own (an input method, the automation)
  connect(OctalEdit, &QLineEdit::textChanged, this, [this]()
  {
    if (FSettingOctal == 0)
    {
      OctalEditChanged();
    }
  });
  connect(OctalEdit, &QLineEdit::editingFinished, this, [this]() { OctalEditFinished(); });

  DirectoriesXCheck = new QCheckBox(tr("Add &X to directories"), this);
  DirectoriesXCheck->setObjectName(QStringLiteral("DirectoriesXCheck"));
  Layout->addWidget(DirectoriesXCheck, 4, 0, 1, 5);
  connect(DirectoriesXCheck, &QCheckBox::clicked, this, [this]() { UpdateControls(); });

  CloseButton = new QPushButton(tr("Close"), this);
  CloseButton->setObjectName(QStringLiteral("CloseButton"));
  CloseButton->setVisible(false);
  Layout->addWidget(CloseButton, 4, 4, Qt::AlignRight);
  connect(CloseButton, &QPushButton::clicked, this, [this]() { CloseUp(); });
  Layout->setColumnStretch(5, 1);

  NoRightsAction = CreateAction("NoRightsAction", tr("&No Rights"), QString(), QKeySequence(Qt::CTRL | Qt::Key_N));
  DefaultRightsAction = CreateAction("DefaultRightsAction", tr("&Default Rights"), QString(), QKeySequence(Qt::CTRL | Qt::Key_D));
  AllRightsAction = CreateAction("AllRightsAction", tr("&All Rights"), QString(), QKeySequence(Qt::CTRL | Qt::Key_A));
  LeaveRightsAsIsAction = CreateAction("LeaveRightsAsIsAction", tr("&Leave as Is"), QString(), QKeySequence(Qt::CTRL | Qt::Key_L));
  CopyTextAction = CreateAction("CopyTextAction", tr("&Copy as Text"), QStringLiteral("edit-copy"), QKeySequence(Qt::CTRL | Qt::Key_C));
  CopyOctalAction = CreateAction("CopyOctalAction", tr("Copy as &Octal"), QString(), QKeySequence(Qt::CTRL | Qt::Key_O));
  PasteAction = CreateAction("PasteAction", tr("&Paste"), QStringLiteral("edit-paste"), QKeySequence(Qt::CTRL | Qt::Key_V));
  for (QAction * Action : { NoRightsAction, DefaultRightsAction, AllRightsAction, LeaveRightsAsIsAction })
  {
    Action->setCheckable(true);
  }

  SetRights(TRights());
}
//---------------------------------------------------------------------------
QAction * TRightsFrame::CreateAction(const char * Name, const QString & Caption, const QString & IconName,
  const QKeySequence & ShortCut)
{
  QAction * Action = new QAction(Caption, this);
  Action->setObjectName(QString::fromLatin1(Name));
  if (!IconName.isEmpty())
  {
    Action->setIcon(QIcon::fromTheme(IconName));
  }
  Action->setShortcut(ShortCut);
  // Only while the frame has the focus (the dialog has other uses of
  // the keys)
  Action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  addAction(Action);
  connect(Action, &QAction::triggered, this, [this, Action]() { ActionExecute(Action); });
  return Action;
}
//---------------------------------------------------------------------------
void TRightsFrame::SetState(TRights::TRight Right, TRights::TState Value)
{
  DebugAssert(((Value == TRights::rsNo) || (Value == TRights::rsYes) || (Value == TRights::rsUndef)));
  QCheckBox * Check = FChecks[Right];
  switch (Value)
  {
    case TRights::rsNo: Check->setCheckState(Qt::Unchecked); break;
    case TRights::rsYes: Check->setCheckState(Qt::Checked); break;
    case TRights::rsUndef: Check->setCheckState(Qt::PartiallyChecked); break;
  }
}
//---------------------------------------------------------------------------
TRights::TState TRightsFrame::GetState(TRights::TRight Right) const
{
  switch (FChecks[Right]->checkState())
  {
    case Qt::Unchecked: return TRights::rsNo;
    case Qt::Checked: return TRights::rsYes;
    case Qt::PartiallyChecked:
    default: return TRights::rsUndef;
  }
}
//---------------------------------------------------------------------------
void TRightsFrame::SetRights(const TRights & Value)
{
  bool Changed = (GetAllowUndef() != Value.AllowUndef);
  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    SetAllowUndef(true); // temporarily
    for (int Right = TRights::rrFirst; Right <= TRights::rrLast; Right++)
    {
      TRights::TRight R = static_cast<TRights::TRight>(Right);
      if (GetState(R) != Value.RightUndef[R])
      {
        SetState(R, Value.RightUndef[R]);
        Changed = true;
      }
    }
    SetAllowUndef(Value.AllowUndef);
  }
  if (Changed || !FInitialized)
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
TRights TRightsFrame::GetRights()
{
  TRights Result;
  Result.AllowUndef = GetAllowUndef();
  for (int Right = TRights::rrFirst; Right <= TRights::rrLast; Right++)
  {
    TRights::TRight R = static_cast<TRights::TRight>(Right);
    Result.RightUndef[R] = GetState(R);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRightsFrame::SetAllowUndef(bool Value)
{
  for (QCheckBox * Check : FChecks)
  {
    Check->setTristate(Value);
  }
  UpdateActions();
}
//---------------------------------------------------------------------------
bool TRightsFrame::GetAllowUndef() const
{
  return FChecks[TRights::rrFirst]->isTristate();
}
//---------------------------------------------------------------------------
void TRightsFrame::SetAddXToDirectories(bool Value)
{
  DirectoriesXCheck->setChecked(Value);
}
//---------------------------------------------------------------------------
bool TRightsFrame::GetAddXToDirectories() const
{
  return DirectoriesXCheck->isChecked();
}
//---------------------------------------------------------------------------
void TRightsFrame::SetAllowAddXToDirectories(bool Value)
{
  if ((FAllowAddXToDirectories != Value) || !FInitialized)
  {
    FAllowAddXToDirectories = Value;
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
bool TRightsFrame::DirectoriesXEffective()
{
  return !((GetRights().NumberSet & TRights::rfExec) == TRights::rfExec);
}
//---------------------------------------------------------------------------
void TRightsFrame::UpdateControls()
{
  if (FUpdating > 0)
  {
    return;
  }
  if (!OctalEdit->hasFocus())
  {
    UpdateOctalEdit();
  }
  DirectoriesXCheck->setVisible(FAllowAddXToDirectories);
  DirectoriesXCheck->setEnabled(DirectoriesXEffective());
  UpdateActions();
  FInitialized = true;
  DoChange();
}
//---------------------------------------------------------------------------
void TRightsFrame::UpdateActions()
{
  TRights R = GetRights();
  NoRightsAction->setEnabled(!FAcl);
  NoRightsAction->setChecked(!R.IsUndef && (R.NumberSet == TRights::rfNo));
  DefaultRightsAction->setEnabled(!FAcl);
  DefaultRightsAction->setChecked(!R.IsUndef && (R.NumberSet == TRights::rfDefault));
  AllRightsAction->setEnabled(!FAcl);
  AllRightsAction->setChecked(!R.IsUndef && (R.NumberSet == TRights::rfAll));
  LeaveRightsAsIsAction->setEnabled(!FAcl && R.AllowUndef);
  LeaveRightsAsIsAction->setVisible(R.AllowUndef);
  LeaveRightsAsIsAction->setChecked((R.NumberSet == TRights::rfNo) && (R.NumberUnset == TRights::rfNo));
  CopyTextAction->setEnabled(!FAcl && !R.IsUndef);
  CopyOctalAction->setEnabled(!FAcl && !R.IsUndef);
  PasteAction->setEnabled(!FAcl);
}
//---------------------------------------------------------------------------
void TRightsFrame::DoChange()
{
  if (OnChange)
  {
    OnChange();
  }
}
//---------------------------------------------------------------------------
void TRightsFrame::changeEvent(QEvent * Event)
{
  QFrame::changeEvent(Event);
  if (Event->type() == QEvent::EnabledChange)
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TRightsFrame::CycleRights(TRights::TRightGroup RightGroup)
{
  int Last = FAcl ? TRights::rlLastAcl : TRights::rlLastNormal;
  TRights::TState State = TRights::rsNo;
  bool Any = false;
  bool Same = true;
  for (int RightLevelI = TRights::rlFirst; RightLevelI <= Last; RightLevelI++)
  {
    TRights::TRightLevel RightLevel = static_cast<TRights::TRightLevel>(RightLevelI);
    TRights::TRight Right = TRights::CalculateRight(RightGroup, RightLevel);
    if (!FChecks[Right]->isHidden())
    {
      TRights::TState RightState = GetState(Right);
      if (!Any)
      {
        State = RightState;
        Any = true;
      }
      else if (State != RightState)
      {
        Same = false;
        break;
      }
    }
  }

  if (DebugAlwaysTrue(Any))
  {
    if (!Same)
    {
      State = TRights::rsYes;
    }
    else
    {
      switch (State)
      {
        case TRights::rsYes:
          State = TRights::rsNo;
          break;

        case TRights::rsNo:
          State = GetAllowUndef() ? TRights::rsUndef : TRights::rsYes;
          break;

        case TRights::rsUndef:
          State = TRights::rsYes;
          break;
      }
    }

    {
      TAutoNestingCounter UpdatingCounter(FUpdating);
      for (int RightLevelI = TRights::rlFirst; RightLevelI <= Last; RightLevelI++)
      {
        TRights::TRightLevel RightLevel = static_cast<TRights::TRightLevel>(RightLevelI);
        SetState(TRights::CalculateRight(RightGroup, RightLevel), State);
      }
    }
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TRightsFrame::ActionExecute(QAction * Action)
{
  bool Changed = true;
  TRights R = GetRights();
  R.Number = TRights::rfNo;

  if (Action == NoRightsAction)
  {
    R = TRights::rfNo;
  }
  else if (Action == DefaultRightsAction)
  {
    R = TRights::rfDefault;
  }
  else if (Action == AllRightsAction)
  {
    R = TRights::rfAll;
  }
  else if (Action == LeaveRightsAsIsAction)
  {
    R.AllUndef();
  }
  else if (Action == CopyTextAction)
  {
    QApplication::clipboard()->setText(ToQString(GetText()));
    Changed = false;
  }
  else if (Action == CopyOctalAction)
  {
    TRights Rights = GetRights();
    if (DebugAlwaysTrue(!Rights.IsUndef))
    {
      QApplication::clipboard()->setText(ToQString(Rights.Octal));
    }
    Changed = false;
  }
  else if (Action == PasteAction)
  {
    QString Text = QApplication::clipboard()->text();
    if (!Text.isEmpty())
    {
      SetText(ToUnicodeString(Text.trimmed()));
    }
    // As upstream, even when nothing changed, the parent reacts
    DoChange();
    UpdateOctalEdit();
    Changed = false;
  }

  if (Changed)
  {
    SetRights(R);
    UpdateOctalEdit();
    DoChange();
  }
}
//---------------------------------------------------------------------------
void TRightsFrame::contextMenuEvent(QContextMenuEvent * Event)
{
  if (!FAcl)
  {
    UpdateActions();
    const QMimeData * MimeData = QApplication::clipboard()->mimeData();
    PasteAction->setEnabled((MimeData != nullptr) && MimeData->hasText());
    QMenu Menu(this);
    Menu.setObjectName(QStringLiteral("RightsPopup"));
    Menu.addAction(NoRightsAction);
    Menu.addAction(DefaultRightsAction);
    Menu.addAction(AllRightsAction);
    Menu.addAction(LeaveRightsAsIsAction);
    Menu.addSeparator();
    Menu.addAction(CopyTextAction);
    Menu.addAction(CopyOctalAction);
    Menu.addAction(PasteAction);
    Menu.exec(Event->globalPos());
    PasteAction->setEnabled(!FAcl);
  }
  Event->accept();
}
//---------------------------------------------------------------------------
void TRightsFrame::UpdateOctalEdit()
{
  TRights R = GetRights();
  {
    TAutoNestingCounter SettingCounter(FSettingOctal);
    OctalEdit->setText(R.IsUndef ? QString() : ToQString(R.Octal));
  }
  FOctalModified = false;
  OctalEdit->selectAll();
}
//---------------------------------------------------------------------------
void TRightsFrame::UpdateByOctal()
{
  if (!OctalEdit->text().isEmpty())
  {
    TRights R = GetRights();
    R.Octal = ToUnicodeString(OctalEdit->text());
    SetRights(R);
  }
  UpdateControls();
  FOctalModified = false;
}
//---------------------------------------------------------------------------
void TRightsFrame::OctalEditChanged()
{
  FOctalModified = true;
  if (OctalEdit->text().length() >= 3)
  {
    try
    {
      UpdateByOctal();
    }
    catch (Exception &)
    {
      // Not complete yet, or invalid; reported when the edit is left
      FOctalModified = true;
    }
  }
}
//---------------------------------------------------------------------------
void TRightsFrame::OctalEditFinished()
{
  if (FOctalModified)
  {
    // The text is invalid now, as a valid one was taken already
    try
    {
      UpdateByOctal();
    }
    catch (Exception & E)
    {
      // The value is taken back, rather than keeping the focus in the edit
      UpdateOctalEdit();
      ShowExtendedException(&E);
    }
  }
  else
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
UnicodeString TRightsFrame::GetText()
{
  UnicodeString Result = GetRights().Text;
  if (FAllowAddXToDirectories && DirectoriesXEffective() && GetAddXToDirectories())
  {
    Result = FORMAT(L"%s (%s)", (Result, AddXToDirectoriesSuffix));
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRightsFrame::SetText(const UnicodeString & Value)
{
  if (GetText() != Value)
  {
    UnicodeString RightsStr = Value;

    UnicodeString Suffix(AddXToDirectoriesSuffix);
    int P = RightsStr.LowerCase().Pos(Suffix);
    bool AAddXToDirectories = (P > 0);
    if (AAddXToDirectories)
    {
      RightsStr.Delete(P, Suffix.Length());
    }
    RightsStr = DeleteChar(DeleteChar(RightsStr, L'('), L')').Trim();
    TRights R = GetRights();
    if (((RightsStr.Length() == 3) || (RightsStr.Length() == 4)) &&
        IsNumber(RightsStr))
    {
      R.Octal = RightsStr;
    }
    else
    {
      R.Text = RightsStr;
    }

    SetRights(R);
    SetAddXToDirectories(AAddXToDirectories);
  }
}
//---------------------------------------------------------------------------
void TRightsFrame::DisplayAsAcl(TRights::TRight ReadRight, TRights::TRight WriteRight,
  TRights::TRight ExecRight, TRights::TRight SpecialRight)
{
  DebugUsedParam(ReadRight);
  FChecks[WriteRight]->setVisible(false);

  QCheckBox * ReadAclCheck = FChecks[ExecRight];
  ReadAclCheck->setText(QStringLiteral("R ACL"));
  ReadAclCheck->setToolTip(ToQString(LoadStr(PROPERTIES_S3_R_ACL_HINT)));

  QCheckBox * WriteAclCheck = FChecks[SpecialRight];
  WriteAclCheck->setText(QStringLiteral("W ACL"));
  WriteAclCheck->setToolTip(ToQString(LoadStr(PROPERTIES_S3_W_ACL_HINT)));
}
//---------------------------------------------------------------------------
void TRightsFrame::DisplayAsAcl()
{
  SetAllowAddXToDirectories(false);
  OctalLabel->setVisible(false);
  OctalEdit->setVisible(false);
  DisplayAsAcl(TRights::rrUserRead, TRights::rrUserWrite, TRights::rrUserExec, TRights::rrUserIDExec);
  DisplayAsAcl(TRights::rrGroupRead, TRights::rrGroupWrite, TRights::rrGroupExec, TRights::rrGroupIDExec);
  DisplayAsAcl(TRights::rrOtherRead, TRights::rrOtherWrite, TRights::rrOtherExec, TRights::rrStickyBit);
  GroupButton->setText(ToQString(LoadStr(PROPERTIES_S3_USERS)));
  OthersButton->setText(ToQString(LoadStr(PROPERTIES_S3_EVERYONE)));
  FAcl = true;
  UpdateActions();
}
//---------------------------------------------------------------------------
void TRightsFrame::SetPopup(bool Value)
{
  if (FPopup != Value)
  {
    FPopup = Value;
    if (FPopup)
    {
      setWindowFlags(Qt::Popup);
      setFrameShape(QFrame::StyledPanel);
      setAutoFillBackground(true);
      layout()->setContentsMargins(6, 6, 6, 6);
    }
    CloseButton->setVisible(FPopup);
    CloseButton->setDefault(FPopup);
    // Return in the edits closes the popup
    for (QWidget * Widget : findChildren<QWidget *>())
    {
      Widget->installEventFilter(this);
    }
  }
}
//---------------------------------------------------------------------------
void TRightsFrame::DropDown(QWidget * PopupParent)
{
  DebugAssert(FPopup);
  adjustSize();
  QPoint Origin = PopupParent->mapToGlobal(QPoint(0, PopupParent->height()));
  QScreen * Screen = PopupParent->screen();
  if ((Screen != nullptr) && (Origin.x() + width() > Screen->availableGeometry().right()))
  {
    Origin.rx() += PopupParent->width() - width();
  }
  move(Origin);
  show();
  FChecks[TRights::rrUserRead]->setFocus();
}
//---------------------------------------------------------------------------
void TRightsFrame::CloseUp()
{
  DebugAssert(FPopup);
  if (OctalEdit->hasFocus())
  {
    OctalEditFinished();
  }
  hide();
}
//---------------------------------------------------------------------------
void TRightsFrame::hideEvent(QHideEvent * Event)
{
  QFrame::hideEvent(Event);
  if (FPopup && OnCloseUp)
  {
    OnCloseUp();
  }
}
//---------------------------------------------------------------------------
bool TRightsFrame::eventFilter(QObject * Object, QEvent * Event)
{
  if (FPopup && (Event->type() == QEvent::KeyPress))
  {
    QKeyEvent * KeyEvent = static_cast<QKeyEvent *>(Event);
    if (((KeyEvent->key() == Qt::Key_Return) || (KeyEvent->key() == Qt::Key_Enter)) &&
        (KeyEvent->modifiers() == Qt::NoModifier))
    {
      CloseUp();
      return true;
    }
  }
  return QFrame::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
