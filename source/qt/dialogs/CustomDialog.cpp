// The base of simple dialogs and the input dialog (upstream's
// forms/Custom.cpp and forms/InputDlg.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Cryptography.h>
#include <FileMasks.h>
#include <PuttyTools.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"
#include "ShortCuts.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextBlock>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
// As upstream's THistoryComboBox
static const int MaxHistorySize = 30;
//---------------------------------------------------------------------------
TCustomDialog::TCustomDialog(QWidget * Parent) :
  QDialog(Parent != nullptr ? Parent : QApplication::activeWindow())
{
  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  FControlsLayout = new QVBoxLayout();
  MainLayout->addLayout(FControlsLayout);
  MainLayout->addStretch(1);

  FButtons = new QDialogButtonBox(this);
  OKButton = FButtons->addButton(QDialogButtonBox::Ok);
  OKButton->setObjectName(QStringLiteral("OKButton"));
  CancelButton = FButtons->addButton(QDialogButtonBox::Cancel);
  CancelButton->setObjectName(QStringLiteral("CancelButton"));
  connect(FButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(FButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(FButtons);
}
//---------------------------------------------------------------------------
void TCustomDialog::AddDialogButton(QPushButton * Button)
{
  Button->setAutoDefault(false);
  FButtons->addButton(Button, QDialogButtonBox::ActionRole);
}
//---------------------------------------------------------------------------
QLabel * TCustomDialog::CreateLabel(const UnicodeString & Caption)
{
  QLabel * Label = new QLabel(ToQString(Caption), this);
  return Label;
}
//---------------------------------------------------------------------------
void TCustomDialog::AddEditLikeControl(QWidget * Edit, QLabel * Label)
{
  if (Label != nullptr)
  {
    Label->setBuddy(Edit);
    FControlsLayout->addWidget(Label);
  }
  FControlsLayout->addWidget(Edit);
  if (QLineEdit * LineEdit = qobject_cast<QLineEdit *>(Edit))
  {
    connect(LineEdit, &QLineEdit::textChanged, this, [this]() { Change(); });
  }
  else if (QComboBox * Combo = qobject_cast<QComboBox *>(Edit))
  {
    connect(Combo, &QComboBox::currentTextChanged, this, [this]() { Change(); });
  }
}
//---------------------------------------------------------------------------
QCheckBox * TCustomDialog::CreateAndAddCheckBox(const UnicodeString & Caption)
{
  QCheckBox * CheckBox = new QCheckBox(ToQString(Caption), this);
  connect(CheckBox, &QCheckBox::toggled, this, [this]() { Change(); });
  FControlsLayout->addWidget(CheckBox);
  return CheckBox;
}
//---------------------------------------------------------------------------
void TCustomDialog::AddWidget(QWidget * Widget)
{
  FControlsLayout->addWidget(Widget);
}
//---------------------------------------------------------------------------
void TCustomDialog::Change()
{
  bool CanSubmit = true;
  DoChange(CanSubmit);
  OKButton->setEnabled(CanSubmit);
}
//---------------------------------------------------------------------------
void TCustomDialog::DoChange(bool & /*CanSubmit*/)
{
}
//---------------------------------------------------------------------------
void TCustomDialog::DoValidate()
{
}
//---------------------------------------------------------------------------
void TCustomDialog::accept()
{
  try
  {
    DoValidate();
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
    return;
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
bool TCustomDialog::Execute()
{
  Change();
  return (exec() == QDialog::Accepted);
}
//---------------------------------------------------------------------------
void SaveToHistory(QComboBox * Combo)
{
  QString Text = Combo->currentText();
  if (!Text.isEmpty())
  {
    int Index;
    while ((Index = Combo->findText(Text, Qt::MatchExactly | Qt::MatchCaseSensitive)) >= 0)
    {
      Combo->removeItem(Index);
    }
    Combo->insertItem(0, Text);
    while (Combo->count() > MaxHistorySize)
    {
      Combo->removeItem(Combo->count() - 1);
    }
    Combo->setCurrentIndex(0);
  }
}
//---------------------------------------------------------------------------
static void ValidateMaskEditT(const UnicodeString & Mask, QWidget * Edit, int ForceDirectoryMasks,
  std::function<void(int Start, int Length)> Select)
{
  try
  {
    TFileMasks Masks(ForceDirectoryMasks);
    Masks = Mask;
  }
  catch (EFileMasksException & E)
  {
    ShowExtendedException(&E);
    Edit->setFocus();
    Select(E.ErrorStart - 1, E.ErrorLen);
    Abort();
  }
}
//---------------------------------------------------------------------------
void ValidateMaskEdit(QComboBox * Edit, int ForceDirectoryMasks)
{
  ValidateMaskEditT(ToUnicodeString(Edit->currentText()), Edit, ForceDirectoryMasks,
    [Edit](int Start, int Length) { Edit->lineEdit()->setSelection(Start, Length); });
}
//---------------------------------------------------------------------------
void ValidateMaskEdit(QLineEdit * Edit, int ForceDirectoryMasks)
{
  ValidateMaskEditT(ToUnicodeString(Edit->text()), Edit, ForceDirectoryMasks,
    [Edit](int Start, int Length) { Edit->setSelection(Start, Length); });
}
//---------------------------------------------------------------------------
void ValidateMaskEdit(QPlainTextEdit * Edit, bool Directory)
{
  std::unique_ptr<TStrings> Lines(TextToStringList(ToUnicodeString(Edit->toPlainText())));
  UnicodeString Mask = TFileMasks::ComposeMaskStr(Lines.get(), Directory);
  // The position is in the composed mask, which is not the text of the
  // edit; the whole text is selected
  ValidateMaskEditT(Mask, Edit, Directory ? 1 : 0,
    [Edit](int, int) { Edit->selectAll(); });
}
//---------------------------------------------------------------------------
void BrowseForExecutable(QComboBox * Control, const UnicodeString & Title, bool FileNameCommand)
{
  UnicodeString Executable = ToUnicodeString(Control->currentText());
  if (FileNameCommand)
  {
    ReformatFileNameCommand(Executable);
  }
  UnicodeString Program, Params, Dir;
  SplitCommand(Executable, Program, Params, Dir);
  UnicodeString ExpandedProgram = ExpandEnvironmentVariables(Program);
  QString FileName = QFileDialog::getOpenFileName(Control->window(), ToQString(Title), ToQString(ExpandedProgram));
  if (!FileName.isEmpty())
  {
    // preserve unexpanded file, if the destination has not changed actually
    if (!IsPathToSameFile(ExpandedProgram, ToUnicodeString(FileName)))
    {
      Program = ToUnicodeString(FileName);
    }
    Control->setEditText(ToQString(FormatCommand(Program, Params)));
  }
}
//---------------------------------------------------------------------------
class TInputDialog : public TCustomDialog
{
public:
  TInputDialog(const UnicodeString & Caption, const UnicodeString & Prompt,
    TStrings * History, bool Echo);

  bool Execute(UnicodeString & Value);

private:
  TStrings * FHistory;
  QLineEdit * FEdit;
  QComboBox * FHistoryCombo;
};
//---------------------------------------------------------------------------
TInputDialog::TInputDialog(const UnicodeString & Caption, const UnicodeString & Prompt,
    TStrings * History, bool Echo)
{
  setObjectName(QStringLiteral("InputDialog"));
  setWindowTitle(ToQString(Caption));
  FHistory = History;
  QLabel * Label = CreateLabel(Prompt);
  if (History == NULL)
  {
    FEdit = new QLineEdit(this);
    FEdit->setObjectName(QStringLiteral("Edit"));
    if (!Echo)
    {
      FEdit->setEchoMode(QLineEdit::Password);
    }
    FHistoryCombo = nullptr;
    AddEditLikeControl(FEdit, Label);
  }
  else
  {
    DebugAssert(Echo);
    FHistoryCombo = new QComboBox(this);
    FHistoryCombo->setObjectName(QStringLiteral("HistoryCombo"));
    FHistoryCombo->setEditable(true);
    FHistoryCombo->setInsertPolicy(QComboBox::NoInsert);
    FEdit = nullptr;
    AddEditLikeControl(FHistoryCombo, Label);
  }
  setMinimumWidth(fontMetrics().averageCharWidth() * 50);
}
//---------------------------------------------------------------------------
bool TInputDialog::Execute(UnicodeString & Value)
{
  if (FHistory == NULL)
  {
    FEdit->setText(ToQString(Value));
    FEdit->selectAll();
  }
  else
  {
    for (int Index = 0; Index < FHistory->Count; Index++)
    {
      FHistoryCombo->addItem(ToQString(FHistory->Strings[Index]));
    }
    FHistoryCombo->setEditText(ToQString(Value));
    FHistoryCombo->lineEdit()->selectAll();
  }
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    if (FHistory != NULL)
    {
      Value = ToUnicodeString(FHistoryCombo->currentText());
      SaveToHistory(FHistoryCombo);
      FHistory->Clear();
      for (int Index = 0; Index < FHistoryCombo->count(); Index++)
      {
        FHistory->Add(ToUnicodeString(FHistoryCombo->itemText(Index)));
      }
    }
    else
    {
      Value = ToUnicodeString(FEdit->text());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool InputDialog(const UnicodeString & ACaption,
  const UnicodeString & APrompt, UnicodeString & Value,
  TStrings * History, bool /*PathInput*/, bool Echo)
{
  TInputDialog Dialog(ACaption, APrompt, History, Echo);
  return Dialog.Execute(Value);
}
//---------------------------------------------------------------------------
static void AskForMasterPasswordIfNotSetAndNeededToPersistSessionData(TSessionData * SessionData)
{
  if (!Configuration->DisablePasswordStoring &&
      SessionData->HasAnyPassword() &&
      AppConfiguration->UseMasterPassword)
  {
    AppConfiguration->AskForMasterPasswordIfNotSet();
  }
}
//---------------------------------------------------------------------------
class TSaveSessionDialog : public TCustomDialog
{
public:
  TSaveSessionDialog(bool CanSavePassword, bool NotRecommendedSavingPassword, TStrings * AdditionalFolders);

  bool Execute(UnicodeString & SessionName, bool & SavePassword, const UnicodeString & OriginalSessionName);

protected:
  virtual void DoValidate() override;
  virtual void DoChange(bool & CanSubmit) override;

private:
  UnicodeString FOriginalSessionName;
  QLineEdit * SessionNameEdit;
  QComboBox * FolderCombo;
  QCheckBox * SavePasswordCheck;
  UnicodeString FRootFolder;

  UnicodeString GetSessionName();
};
//---------------------------------------------------------------------------
TSaveSessionDialog::TSaveSessionDialog(
  bool CanSavePassword, bool NotRecommendedSavingPassword, TStrings * AdditionalFolders)
{
  setObjectName(QStringLiteral("SaveSessionDialog"));
  setWindowTitle(ToQString(LoadStr(SAVE_SESSION_CAPTION)));

  SessionNameEdit = new QLineEdit(this);
  SessionNameEdit->setObjectName(QStringLiteral("SessionNameEdit"));
  AddEditLikeControl(SessionNameEdit, CreateLabel(LoadStr(SAVE_SESSION_PROMPT)));

  FRootFolder = LoadStr(SAVE_SESSION_ROOT_FOLDER2);
  std::unique_ptr<TStringList> Folders(new TStringList());
  if (AdditionalFolders != NULL)
  {
    Folders->AddStrings(AdditionalFolders);
  }
  for (int Index = 0; Index < StoredSessions->Count; Index++)
  {
    TSessionData * Data = StoredSessions->Sessions[Index];
    if (!Data->Special && !Data->IsWorkspace)
    {
      UnicodeString Folder = Data->FolderName;
      if (!Folder.IsEmpty() && Folders->IndexOf(Folder) < 0)
      {
        Folders->Add(Folder);
      }
    }
  }
  Folders->Sort();
  FolderCombo = new QComboBox(this);
  FolderCombo->setObjectName(QStringLiteral("FolderCombo"));
  FolderCombo->setEditable(true);
  FolderCombo->setInsertPolicy(QComboBox::NoInsert);
  FolderCombo->addItem(ToQString(FRootFolder));
  for (int Index = 0; Index < Folders->Count; Index++)
  {
    FolderCombo->addItem(ToQString(Folders->Strings[Index]));
  }
  AddEditLikeControl(FolderCombo, CreateLabel(LoadStr(SAVE_SESSION_FOLDER)));

  int SavePasswordCaption =
    NotRecommendedSavingPassword ? SAVE_SESSION_PASSWORD :
      (AppConfiguration->UseMasterPassword ? SAVE_SESSION_PASSWORD_MASTER : SAVE_SESSION_PASSWORD_RECOMMENDED);
  SavePasswordCheck = CreateAndAddCheckBox(LoadStr(SavePasswordCaption));
  SavePasswordCheck->setObjectName(QStringLiteral("SavePasswordCheck"));
  SavePasswordCheck->setEnabled(CanSavePassword);
  setMinimumWidth(fontMetrics().averageCharWidth() * 50);
}
//---------------------------------------------------------------------------
bool TSaveSessionDialog::Execute(
  UnicodeString & SessionName, bool & SavePassword, const UnicodeString & OriginalSessionName)
{
  FOriginalSessionName = OriginalSessionName;
  SessionNameEdit->setText(ToQString(TSessionData::ExtractLocalName(SessionName)));
  UnicodeString Folder = TSessionData::ExtractFolderName(SessionName);
  FolderCombo->setEditText(ToQString(Folder.IsEmpty() ? FRootFolder : Folder));
  SavePasswordCheck->setChecked(SavePassword);
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    SessionName = GetSessionName();
    SavePassword = SavePasswordCheck->isChecked();
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TSaveSessionDialog::GetSessionName()
{
  UnicodeString Folder;
  UnicodeString FolderText = ToUnicodeString(FolderCombo->currentText());
  if (FolderText != FRootFolder)
  {
    Folder = FolderText;
  }
  return TSessionData::ComposePath(Folder, ToUnicodeString(SessionNameEdit->text()));
}
//---------------------------------------------------------------------------
void TSaveSessionDialog::DoValidate()
{
  TSessionData::ValidateName(ToUnicodeString(SessionNameEdit->text()));
  SessionNameValidate(GetSessionName(), FOriginalSessionName);
  UnicodeString Folder = TSessionData::ExtractFolderName(GetSessionName());
  if (!Folder.IsEmpty() && StoredSessions->IsWorkspace(Folder))
  {
    throw Exception(FMTLOAD(WORKSPACE_NOT_FOLDER, (Folder)));
  }
  if (SavePasswordCheck->isEnabled() && SavePasswordCheck->isChecked() &&
      AppConfiguration->UseMasterPassword)
  {
    AppConfiguration->AskForMasterPasswordIfNotSet();
  }
  TCustomDialog::DoValidate();
}
//---------------------------------------------------------------------------
void TSaveSessionDialog::DoChange(bool & CanSubmit)
{
  CanSubmit = !SessionNameEdit->text().isEmpty();
  TCustomDialog::DoChange(CanSubmit);
}
//---------------------------------------------------------------------------
TSessionData * DoSaveSession(TSessionData * SessionData,
  TSessionData * OriginalSession, bool ForceDialog, TStrings * AdditionalFolders)
{
  bool SavePassword = false;
  bool * PSavePassword;
  bool NotRecommendedSavingPassword =
    !AppConfiguration->UseMasterPassword &&
    !SameText(SessionData->UserName, AnonymousUserName);
  if (Configuration->DisablePasswordStoring ||
      !SessionData->HasAnySessionPassword())
  {
    PSavePassword = NULL;
  }
  else
  {
    PSavePassword = &SavePassword;
    SavePassword =
      ((OriginalSession != NULL) && OriginalSession->HasAnySessionPassword()) ||
      !NotRecommendedSavingPassword;
  }

  UnicodeString SessionName = SessionData->SessionName;
  bool Result;
  if (!ForceDialog && ((PSavePassword == NULL) || SavePassword))
  {
    // Asks before the site starts being saved, so that cancelling the
    // prompt does not leave it saved partially
    AskForMasterPasswordIfNotSetAndNeededToPersistSessionData(SessionData);
    Result = true;
  }
  else
  {
    TSaveSessionDialog Dialog((PSavePassword != NULL), NotRecommendedSavingPassword, AdditionalFolders);
    Result = Dialog.Execute(SessionName, SavePassword, SessionData->Name);
  }

  TSessionData * NewSession = NULL;
  if (Result)
  {
    if ((PSavePassword != NULL) && !SavePassword)
    {
      SessionData->ClearSessionPasswords();
    }

    NewSession = StoredSessions->NewSession(SessionName, SessionData);
    // modified only, explicit
    StoredSessions->Save(false, true);
    if (!SessionData->HostKey.IsEmpty())
    {
      SessionData->CacheHostKeyIfNotCached();
    }
  }
  return NewSession;
}
//---------------------------------------------------------------------------
class TSaveWorkspaceDialog : public TCustomDialog
{
public:
  TSaveWorkspaceDialog(bool CanSavePasswords, bool NotRecommendedSavingPasswords);

  bool Execute(
    UnicodeString & WorkspaceName, bool & SavePasswords, bool & CreateShortcut,
    bool & EnableAutoSave);

protected:
  virtual void DoValidate() override;
  virtual void DoChange(bool & CanSubmit) override;

private:
  QComboBox * WorkspaceNameCombo;
  QCheckBox * SavePasswordsCheck;
  QCheckBox * CreateShortcutCheck;
  QCheckBox * EnableAutoSaveCheck;
};
//---------------------------------------------------------------------------
TSaveWorkspaceDialog::TSaveWorkspaceDialog(
    bool CanSavePasswords, bool NotRecommendedSavingPasswords)
{
  setObjectName(QStringLiteral("SaveWorkspaceDialog"));
  setWindowTitle(ToQString(LoadStr(SAVE_WORKSPACE_CAPTION)));

  WorkspaceNameCombo = new QComboBox(this);
  WorkspaceNameCombo->setObjectName(QStringLiteral("WorkspaceNameCombo"));
  WorkspaceNameCombo->setEditable(true);
  WorkspaceNameCombo->setInsertPolicy(QComboBox::NoInsert);
  WorkspaceNameCombo->setCompleter(nullptr);
  AddEditLikeControl(WorkspaceNameCombo, CreateLabel(LoadStr(SAVE_WORKSPACE_PROMPT)));

  std::unique_ptr<TStrings> Workspaces(StoredSessions->GetWorkspaces());
  for (int Index = 0; Index < Workspaces->Count; Index++)
  {
    WorkspaceNameCombo->addItem(ToQString(Workspaces->Strings[Index]));
  }

  SavePasswordsCheck = CreateAndAddCheckBox(
    LoadStr(NotRecommendedSavingPasswords ? SAVE_WORKSPACE_PASSWORDS :
      (AppConfiguration->UseMasterPassword ?
        SAVE_WORKSPACE_PASSWORDS_MASTER : SAVE_WORKSPACE_PASSWORDS_RECOMMENDED)));
  SavePasswordsCheck->setObjectName(QStringLiteral("SavePasswordsCheck"));
  SavePasswordsCheck->setEnabled(CanSavePasswords);

  CreateShortcutCheck = CreateAndAddCheckBox(LoadStr(SAVE_SITE_WORKSPACE_SHORTCUT));
  CreateShortcutCheck->setObjectName(QStringLiteral("CreateShortcutCheck"));

  EnableAutoSaveCheck = CreateAndAddCheckBox(LoadStr(SAVE_WORKSPACE_AUTO));
  EnableAutoSaveCheck->setObjectName(QStringLiteral("EnableAutoSaveCheck"));
  setMinimumWidth(fontMetrics().averageCharWidth() * 50);
}
//---------------------------------------------------------------------------
bool TSaveWorkspaceDialog::Execute(
  UnicodeString & WorkspaceName, bool & SavePasswords, bool & CreateShortcut,
  bool & EnableAutoSave)
{
  WorkspaceNameCombo->setEditText(ToQString(WorkspaceName));
  SavePasswordsCheck->setChecked(SavePasswords);
  CreateShortcutCheck->setChecked(CreateShortcut);
  EnableAutoSaveCheck->setChecked(EnableAutoSave);
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    WorkspaceName = ToUnicodeString(WorkspaceNameCombo->currentText());
    SavePasswords = SavePasswordsCheck->isChecked();
    CreateShortcut = CreateShortcutCheck->isChecked();
    EnableAutoSave = EnableAutoSaveCheck->isChecked();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSaveWorkspaceDialog::DoValidate()
{
  UnicodeString Name = ToUnicodeString(WorkspaceNameCombo->currentText());
  TSessionData::ValidateName(Name);

  if (StoredSessions->IsFolder(Name))
  {
    throw Exception(FMTLOAD(FOLDER_NOT_WORKSPACE, (Name)));
  }

  if (SavePasswordsCheck->isEnabled() && SavePasswordsCheck->isChecked() &&
      AppConfiguration->UseMasterPassword)
  {
    AppConfiguration->AskForMasterPasswordIfNotSet();
  }

  TCustomDialog::DoValidate();
}
//---------------------------------------------------------------------------
void TSaveWorkspaceDialog::DoChange(bool & CanSubmit)
{
  CanSubmit = !WorkspaceNameCombo->currentText().isEmpty();

  TCustomDialog::DoChange(CanSubmit);
}
//---------------------------------------------------------------------------
bool DoSaveWorkspaceDialog(UnicodeString & WorkspaceName,
  bool * SavePasswords, bool NotRecommendedSavingPasswords,
  bool & CreateShortcut, bool & EnableAutoSave)
{
  TSaveWorkspaceDialog Dialog((SavePasswords != NULL), NotRecommendedSavingPasswords);

  bool Dummy = false;
  if (SavePasswords == NULL)
  {
    SavePasswords = &Dummy;
  }
  return
    Dialog.Execute(
      WorkspaceName, *SavePasswords, CreateShortcut, EnableAutoSave);
}
//---------------------------------------------------------------------------
void SessionNameValidate(const UnicodeString & Text, const UnicodeString & OriginalName)
{
  TSessionData::ValidatePath(Text);

  DebugAssert(StoredSessions);
  TSessionData * Data = static_cast<TSessionData *>(StoredSessions->FindByName(Text));
  if (Data && Data->Special)
  {
    MessageDialog(FMTLOAD(CANNOT_OVERWRITE_SPECIAL_SESSION, (Text)), qtError, qaOK);
    Abort();
  }
  else if ((Data != NULL) && !Data->IsSameName(OriginalName) &&
    MessageDialog(MainInstructions(FMTLOAD(CONFIRM_OVERWRITE_SESSION, (Text))),
      qtConfirmation, qaYes | qaNo) != qaYes)
  {
    Abort();
  }
}
//---------------------------------------------------------------------------
// The current master password, or a new one (upstream's
// TMasterPasswordDialog of windows/UserInterface.cpp)
class TMasterPasswordDialog : public TCustomDialog
{
public:
  explicit TMasterPasswordDialog(bool Current);

  bool Execute(UnicodeString & CurrentPassword, UnicodeString & NewPassword);

protected:
  virtual void DoValidate() override;
  virtual void DoChange(bool & CanSubmit) override;

private:
  QLineEdit * CurrentEdit;
  QLineEdit * NewEdit;
  QLineEdit * ConfirmEdit;

  QLineEdit * CreatePasswordEdit(const char * Name, int Label);
};
//---------------------------------------------------------------------------
TMasterPasswordDialog::TMasterPasswordDialog(bool Current)
{
  setObjectName(QStringLiteral("MasterPasswordDialog"));
  setWindowTitle(ToQString(LoadStr(MASTER_PASSWORD_CAPTION)));

  CurrentEdit = CreatePasswordEdit("CurrentEdit", MASTER_PASSWORD_CURRENT);
  CurrentEdit->setEnabled(Current || AppConfiguration->UseMasterPassword);

  if (!Current)
  {
    NewEdit = CreatePasswordEdit("NewEdit", MASTER_PASSWORD_NEW);

    if (!AppConfiguration->UseMasterPassword)
    {
      NewEdit->setFocus();
    }

    ConfirmEdit = CreatePasswordEdit("ConfirmEdit", MASTER_PASSWORD_CONFIRM);
  }
  else
  {
    NewEdit = NULL;
    ConfirmEdit = NULL;
  }
  setMinimumWidth(fontMetrics().averageCharWidth() * 50);
}
//---------------------------------------------------------------------------
QLineEdit * TMasterPasswordDialog::CreatePasswordEdit(const char * Name, int Label)
{
  QLineEdit * Result = new QLineEdit(this);
  Result->setObjectName(QString::fromLatin1(Name));
  Result->setEchoMode(QLineEdit::Password);
  Result->setMaxLength(PasswordMaxLength());
  AddEditLikeControl(Result, CreateLabel(LoadStr(Label)));
  return Result;
}
//---------------------------------------------------------------------------
bool TMasterPasswordDialog::Execute(UnicodeString & CurrentPassword, UnicodeString & NewPassword)
{
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    if (CurrentEdit->isEnabled())
    {
      CurrentPassword = ToUnicodeString(CurrentEdit->text());
    }
    if (NewEdit != NULL)
    {
      NewPassword = ToUnicodeString(NewEdit->text());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TMasterPasswordDialog::DoChange(bool & CanSubmit)
{
  CanSubmit =
    (!AppConfiguration->UseMasterPassword || (IsValidPassword(ToUnicodeString(CurrentEdit->text())) >= 0)) &&
    ((NewEdit == NULL) || (IsValidPassword(ToUnicodeString(NewEdit->text())) >= 0)) &&
    ((ConfirmEdit == NULL) || (IsValidPassword(ToUnicodeString(ConfirmEdit->text())) >= 0));
  TCustomDialog::DoChange(CanSubmit);
}
//---------------------------------------------------------------------------
void TMasterPasswordDialog::DoValidate()
{
  TCustomDialog::DoValidate();

  if (AppConfiguration->UseMasterPassword &&
      !AppConfiguration->ValidateMasterPassword(ToUnicodeString(CurrentEdit->text())))
  {
    CurrentEdit->setFocus();
    CurrentEdit->selectAll();
    throw Exception(MainInstructions(LoadStr(MASTER_PASSWORD_INCORRECT)));
  }

  if (NewEdit != NULL)
  {
    if (NewEdit->text() != ConfirmEdit->text())
    {
      ConfirmEdit->setFocus();
      ConfirmEdit->selectAll();
      throw Exception(MainInstructions(LoadStr(MASTER_PASSWORD_DIFFERENT)));
    }

    int Valid = IsValidPassword(ToUnicodeString(NewEdit->text()));
    if (Valid <= 0)
    {
      DebugAssert(Valid == 0);
      if (MessageDialog(LoadStr(MASTER_PASSWORD_SIMPLE2), qtWarning, qaOK | qaCancel) == qaCancel)
      {
        NewEdit->setFocus();
        NewEdit->selectAll();
        Abort();
      }
    }
  }
}
//---------------------------------------------------------------------------
static bool DoMasterPasswordDialog(bool Current, UnicodeString & NewPassword)
{
  TMasterPasswordDialog Dialog(Current);
  UnicodeString CurrentPassword;
  bool Result = Dialog.Execute(CurrentPassword, NewPassword);
  if (Result)
  {
    if ((Current || AppConfiguration->UseMasterPassword) &&
        DebugAlwaysTrue(!CurrentPassword.IsEmpty()))
    {
      AppConfiguration->SetMasterPassword(CurrentPassword);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool DoMasterPasswordDialog()
{
  UnicodeString NewPassword;
  bool Result = DoMasterPasswordDialog(true, NewPassword);
  DebugAssert(NewPassword.IsEmpty());
  return Result;
}
//---------------------------------------------------------------------------
bool DoChangeMasterPasswordDialog(UnicodeString & NewPassword)
{
  return DoMasterPasswordDialog(false, NewPassword);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TSshHostCADialog : public TCustomDialog
{
public:
  explicit TSshHostCADialog(bool Add);
  bool Execute(TSshHostCA & SshHostCA);

protected:
  virtual void DoChange(bool & CanSubmit) override;
  virtual void DoValidate() override;

private:
  QLineEdit * NameEdit;
  QLineEdit * PublicKeyEdit;
  QLineEdit * PublicKeyLabel;
  QLineEdit * ValidityExpressionEdit;
  QCheckBox * PermitRsaSha1Check;
  QCheckBox * PermitRsaSha256Check;
  QCheckBox * PermitRsaSha512Check;

  void BrowseButtonClick();
  QCheckBox * AddValidityCheckBox(QWidget * Parent, int CaptionStrPart);
  bool ValidatePublicKey(UnicodeString & Status);
};
//---------------------------------------------------------------------------
TSshHostCADialog::TSshHostCADialog(bool Add)
{
  setObjectName(QStringLiteral("SshHostCADialog"));
  setWindowTitle(ToQString(LoadStr(Add ? SSH_HOST_CA_ADD : SSH_HOST_CA_EDIT)));

  NameEdit = new QLineEdit(this);
  NameEdit->setObjectName(QStringLiteral("NameEdit"));
  AddEditLikeControl(NameEdit, CreateLabel(LoadStr(SSH_HOST_CA_NAME)));

  QWidget * PublicKeyRow = new QWidget(this);
  QHBoxLayout * PublicKeyLayout = new QHBoxLayout(PublicKeyRow);
  PublicKeyLayout->setContentsMargins(0, 0, 0, 0);
  PublicKeyEdit = new QLineEdit(PublicKeyRow);
  PublicKeyEdit->setObjectName(QStringLiteral("PublicKeyEdit"));
  PublicKeyLayout->addWidget(PublicKeyEdit, 1);
  QPushButton * BrowseButton = new QPushButton(ToQString(LoadStr(SSH_HOST_CA_BROWSE)), PublicKeyRow);
  BrowseButton->setObjectName(QStringLiteral("BrowseButton"));
  BrowseButton->setAutoDefault(false);
  connect(BrowseButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { BrowseButtonClick(); }); });
  PublicKeyLayout->addWidget(BrowseButton);
  QLabel * PublicKeyEditLabel = CreateLabel(LoadStr(SSH_HOST_CA_PUBLIC_KEY));
  PublicKeyEditLabel->setBuddy(PublicKeyEdit);
  AddWidget(PublicKeyEditLabel);
  AddWidget(PublicKeyRow);
  connect(PublicKeyEdit, &QLineEdit::textChanged, this, [this]() { Change(); });

  PublicKeyLabel = new QLineEdit(this);
  PublicKeyLabel->setObjectName(QStringLiteral("PublicKeyLabel"));
  PublicKeyLabel->setReadOnly(true);
  PublicKeyLabel->setFrame(false);
  PublicKeyLabel->setFocusPolicy(Qt::NoFocus);
  AddWidget(PublicKeyLabel);

  ValidityExpressionEdit = new QLineEdit(this);
  ValidityExpressionEdit->setObjectName(QStringLiteral("ValidityExpressionEdit"));
  AddEditLikeControl(ValidityExpressionEdit, CreateLabel(LoadStr(SSH_HOST_CA_PUBLIC_HOSTS)));

  QWidget * SignaturesRow = new QWidget(this);
  QHBoxLayout * SignaturesLayout = new QHBoxLayout(SignaturesRow);
  SignaturesLayout->setContentsMargins(0, 0, 0, 0);
  SignaturesLayout->addWidget(new QLabel(ToQString(LoadStr(SSH_HOST_CA_SIGNATURE_TYPES)), SignaturesRow));
  PermitRsaSha1Check = AddValidityCheckBox(SignaturesRow, 1);
  PermitRsaSha1Check->setObjectName(QStringLiteral("PermitRsaSha1Check"));
  PermitRsaSha256Check = AddValidityCheckBox(SignaturesRow, 2);
  PermitRsaSha256Check->setObjectName(QStringLiteral("PermitRsaSha256Check"));
  PermitRsaSha512Check = AddValidityCheckBox(SignaturesRow, 3);
  PermitRsaSha512Check->setObjectName(QStringLiteral("PermitRsaSha512Check"));
  SignaturesLayout->addStretch(1);
  AddWidget(SignaturesRow);
  setMinimumWidth(fontMetrics().averageCharWidth() * 80);
}
//---------------------------------------------------------------------------
QCheckBox * TSshHostCADialog::AddValidityCheckBox(QWidget * Parent, int CaptionStrPart)
{
  QCheckBox * Result = new QCheckBox(ToQString(LoadStrPart(SSH_HOST_CA_SIGNATURES, CaptionStrPart)), Parent);
  connect(Result, &QCheckBox::toggled, this, [this]() { Change(); });
  Parent->layout()->addWidget(Result);
  return Result;
}
//---------------------------------------------------------------------------
bool TSshHostCADialog::ValidatePublicKey(UnicodeString & Status)
{
  bool Result = false;
  UnicodeString PublicKeyText = ToUnicodeString(PublicKeyEdit->text()).Trim();
  if (PublicKeyText.IsEmpty())
  {
    Status = LoadStr(SSH_HOST_CA_NO_KEY);
  }
  else
  {
    RawByteString PublicKeyDummy;
    try
    {
      ParseCertificatePublicKey(PublicKeyText, PublicKeyDummy, Status);
      Result = true;
    }
    catch (Exception & E)
    {
      Status = E.Message;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSshHostCADialog::DoChange(bool & CanSubmit)
{
  TCustomDialog::DoChange(CanSubmit);

  UnicodeString PublicKeyStatus;
  ValidatePublicKey(PublicKeyStatus);
  // Should drop "SHA256:" prefix, the way we do in TSecureShell::VerifyHostKey
  PublicKeyLabel->setText(ToQString(PublicKeyStatus));

  CanSubmit = !NameEdit->text().trimmed().isEmpty() && !PublicKeyEdit->text().trimmed().isEmpty();
}
//---------------------------------------------------------------------------
void TSshHostCADialog::DoValidate()
{
  TCustomDialog::DoValidate();

  UnicodeString PublicKeyStatus;
  if (!ValidatePublicKey(PublicKeyStatus))
  {
    PublicKeyEdit->setFocus();
    throw Exception(MainInstructions(PublicKeyStatus));
  }

  if (ValidityExpressionEdit->text().trimmed().isEmpty())
  {
    ValidityExpressionEdit->setFocus();
    throw Exception(MainInstructions(LoadStr(SSH_HOST_CA_NO_HOSTS)));
  }

  UnicodeString Error;
  int ErrorStart, ErrorLen;
  if (!IsCertificateValidityExpressionValid(ToUnicodeString(ValidityExpressionEdit->text()), Error, ErrorStart, ErrorLen))
  {
    std::unique_ptr<TStrings> MoreMessages(TextToStringList(Error));
    MoreMessageDialog(MainInstructions(LoadStr(SSH_HOST_CA_HOSTS_INVALID)), MoreMessages.get(), qtError, qaOK);

    ValidityExpressionEdit->setFocus();
    ValidityExpressionEdit->setSelection(ErrorStart, ErrorLen);
    Abort();
  }
}
//---------------------------------------------------------------------------
bool TSshHostCADialog::Execute(TSshHostCA & SshHostCA)
{
  NameEdit->setText(ToQString(SshHostCA.Name));
  RawByteString PublicKey = SshHostCA.PublicKey;
  PublicKeyEdit->setText(ToQString(EncodeStrToBase64(PublicKey)));
  ValidityExpressionEdit->setText(ToQString(SshHostCA.ValidityExpression));
  PermitRsaSha1Check->setChecked(SshHostCA.PermitRsaSha1);
  PermitRsaSha256Check->setChecked(SshHostCA.PermitRsaSha256);
  PermitRsaSha512Check->setChecked(SshHostCA.PermitRsaSha512);
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    SshHostCA.Name = ToUnicodeString(NameEdit->text());
    SshHostCA.PublicKey = DecodeBase64ToStr(ToUnicodeString(PublicKeyEdit->text()));
    SshHostCA.ValidityExpression = ToUnicodeString(ValidityExpressionEdit->text());
    SshHostCA.PermitRsaSha1 = PermitRsaSha1Check->isChecked();
    SshHostCA.PermitRsaSha256 = PermitRsaSha256Check->isChecked();
    SshHostCA.PermitRsaSha512 = PermitRsaSha512Check->isChecked();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSshHostCADialog::BrowseButtonClick()
{
  QString FileName =
    QFileDialog::getOpenFileName(this, ToQString(LoadStr(SSH_HOST_CA_BROWSE_TITLE)), QString(),
      FileDialogFilter(LoadStr(SSH_HOST_CA_BROWSE_FILTER)));
  if (!FileName.isEmpty())
  {
    UnicodeString AFileName = ToUnicodeString(FileName);
    UnicodeString Algorithm, Comment;
    bool HasCertificate;
    RawByteString PublicKey;
    try
    {
      PublicKey = LoadPublicKey(AFileName, Algorithm, Comment, HasCertificate);
    }
    catch (Exception & E)
    {
      throw ExtException(&E, MainInstructions(FMTLOAD(SSH_HOST_CA_LOAD_ERROR, (AFileName))));
    }

    if (NameEdit->text().isEmpty())
    {
      NameEdit->setText(ToQString(Comment));
    }
    PublicKeyEdit->setText(ToQString(EncodeStrToBase64(PublicKey)));
  }
}
//---------------------------------------------------------------------------
bool DoSshHostCADialog(bool Add, TSshHostCA & SshHostCA)
{
  TSshHostCADialog Dialog(Add);
  return Dialog.Execute(SshHostCA);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTagDialog : public TCustomDialog
{
public:
  TTagDialog(bool Add, TStrings * Tags);
  bool Execute(UnicodeString & Key, UnicodeString & Value);

protected:
  virtual void DoChange(bool & CanSubmit) override;
  virtual void DoValidate() override;

private:
  QLineEdit * KeyEdit;
  QLineEdit * ValueEdit;
  TStrings * FTags;

  void ValidateTag(QLineEdit * Edit);
};
//---------------------------------------------------------------------------
TTagDialog::TTagDialog(bool Add, TStrings * Tags)
{
  setObjectName(QStringLiteral("TagDialog"));
  setWindowTitle(ToQString(LoadStr(Add ? TAG_ADD : TAG_EDIT)));
  FTags = Tags;

  KeyEdit = new QLineEdit(this);
  KeyEdit->setObjectName(QStringLiteral("KeyEdit"));
  KeyEdit->setMaxLength(128);
  AddEditLikeControl(KeyEdit, CreateLabel(LoadStr(TAG_KEY)));

  ValueEdit = new QLineEdit(this);
  ValueEdit->setObjectName(QStringLiteral("ValueEdit"));
  ValueEdit->setMaxLength(256);
  AddEditLikeControl(ValueEdit, CreateLabel(LoadStr(TAG_VALUE)));
  setMinimumWidth(fontMetrics().averageCharWidth() * 50);
}
//---------------------------------------------------------------------------
bool TTagDialog::Execute(UnicodeString & Key, UnicodeString & Value)
{
  // if we happen to get values longer than what we believed was possible, allow them.
  KeyEdit->setMaxLength(std::max(KeyEdit->maxLength(), Key.Length()));
  KeyEdit->setText(ToQString(Key));
  ValueEdit->setMaxLength(std::max(ValueEdit->maxLength(), Value.Length()));
  ValueEdit->setText(ToQString(Value));
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    Key = ToUnicodeString(KeyEdit->text());
    Value = ToUnicodeString(ValueEdit->text());
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTagDialog::DoChange(bool & CanSubmit)
{
  CanSubmit = !KeyEdit->text().isEmpty();
}
//---------------------------------------------------------------------------
void TTagDialog::ValidateTag(QLineEdit * Edit)
{
  // there are lot more in various scripts
  UnicodeString InvalidChars =
    L"!\"#$%&'()*,;<>?[\\]^`{|}~"
    L"\u00A1\u00A2\u00A3\u00A4\u00A5\u00A6\u00A7\u00A8\u00A9\u00AB\u00AC\u00AD\u00AE\u00AF"
    L"\u00B0\u00B1\u00B4\u00B6\u00B7\u00B8\u00BB\u00BF\u00D7\u00F7";
  UnicodeString Text = ToUnicodeString(Edit->text());
  for (int Index = 1; Index <= Text.Length(); Index++)
  {
    wchar_t Ch = Text[Index];
    if (iswcntrl(Ch) ||
        (InvalidChars.Pos(Ch) > 0))
    {
      UnicodeString Message = MainInstructions(FMTLOAD(TAG_INVALID_CHAR, (Ch)));
      if (MoreMessageDialog(Message, NULL, qtWarning, qaIgnore | qaAbort) != qaIgnore)
      {
        Edit->setFocus();
        Abort();
      }
      else
      {
        break;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTagDialog::DoValidate()
{
  UnicodeString Key = ToUnicodeString(KeyEdit->text());
  if (FTags->IndexOf(Key) >= 0)
  {
    throw Exception(MainInstructions(LoadStr(TAG_NOT_UNIQUE)));
  }

  ValidateTag(KeyEdit);
  ValidateTag(ValueEdit);

  TCustomDialog::DoValidate();
}
//---------------------------------------------------------------------------
bool DoTagDialog(bool Add, TStrings * Tags, UnicodeString & Key, UnicodeString & Value)
{
  TTagDialog Dialog(Add, Tags);
  return Dialog.Execute(Key, Value);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TRemoteMoveDialog : public TCustomDialog
{
public:
  TRemoteMoveDialog(bool Multi, TDirectoryExistsEvent OnDirectoryExists);

  bool Execute(UnicodeString & Target, UnicodeString & FileMask);

protected:
  virtual void DoValidate() override;
  virtual void DoChange(bool & CanSubmit) override;

private:
  THistoryComboBox * Combo;
  bool FMulti;
  TDirectoryExistsEvent FOnDirectoryExists;

  UnicodeString GetFileMask();
};
//---------------------------------------------------------------------------
TRemoteMoveDialog::TRemoteMoveDialog(bool Multi, TDirectoryExistsEvent OnDirectoryExists)
{
  setObjectName(QStringLiteral("RemoteMoveDialog"));
  setWindowTitle(ToQString(LoadStr(REMOTE_MOVE_TITLE)));
  FMulti = Multi;
  FOnDirectoryExists = OnDirectoryExists;

  Combo = new THistoryComboBox(this);
  Combo->setObjectName(QStringLiteral("Combo"));
  AddEditLikeControl(Combo, CreateLabel(LoadStr(REMOTE_TRANSFER_PROMPT2)));
  // The same as TRemoteTransferDialog
  setMinimumWidth(fontMetrics().averageCharWidth() * 70);
}
//---------------------------------------------------------------------------
bool TRemoteMoveDialog::Execute(UnicodeString & Target, UnicodeString & FileMask)
{
  Combo->SetHistoryKey(L"RemoteTarget");
  Combo->SetText(UnixIncludeTrailingBackslash(Target) + FileMask);
  Combo->lineEdit()->selectAll();
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    Target = UnixExtractFilePath(Combo->GetText());
    FileMask = GetFileMask();
    Combo->SaveToHistory();
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteMoveDialog::GetFileMask()
{
  return UnixExtractFileName(Combo->GetText());
}
//---------------------------------------------------------------------------
void TRemoteMoveDialog::DoChange(bool & CanSubmit)
{
  CanSubmit = !Combo->currentText().isEmpty();
}
//---------------------------------------------------------------------------
void TRemoteMoveDialog::DoValidate()
{
  if (FOnDirectoryExists(NULL, Combo->GetText()))
  {
    Combo->SetText(UnixCombinePaths(Combo->GetText(), AnyMask));
  }

  if (!IsFileNameMask(GetFileMask()) && FMulti)
  {
    UnicodeString Message =
      FormatMultiFilesToOneConfirmation(Combo->GetText(), true);
    if (MessageDialog(Message, qtConfirmation, qaOK | qaCancel) == qaCancel)
    {
      Abort();
    }
  }

  TCustomDialog::DoValidate();
}
//---------------------------------------------------------------------------
bool DoRemoteMoveDialog(
  bool Multi, UnicodeString & Target, UnicodeString & FileMask, TDirectoryExistsEvent OnDirectoryExists)
{
  TRemoteMoveDialog Dialog(Multi, OnDirectoryExists);
  return Dialog.Execute(Target, FileMask);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TShortCutDialog : public TCustomDialog
{
public:
  explicit TShortCutDialog(const TShortCuts & ShortCuts);

  bool Execute(TShortCut & ShortCut);

private:
  QComboBox * ShortCutCombo;
};
//---------------------------------------------------------------------------
TShortCutDialog::TShortCutDialog(const TShortCuts & ShortCuts)
{
  setObjectName(QStringLiteral("ShortCutDialog"));
  setWindowTitle(ToQString(LoadStr(SHORTCUT_CAPTION)));

  ShortCutCombo = new QComboBox(this);
  ShortCutCombo->setObjectName(QStringLiteral("ShortCutCombo"));
  InitializeShortCutCombo(ShortCutCombo, ShortCuts);
  ShortCutCombo->setMaxVisibleItems(std::max(ShortCutCombo->maxVisibleItems(), 15));
  AddEditLikeControl(ShortCutCombo, CreateLabel(LoadStr(SHORTCUT_LABEL)));
}
//---------------------------------------------------------------------------
bool TShortCutDialog::Execute(TShortCut & ShortCut)
{
  SetShortCutCombo(ShortCutCombo, ShortCut);
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    ShortCut = GetShortCutCombo(ShortCutCombo);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool DoShortCutDialog(TShortCut & ShortCut, const TShortCuts & ShortCuts)
{
  TShortCutDialog Dialog(ShortCuts);
  return Dialog.Execute(ShortCut);
}
//---------------------------------------------------------------------------
class TSiteRawAddDialog : public TCustomDialog
{
public:
  TSiteRawAddDialog(QWidget * Parent, TStrings * Names);

  QComboBox * AddComboBox;
};
//---------------------------------------------------------------------------
TSiteRawAddDialog::TSiteRawAddDialog(QWidget * Parent, TStrings * Names) :
  TCustomDialog(Parent)
{
  setObjectName(QStringLiteral("SiteRawAddDialog"));
  setWindowTitle(ToQString(LoadStr(SITE_RAW_ADD_CAPTION)));
  AddComboBox = new QComboBox(this);
  AddComboBox->setObjectName(QStringLiteral("AddComboBox"));
  for (int Index = 0; Index < Names->Count; Index++)
  {
    AddComboBox->addItem(ToQString(Names->Strings[Index]));
  }
  AddComboBox->setMaxVisibleItems(std::max(AddComboBox->maxVisibleItems(), 15));
  AddEditLikeControl(AddComboBox, CreateLabel(LoadStr(SITE_RAW_ADD_LABEL)));
}
//---------------------------------------------------------------------------
class TSiteRawDialog : public TCustomDialog
{
public:
  TSiteRawDialog();

  bool Execute(TSessionData * Data);

private:
  QPlainTextEdit * SettingsMemo;

  void AddButtonClick();
  void DeleteNames(TStrings * Names, TStrings * Options);
  TStrings * GetSettings();
};
//---------------------------------------------------------------------------
TSiteRawDialog::TSiteRawDialog()
{
  setObjectName(QStringLiteral("SiteRawDialog"));
  setWindowTitle(ToQString(LoadStr(SITE_RAW_CAPTION)));
  setMinimumWidth(fontMetrics().averageCharWidth() * 70);

  SettingsMemo = new QPlainTextEdit(this);
  SettingsMemo->setObjectName(QStringLiteral("SettingsMemo"));
  SettingsMemo->setTabChangesFocus(true);
  SettingsMemo->setMinimumHeight(fontMetrics().lineSpacing() * 20);
  AddWidget(SettingsMemo);

  QPushButton * AddButton = new QPushButton(ToQString(LoadStr(SITE_RAW_ADD)), this);
  AddButton->setObjectName(QStringLiteral("AddButton"));
  connect(AddButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddButtonClick(); }); });
  AddDialogButton(AddButton);
}
//---------------------------------------------------------------------------
bool TSiteRawDialog::Execute(TSessionData * Data)
{
  std::unique_ptr<TSessionData> FactoryDefaults(new TSessionData(L""));
  std::unique_ptr<TSessionData> RawData(new TSessionData(L""));
  RawData->Assign(Data);
  // SFTP-only is not reflected by the protocol prefix, the raw settings
  // have it
  if (RawData->FSProtocol != fsSFTPonly)
  {
    RawData->FSProtocol = FactoryDefaults->FSProtocol;
  }
  RawData->HostName = FactoryDefaults->HostName;
  RawData->PortNumber = FactoryDefaults->PortNumber;
  RawData->UserName = FactoryDefaults->UserName;
  RawData->Password = FactoryDefaults->Password;
  RawData->Ftps = FactoryDefaults->Ftps;

  std::unique_ptr<TStrings> Options(RawData->SaveToOptions(FactoryDefaults.get(), false, false));

  SettingsMemo->setPlainText(ToQString(Options->Text));

  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    std::unique_ptr<TSessionData> BackupData(new TSessionData(L""));
    BackupData->Assign(Data);
    Data->DefaultSettings();

    Data->FSProtocol = BackupData->FSProtocol;
    Data->HostName = BackupData->HostName;
    Data->PortNumber = BackupData->PortNumber;
    Data->UserName = BackupData->UserName;
    Data->Password = BackupData->Password;
    Data->Ftps = BackupData->Ftps;

    std::unique_ptr<TStrings> Settings(GetSettings());
    Data->ApplyRawSettings(Settings.get(), false);
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * TSiteRawDialog::GetSettings()
{
  return TextToStringList(ToUnicodeString(SettingsMemo->toPlainText()));
}
//---------------------------------------------------------------------------
void TSiteRawDialog::AddButtonClick()
{
  std::unique_ptr<TSessionData> FactoryDefaults(new TSessionData(L""));
  std::unique_ptr<TSessionData> BasicData(new TSessionData(L""));
  BasicData->FSProtocol = TFSProtocol(FactoryDefaults->FSProtocol + 1);
  UnicodeString RandomAppendix(L"_");
  BasicData->HostName = FactoryDefaults->HostName + RandomAppendix;
  BasicData->Ftps = TFtps(FactoryDefaults->Ftps + 1);
  BasicData->PortNumber = BasicData->GetDefaultPort() + 1;
  BasicData->UserName = FactoryDefaults->UserName + RandomAppendix;
  BasicData->Password = FactoryDefaults->Password + RandomAppendix;

  std::unique_ptr<TStrings> BasicOptions(BasicData->SaveToOptions(FactoryDefaults.get(), false, false));

  std::unique_ptr<TStrings> AllOptions(TSessionData::GetAllOptionNames(false));

  std::unique_ptr<TStrings> Names(CreateSortedStringList());
  for (int Index = 0; Index < AllOptions->Count; Index++)
  {
    Names->Add(AllOptions->Names[Index]);
  }
  DeleteNames(Names.get(), BasicOptions.get());
  std::unique_ptr<TStrings> Settings(GetSettings());
  DeleteNames(Names.get(), Settings.get());

  TSiteRawAddDialog AddDialog(this, Names.get());
  QComboBox * AddComboBox = AddDialog.AddComboBox;
  if ((AddComboBox->count() > 0) && AddDialog.Execute())
  {
    UnicodeString Name = ToUnicodeString(AddComboBox->currentText());
    UnicodeString Value = AllOptions->Values[Name];
    QString Text = SettingsMemo->toPlainText();
    if (!Text.isEmpty() && !Text.endsWith(QLatin1Char('\n')))
    {
      Text += QLatin1Char('\n');
    }
    Text += ToQString(Name) + QLatin1Char('=');
    int Start = Text.length();
    Text += ToQString(Value);
    SettingsMemo->setPlainText(Text);
    SettingsMemo->setFocus();
    QTextCursor Cursor = SettingsMemo->textCursor();
    Cursor.setPosition(Start);
    Cursor.setPosition(Start + ToQString(Value).length(), QTextCursor::KeepAnchor);
    SettingsMemo->setTextCursor(Cursor);
  }
}
//---------------------------------------------------------------------------
void TSiteRawDialog::DeleteNames(TStrings * Names, TStrings * Options)
{
  for (int Index = 0; Index < Options->Count; Index++)
  {
    UnicodeString Name = Options->Names[Index];
    if (!Name.IsEmpty())
    {
      int I = Names->IndexOf(Name);
      if (I >= 0)
      {
        Names->Delete(I);
      }
    }
  }
}
//---------------------------------------------------------------------------
void DoSiteRawDialog(TSessionData * Data)
{
  TSiteRawDialog Dialog;
  Dialog.Execute(Data);
}
//---------------------------------------------------------------------------
