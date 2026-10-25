// The synchronization of directories (upstream's TFullSynchronizeDialog,
// forms/FullSynchronize.dfm): the directories, the direction, the mode,
// the options, the comparison criteria and the transfer settings.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Terminal.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"
#include "SplitButton.h"

#include <QApplication>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
class TFullSynchronizeDialog : public QDialog
{
public:
  TFullSynchronizeDialog();

  void Init(int Options, const TUsableCopyParamAttrs & CopyParamAttrs);
  bool Execute();

  UnicodeString GetDirectory1() { return LocalDirectoryEdit->GetText(); }
  void SetDirectory1(const UnicodeString & Value) { LocalDirectoryEdit->SetText(Value); }
  UnicodeString GetDirectory2() { return RemoteDirectoryEdit->GetText(); }
  void SetDirectory2(const UnicodeString & Value) { RemoteDirectoryEdit->SetText(Value); }
  TSynchronizeMode GetMode();
  void SetMode(TSynchronizeMode Value);
  int GetParams();
  void SetParams(int Value);
  bool GetSaveSettings() { return SaveSettingsCheck->isChecked(); }
  void SetSaveSettings(bool Value) { SaveSettingsCheck->setChecked(Value); }
  bool FSaveMode;
  TCopyParamType GetCopyParams();
  void SetCopyParams(const TCopyParamType & Value);

protected:
  virtual void accept() override;
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  int FParams;
  TSynchronizeMode FOrigMode;
  int FOptions;
  TUsableCopyParamAttrs FCopyParamAttrs;
  TCopyParamType FCopyParams;
  UnicodeString FPreset;
  QString FSynchronizeBySizeCaption;

  QLabel * LocalDirectoryLabel;
  THistoryComboBox * LocalDirectoryEdit;
  QPushButton * LocalDirectoryBrowseButton;
  QLabel * RemoteDirectoryLabel;
  THistoryComboBox * RemoteDirectoryEdit;
  QPushButton * OtherLocalDirectoryBrowseButton;
  QRadioButton * SynchronizeBothButton;
  QRadioButton * SynchronizeRemoteButton;
  QRadioButton * SynchronizeLocalButton;
  QRadioButton * SynchronizeFilesButton;
  QRadioButton * MirrorFilesButton;
  QRadioButton * SynchronizeTimestampsButton;
  QCheckBox * SynchronizeDeleteCheck;
  QCheckBox * SynchronizeSelectedOnlyCheck;
  QCheckBox * SynchronizeExistingOnlyCheck;
  QCheckBox * SynchronizePreviewChangesCheck;
  QCheckBox * SynchronizeByTimeCheck;
  QCheckBox * SynchronizeBySizeCheck;
  QCheckBox * SynchronizeCaseSensitiveCheck;
  QCheckBox * SynchronizeByChecksumCheck;
  QCheckBox * SaveSettingsCheck;
  QGroupBox * CopyParamGroup;
  QLabel * CopyParamLabel;
  TSplitButton * TransferSettingsButton;
  QPushButton * OkButton;
  int FUpdating;

