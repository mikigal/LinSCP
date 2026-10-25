// Keeping the remote directory up to date (upstream's TSynchronizeDialog,
// forms/Synchronize.dfm): the directories, the options and the transfer
// settings; while it watches for changes, the log of what was detected and
// uploaded.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Terminal.h>
#include <TerminalManager.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"
#include "RightsFrame.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDialog>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// An entry of the log (upstream's TLogItemData)
class TLogItem : public QTreeWidgetItem
{
public:
  TSynchronizeLogEntry Entry;
  UnicodeString Message;
  std::unique_ptr<TStrings> MoreMessages;
  TQueryType Type;
};
//---------------------------------------------------------------------------
class TSynchronizeDialog : public QDialog
{
public:
  TSynchronizeDialog();
  virtual ~TSynchronizeDialog();

  void Init(TSynchronizeStartStopEvent OnStartStop, TGetSynchronizeOptionsEvent OnGetOptions,
    TSynchronizeSessionLog OnSynchronizeSessionLog, TFeedSynchronizeError & OnFeedSynchronizeError,
    std::function<void()> & OnSynchronizeAbort);
  bool Execute();

  TSynchronizeParamType GetParams();
  void SetParams(const TSynchronizeParamType & Value);
  TCopyParamType GetCopyParams();
  void SetCopyParams(const TCopyParamType & Value);
  bool GetSaveSettings() { return SaveSettingsCheck->isChecked(); }
  void SetSaveSettings(bool Value) { SaveSettingsCheck->setChecked(Value); }
  void SetOptions(int Value);
  void SetCopyParamAttrs(int Value) { FCopyParamAttrs = Value; }

  static const int MaxLogItems;

protected:
  virtual void reject() override;
  virtual void keyPressEvent(QKeyEvent * Event) override;
  virtual void resizeEvent(QResizeEvent * Event) override;
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  TSynchronizeStartStopEvent FOnStartStop;
  TGetSynchronizeOptionsEvent FOnGetOptions;
  TSynchronizeSessionLog FOnSynchronizeSessionLog;
  TFeedSynchronizeError * FOnFeedSynchronizeError;
  TSynchronizeParamType FParams;
  TCopyParamType FCopyParams;
  UnicodeString FPreset;
  std::unique_ptr<TSynchronizeOptions> FSynchronizeOptions;
  int FOptions;
  int FCopyParamAttrs;
  bool FSynchronizing;
  bool FAbort;
  bool FClose;
  bool FMinimizedByMe;

  QGroupBox * DirectoriesGroup;
  THistoryComboBox * LocalDirectoryEdit;
  QPushButton * LocalDirectoryBrowseButton;
  THistoryComboBox * RemoteDirectoryEdit;
  QGroupBox * OptionsGroup;
  QCheckBox * SynchronizeDeleteCheck;
  QCheckBox * SynchronizeExistingOnlyCheck;
  QCheckBox * SynchronizeRecursiveCheck;
  QCheckBox * SynchronizeSelectedOnlyCheck;
  QCheckBox * ContinueOnErrorCheck;
  TGrayedCheckBox * SynchronizeSynchronizeCheck;
  QCheckBox * SaveSettingsCheck;
  QGroupBox * CopyParamGroup;
  QLabel * CopyParamLabel;
  QPushButton * TransferSettingsButton;
  QToolButton * TransferSettingsDropDownButton;
  QPushButton * StartButton;
  QPushButton * StopButton;
  QPushButton * MinimizeButton;
  QToolButton * MinimizeDropDownButton;
  QMenu * MinimizeMenu;
  QPushButton * CancelButton;
  QWidget * LogPanel;
  QTreeWidget * LogView;

