// The duplication of remote files (upstream's TRemoteTransferDialog,
// forms/RemoteTransfer.dfm): the target session and path, directly or via
// a local temporary copy.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <FileMasks.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
class TRemoteTransferDialog : public QDialog
{
public:
  TRemoteTransferDialog();

  void Init(bool Multi, TStrings * Sessions, TStrings * Directories, TDirectRemoteCopy AllowDirectCopy,
    void * CurrentSession, TDirectoryExistsEvent OnDirectoryExists, bool TargetConfirmed);
  bool Execute(void *& Session, UnicodeString & Target, UnicodeString & FileMask, bool & DirectCopy);

protected:
  virtual void accept() override;

private:
  QComboBox * SessionCombo;
  THistoryComboBox * DirectoryEdit;
  QCheckBox * NotDirectCopyCheck;
  QPushButton * OkButton;
  bool FMulti;
  TStrings * FSessions;
  TStrings * FDirectories;
  TDirectRemoteCopy FAllowDirectCopy;
  void * FCurrentSession;
  TDirectoryExistsEvent FOnDirectoryExists;
  bool FTargetConfirmed;
  bool FDirectCopy;
  UnicodeString FOriginalTarget;

  void UpdateControls();
  void SessionComboChange();
  void UpdateNotDirectCopyCheck();
  void * GetSelectedSession();
  bool IsCurrentSessionSelected();
  UnicodeString GetTarget();
  UnicodeString GetFileMask();
};
//---------------------------------------------------------------------------
TRemoteTransferDialog::TRemoteTransferDialog() :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("RemoteTransferDialog"));
  setWindowTitle(ToQString(LoadStr(REMOTE_COPY_TITLE)));
  FMulti = false;
  FSessions = NULL;
  FDirectories = NULL;
  FAllowDirectCopy = drcDisallow;
  FCurrentSession = NULL;
  FTargetConfirmed = false;
  FDirectCopy = false;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QGroupBox * Group = new QGroupBox(this);
  Group->setObjectName(QStringLiteral("Group"));
  QVBoxLayout * GroupLayout = new QVBoxLayout(Group);
  QLabel * SessionLabel = new QLabel(tr("Target &session:"), Group);
  SessionLabel->setObjectName(QStringLiteral("SessionLabel"));
  GroupLayout->addWidget(SessionLabel);
  SessionCombo = new QComboBox(Group);
  SessionCombo->setObjectName(QStringLiteral("SessionCombo"));
  SessionLabel->setBuddy(SessionCombo);
  GroupLayout->addWidget(SessionCombo);
  QLabel * Label3 = new QLabel(tr("Target remote &path:"), Group);
  Label3->setObjectName(QStringLiteral("Label3"));
  GroupLayout->addWidget(Label3);
  DirectoryEdit = new THistoryComboBox(Group);
  DirectoryEdit->setObjectName(QStringLiteral("DirectoryEdit"));
  Label3->setBuddy(DirectoryEdit);
  connect(DirectoryEdit, &QComboBox::currentTextChanged, this, [this]() { UpdateControls(); });
  GroupLayout->addWidget(DirectoryEdit);
  NotDirectCopyCheck = new QCheckBox(tr("Duplicate via local &temporary copy"), Group);
  NotDirectCopyCheck->setObjectName(QStringLiteral("NotDirectCopyCheck"));
  connect(NotDirectCopyCheck, &QCheckBox::clicked, this, [this]()
  {
    if (IsCurrentSessionSelected())
    {
      FDirectCopy = !NotDirectCopyCheck->isChecked();
    }
  });
  GroupLayout->addWidget(NotDirectCopyCheck);
  MainLayout->addWidget(Group);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  OkButton = Buttons->button(QDialogButtonBox::Ok);
  OkButton->setObjectName(QStringLiteral("OkButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
  AddHelpButton(Buttons, FormHelpKeyword(L"RemoteTransfer.dfm"));
  setMinimumWidth(fontMetrics().averageCharWidth() * 70);
}
//---------------------------------------------------------------------------
void TRemoteTransferDialog::Init(bool Multi, TStrings * Sessions, TStrings * Directories,
  TDirectRemoteCopy AllowDirectCopy, void * CurrentSession, TDirectoryExistsEvent OnDirectoryExists, bool TargetConfirmed)
{
  FMulti = Multi;
  FSessions = Sessions;
  for (int Index = 0; Index < Sessions->Count; Index++)
  {
    SessionCombo->addItem(ToQString(Sessions->Strings[Index]));
  }
  FDirectories = Directories;
  DebugAssert(SessionCombo->count() > 0);
  DebugAssert(SessionCombo->count() == FDirectories->Count);
  FAllowDirectCopy = AllowDirectCopy;
  FCurrentSession = CurrentSession;
  FOnDirectoryExists = OnDirectoryExists;
  FTargetConfirmed = TargetConfirmed;
  connect(SessionCombo, &QComboBox::currentIndexChanged, this, [this]() { SessionComboChange(); });
}
//---------------------------------------------------------------------------
bool TRemoteTransferDialog::Execute(void *& Session, UnicodeString & Target,
  UnicodeString & FileMask, bool & DirectCopy)
{
  for (int Index = 0; Index < FSessions->Count; Index++)
  {
    if (FSessions->Objects[Index] == Session)
    {
      QSignalBlocker Blocker(SessionCombo);
      SessionCombo->setCurrentIndex(Index);
      break;
    }
  }
  DirectoryEdit->SetHistoryKey(L"RemoteTarget");
  DirectoryEdit->SetText(UnixIncludeTrailingBackslash(Target) + FileMask);
  DirectoryEdit->setFocus();
  FDirectCopy = DirectCopy;
  FOriginalTarget = Target;
  UpdateNotDirectCopyCheck();
  UpdateControls();
  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    Session = GetSelectedSession();
    DirectoryEdit->SaveToHistory();
    Target = GetTarget();
    FileMask = GetFileMask();
    DirectCopy = !NotDirectCopyCheck->isChecked();
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteTransferDialog::GetTarget()
{
  return UnixExtractFilePath(DirectoryEdit->GetText());
}
//---------------------------------------------------------------------------
UnicodeString TRemoteTransferDialog::GetFileMask()
{
  return UnixExtractFileName(DirectoryEdit->GetText());
}
//---------------------------------------------------------------------------
void TRemoteTransferDialog::UpdateControls()
{
  NotDirectCopyCheck->setEnabled(IsCurrentSessionSelected() && (FAllowDirectCopy != drcDisallow));
  OkButton->setEnabled(!DirectoryEdit->currentText().isEmpty());
}
//---------------------------------------------------------------------------
void TRemoteTransferDialog::SessionComboChange()
{
  DirectoryEdit->SetText(
    UnixIncludeTrailingBackslash(FDirectories->Strings[SessionCombo->currentIndex()]) +
    UnixExtractFileName(DirectoryEdit->GetText()));
  UpdateNotDirectCopyCheck();
  UpdateControls();
}
//---------------------------------------------------------------------------
void TRemoteTransferDialog::UpdateNotDirectCopyCheck()
{
  if (IsCurrentSessionSelected())
  {
    NotDirectCopyCheck->setChecked(!FDirectCopy);
  }
  else
  {
    NotDirectCopyCheck->setChecked(true);
  }
}
//---------------------------------------------------------------------------
void TRemoteTransferDialog::accept()
{
  try
  {
    bool TargetConfirmed =
      FTargetConfirmed &&
      UnixSamePath(GetTarget(), FOriginalTarget);

    if (!TargetConfirmed &&
        !IsFileNameMask(GetFileMask()) &&
        FOnDirectoryExists(GetSelectedSession(), DirectoryEdit->GetText()))
    {
      DirectoryEdit->SetText(UnixCombinePaths(DirectoryEdit->GetText(), AnyMask));
    }

    if (!IsFileNameMask(GetFileMask()) && FMulti)
    {
      UnicodeString Message =
        FormatMultiFilesToOneConfirmation(DirectoryEdit->GetText(), true);
      if (MessageDialog(Message, qtConfirmation, qaOK | qaCancel) == qaCancel)
      {
        Abort();
      }
    }

    if (IsCurrentSessionSelected() &&
        ((FAllowDirectCopy == drcConfirmCommandSession) || (FAllowDirectCopy == drcConfirmCommandSessionDirs)) &&
        !NotDirectCopyCheck->isChecked() &&
        GUIConfiguration->ConfirmCommandSession)
    {
      TMessageParams Params(mpNeverAskAgainCheck);
      int ObjectNamePart = (FAllowDirectCopy == drcConfirmCommandSession) ? 1 : 2;
      UnicodeString ObjectName = LoadStrPart(REMOTE_COPY_COMMAND_SESSION_FILES_DIRECTORIES, ObjectNamePart);
      UnicodeString Message = FMTLOAD(REMOTE_COPY_COMMAND_SESSION3, (ObjectName, ObjectName, ObjectName));
      unsigned int Answer = MessageDialog(Message, qtConfirmation, qaOK | qaCancel, &Params);
      if (Answer == qaNeverAskAgain)
      {
        GUIConfiguration->ConfirmCommandSession = false;
      }
      else if (Answer != qaOK)
      {
        Abort();
      }
    }
  }
  catch (EAbort &)
  {
    return;
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
void * TRemoteTransferDialog::GetSelectedSession()
{
  return FSessions->Objects[SessionCombo->currentIndex()];
}
//---------------------------------------------------------------------------
bool TRemoteTransferDialog::IsCurrentSessionSelected()
{
  return (GetSelectedSession() == FCurrentSession);
}
//---------------------------------------------------------------------------
bool DoRemoteCopyDialog(
  TStrings * Sessions, TStrings * Directories,
  TDirectRemoteCopy AllowDirectCopy, bool Multi, void *& Session, UnicodeString & Target, UnicodeString & FileMask,
  bool & DirectCopy, void * CurrentSession, TDirectoryExistsEvent OnDirectoryExists, bool TargetConfirmed)
{
  TRemoteTransferDialog Dialog;
  Dialog.Init(Multi, Sessions, Directories, AllowDirectCopy, CurrentSession, OnDirectoryExists, TargetConfirmed);
  return Dialog.Execute(Session, Target, FileMask, DirectCopy);
}
//---------------------------------------------------------------------------