  void UpdateControls();
  int ActualCopyParamAttrs();
  bool CanSynchronizeTimestamps();
  void CopyParamGroupClick();
  void CopyParamListPopup(const QPoint & Pos);
  void DoLocalDirectoryBrowseButtonClick(THistoryComboBox * ComboBox);
};
//---------------------------------------------------------------------------
static QRadioButton * CreateRadio(QWidget * Parent, QBoxLayout * Layout, const char * Name, const QString & Caption)
{
  QRadioButton * Button = new QRadioButton(Caption, Parent);
  Button->setObjectName(QString::fromLatin1(Name));
  Layout->addWidget(Button);
  return Button;
}
//---------------------------------------------------------------------------
static QCheckBox * CreateCheck(QWidget * Parent, QBoxLayout * Layout, const char * Name, const QString & Caption)
{
  QCheckBox * Check = new QCheckBox(Caption, Parent);
  Check->setObjectName(QString::fromLatin1(Name));
  Layout->addWidget(Check);
  return Check;
}
//---------------------------------------------------------------------------
TFullSynchronizeDialog::TFullSynchronizeDialog() :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("FullSynchronizeDialog"));
  setWindowTitle(tr("Synchronize"));
  FParams = 0;
  FSaveMode = false;
  FOrigMode = smRemote;
  FOptions = 0;
  FUpdating = 0;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QGroupBox * DirectoriesGroup = new QGroupBox(tr("Directories"), this);
  DirectoriesGroup->setObjectName(QStringLiteral("DirectoriesGroup"));
  QGridLayout * DirectoriesLayout = new QGridLayout(DirectoriesGroup);
  LocalDirectoryLabel = new QLabel(tr("Lo&cal directory:"), DirectoriesGroup);
  LocalDirectoryLabel->setObjectName(QStringLiteral("LocalDirectoryLabel"));
  DirectoriesLayout->addWidget(LocalDirectoryLabel, 0, 0, 1, 2);
  LocalDirectoryEdit = new THistoryComboBox(DirectoriesGroup);
  LocalDirectoryEdit->setObjectName(QStringLiteral("LocalDirectoryEdit"));
  LocalDirectoryLabel->setBuddy(LocalDirectoryEdit);
  DirectoriesLayout->addWidget(LocalDirectoryEdit, 1, 0);
  LocalDirectoryBrowseButton = new QPushButton(tr("Bro&wse..."), DirectoriesGroup);
  LocalDirectoryBrowseButton->setObjectName(QStringLiteral("LocalDirectoryBrowseButton"));
  LocalDirectoryBrowseButton->setAutoDefault(false);
  connect(LocalDirectoryBrowseButton, &QPushButton::clicked, this,
    [this]() { DoLocalDirectoryBrowseButtonClick(LocalDirectoryEdit); });
  DirectoriesLayout->addWidget(LocalDirectoryBrowseButton, 1, 1);
  RemoteDirectoryLabel = new QLabel(tr("R&emote directory:"), DirectoriesGroup);
  RemoteDirectoryLabel->setObjectName(QStringLiteral("RemoteDirectoryLabel"));
  DirectoriesLayout->addWidget(RemoteDirectoryLabel, 2, 0, 1, 2);
  RemoteDirectoryEdit = new THistoryComboBox(DirectoriesGroup);
  RemoteDirectoryEdit->setObjectName(QStringLiteral("RemoteDirectoryEdit"));
  RemoteDirectoryLabel->setBuddy(RemoteDirectoryEdit);
  DirectoriesLayout->addWidget(RemoteDirectoryEdit, 3, 0);
  OtherLocalDirectoryBrowseButton = new QPushButton(tr("Brows&e..."), DirectoriesGroup);
  OtherLocalDirectoryBrowseButton->setObjectName(QStringLiteral("OtherLocalDirectoryBrowseButton"));
  OtherLocalDirectoryBrowseButton->setAutoDefault(false);
  connect(OtherLocalDirectoryBrowseButton, &QPushButton::clicked, this,
    [this]() { DoLocalDirectoryBrowseButtonClick(RemoteDirectoryEdit); });
  DirectoriesLayout->addWidget(OtherLocalDirectoryBrowseButton, 3, 1);
  DirectoriesLayout->setColumnStretch(0, 1);
  for (THistoryComboBox * Edit : { LocalDirectoryEdit, RemoteDirectoryEdit })
  {
    connect(Edit, &QComboBox::currentTextChanged, this, [this]() { UpdateControls(); });
  }
  MainLayout->addWidget(DirectoriesGroup);

  // upstream's layout: the direction and the mode in rows of the width of
  // the dialog, the options and the criteria next to each other
  QGridLayout * GroupsLayout = new QGridLayout();
  QGroupBox * DirectionGroup = new QGroupBox(tr("Direction/Target directory"), this);
  DirectionGroup->setObjectName(QStringLiteral("DirectionGroup"));
  QHBoxLayout * DirectionLayout = new QHBoxLayout(DirectionGroup);
  SynchronizeBothButton = CreateRadio(DirectionGroup, DirectionLayout, "SynchronizeBothButton", tr("&Both"));
  SynchronizeRemoteButton = CreateRadio(DirectionGroup, DirectionLayout, "SynchronizeRemoteButton", tr("&Remote"));
  SynchronizeLocalButton = CreateRadio(DirectionGroup, DirectionLayout, "SynchronizeLocalButton", tr("&Local"));
  GroupsLayout->addWidget(DirectionGroup, 0, 0, 1, 2);

  QGroupBox * ModeGroup = new QGroupBox(tr("Mode"), this);
  ModeGroup->setObjectName(QStringLiteral("ModeGroup"));
  QHBoxLayout * ModeLayout = new QHBoxLayout(ModeGroup);
  SynchronizeFilesButton = CreateRadio(ModeGroup, ModeLayout, "SynchronizeFilesButton", tr("Synchronize &files"));
  MirrorFilesButton = CreateRadio(ModeGroup, ModeLayout, "MirrorFilesButton", tr("&Mirror files"));
  SynchronizeTimestampsButton =
    CreateRadio(ModeGroup, ModeLayout, "SynchronizeTimestampsButton", tr("Synchronize &timestamps"));
  GroupsLayout->addWidget(ModeGroup, 1, 0, 1, 2);

  QGroupBox * OptionsGroup = new QGroupBox(tr("Synchronize options"), this);
  OptionsGroup->setObjectName(QStringLiteral("OptionsGroup"));
  QVBoxLayout * OptionsLayout = new QVBoxLayout(OptionsGroup);
  SynchronizeDeleteCheck = CreateCheck(OptionsGroup, OptionsLayout, "SynchronizeDeleteCheck", tr("&Delete files"));
  SynchronizePreviewChangesCheck =
    CreateCheck(OptionsGroup, OptionsLayout, "SynchronizePreviewChangesCheck", tr("Pre&view changes"));
  SynchronizeExistingOnlyCheck =
    CreateCheck(OptionsGroup, OptionsLayout, "SynchronizeExistingOnlyCheck", tr("E&xisting files only"));
  SynchronizeSelectedOnlyCheck =
    CreateCheck(OptionsGroup, OptionsLayout, "SynchronizeSelectedOnlyCheck", tr("Selected files o&nly"));
  GroupsLayout->addWidget(OptionsGroup, 2, 0);

  QGroupBox * CompareCriterionsGroup = new QGroupBox(tr("Comparison criteria"), this);
  CompareCriterionsGroup->setObjectName(QStringLiteral("CompareCriterionsGroup"));
  QVBoxLayout * CompareLayout = new QVBoxLayout(CompareCriterionsGroup);
  SynchronizeByTimeCheck = CreateCheck(CompareCriterionsGroup, CompareLayout, "SynchronizeByTimeCheck", tr("M&odification time"));
  SynchronizeBySizeCheck = CreateCheck(CompareCriterionsGroup, CompareLayout, "SynchronizeBySizeCheck", tr("File si&ze"));
  FSynchronizeBySizeCaption = SynchronizeBySizeCheck->text();
  SynchronizeByChecksumCheck = CreateCheck(CompareCriterionsGroup, CompareLayout, "SynchronizeByChecksumCheck", tr("Checks&um"));
  SynchronizeCaseSensitiveCheck =
    CreateCheck(CompareCriterionsGroup, CompareLayout, "SynchronizeCaseSensitiveCheck", tr("C&ase sensitive"));
  GroupsLayout->addWidget(CompareCriterionsGroup, 2, 1);
  MainLayout->addLayout(GroupsLayout);

  SaveSettingsCheck = new QCheckBox(tr("Use &same options next time"), this);
  SaveSettingsCheck->setObjectName(QStringLiteral("SaveSettingsCheck"));
  MainLayout->addWidget(SaveSettingsCheck);

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

  for (QRadioButton * Button : findChildren<QRadioButton *>())
  {
    connect(Button, &QRadioButton::toggled, this, [this]() { UpdateControls(); });
  }
  for (QCheckBox * Check : findChildren<QCheckBox *>())
  {
    connect(Check, &QCheckBox::toggled, this, [this]() { UpdateControls(); });
  }

  QHBoxLayout * ButtonsLayout = new QHBoxLayout();
  // upstream's split button: the presets in its drop-down menu
  TransferSettingsButton = new TSplitButton(this);
  TransferSettingsButton->setText(tr("Transfer settin&gs..."));
  TransferSettingsButton->setObjectName(QStringLiteral("TransferSettingsButton"));
  TransferSettingsButton->setAutoDefault(false);
  connect(TransferSettingsButton, &QPushButton::clicked, this, [this]() { CopyParamGroupClick(); });
  connect(TransferSettingsButton, &TSplitButton::DropDownRequested, this, [this]()
  {
    CopyParamListPopup(TransferSettingsButton->mapToGlobal(QPoint(0, TransferSettingsButton->height())));
  });
  ButtonsLayout->addWidget(TransferSettingsButton);
  ButtonsLayout->addStretch(1);
  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  OkButton = Buttons->button(QDialogButtonBox::Ok);
  OkButton->setObjectName(QStringLiteral("OkButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  ButtonsLayout->addWidget(Buttons);
  MainLayout->addLayout(ButtonsLayout);
  AddHelpButton(Buttons, FormHelpKeyword(L"FullSynchronize.dfm"));
  setMinimumWidth(fontMetrics().averageCharWidth() * 72);
}
//---------------------------------------------------------------------------
bool TFullSynchronizeDialog::CanSynchronizeTimestamps()
{
  return FLAGCLEAR(FOptions, fsoDisableTimestamp) && FLAGCLEAR(FOptions, fsoLocalLocal);
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::Init(int Options, const TUsableCopyParamAttrs & CopyParamAttrs)
{
  FOptions = Options;
  // somewhat redundant
  if (!CanSynchronizeTimestamps() && SynchronizeTimestampsButton->isChecked())
  {
    SynchronizeFilesButton->setChecked(true);
  }
  FCopyParamAttrs = CopyParamAttrs;
  if (FLAGSET(FOptions, fsoLocalLocal))
  {
    LocalDirectoryLabel->setText(ToQString(LoadStr(SYNCHRONIZE_LEFT_DIR)));
    RemoteDirectoryLabel->setText(ToQString(LoadStr(SYNCHRONIZE_RIGHT_DIR)));
    SynchronizeRemoteButton->setText(ToQString(LoadStr(SYNCHRONIZE_RIGHT)));
    SynchronizeLocalButton->setText(ToQString(LoadStr(SYNCHRONIZE_LEFT)));
    CopyParamGroup->setVisible(false);
    TransferSettingsButton->setVisible(false);
  }
  else
  {
    OtherLocalDirectoryBrowseButton->setVisible(false);
  }
  TransferSettingsButton->SetDropDownVisible(FLAGCLEAR(FOptions, fsoDoNotUsePresets));

  UpdateControls();
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::UpdateControls()
{
  if (FUpdating > 0)
  {
    return;
  }
  TAutoNestingCounter UpdatingCounter(FUpdating);
  SynchronizeTimestampsButton->setEnabled(CanSynchronizeTimestamps());
  if (SynchronizeTimestampsButton->isChecked())
  {
    SynchronizeExistingOnlyCheck->setChecked(true);
    SynchronizeDeleteCheck->setChecked(false);
    SynchronizeByTimeCheck->setChecked(true);
    SynchronizeByChecksumCheck->setChecked(false);
  }
  if (SynchronizeBothButton->isChecked())
  {
    SynchronizeByTimeCheck->setChecked(true);
    SynchronizeBySizeCheck->setChecked(false);
    SynchronizeByChecksumCheck->setChecked(false);
    if (MirrorFilesButton->isChecked())
    {
      SynchronizeFilesButton->setChecked(true);
    }
  }
  MirrorFilesButton->setEnabled(!SynchronizeBothButton->isChecked());
  SynchronizeDeleteCheck->setEnabled(!SynchronizeBothButton->isChecked() &&
    !SynchronizeTimestampsButton->isChecked());
  SynchronizeExistingOnlyCheck->setEnabled(!SynchronizeTimestampsButton->isChecked());
  SynchronizeByTimeCheck->setEnabled(!SynchronizeBothButton->isChecked() &&
    !SynchronizeTimestampsButton->isChecked());
  SynchronizeBySizeCheck->setEnabled(!SynchronizeBothButton->isChecked());
  SynchronizeByChecksumCheck->setEnabled(
    FLAGCLEAR(FOptions, fsoDisableByChecksum) && !SynchronizeBothButton->isChecked() &&
    !SynchronizeTimestampsButton->isChecked());
  SynchronizeSelectedOnlyCheck->setEnabled(FLAGSET(FOptions, fsoAllowSelectedOnly));

  OkButton->setEnabled(!LocalDirectoryEdit->currentText().isEmpty() &&
    !RemoteDirectoryEdit->currentText().isEmpty());

  UnicodeString InfoStr = FCopyParams.GetInfoStr(L"; ", ActualCopyParamAttrs());
  CopyParamLabel->setText(ToQString(InfoStr));

  SynchronizeBySizeCheck->setText(SynchronizeTimestampsButton->isChecked() ?
    ToQString(LoadStr(SYNCHRONIZE_SAME_SIZE)) : FSynchronizeBySizeCaption);
}
//---------------------------------------------------------------------------
int TFullSynchronizeDialog::ActualCopyParamAttrs()
{
  int Result;
  if (SynchronizeTimestampsButton->isChecked())
  {
    Result = cpaIncludeMaskOnly;
  }
  else
  {
    switch (GetMode())
    {
      case smRemote:
        Result = FCopyParamAttrs.Upload;
        break;

      case smLocal:
        Result = FCopyParamAttrs.Download;
        break;

      default:
        DebugFail();
        //fallthru
      case smBoth:
        Result = FCopyParamAttrs.General;
        break;
    }
  }
  return
    Result |
    FLAGMASK(SynchronizeByTimeCheck->isChecked(), cpaNoPreserveTime) |
    cpaNoNewerOnly;
}
//---------------------------------------------------------------------------
bool TFullSynchronizeDialog::Execute()
{
  // at start assume that copy param is current preset
  FPreset = GUIConfiguration->CopyParamCurrent;
  LocalDirectoryEdit->SetHistoryKey(L"LocalDirectory");
  RemoteDirectoryEdit->SetHistoryKey(FLAGCLEAR(FOptions, fsoLocalLocal) ? L"RemoteDirectory" : L"LocalDirectory2");
  UpdateControls();
  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    LocalDirectoryEdit->SaveToHistory();
    RemoteDirectoryEdit->SaveToHistory();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::SetMode(TSynchronizeMode Value)
{
  FOrigMode = Value;
  switch (Value)
  {
    case smRemote:
      SynchronizeRemoteButton->setChecked(true);
      break;

    case smLocal:
      SynchronizeLocalButton->setChecked(true);
      break;

    case smBoth:
      SynchronizeBothButton->setChecked(true);
      break;

    default:
      DebugFail();
  }
}
//---------------------------------------------------------------------------
TSynchronizeMode TFullSynchronizeDialog::GetMode()
{
  if (SynchronizeRemoteButton->isChecked())
  {
    return smRemote;
  }
  else if (SynchronizeLocalButton->isChecked())
  {
    return smLocal;
  }
  else
  {
    DebugAssert(SynchronizeBothButton->isChecked());
    return smBoth;
  }
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::SetParams(int Value)
{
  FParams = Value &
    ~(TTerminal::spDelete | TTerminal::spExistingOnly | TTerminal::spPreviewChanges | TTerminal::spTimestamp |
      TTerminal::spNotByTime | TTerminal::spBySize | TTerminal::spSelectedOnly | TTerminal::spMirror |
      TTerminal::spCaseSensitive | TTerminal::spByChecksum);
  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    SynchronizeDeleteCheck->setChecked(FLAGSET(Value, TTerminal::spDelete));
    SynchronizeExistingOnlyCheck->setChecked(FLAGSET(Value, TTerminal::spExistingOnly));
    SynchronizePreviewChangesCheck->setChecked(FLAGSET(Value, TTerminal::spPreviewChanges));
    SynchronizeSelectedOnlyCheck->setChecked(FLAGSET(Value, TTerminal::spSelectedOnly));
    if (FLAGSET(Value, TTerminal::spTimestamp) && CanSynchronizeTimestamps())
    {
      SynchronizeTimestampsButton->setChecked(true);
    }
    else if (FLAGSET(Value, TTerminal::spMirror))
    {
      MirrorFilesButton->setChecked(true);
    }
    else
    {
      SynchronizeFilesButton->setChecked(true);
    }
    SynchronizeByTimeCheck->setChecked(FLAGCLEAR(Value, TTerminal::spNotByTime));
    SynchronizeBySizeCheck->setChecked(FLAGSET(Value, TTerminal::spBySize));
    SynchronizeByChecksumCheck->setChecked(FLAGSET(Value, TTerminal::spByChecksum));
    SynchronizeCaseSensitiveCheck->setChecked(FLAGSET(Value, TTerminal::spCaseSensitive));
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
int TFullSynchronizeDialog::GetParams()
{
  return FParams |
    FLAGMASK(SynchronizeDeleteCheck->isChecked(), TTerminal::spDelete) |
    FLAGMASK(SynchronizeExistingOnlyCheck->isChecked(), TTerminal::spExistingOnly) |
    FLAGMASK(SynchronizePreviewChangesCheck->isChecked(), TTerminal::spPreviewChanges) |
    FLAGMASK(SynchronizeSelectedOnlyCheck->isChecked(), TTerminal::spSelectedOnly) |
    FLAGMASK(SynchronizeTimestampsButton->isChecked() && CanSynchronizeTimestamps(), TTerminal::spTimestamp) |
    FLAGMASK(MirrorFilesButton->isChecked(), TTerminal::spMirror) |
    FLAGMASK(!SynchronizeByTimeCheck->isChecked(), TTerminal::spNotByTime) |
    FLAGMASK(SynchronizeBySizeCheck->isChecked(), TTerminal::spBySize) |
    FLAGMASK(SynchronizeByChecksumCheck->isEnabled() && SynchronizeByChecksumCheck->isChecked(), TTerminal::spByChecksum) |
    FLAGMASK(SynchronizeCaseSensitiveCheck->isChecked(), TTerminal::spCaseSensitive);
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::DoLocalDirectoryBrowseButtonClick(THistoryComboBox * ComboBox)
{
  QString Directory =
    QFileDialog::getExistingDirectory(this, ToQString(LoadStr(SELECT_LOCAL_DIRECTORY)), ComboBox->currentText());
  if (!Directory.isEmpty())
  {
    ComboBox->setEditText(Directory);
  }
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::CopyParamListPopup(const QPoint & Pos)
{
  if (FLAGCLEAR(FOptions, fsoDoNotUsePresets))
  {
    // PreserveTime is forced for some settings, but avoid hard-setting it until
    // user really confirms it on custom dialog
    TCopyParamType ACopyParams = GetCopyParams();
    if (::CopyParamListPopup(Pos, ACopyParams, FPreset, cplCustomizeDefault, ActualCopyParamAttrs(), NULL) > 0)
    {
      FCopyParams = ACopyParams;
      UpdateControls();
    }
  }
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::CopyParamGroupClick()
{
  // PreserveTime is forced for some settings, but avoid hard-setting it until
  // user really confirms it on custom dialog
  TCopyParamType ACopyParams = GetCopyParams();
  if (DoCopyParamCustomDialog(ACopyParams, ActualCopyParamAttrs()))
  {
    FCopyParams = ACopyParams;
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
bool TFullSynchronizeDialog::eventFilter(QObject * Object, QEvent * Event)
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
      CopyParamListPopup(static_cast<QContextMenuEvent *>(Event)->globalPos());
      return true;
    }
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::accept()
{
  if (GetSaveSettings() && (FOrigMode != GetMode()) && !FSaveMode)
  {
    switch (MessageDialog(LoadStr(SAVE_SYNCHRONIZE_MODE2), qtConfirmation, qaYes | qaNo | qaCancel))
    {
      case qaYes:
        FSaveMode = true;
        break;

      case qaCancel:
        return;
    }
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
TCopyParamType TFullSynchronizeDialog::GetCopyParams()
{
  TCopyParamType Result = FCopyParams;
  // when synchronizing by time, we force preserving time,
  // otherwise it does not make any sense
  if (FLAGCLEAR(GetParams(), TTerminal::spNotByTime))
  {
    Result.PreserveTime = true;
  }
  Result.NewerOnly = false;
  return Result;
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::SetCopyParams(const TCopyParamType & Value)
{
  FCopyParams = Value;
  UpdateControls();
}
//---------------------------------------------------------------------------
bool DoFullSynchronizeDialog(TSynchronizeMode & Mode, int & Params,
  UnicodeString & Directory1, UnicodeString & Directory2,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, int Options,
  const TUsableCopyParamAttrs & CopyParamAttrs)
{
  TFullSynchronizeDialog Dialog;
  Dialog.Init(Options, CopyParamAttrs);
  Dialog.SetMode(Mode);
  Dialog.SetParams(Params);
  Dialog.SetDirectory1(Directory1);
  Dialog.SetDirectory2(Directory2);
  if (CopyParams != NULL)
  {
    Dialog.SetCopyParams(*CopyParams);
  }
  Dialog.SetSaveSettings(SaveSettings);
  Dialog.FSaveMode = SaveMode;
  bool Result = Dialog.Execute();
  if (Result)
  {
    Mode = Dialog.GetMode();
    Params = Dialog.GetParams();
    Directory1 = Dialog.GetDirectory1();
    Directory2 = Dialog.GetDirectory2();
    if (CopyParams != NULL)
    {
      *CopyParams = Dialog.GetCopyParams();
    }
    SaveSettings = Dialog.GetSaveSettings();
    SaveMode = Dialog.FSaveMode;
  }
  return Result;
}
//---------------------------------------------------------------------------