  void UpdateControls();
  int ActualCopyParamAttrs();
  void CopyParamGroupClick();
  void CopyParamListPopup(const QPoint & Pos);
  void LocalDirectoryBrowseButtonClick();
  void Start();
  void Stop();
  void OnlyStop();
  void DoStartStop(bool Start, bool Synchronize);
  void Abort(bool Close);
  void AbortStop();
  void SaveHistory();
  void Minimize(bool ToTray);
  void DoAbort(TObject * Sender, bool Close);
  void DoLog(TSynchronizeController * Controller, TSynchronizeLogEntry Entry, const UnicodeString Message);
  void DoLogInternal(TSynchronizeLogEntry Entry, const UnicodeString & Message,
    TStrings * MoreMessages, TQueryType Type);
  void FeedSynchronizeError(const UnicodeString & Message, TStrings * MoreMessages, TQueryType Type);
  void ClearLog();
  void CopyLog();
  void LogViewDblClick();
};
//---------------------------------------------------------------------------
const int TSynchronizeDialog::MaxLogItems = 1000;
//---------------------------------------------------------------------------
QCheckBox * CreateCheck(QWidget * Parent, const char * Name, const QString & Caption)
{
  QCheckBox * Check = new QCheckBox(Caption, Parent);
  Check->setObjectName(QString::fromLatin1(Name));
  return Check;
}
//---------------------------------------------------------------------------
QPushButton * CreateButton(QWidget * Parent, const char * Name, const QString & Caption)
{
  QPushButton * Button = new QPushButton(Caption, Parent);
  Button->setObjectName(QString::fromLatin1(Name));
  Button->setAutoDefault(false);
  return Button;
}
//---------------------------------------------------------------------------
TSynchronizeDialog::TSynchronizeDialog() :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("SynchronizeDialog"));
  FOnFeedSynchronizeError = nullptr;
  FParams.Params = 0;
  FParams.Options = 0;
  FOptions = 0;
  FCopyParamAttrs = 0;
  FSynchronizing = false;
  FAbort = false;
  FClose = false;
  FMinimizedByMe = false;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  DirectoriesGroup = new QGroupBox(tr("Directories"), this);
  DirectoriesGroup->setObjectName(QStringLiteral("DirectoriesGroup"));
  QGridLayout * DirectoriesLayout = new QGridLayout(DirectoriesGroup);
  QLabel * LocalDirectoryLabel = new QLabel(tr("&Watch for changes in the local directory:"), DirectoriesGroup);
  LocalDirectoryLabel->setObjectName(QStringLiteral("LocalDirectoryLabel"));
  DirectoriesLayout->addWidget(LocalDirectoryLabel, 0, 0, 1, 2);
  LocalDirectoryEdit = new THistoryComboBox(DirectoriesGroup);
  LocalDirectoryEdit->setObjectName(QStringLiteral("LocalDirectoryEdit"));
  LocalDirectoryLabel->setBuddy(LocalDirectoryEdit);
  DirectoriesLayout->addWidget(LocalDirectoryEdit, 1, 0);
  LocalDirectoryBrowseButton = CreateButton(DirectoriesGroup, "LocalDirectoryBrowseButton", tr("B&rowse..."));
  connect(LocalDirectoryBrowseButton, &QPushButton::clicked, this, [this]() { LocalDirectoryBrowseButtonClick(); });
  DirectoriesLayout->addWidget(LocalDirectoryBrowseButton, 1, 1);
  QLabel * RemoteDirectoryLabel =
    new QLabel(tr("... &and automatically reflect them on the remote directory:"), DirectoriesGroup);
  RemoteDirectoryLabel->setObjectName(QStringLiteral("RemoteDirectoryLabel"));
  DirectoriesLayout->addWidget(RemoteDirectoryLabel, 2, 0, 1, 2);
  RemoteDirectoryEdit = new THistoryComboBox(DirectoriesGroup);
  RemoteDirectoryEdit->setObjectName(QStringLiteral("RemoteDirectoryEdit"));
  RemoteDirectoryLabel->setBuddy(RemoteDirectoryEdit);
  DirectoriesLayout->addWidget(RemoteDirectoryEdit, 3, 0, 1, 2);
  DirectoriesLayout->setColumnStretch(0, 1);
  for (THistoryComboBox * Edit : { LocalDirectoryEdit, RemoteDirectoryEdit })
  {
    connect(Edit, &QComboBox::currentTextChanged, this, [this]() { UpdateControls(); });
  }
  MainLayout->addWidget(DirectoriesGroup);

  OptionsGroup = new QGroupBox(tr("Synchronize options"), this);
  OptionsGroup->setObjectName(QStringLiteral("OptionsGroup"));
  QGridLayout * OptionsLayout = new QGridLayout(OptionsGroup);
  SynchronizeDeleteCheck = CreateCheck(OptionsGroup, "SynchronizeDeleteCheck", tr("&Delete files"));
  OptionsLayout->addWidget(SynchronizeDeleteCheck, 0, 0);
  SynchronizeExistingOnlyCheck =
    CreateCheck(OptionsGroup, "SynchronizeExistingOnlyCheck", tr("&Existing files only"));
  OptionsLayout->addWidget(SynchronizeExistingOnlyCheck, 0, 1);
  SynchronizeRecursiveCheck =
    CreateCheck(OptionsGroup, "SynchronizeRecursiveCheck", tr("Update s&ubdirectories"));
  OptionsLayout->addWidget(SynchronizeRecursiveCheck, 1, 0);
  SynchronizeSelectedOnlyCheck =
    CreateCheck(OptionsGroup, "SynchronizeSelectedOnlyCheck", tr("Selected files o&nly"));
  OptionsLayout->addWidget(SynchronizeSelectedOnlyCheck, 1, 1);
  ContinueOnErrorCheck = CreateCheck(OptionsGroup, "ContinueOnErrorCheck", tr("Continue on &error"));
  OptionsLayout->addWidget(ContinueOnErrorCheck, 2, 0);
  SynchronizeSynchronizeCheck = new TGrayedCheckBox(tr("Synchronize on s&tart"), OptionsGroup);
  SynchronizeSynchronizeCheck->setObjectName(QStringLiteral("SynchronizeSynchronizeCheck"));
  // The grayed state is "ask" (soSynchronizeAsk)
  SynchronizeSynchronizeCheck->setTristate(true);
  OptionsLayout->addWidget(SynchronizeSynchronizeCheck, 2, 1);
  SaveSettingsCheck = CreateCheck(OptionsGroup, "SaveSettingsCheck", tr("Use same &options next time"));
  OptionsLayout->addWidget(SaveSettingsCheck, 3, 0);
  MainLayout->addWidget(OptionsGroup);

  CopyParamGroup = new QGroupBox(tr("Transfer settings"), this);
  CopyParamGroup->setObjectName(QStringLiteral("CopyParamGroup"));
  CopyParamGroup->installEventFilter(this);
  QVBoxLayout * CopyParamLayout = new QVBoxLayout(CopyParamGroup);
  CopyParamLabel = new QLabel(CopyParamGroup);
  CopyParamLabel->setObjectName(QStringLiteral("CopyParamLabel"));
  CopyParamLabel->setTextFormat(Qt::PlainText);
  CopyParamLabel->setWordWrap(true);
  CopyParamLabel->setCursor(Qt::PointingHandCursor);
  CopyParamLabel->installEventFilter(this);
  CopyParamLayout->addWidget(CopyParamLabel);
  MainLayout->addWidget(CopyParamGroup);

  for (QCheckBox * Check : OptionsGroup->findChildren<QCheckBox *>())
  {
    connect(Check, &QCheckBox::toggled, this, [this]() { UpdateControls(); });
  }

  QHBoxLayout * ButtonsLayout = new QHBoxLayout();
  TransferSettingsButton = CreateButton(this, "TransferSettingsButton", tr("Transfer settin&gs..."));
  connect(TransferSettingsButton, &QPushButton::clicked, this, [this]() { CopyParamGroupClick(); });
  ButtonsLayout->addWidget(TransferSettingsButton);
  TransferSettingsDropDownButton = new QToolButton(this);
  TransferSettingsDropDownButton->setObjectName(QStringLiteral("TransferSettingsDropDownButton"));
  TransferSettingsDropDownButton->setArrowType(Qt::DownArrow);
  connect(TransferSettingsDropDownButton, &QToolButton::clicked, this, [this]()
  {
    CopyParamListPopup(TransferSettingsButton->mapToGlobal(QPoint(0, TransferSettingsButton->height())));
  });
  ButtonsLayout->addWidget(TransferSettingsDropDownButton);
  ButtonsLayout->addStretch(1);
  StartButton = CreateButton(this, "StartButton", tr("&Start"));
  connect(StartButton, &QPushButton::clicked, this, [this]() { Start(); });
  ButtonsLayout->addWidget(StartButton);
  StopButton = CreateButton(this, "StopButton", tr("&Stop"));
  StopButton->setVisible(false);
  connect(StopButton, &QPushButton::clicked, this, [this]() { Stop(); });
  ButtonsLayout->addWidget(StopButton);
  // Upstream's split button: the drop down minimizes to the system tray
  // too (unless that is what the button does already)
  MinimizeButton = CreateButton(this, "MinimizeButton", tr("&Minimize"));
  connect(MinimizeButton, &QPushButton::clicked, this, [this]() { Minimize(false); });
  ButtonsLayout->addWidget(MinimizeButton);
  MinimizeDropDownButton = new QToolButton(this);
  MinimizeDropDownButton->setObjectName(QStringLiteral("MinimizeDropDownButton"));
  MinimizeDropDownButton->setArrowType(Qt::DownArrow);
  MinimizeMenu = new QMenu(this);
  MinimizeMenu->setObjectName(QStringLiteral("MinimizeMenu"));
  QAction * MinimizeItem = MinimizeMenu->addAction(tr("&Minimize"));
  MinimizeItem->setObjectName(QStringLiteral("Minimize1"));
  connect(MinimizeItem, &QAction::triggered, this, [this]() { Minimize(false); });
  QAction * MinimizeToTrayItem = MinimizeMenu->addAction(tr("Minimize to System &Tray"));
  MinimizeToTrayItem->setObjectName(QStringLiteral("MinimizetoTray1"));
  connect(MinimizeToTrayItem, &QAction::triggered, this, [this]() { Minimize(true); });
  connect(MinimizeDropDownButton, &QToolButton::clicked, this, [this]()
  {
    MinimizeMenu->popup(MinimizeDropDownButton->mapToGlobal(QPoint(0, MinimizeDropDownButton->height())));
  });
  ButtonsLayout->addWidget(MinimizeDropDownButton);
  CancelButton = CreateButton(this, "CancelButton", tr("Close"));
  connect(CancelButton, &QPushButton::clicked, this, &QDialog::reject);
  ButtonsLayout->addWidget(CancelButton);
  MainLayout->addLayout(ButtonsLayout);

  LogPanel = new QWidget(this);
  LogPanel->setObjectName(QStringLiteral("LogPanel"));
  QVBoxLayout * LogLayout = new QVBoxLayout(LogPanel);
  LogLayout->setContentsMargins(0, 0, 0, 0);
  LogView = new QTreeWidget(LogPanel);
  LogView->setObjectName(QStringLiteral("LogView"));
  LogView->setColumnCount(2);
  LogView->setHeaderHidden(true);
  LogView->setRootIsDecorated(false);
  LogView->setUniformRowHeights(true);
  LogView->setSelectionMode(QAbstractItemView::SingleSelection);
  LogView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  LogView->header()->setStretchLastSection(true);
  LogView->setMinimumHeight(fontMetrics().height() * 8);
  LogView->installEventFilter(this);
  connect(LogView, &QTreeWidget::itemDoubleClicked, this, [this]() { LogViewDblClick(); });
  LogLayout->addWidget(LogView);
  LogPanel->setVisible(false);
  MainLayout->addWidget(LogPanel, 1);

  // Not on the dialog itself, so that its minimum size follows the layout
  // (with the log or without)
  LocalDirectoryEdit->setMinimumWidth(fontMetrics().averageCharWidth() * 60);
}
//---------------------------------------------------------------------------
TSynchronizeDialog::~TSynchronizeDialog()
{
  // if application is closing reject might not get called
  // (this particularly happens if last terminal is disconnected while dialog is
  // open)
  if (FSynchronizing)
  {
    OnlyStop();
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::Init(TSynchronizeStartStopEvent OnStartStop, TGetSynchronizeOptionsEvent OnGetOptions,
  TSynchronizeSessionLog OnSynchronizeSessionLog, TFeedSynchronizeError & OnFeedSynchronizeError,
  std::function<void()> & OnSynchronizeAbort)
{
  FOnStartStop = OnStartStop;
  FOnGetOptions = OnGetOptions;
  FOnSynchronizeSessionLog = OnSynchronizeSessionLog;
  FOnFeedSynchronizeError = &OnFeedSynchronizeError;
  OnSynchronizeAbort = [this]() { Abort(true); };
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::FeedSynchronizeError(const UnicodeString & Message, TStrings * MoreMessages, TQueryType Type)
{
  DoLogInternal(slContinuedError, Message, MoreMessages, Type);
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::UpdateControls()
{
  StartButton->setEnabled(
    !LocalDirectoryEdit->currentText().isEmpty() && !RemoteDirectoryEdit->currentText().isEmpty());
  QPushButton * OldButton = FSynchronizing ? StartButton : StopButton;
  QPushButton * NewButton = FSynchronizing ? StopButton : StartButton;
  if (!NewButton->isVisibleTo(this) || OldButton->isVisibleTo(this))
  {
    bool Focused = OldButton->hasFocus();
    NewButton->setVisible(true);
    if (Focused)
    {
      NewButton->setFocus();
    }
    OldButton->setDefault(false);
    NewButton->setDefault(true);
    OldButton->setVisible(false);
  }
  UnicodeString Title = LoadStr(FSynchronizing ? SYNCHRONIZE_SYCHRONIZING : SYNCHRONIZE_TITLE);
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  TTerminal * Terminal = (Manager != NULL) ? Manager->ActiveTerminal : NULL;
  UnicodeString SessionName = (Terminal != NULL) ? Terminal->SessionData->SessionName : UnicodeString();
  setWindowTitle(ToQString(FormatMainFormCaption(Title, SessionName)));
  TransferSettingsButton->setEnabled(!FSynchronizing);
  TransferSettingsDropDownButton->setEnabled(!FSynchronizing);
  TransferSettingsDropDownButton->setVisible(FLAGCLEAR(FOptions, soDoNotUsePresets));
  CancelButton->setVisible(!FSynchronizing || FLAGSET(FOptions, soNoMinimize));
  CancelButton->setEnabled(!FSynchronizing);
  DirectoriesGroup->setEnabled(!FSynchronizing);
  OptionsGroup->setEnabled(!FSynchronizing);
  CopyParamGroup->setEnabled(!FSynchronizing);
  MinimizeButton->setVisible(FSynchronizing && FLAGCLEAR(FOptions, soNoMinimize));
  MinimizeDropDownButton->setVisible(
    MinimizeButton->isVisible() && !AppConfiguration->MinimizeToTray && IsSystemTrayAvailable());
  SynchronizeSelectedOnlyCheck->setEnabled(OptionsGroup->isEnabled() && FLAGSET(FOptions, soAllowSelectedOnly));

  UnicodeString InfoStr = FCopyParams.GetInfoStr(L"; ", ActualCopyParamAttrs());
  CopyParamLabel->setText(ToQString(InfoStr));

  if (LogPanel->isVisibleTo(this) != FSynchronizing)
  {
    // As upstream, the dialog grows by the log and shrinks back
    int LogHeight = LogPanel->sizeHint().height() + layout()->spacing();
    int Height = height();
    LogPanel->setVisible(FSynchronizing);
    layout()->activate();
    if (isVisible())
    {
      resize(width(), FSynchronizing ? (Height + LogHeight) : std::max(Height - LogHeight, minimumHeight()));
    }
  }
}
//---------------------------------------------------------------------------
bool TSynchronizeDialog::Execute()
{
  // at start assume that copy param is current preset
  FPreset = GUIConfiguration->CopyParamCurrent;
  LocalDirectoryEdit->SetHistoryKey(L"LocalDirectory");
  RemoteDirectoryEdit->SetHistoryKey(L"RemoteDirectory");
  ClearLog();
  UpdateControls();
  exec();
  return true;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::SetParams(const TSynchronizeParamType & Value)
{
  FParams = Value;
  RemoteDirectoryEdit->SetText(Value.RemoteDirectory);
  LocalDirectoryEdit->SetText(Value.LocalDirectory);
  SynchronizeDeleteCheck->setChecked(FLAGSET(Value.Params, TTerminal::spDelete));
  SynchronizeExistingOnlyCheck->setChecked(FLAGSET(Value.Params, TTerminal::spExistingOnly));
  SynchronizeSelectedOnlyCheck->setChecked(FLAGSET(Value.Params, TTerminal::spSelectedOnly));
  SynchronizeRecursiveCheck->setChecked(FLAGSET(Value.Options, soRecurse));
  SynchronizeSynchronizeCheck->setCheckState(
    FLAGSET(Value.Options, soSynchronizeAsk) ? Qt::PartiallyChecked :
      (FLAGSET(Value.Options, soSynchronize) ? Qt::Checked : Qt::Unchecked));
  ContinueOnErrorCheck->setChecked(FLAGSET(Value.Options, soContinueOnError));
  UpdateControls();
}
//---------------------------------------------------------------------------
TSynchronizeParamType TSynchronizeDialog::GetParams()
{
  TSynchronizeParamType Result = FParams;
  Result.RemoteDirectory = RemoteDirectoryEdit->GetText();
  Result.LocalDirectory = LocalDirectoryEdit->GetText();
  Result.Params =
    (Result.Params & ~(TTerminal::spDelete | TTerminal::spExistingOnly | TTerminal::spSelectedOnly | TTerminal::spTimestamp)) |
    FLAGMASK(SynchronizeDeleteCheck->isChecked(), TTerminal::spDelete) |
    FLAGMASK(SynchronizeExistingOnlyCheck->isChecked(), TTerminal::spExistingOnly) |
    FLAGMASK(SynchronizeSelectedOnlyCheck->isChecked(), TTerminal::spSelectedOnly);
  Qt::CheckState SynchronizeState = SynchronizeSynchronizeCheck->checkState();
  Result.Options =
    (Result.Options & ~(soRecurse | soSynchronize | soSynchronizeAsk | soContinueOnError)) |
    FLAGMASK(SynchronizeRecursiveCheck->isChecked(), soRecurse) |
    FLAGMASK(SynchronizeState == Qt::Checked, soSynchronize) |
    FLAGMASK(SynchronizeState == Qt::PartiallyChecked, soSynchronizeAsk) |
    FLAGMASK(ContinueOnErrorCheck->isChecked(), soContinueOnError);
  return Result;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::LocalDirectoryBrowseButtonClick()
{
  QString Directory =
    QFileDialog::getExistingDirectory(this, ToQString(LoadStr(SELECT_LOCAL_DIRECTORY)), LocalDirectoryEdit->currentText());
  if (!Directory.isEmpty())
  {
    LocalDirectoryEdit->setEditText(Directory);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::SetOptions(int Value)
{
  if (FOptions != Value)
  {
    FOptions = Value;
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::CopyParamListPopup(const QPoint & Pos)
{
  if (FLAGCLEAR(FOptions, soDoNotUsePresets))
  {
    // We pass in FCopyParams, although it may not be the exact copy param
    // that will be used (because of PreserveTime). The reason is to
    // display checkbox next to user-selected preset
    TCopyParamType ACopyParams = FCopyParams;
    if (::CopyParamListPopup(Pos, ACopyParams, FPreset, cplCustomizeDefault, ActualCopyParamAttrs(), NULL) > 0)
    {
      FCopyParams = ACopyParams;
      UpdateControls();
    }
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::CopyParamGroupClick()
{
  if (!FSynchronizing)
  {
    // PreserveTime is forced for some settings, but avoid hard-setting it until
    // user really confirms it on custom dialog
    TCopyParamType ACopyParams = FCopyParams;
    if (DoCopyParamCustomDialog(ACopyParams, ActualCopyParamAttrs()))
    {
      FCopyParams = ACopyParams;
      UpdateControls();
    }
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::DoStartStop(bool Start, bool Synchronize)
{
  if (FOnStartStop != NULL)
  {
    TSynchronizeParamType SParams = GetParams();
    SParams.Options =
      (SParams.Options & ~(soSynchronize | soSynchronizeAsk)) |
      FLAGMASK(Synchronize, soSynchronize);
    if (Start)
    {
      DebugAssert(!*FOnFeedSynchronizeError);
      if (FLAGSET(SParams.Options, soContinueOnError))
      {
        *FOnFeedSynchronizeError = [this](const UnicodeString & Message, TStrings * MoreMessages, TQueryType Type)
        {
          FeedSynchronizeError(Message, MoreMessages, Type);
        };
      }
      else
      {
        *FOnFeedSynchronizeError = nullptr;
      }
      FSynchronizeOptions.reset(new TSynchronizeOptions());
      FOnGetOptions(SParams.Params, *FSynchronizeOptions);
    }
    else
    {
      *FOnFeedSynchronizeError = nullptr;
    }
    FOnStartStop(NULL, Start, SParams, GetCopyParams(), FSynchronizeOptions.get(),
      ::rtl::MethodPtr(this, &TSynchronizeDialog::DoAbort), NULL,
      ::rtl::MethodPtr(this, &TSynchronizeDialog::DoLog));
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::Abort(bool Close)
{
  FAbort = true;
  FClose = Close;
  // Upstream's WM_USER_STOP: the monitor is not stopped from its own handler
  QMetaObject::invokeMethod(this, [this]() { AbortStop(); }, Qt::QueuedConnection);
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::AbortStop()
{
  if (FAbort)
  {
    if (FSynchronizing)
    {
      Stop();
    }
    if (FClose)
    {
      FClose = false;
      reject();
    }
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::DoAbort(TObject * /*Sender*/, bool Close)
{
  Abort(Close);
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::DoLogInternal(
  TSynchronizeLogEntry Entry, const UnicodeString & Message, TStrings * MoreMessages, TQueryType Type)
{
  TLogItem * Item = new TLogItem();
  Item->Entry = Entry;
  Item->Message = Message;
  if (MoreMessages != NULL)
  {
    Item->MoreMessages.reset(new TStringList());
    Item->MoreMessages->Assign(MoreMessages);
  }
  Item->Type = Type;

  Item->setText(0, ToQString(Now().TimeString()));
  UnicodeString UnformattedMessage = UnformatMessage(Message);
  UnformattedMessage = ReplaceStr(UnformattedMessage, L"\r", L"");
  UnformattedMessage = ReplaceStr(UnformattedMessage, L"\n", L" ");
  Item->setText(1, ToQString(UnformattedMessage));
  if (Entry == slContinuedError)
  {
    for (int Column = 0; Column < 2; Column++)
    {
      Item->setForeground(Column, QBrush(Qt::red));
    }
  }
  LogView->addTopLevelItem(Item);
  LogView->scrollToItem(Item);
  while (LogView->topLevelItemCount() > MaxLogItems)
  {
    delete LogView->takeTopLevelItem(0);
  }
  if (Entry == slScan)
  {
    // redraw log before the scanning block update
    LogView->repaint();
  }

  FOnSynchronizeSessionLog(Message);
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::DoLog(TSynchronizeController * /*Controller*/,
  TSynchronizeLogEntry Entry, const UnicodeString Message)
{
  DoLogInternal(Entry, Message,
    // these are unused (as Entry is not slContinuedError here)
    NULL, qtInformation);
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::Start()
{
  bool Synchronize = false; // shut up
  bool Continue = true;
  if (SynchronizeSynchronizeCheck->checkState() == Qt::PartiallyChecked)
  {
    TMessageParams Params(mpNeverAskAgainCheck);
    switch (MoreMessageDialog(LoadStr(SYNCHRONISE_BEFORE_KEEPUPTODATE2),
        NULL, qtConfirmation, qaYes | qaNo | qaCancel, &Params))
    {
      case qaNeverAskAgain:
        SynchronizeSynchronizeCheck->setCheckState(Qt::Checked);
        // fall thru

      case qaYes:
        Synchronize = true;
        break;

      case qaNo:
        Synchronize = false;
        break;

      default:
      case qaCancel:
        Continue = false;
        break;
    };
  }
  else
  {
    Synchronize = SynchronizeSynchronizeCheck->isChecked();
  }

  if (Continue)
  {
    DebugAssert(!FSynchronizing);

    SaveHistory();

    FSynchronizing = true;
    try
    {
      UpdateControls();
      repaint();

      FAbort = false;
      DoStartStop(true, Synchronize);
    }
    catch (...)
    {
      FSynchronizing = false;
      *FOnFeedSynchronizeError = nullptr;
      UpdateControls();
      throw;
    }
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::SaveHistory()
{
  LocalDirectoryEdit->SaveToHistory();
  RemoteDirectoryEdit->SaveToHistory();
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::OnlyStop()
{
  FSynchronizing = false;
  DoStartStop(false, false);
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::Stop()
{
  OnlyStop();
  UpdateControls();
  repaint();
  if (FMinimizedByMe)
  {
    FMinimizedByMe = false;
    RestoreApplication();
    activateWindow();
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::Minimize(bool ToTray)
{
  // Upstream minimizes the application
  MinimizeApplication(ToTray);
  FMinimizedByMe = true;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::reject()
{
  // Closing stops watching first (upstream's FormCloseQuery)
  if (FSynchronizing)
  {
    Stop();
  }
  QDialog::reject();
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::keyPressEvent(QKeyEvent * Event)
{
  // Escape only stops, while watching
  if ((Event->key() == Qt::Key_Escape) && (Event->modifiers() == Qt::NoModifier) && FSynchronizing)
  {
    Stop();
    Event->accept();
  }
  else
  {
    QDialog::keyPressEvent(Event);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::resizeEvent(QResizeEvent * Event)
{
  QDialog::resizeEvent(Event);
  // The compositor may size the window by its size before the log was
  // shown (when it activates the window meanwhile); the minimum size wins
  if (height() < minimumHeight())
  {
    QTimer::singleShot(0, this, [this]()
    {
      if (height() < minimumHeight())
      {
        resize(width(), minimumHeight());
      }
    });
  }
}
//---------------------------------------------------------------------------
TCopyParamType TSynchronizeDialog::GetCopyParams()
{
  TCopyParamType Result = FCopyParams;
  Result.PreserveTime = true;
  Result.NewerOnly = false;
  return Result;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::SetCopyParams(const TCopyParamType & Value)
{
  FCopyParams = Value;
  UpdateControls();
}
//---------------------------------------------------------------------------
int TSynchronizeDialog::ActualCopyParamAttrs()
{
  return FCopyParamAttrs | cpaNoPreserveTime | cpaNoNewerOnly;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::ClearLog()
{
  LogView->clear();
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::CopyLog()
{
  QString Content;
  for (int Index = 0; Index < LogView->topLevelItemCount(); Index++)
  {
    QTreeWidgetItem * Item = LogView->topLevelItem(Index);
    Content += Item->text(0) + QLatin1Char('\t') + Item->text(1) + QLatin1Char('\n');
  }
  QApplication::clipboard()->setText(Content);
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::LogViewDblClick()
{
  TLogItem * Item = static_cast<TLogItem *>(LogView->currentItem());
  if ((Item != nullptr) && (Item->Entry == slContinuedError))
  {
    MoreMessageDialog(Item->Message, Item->MoreMessages.get(), Item->Type, qaOK);
  }
}
//---------------------------------------------------------------------------
bool TSynchronizeDialog::eventFilter(QObject * Object, QEvent * Event)
{
  if ((Object == CopyParamGroup) || (Object == CopyParamLabel))
  {
    if (Event->type() == QEvent::MouseButtonRelease)
    {
      CopyParamGroupClick();
      return true;
    }
    else if (Event->type() == QEvent::ContextMenu)
    {
      if (!FSynchronizing)
      {
        CopyParamListPopup(static_cast<QContextMenuEvent *>(Event)->globalPos());
      }
      return true;
    }
  }
  else if ((Object == LogView) && (Event->type() == QEvent::KeyPress))
  {
    QKeyEvent * KeyEvent = static_cast<QKeyEvent *>(Event);
    if ((KeyEvent->key() == Qt::Key_Delete) && (KeyEvent->modifiers() == Qt::NoModifier))
    {
      ClearLog();
      return true;
    }
    else if (KeyEvent->matches(QKeySequence::Copy) && (LogView->topLevelItemCount() > 0))
    {
      CopyLog();
      return true;
    }
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoSynchronizeDialog(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
  bool & SaveSettings, int Options, int CopyParamAttrs,
  TGetSynchronizeOptionsEvent OnGetOptions,
  TSynchronizeSessionLog OnSynchronizeSessionLog,
  TFeedSynchronizeError & OnFeedSynchronizeError,
  std::function<void()> & OnSynchronizeAbort)
{
  bool Result;
  ::rtl::TFinally Finally([&]()
  {
    OnSynchronizeAbort = nullptr;
    OnFeedSynchronizeError = nullptr;
  });
  TSynchronizeDialog Dialog;
  Dialog.Init(OnStartStop, OnGetOptions, OnSynchronizeSessionLog, OnFeedSynchronizeError, OnSynchronizeAbort);
  Dialog.SetOptions(Options);
  Dialog.SetCopyParamAttrs(CopyParamAttrs);
  Dialog.SetParams(Params);
  Dialog.SetCopyParams(*CopyParams);
  Dialog.SetSaveSettings(SaveSettings);
  Result = Dialog.Execute();
  if (Result)
  {
    SaveSettings = Dialog.GetSaveSettings();
    Params = Dialog.GetParams();
  }
  return Result;
}
//---------------------------------------------------------------------------
