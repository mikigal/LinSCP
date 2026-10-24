// The base of simple dialogs and the input dialog (upstream's
// forms/Custom.cpp and forms/InputDlg.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>

#include "CustomDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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

  QDialogButtonBox * Buttons = new QDialogButtonBox(this);
  OKButton = Buttons->addButton(QDialogButtonBox::Ok);
  OKButton->setObjectName(QStringLiteral("OKButton"));
  CancelButton = Buttons->addButton(QDialogButtonBox::Cancel);
  CancelButton->setObjectName(QStringLiteral("CancelButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
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
