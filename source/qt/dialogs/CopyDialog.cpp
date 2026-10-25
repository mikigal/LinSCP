// The transfer confirmation (upstream's TCopyDialog, forms/Copy.dfm) and the
// directory creation dialog (TCreateDirectoryDialog, forms/CreateDirectory.dfm),
// without the attributes of new directories yet.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <FileMasks.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"
#include "SplitButton.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
static bool CopyDialogValidateLocalDirectory(const UnicodeString & Directory, THistoryComboBox * DirectoryEdit)
{
  bool Result = true;
  if (!DirectoryExists(ApiPath(Directory)))
  {
    if (MessageDialog(MainInstructions(FMTLOAD(CREATE_LOCAL_DIRECTORY, (Directory))),
          qtConfirmation, qaOK | qaCancel) != qaCancel)
    {
      if (!ForceDirectories(ApiPath(Directory)))
      {
        SimpleErrorDialog(FMTLOAD(CREATE_LOCAL_DIR_ERROR, (Directory)));
        Result = false;
      }
    }
    else
    {
      Result = false;
    }
  }

  if (!Result)
  {
    DirectoryEdit->lineEdit()->selectAll();
    DirectoryEdit->setFocus();
  }
  return Result;
}
//---------------------------------------------------------------------------
static bool CopyDialogValidateFileMask(
  const UnicodeString & FileMask, THistoryComboBox * DirectoryEdit, bool MultipleFiles, bool RemotePaths)
{
  bool Result = true;
  if (!IsFileNameMask(FileMask) && MultipleFiles)
  {
    UnicodeString Message =
      FormatMultiFilesToOneConfirmation(DirectoryEdit->GetText(), RemotePaths);
    Result = (MessageDialog(Message, qtConfirmation, qaOK | qaCancel) != qaCancel);
  }

  if (!Result)
  {
    DirectoryEdit->lineEdit()->selectAll();
    DirectoryEdit->setFocus();
  }
  return Result;
}
//---------------------------------------------------------------------------
class TCopyDialog : public QDialog
{
public:
  TCopyDialog(bool ToRemote, bool Move, TStrings * FileList, int Options, int CopyParamAttrs);

  bool Execute();
  UnicodeString GetDirectory();
  void SetDirectory(UnicodeString Value);
  TGUICopyParamType GetParams();
  void SetParams(const TGUICopyParamType & Value);
  int GetOutputOptions();
  void SetOutputOptions(int Value);

protected:
  virtual void accept() override;
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  bool FToRemote;
  TStrings * FFileList;
  bool FMove;
  int FOptions;
  int FOutputOptions;
  TGUICopyParamType FParams;
  UnicodeString FPreset;
  TCopyParamType FCopyParams;
  int FCopyParamAttrs;
  bool FSaveSettings;

  QLabel * DirectoryLabel;
  THistoryComboBox * DirectoryEdit;
  QPushButton * LocalDirectoryBrowseButton;
  QGroupBox * CopyParamGroup;
  QLabel * CopyParamLabel;
  TSplitButton * TransferSettingsButton;
  QCheckBox * QueueCheck2;
  QCheckBox * NeverShowAgainCheck;
  QPushButton * OkButton;

