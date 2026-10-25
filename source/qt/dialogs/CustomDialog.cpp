// The base of simple dialogs and the input dialog (upstream's
// forms/Custom.cpp and forms/InputDlg.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <FileMasks.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"
#include "ShortCuts.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
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
bool DoMasterPasswordDialog()
{
  bool Result;
  bool Valid;
  do
  {
    UnicodeString Password;
    Result = InputDialog(L"Master Password", LoadStr(CONSOLE_MASTER_PASSWORD_PROMPT).TrimRight(), Password, NULL, false, false);
    Valid = !Result || AppConfiguration->ValidateMasterPassword(Password);
    if (!Valid)
    {
      MessageDialog(LoadStr(MASTER_PASSWORD_INCORRECT), qtError, qaOK);
    }
    else if (Result)
    {
      AppConfiguration->SetMasterPassword(Password);
    }
  }
  while (!Valid);
  return Result;
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