  UnicodeString GetFileMask();
  void AdjustControls();
  void AdjustTransferControls();
  void UpdateControls();
  int ActualCopyParamAttrs();
  void CopyParamGroupClick();
  void CopyParamListPopup(const QPoint & Pos, int AdditionalOptions);
  void LocalDirectoryBrowseButtonClick();
  void LocalDirectoryEditExit();
};
//---------------------------------------------------------------------------
TCopyDialog::TCopyDialog(bool ToRemote, bool Move, TStrings * FileList, int Options, int CopyParamAttrs) :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("CopyDialog"));
  FToRemote = ToRemote;
  FMove = Move;
  FOptions = Options;
  FCopyParamAttrs = CopyParamAttrs;
  FFileList = FileList;
  FOutputOptions = 0;
  FSaveSettings = false;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  DirectoryLabel = new QLabel(this);
  DirectoryLabel->setObjectName(QStringLiteral("DirectoryLabel"));
  DirectoryLabel->setTextFormat(Qt::PlainText);
  MainLayout->addWidget(DirectoryLabel);
  QHBoxLayout * DirectoryLayout = new QHBoxLayout();
  DirectoryEdit = new THistoryComboBox(this);
  DirectoryEdit->setObjectName(FToRemote ? QStringLiteral("RemoteDirectoryEdit") : QStringLiteral("LocalDirectoryEdit"));
  DirectoryEdit->installEventFilter(this);
  DirectoryLabel->setBuddy(DirectoryEdit);
  connect(DirectoryEdit, &QComboBox::currentTextChanged, this, [this]() { UpdateControls(); });
  DirectoryLayout->addWidget(DirectoryEdit, 1);
  LocalDirectoryBrowseButton = new QPushButton(tr("B&rowse..."), this);
  LocalDirectoryBrowseButton->setObjectName(QStringLiteral("LocalDirectoryBrowseButton"));
  LocalDirectoryBrowseButton->setAutoDefault(false);
  connect(LocalDirectoryBrowseButton, &QPushButton::clicked, this, [this]() { LocalDirectoryBrowseButtonClick(); });
  DirectoryLayout->addWidget(LocalDirectoryBrowseButton);
  MainLayout->addLayout(DirectoryLayout);

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

  QueueCheck2 = new QCheckBox(ToQString(FMTLOAD(COPY_QUEUE, (LoadStr(COPY_BACKGROUND)))), this);
  QueueCheck2->setObjectName(QStringLiteral("QueueCheck2"));
  connect(QueueCheck2, &QCheckBox::toggled, this, [this]() { UpdateControls(); });
  MainLayout->addWidget(QueueCheck2);
  NeverShowAgainCheck = new QCheckBox(tr("&Do not show this dialog box again"), this);
  NeverShowAgainCheck->setObjectName(QStringLiteral("NeverShowAgainCheck"));
  connect(NeverShowAgainCheck, &QCheckBox::clicked, this, [this]()
  {
    FSaveSettings = NeverShowAgainCheck->isChecked();
    UpdateControls();
  });
  MainLayout->addStretch(1);

  QHBoxLayout * ButtonsLayout = new QHBoxLayout();
  // upstream's split button: the presets in its drop-down menu
  TransferSettingsButton = new TSplitButton(this);
  TransferSettingsButton->setText(tr("Transfer settin&gs..."));
  TransferSettingsButton->setObjectName(QStringLiteral("TransferSettingsButton"));
  TransferSettingsButton->setAutoDefault(false);
  connect(TransferSettingsButton, &QPushButton::clicked, this, [this]() { CopyParamGroupClick(); });
  connect(TransferSettingsButton, &TSplitButton::DropDownRequested, this, [this]()
  {
    CopyParamListPopup(TransferSettingsButton->mapToGlobal(QPoint(0, TransferSettingsButton->height())), cplCustomizeDefault);
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
  AddHelpButton(Buttons, FormHelpKeyword(L"Copy.dfm"));
  // Below the buttons, as upstream's
  MainLayout->addWidget(NeverShowAgainCheck);
  setMinimumWidth(fontMetrics().averageCharWidth() * 70);

  AdjustControls();
}
//---------------------------------------------------------------------------
void TCopyDialog::AdjustTransferControls()
{
  if ((FFileList != NULL) && (FFileList->Count > 0))
  {
    UnicodeString TransferStr = LoadStr(FToRemote ? COPY_COPY_TOREMOTE : COPY_COPY_TOLOCAL);
    // currently the copy dialog is shown when downloading to temp folder
    // only for drag&drop downloads, so we dare to display d&d specific prompt
    UnicodeString DirectionStr =
      LoadStr(((FOptions & coTemp) != 0) ? COPY_TODROP : (FToRemote ? COPY_TOREMOTE : COPY_TOLOCAL));

    UnicodeString Caption;
    if (FFileList->Count == 1)
    {
      UnicodeString FileName;
      if (!FToRemote) FileName = UnixExtractFileName(FFileList->Strings[0]);
        else FileName = ExtractFileName(FFileList->Strings[0]);
      Caption = FMTLOAD((FMove ? MOVE_FILE : COPY_FILE), (TransferStr, FileName, DirectionStr));
    }
    else
    {
      Caption = FMTLOAD((FMove ? MOVE_FILES : COPY_FILES), (TransferStr, FFileList->Count, DirectionStr));
    }
    DirectoryLabel->setText(ToQString(Caption));
  }

  UnicodeString ACaption;
  if (!FMove)
  {
    ACaption = LoadStr(FToRemote ? COPY_COPY_TOREMOTE_CAPTION : COPY_COPY_TOLOCAL_CAPTION);
  }
  else
  {
    ACaption = LoadStr(FToRemote ? COPY_MOVE_TOREMOTE_CAPTION : COPY_MOVE_TOLOCAL_CAPTION);
  }
  setWindowTitle(ToQString(ACaption));
}
//---------------------------------------------------------------------------
void TCopyDialog::AdjustControls()
{
  DirectoryEdit->setVisible(FLAGCLEAR(FOptions, coTemp));
  DirectoryLabel->setEnabled(DirectoryEdit->isEnabled());
  LocalDirectoryBrowseButton->setEnabled(DirectoryEdit->isEnabled());

  AdjustTransferControls();

  LocalDirectoryBrowseButton->setVisible(!FToRemote && FLAGCLEAR(FOptions, coTemp));

  if (FLAGCLEAR(FOptions, coDoNotShowAgain))
  {
    // Command-line transfers
    NeverShowAgainCheck->setVisible(false);
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
void TCopyDialog::SetOutputOptions(int Value)
{
  if (GetOutputOptions() != Value)
  {
    FSaveSettings = FLAGSET(Value, cooSaveSettings);
    NeverShowAgainCheck->setChecked(FLAGSET(Value, cooDoNotShowAgain));
    FOutputOptions = (Value & ~(cooDoNotShowAgain | cooSaveSettings));
  }
}
//---------------------------------------------------------------------------
int TCopyDialog::GetOutputOptions()
{
  return FOutputOptions |
    FLAGMASK(FSaveSettings, cooSaveSettings) |
    FLAGMASK(NeverShowAgainCheck->isChecked(), cooDoNotShowAgain);
}
//---------------------------------------------------------------------------
UnicodeString TCopyDialog::GetFileMask()
{
  return ExtractFileName(DirectoryEdit->GetText(), FToRemote);
}
//---------------------------------------------------------------------------
void TCopyDialog::SetParams(const TGUICopyParamType & Value)
{
  FParams = Value;
  FCopyParams = Value;
  DirectoryEdit->SetText(GetDirectory() + FParams.FileMask);
  QueueCheck2->setChecked(FParams.Queue);
  UpdateControls();
}
//---------------------------------------------------------------------------
TGUICopyParamType TCopyDialog::GetParams()
{
  // overwrites TCopyParamType fields only
  FParams = FCopyParams;
  FParams.FileMask = GetFileMask();
  FParams.Queue = QueueCheck2->isChecked();
  return FParams;
}
//---------------------------------------------------------------------------
void TCopyDialog::SetDirectory(UnicodeString Value)
{
  if (!Value.IsEmpty())
  {
    Value = UniversalIncludeTrailingBackslash(FToRemote, Value);
  }
  DirectoryEdit->SetText(Value + GetFileMask());
}
//---------------------------------------------------------------------------
UnicodeString TCopyDialog::GetDirectory()
{
  UnicodeString Result = DirectoryEdit->GetText();
  if (FToRemote)
  {
    Result = UnixExtractFilePath(Result);
  }
  else
  {
    Result = ExtractFilePath(Result);
  }
  if (!Result.IsEmpty())
  {
    Result = UniversalIncludeTrailingBackslash(FToRemote, Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
int TCopyDialog::ActualCopyParamAttrs()
{
  return FCopyParamAttrs | FLAGMASK(QueueCheck2->isChecked() && GUIConfiguration->DefaultCopyParam.QueueParallel, cpaNoCalculateSize);
}
//---------------------------------------------------------------------------
void TCopyDialog::UpdateControls()
{
  UnicodeString InfoStr = FCopyParams.GetInfoStr(L"; ", ActualCopyParamAttrs());
  CopyParamLabel->setText(ToQString(InfoStr));
  CopyParamLabel->setToolTip(ToQString(FCopyParams.GetInfoStr(L"\n", ActualCopyParamAttrs())));

  QueueCheck2->setEnabled((FOptions & (coDisableQueue | coTemp)) == 0);
}
//---------------------------------------------------------------------------
bool TCopyDialog::Execute()
{
  // at start assume that copy param is current preset
  FPreset = GUIConfiguration->CopyParamCurrent;
  DirectoryEdit->SetHistoryKey(FToRemote ? L"RemoteTarget" : L"LocalTarget");
  if (DirectoryEdit->isEnabled() && !DirectoryEdit->isHidden())
  {
    DirectoryEdit->setFocus();
    DirectoryEdit->lineEdit()->selectAll();
  }
  else
  {
    OkButton->setFocus();
  }
  UpdateControls();
  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    Configuration->BeginUpdate();
    ::rtl::TFinally Finally([&]() { Configuration->EndUpdate(); });
    if (FLAGSET(GetOutputOptions(), cooSaveSettings))
    {
      GUIConfiguration->DefaultCopyParam = GetParams();
    }
    DirectoryEdit->SaveToHistory();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCopyDialog::accept()
{
  if (!FToRemote)
  {
    LocalDirectoryEditExit();
  }

  bool CanClose = true;
  if (!FToRemote && ((FOptions & coTemp) == 0))
  {
    CanClose = CopyDialogValidateLocalDirectory(GetDirectory(), DirectoryEdit);
  }

  if (CanClose)
  {
    CanClose = CopyDialogValidateFileMask(GetFileMask(), DirectoryEdit, (FFileList->Count > 1), FToRemote);
  }

  if (CanClose)
  {
    QDialog::accept();
  }
}
//---------------------------------------------------------------------------
bool TCopyDialog::eventFilter(QObject * Object, QEvent * Event)
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
      CopyParamListPopup(static_cast<QContextMenuEvent *>(Event)->globalPos(), cplCustomizeDefault);
      return true;
    }
  }
  else if ((Object == DirectoryEdit) && (Event->type() == QEvent::FocusOut) && !FToRemote)
  {
    LocalDirectoryEditExit();
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
void TCopyDialog::LocalDirectoryBrowseButtonClick()
{
  DebugAssert(!FToRemote);
  QString ADirectory =
    QFileDialog::getExistingDirectory(this, ToQString(LoadStr(SELECT_LOCAL_DIRECTORY)), ToQString(GetDirectory()));
  if (!ADirectory.isEmpty())
  {
    SetDirectory(ToUnicodeString(ADirectory));
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TCopyDialog::LocalDirectoryEditExit()
{
  // A directory is the target, not a new name of the file
  if (DirectoryExists(ApiPath(DirectoryEdit->GetText())))
  {
    DirectoryEdit->SetText(IncludeTrailingBackslash(DirectoryEdit->GetText()) + AnyMask);
  }
}
//---------------------------------------------------------------------------
void TCopyDialog::CopyParamGroupClick()
{
  if (CopyParamGroup->isEnabled())
  {
    if (DoCopyParamCustomDialog(FCopyParams, ActualCopyParamAttrs()))
    {
      UpdateControls();
    }
  }
}
//---------------------------------------------------------------------------
void TCopyDialog::CopyParamListPopup(const QPoint & Pos, int AdditionalOptions)
{
  // Save including the preset-unspecific queue properties,
  // so that they are preserved when assigning back later
  TGUICopyParamType Param = GetParams();
  bool PrevSaveSettings = FSaveSettings;
  int Result =
    ::CopyParamListPopup(Pos, Param, FPreset, AdditionalOptions | cplSaveSettings, ActualCopyParamAttrs(), &FSaveSettings);
  if (Result > 0)
  {
    SetParams(Param);
  }
  else
  {
    UpdateControls();
  }

  if (PrevSaveSettings && !FSaveSettings)
  {
    NeverShowAgainCheck->setChecked(false);
  }
}
//---------------------------------------------------------------------------
bool DoCopyDialog(
  bool ToRemote, bool Move, TStrings * FileList, UnicodeString & TargetDirectory,
  TGUICopyParamType * Params, int Options, int CopyParamAttrs, int * OutputOptions)
{
  TCopyDialog Dialog(ToRemote, Move, FileList, Options, CopyParamAttrs);
  if (FLAGSET(CopyParamAttrs, cpaNoTransferMode))
  {
    // If local and remote EOL types are the same, there is no need
    // for ASCII (or Automatic) mode
    Params->TransferMode = tmBinary;
  }
  if (OutputOptions != NULL)
  {
    Dialog.SetOutputOptions(*OutputOptions);
  }
  Dialog.SetDirectory(TargetDirectory);
  Dialog.SetParams(*Params);
  bool Result = Dialog.Execute();
  if (Result)
  {
    TargetDirectory = Dialog.GetDirectory();
    *Params = Dialog.GetParams();
    if (OutputOptions != NULL)
    {
      *OutputOptions = Dialog.GetOutputOptions();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
class TCreateDirectoryDialog : public TCustomDialog
{
public:
  TCreateDirectoryDialog();

  bool Execute(UnicodeString & Directory);

protected:
  virtual void DoChange(bool & CanSubmit) override;

private:
  QLineEdit * DirectoryEdit;
};
//---------------------------------------------------------------------------
TCreateDirectoryDialog::TCreateDirectoryDialog()
{
  setObjectName(QStringLiteral("CreateDirectoryDialog"));
  SetHelpKeyword(FormHelpKeyword(L"CreateDirectory.dfm"));
  setWindowTitle(tr("Create folder"));
  QLabel * EditLabel = CreateLabel(L"New &folder name:");
  EditLabel->setObjectName(QStringLiteral("EditLabel"));
  DirectoryEdit = new QLineEdit(this);
  DirectoryEdit->setObjectName(QStringLiteral("DirectoryEdit"));
  AddEditLikeControl(DirectoryEdit, EditLabel);
  setMinimumWidth(fontMetrics().averageCharWidth() * 50);
}
//---------------------------------------------------------------------------
void TCreateDirectoryDialog::DoChange(bool & CanSubmit)
{
  CanSubmit = !DirectoryEdit->text().trimmed().isEmpty();
}
//---------------------------------------------------------------------------
bool TCreateDirectoryDialog::Execute(UnicodeString & Directory)
{
  DirectoryEdit->setText(ToQString(Directory));
  DirectoryEdit->selectAll();
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    Directory = ToUnicodeString(DirectoryEdit->text()).Trim();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool DoCreateDirectoryDialog(UnicodeString & Directory)
{
  TCreateDirectoryDialog Dialog;
  return Dialog.Execute(Directory);
}
//---------------------------------------------------------------------------
