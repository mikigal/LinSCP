// The custom commands: adding, editing or entering one (upstream's
// TCustomCommandDialog, forms/CustomCommand.dfm), the parameters prompted
// for (TCustomCommandPromptsDialog and TWinInteractiveCustomCommand of
// windows/WinInterface.cpp) and the list of the commands (the "Commands"
// page of upstream's preferences, forms/Preferences.dfm, until they are
// ported).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Bookmarks.h>
#include <CustomCommands.h>
#include <Terminal.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"
#include "ShortCuts.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QToolTip>
#include <QTreeWidget>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
class TCustomCommandDialog : public QDialog
{
public:
  TCustomCommandDialog(const TCustomCommandList * CustomCommandList, TCustomCommandsMode Mode,
    int Options, TCustomCommandValidate OnValidate, const TShortCuts * ShortCuts);

  bool Execute(TCustomCommandType & Command);

protected:
  virtual void accept() override;

private:
  const TCustomCommandList * FCustomCommandList;
  TCustomCommandsMode FMode;
  int FOptions;
  int FParams;
  UnicodeString FOrigDescription;
  TCustomCommandValidate FOnValidate;

  QLineEdit * DescriptionEdit;
  THistoryComboBox * CommandEdit;
  QRadioButton * RemoteCommandButton;
  QRadioButton * LocalCommandButton;
  QCheckBox * ApplyToDirectoriesCheck;
  QCheckBox * RecursiveCheck;
  QCheckBox * ShowResultsCheck;
  QCheckBox * CopyResultsCheck;
  QCheckBox * RemoteFilesCheck;
  QComboBox * ShortCutCombo;
  QPushButton * OkButton;

  void UpdateControls();
  void SetParams(int Value);
  int GetParams();
  void GetCommand(TCustomCommandType & Command);
};
//---------------------------------------------------------------------------
QCheckBox * CreateCheck(QWidget * Parent, const char * Name, const QString & Caption)
{
  QCheckBox * Check = new QCheckBox(Caption, Parent);
  Check->setObjectName(QString::fromLatin1(Name));
  return Check;
}
//---------------------------------------------------------------------------
TCustomCommandDialog::TCustomCommandDialog(const TCustomCommandList * CustomCommandList,
  TCustomCommandsMode Mode, int Options, TCustomCommandValidate OnValidate, const TShortCuts * ShortCuts) :
  QDialog(QApplication::activeModalWidget() != nullptr ? QApplication::activeModalWidget() : QApplication::activeWindow())
{
  setObjectName(QStringLiteral("CustomCommandDialog"));
  FCustomCommandList = CustomCommandList;
  FMode = Mode;
  FOptions = Options;
  FParams = 0;
  FOnValidate = OnValidate;

  int CaptionRes;
  switch (FMode)
  {
    case ccmAdd:
      CaptionRes = CUSTOM_COMMAND_ADD;
      break;
    case ccmEdit:
      CaptionRes = CUSTOM_COMMAND_EDIT;
      break;
    case ccmAdHoc:
    default:
      CaptionRes = CUSTOM_COMMAND_AD_HOC;
      break;
  }
  setWindowTitle(ToQString(LoadStr(CaptionRes)));

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QGroupBox * Group = new QGroupBox(this);
  Group->setObjectName(QStringLiteral("Group"));
  QGridLayout * Layout = new QGridLayout(Group);
  int Row = 0;
  QLabel * DescriptionLabel = new QLabel(tr("&Description:"), Group);
  DescriptionLabel->setObjectName(QStringLiteral("DescriptionLabel"));
  DescriptionEdit = new QLineEdit(Group);
  DescriptionEdit->setObjectName(QStringLiteral("DescriptionEdit"));
  DescriptionLabel->setBuddy(DescriptionEdit);
  if (FMode != ccmAdHoc)
  {
    Layout->addWidget(DescriptionLabel, Row++, 0, 1, 2);
    Layout->addWidget(DescriptionEdit, Row++, 0, 1, 2);
  }
  else
  {
    DescriptionLabel->setVisible(false);
    DescriptionEdit->setVisible(false);
  }
  QLabel * Label1 = new QLabel(tr("&Custom command:"), Group);
  Label1->setObjectName(QStringLiteral("Label1"));
  Layout->addWidget(Label1, Row++, 0, 1, 2);
  CommandEdit = new THistoryComboBox(Group);
  CommandEdit->setObjectName(QStringLiteral("CommandEdit"));
  CommandEdit->setMinimumWidth(fontMetrics().averageCharWidth() * 60);
  Label1->setBuddy(CommandEdit);
  Layout->addWidget(CommandEdit, Row++, 0, 1, 2);
  // The patterns in a tooltip (upstream's hint of HintText)
  std::unique_ptr<TStrings> HintStrings(TextToStringList(LoadStr(CUSTOM_COMMAND_PATTERNS_HINT5)));
  HintStrings->Insert(6, LoadStr(CUSTOM_COMMAND_PATTERNS_HINT6));
  HintStrings->Insert(11, LoadStr(PATTERNS_HINT_K));
  HintStrings->Insert(18, LoadStr(PATTERNS_HINT_BACKSLASH));
  QString Hint = ToQString(TrimRight(StringsToText(HintStrings.get())));
  QLabel * HintText = new QLabel(QStringLiteral("<a href=\"#\">%1</a>").arg(tr("patterns")), Group);
  HintText->setObjectName(QStringLiteral("HintText"));
  HintText->setToolTip(Hint);
  connect(HintText, &QLabel::linkActivated, this, [HintText, Hint]()
  {
    QToolTip::showText(HintText->mapToGlobal(QPoint(0, HintText->height())), Hint, HintText);
  });
  Layout->addWidget(HintText, Row++, 1, Qt::AlignRight);
  RemoteCommandButton = new QRadioButton(tr("&Remote command"), Group);
  RemoteCommandButton->setObjectName(QStringLiteral("RemoteCommandButton"));
  LocalCommandButton = new QRadioButton(tr("&Local command"), Group);
  LocalCommandButton->setObjectName(QStringLiteral("LocalCommandButton"));
  Layout->addWidget(RemoteCommandButton, Row, 0);
  Layout->addWidget(LocalCommandButton, Row++, 1);
  ApplyToDirectoriesCheck = CreateCheck(Group, "ApplyToDirectoriesCheck", tr("&Apply to directories"));
  RecursiveCheck = CreateCheck(Group, "RecursiveCheck", tr("&Execute recursively"));
  Layout->addWidget(ApplyToDirectoriesCheck, Row, 0);
  Layout->addWidget(RecursiveCheck, Row++, 1);
  ShowResultsCheck = CreateCheck(Group, "ShowResultsCheck", tr("&Show results in terminal"));
  RemoteFilesCheck = CreateCheck(Group, "RemoteFilesCheck", tr("&Use remote files"));
  Layout->addWidget(ShowResultsCheck, Row, 0);
  Layout->addWidget(RemoteFilesCheck, Row++, 1);
  CopyResultsCheck = CreateCheck(Group, "CopyResultsCheck", tr("Copy results to clip&board"));
  Layout->addWidget(CopyResultsCheck, Row++, 0);
  QLabel * ShortCutLabel = new QLabel(tr("&Keyboard shortcut:"), Group);
  ShortCutLabel->setObjectName(QStringLiteral("ShortCutLabel"));
  ShortCutCombo = new QComboBox(Group);
  ShortCutCombo->setObjectName(QStringLiteral("ShortCutCombo"));
  ShortCutLabel->setBuddy(ShortCutCombo);
  if (FMode != ccmAdHoc)
  {
    DebugAssert(ShortCuts != NULL);
    InitializeShortCutCombo(ShortCutCombo, *ShortCuts);
    Layout->addWidget(ShortCutLabel, Row, 0);
    Layout->addWidget(ShortCutCombo, Row++, 1);
  }
  else
  {
    ShortCutLabel->setVisible(false);
    ShortCutCombo->setVisible(false);
  }
  MainLayout->addWidget(Group);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  OkButton = Buttons->button(QDialogButtonBox::Ok);
  OkButton->setObjectName(QStringLiteral("OkButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);

  connect(DescriptionEdit, &QLineEdit::textChanged, this, [this]() { UpdateControls(); });
  connect(CommandEdit, &QComboBox::currentTextChanged, this, [this]() { UpdateControls(); });
  for (QAbstractButton * Button : Group->findChildren<QAbstractButton *>())
  {
    connect(Button, &QAbstractButton::toggled, this, [this]() { UpdateControls(); });
  }
}
//---------------------------------------------------------------------------
void TCustomCommandDialog::UpdateControls()
{
  RemoteCommandButton->setEnabled(FLAGCLEAR(FOptions, ccoDisableRemote));

  UnicodeString Command = CommandEdit->GetText();
  OkButton->setEnabled(!Command.IsEmpty() && ((FMode == ccmAdHoc) || !DescriptionEdit->text().isEmpty()));

  bool RemoteCommand = RemoteCommandButton->isChecked();
  bool AllowRecursive = true;
  bool AllowApplyToDirectories = true;
  bool AllowRemoteFiles = false;
  try
  {
    TRemoteCustomCommand RemoteCustomCommand;
    TLocalCustomCommand LocalCustomCommand;
    TFileCustomCommand * FileCustomCommand =
      (RemoteCommand ? &RemoteCustomCommand : &LocalCustomCommand);

    TInteractiveCustomCommand InteractiveCustomCommand(FileCustomCommand);
    UnicodeString Cmd = InteractiveCustomCommand.Complete(Command, false);
    bool FileCommand = FileCustomCommand->IsFileCommand(Cmd);
    AllowRemoteFiles = !RemoteCommand && FileCustomCommand->IsRemoteFileCommand(Cmd);
    AllowRecursive = FileCommand && !FileCustomCommand->IsFileListCommand(Cmd);
    if (AllowRecursive && !RemoteCommand)
    {
      AllowRecursive = !LocalCustomCommand.HasLocalFileName(Cmd);
    }
    AllowApplyToDirectories = FileCommand;
  }
  catch (...)
  {
  }

  RecursiveCheck->setEnabled(AllowRecursive && (!RemoteFilesCheck->isEnabled() || !RemoteFilesCheck->isChecked()));
  ApplyToDirectoriesCheck->setEnabled(AllowApplyToDirectories);
  ShowResultsCheck->setEnabled(RemoteCommand);
  RemoteFilesCheck->setEnabled(
    FLAGCLEAR(FOptions, ccoDisableRemoteFiles) && AllowRemoteFiles &&
    (!RecursiveCheck->isEnabled() || !RecursiveCheck->isChecked()));
}
//---------------------------------------------------------------------------
void TCustomCommandDialog::SetParams(int Value)
{
  FParams = Value;
  ApplyToDirectoriesCheck->setChecked(FLAGSET(Value, ccApplyToDirectories));
  RecursiveCheck->setChecked(FLAGSET(Value, ccRecursive));
  (FLAGSET(Value, ccLocal) ? LocalCommandButton : RemoteCommandButton)->setChecked(true);
  ShowResultsCheck->setChecked(FLAGSET(Value, ccShowResults));
  CopyResultsCheck->setChecked(FLAGSET(Value, ccCopyResults));
  RemoteFilesCheck->setChecked(FLAGSET(Value, ccRemoteFiles));
}
//---------------------------------------------------------------------------
int TCustomCommandDialog::GetParams()
{
  return
    (FParams & ~(ccApplyToDirectories | ccRecursive | ccLocal |
       ccShowResults | ccCopyResults | ccRemoteFiles)) |
    FLAGMASK(!RemoteCommandButton->isChecked(), ccLocal) |
    FLAGMASK(ApplyToDirectoriesCheck->isChecked(), ccApplyToDirectories) |
    FLAGMASK(RecursiveCheck->isChecked() && RecursiveCheck->isEnabled(), ccRecursive) |
    FLAGMASK(ShowResultsCheck->isChecked() && ShowResultsCheck->isEnabled(), ccShowResults) |
    FLAGMASK(CopyResultsCheck->isChecked() && CopyResultsCheck->isEnabled(), ccCopyResults) |
    FLAGMASK(RemoteFilesCheck->isChecked() && RemoteFilesCheck->isEnabled(), ccRemoteFiles);
}
//---------------------------------------------------------------------------
bool TCustomCommandDialog::Execute(TCustomCommandType & Command)
{
  CommandEdit->SetHistoryKey(L"CustomCommand");
  if (CommandEdit->count() == 0)
  {
    for (int Index = 0; Index < FCustomCommandList->Count; Index++)
    {
      CommandEdit->addItem(ToQString(FCustomCommandList->Commands[Index]->Name));
    }
  }

  DescriptionEdit->setText(ToQString(Command.Name));
  FOrigDescription = Command.Name;
  CommandEdit->SetText(Command.Command);
  SetParams(Command.Params);
  if (FMode != ccmAdHoc)
  {
    SetShortCutCombo(ShortCutCombo, Command.ShortCut);
  }
  UpdateControls();

  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    GetCommand(Command);

    CommandEdit->SaveToHistory();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCustomCommandDialog::accept()
{
  if ((FMode == ccmAdd) || (FMode == ccmEdit))
  {
    UnicodeString Desc = ToUnicodeString(DescriptionEdit->text());

    if (Desc.Pos(L"=") > 0)
    {
      DescriptionEdit->setFocus();
      throw Exception(FMTLOAD(CUSTOM_COMMAND_INVALID, (L"=")));
    }

    if (((FMode == ccmAdd) || ((FMode == ccmEdit) && (Desc != FOrigDescription))) &&
        (FCustomCommandList->Find(Desc) != NULL))
    {
      DescriptionEdit->setFocus();
      throw Exception(FMTLOAD(CUSTOM_COMMAND_DUPLICATE, (Desc)));
    }
  }

  try
  {
    bool RemoteCommand = RemoteCommandButton->isChecked();

    TRemoteCustomCommand RemoteCustomCommand;
    TLocalCustomCommand LocalCustomCommand;
    TFileCustomCommand * FileCustomCommand =
      (RemoteCommand ? &RemoteCustomCommand : &LocalCustomCommand);

    TInteractiveCustomCommand InteractiveCustomCommand(FileCustomCommand);

    UnicodeString Command = CommandEdit->GetText();
    InteractiveCustomCommand.Validate(Command);
    Command = InteractiveCustomCommand.Complete(Command, false);
    FileCustomCommand->Validate(Command);
  }
  catch (...)
  {
    CommandEdit->setFocus();
    throw;
  }

  if (FOnValidate)
  {
    TCustomCommandType Command;
    GetCommand(Command);
    FOnValidate(Command);
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
void TCustomCommandDialog::GetCommand(TCustomCommandType & Command)
{
  Command.Name = ToUnicodeString(DescriptionEdit->text());
  Command.Command = CommandEdit->GetText();
  Command.Params = GetParams();
  if (FMode != ccmAdHoc)
  {
    Command.ShortCut = GetShortCutCombo(ShortCutCombo);
  }
}
//---------------------------------------------------------------------------
class TCustomCommandPromptsDialog : public TCustomDialog
{
public:
  TCustomCommandPromptsDialog(const UnicodeString & CustomCommandName,
    const std::vector<UnicodeString> & Prompts, const std::vector<UnicodeString> & Defaults);

  bool Execute(std::vector<UnicodeString> & Values);

private:
  std::vector<THistoryComboBox *> FEdits;
};
//---------------------------------------------------------------------------
TCustomCommandPromptsDialog::TCustomCommandPromptsDialog(const UnicodeString & CustomCommandName,
  const std::vector<UnicodeString> & Prompts, const std::vector<UnicodeString> & Defaults)
{
  setObjectName(QStringLiteral("CustomCommandPromptsDialog"));
  setWindowTitle(ToQString(FMTLOAD(CUSTOM_COMMANDS_PARAMS_TITLE, (CustomCommandName))));

  DebugAssert(Prompts.size() == Defaults.size());
  for (size_t Index = 0; Index < Prompts.size(); Index++)
  {
    UnicodeString Prompt = Prompts[Index];
    if (Prompt.IsEmpty())
    {
      Prompt = LoadStr(CUSTOM_COMMANDS_PARAM_PROMPT2);
    }
    THistoryComboBox * ComboBox = new THistoryComboBox(this);
    ComboBox->setObjectName(QStringLiteral("PromptEdit%1").arg(static_cast<int>(Index)));
    AddEditLikeControl(ComboBox, CreateLabel(Prompt));

    UnicodeString HistoryKey = Prompts[Index];
    if (HistoryKey.IsEmpty())
    {
      HistoryKey = IntToStr(static_cast<int>(Index));
    }
    HistoryKey = FORMAT(L"%s_%s", (CustomCommandName, HistoryKey));
    HistoryKey = AppConfiguration->GetValidHistoryKey(HistoryKey);
    HistoryKey = L"CustomCommandParam_" + HistoryKey;
    ComboBox->SetHistoryKey(HistoryKey);

    ComboBox->SetText(Defaults[Index]);
    FEdits.push_back(ComboBox);
  }
  setMinimumWidth(fontMetrics().averageCharWidth() * 50);
}
//---------------------------------------------------------------------------
bool TCustomCommandPromptsDialog::Execute(std::vector<UnicodeString> & Values)
{
  bool Result = TCustomDialog::Execute();

  if (Result)
  {
    for (THistoryComboBox * Edit : FEdits)
    {
      Values.push_back(Edit->GetText());
      Edit->SaveToHistory();
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
// The list of the custom commands
class TCustomCommandsDialog : public QDialog
{
public:
  TCustomCommandsDialog();

  bool Execute();

protected:
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  std::unique_ptr<TCustomCommandList> FCustomCommandList;

  QTreeWidget * CustomCommandsView;
  QPushButton * AddCommandButton;
  QPushButton * EditCommandButton;
  QPushButton * RemoveCommandButton;
  QPushButton * UpCommandButton;
  QPushButton * DownCommandButton;

  void UpdateCustomCommandsView();
  void UpdateControls();
  TShortCuts GetShortCuts();
  void AddEditCommand(bool Edit);
  void RemoveCommand();
  void CustomCommandMove(int Source, int Dest);
};
//---------------------------------------------------------------------------
QPushButton * CreateButton(QWidget * Parent, const char * Name, const QString & Caption)
{
  QPushButton * Button = new QPushButton(Caption, Parent);
  Button->setObjectName(QString::fromLatin1(Name));
  Button->setAutoDefault(false);
  return Button;
}
//---------------------------------------------------------------------------
TCustomCommandsDialog::TCustomCommandsDialog() :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("CustomCommandsDialog"));
  setWindowTitle(tr("Custom Commands"));
  FCustomCommandList.reset(new TCustomCommandList());

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QGroupBox * CustomCommandsGroup = new QGroupBox(tr("Custom commands"), this);
  CustomCommandsGroup->setObjectName(QStringLiteral("CustomCommandsGroup"));
  QHBoxLayout * GroupLayout = new QHBoxLayout(CustomCommandsGroup);
  CustomCommandsView = new QTreeWidget(CustomCommandsGroup);
  CustomCommandsView->setObjectName(QStringLiteral("CustomCommandsView"));
  CustomCommandsView->setRootIsDecorated(false);
  CustomCommandsView->setUniformRowHeights(true);
  CustomCommandsView->setHeaderLabels({ tr("Description"), tr("Command"), tr("L/R") });
  CustomCommandsView->header()->setStretchLastSection(false);
  CustomCommandsView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  CustomCommandsView->setMinimumSize(fontMetrics().averageCharWidth() * 80, fontMetrics().height() * 15);
  CustomCommandsView->installEventFilter(this);
  connect(CustomCommandsView, &QTreeWidget::itemSelectionChanged, this, [this]() { UpdateControls(); });
  connect(CustomCommandsView, &QTreeWidget::itemDoubleClicked, this, [this]()
  {
    if (EditCommandButton->isEnabled())
    {
      AddEditCommand(true);
    }
  });
  GroupLayout->addWidget(CustomCommandsView, 1);
  QVBoxLayout * ButtonsLayout = new QVBoxLayout();
  AddCommandButton = CreateButton(CustomCommandsGroup, "AddCommandButton", tr("&Add..."));
  connect(AddCommandButton, &QPushButton::clicked, this, [this]() { AddEditCommand(false); });
  EditCommandButton = CreateButton(CustomCommandsGroup, "EditCommandButton", tr("&Edit..."));
  connect(EditCommandButton, &QPushButton::clicked, this, [this]() { AddEditCommand(true); });
  RemoveCommandButton = CreateButton(CustomCommandsGroup, "RemoveCommandButton", tr("&Remove"));
  connect(RemoveCommandButton, &QPushButton::clicked, this, [this]() { RemoveCommand(); });
  UpCommandButton = CreateButton(CustomCommandsGroup, "UpCommandButton", tr("&Up"));
  connect(UpCommandButton, &QPushButton::clicked, this, [this]()
  {
    int Index = CustomCommandsView->indexOfTopLevelItem(CustomCommandsView->currentItem());
    CustomCommandMove(Index, Index - 1);
  });
  DownCommandButton = CreateButton(CustomCommandsGroup, "DownCommandButton", tr("&Down"));
  connect(DownCommandButton, &QPushButton::clicked, this, [this]()
  {
    int Index = CustomCommandsView->indexOfTopLevelItem(CustomCommandsView->currentItem());
    CustomCommandMove(Index, Index + 1);
  });
  for (QPushButton * Button : { AddCommandButton, EditCommandButton, RemoveCommandButton, UpCommandButton, DownCommandButton })
  {
    ButtonsLayout->addWidget(Button);
  }
  ButtonsLayout->addStretch(1);
  GroupLayout->addLayout(ButtonsLayout);
  MainLayout->addWidget(CustomCommandsGroup, 1);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  Buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("OKButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CloseButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
}
//---------------------------------------------------------------------------
bool TCustomCommandsDialog::Execute()
{
  FCustomCommandList->Assign(AppConfiguration->CustomCommandList);
  UpdateCustomCommandsView();
  UpdateControls();
  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    AppConfiguration->CustomCommandList = FCustomCommandList.get();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCustomCommandsDialog::UpdateCustomCommandsView()
{
  int Current = CustomCommandsView->indexOfTopLevelItem(CustomCommandsView->currentItem());
  CustomCommandsView->clear();
  for (int Index = 0; Index < FCustomCommandList->Count; Index++)
  {
    const TCustomCommandType * Command = FCustomCommandList->Commands[Index];
    QTreeWidgetItem * Item = new QTreeWidgetItem(CustomCommandsView);
    Item->setText(0, ToQString(StripHotkey(Command->Name)));
    Item->setText(1, ToQString(Command->Command));
    Item->setText(2, ToQString(LoadStr(FLAGSET(Command->Params, ccLocal) ? CUSTOM_COMMAND_LOCAL : CUSTOM_COMMAND_REMOTE)));
  }
  CustomCommandsView->resizeColumnToContents(0);
  CustomCommandsView->resizeColumnToContents(2);
  if ((Current >= 0) && (Current < CustomCommandsView->topLevelItemCount()))
  {
    CustomCommandsView->setCurrentItem(CustomCommandsView->topLevelItem(Current));
  }
}
//---------------------------------------------------------------------------
void TCustomCommandsDialog::UpdateControls()
{
  QTreeWidgetItem * Item = CustomCommandsView->currentItem();
  int Index = ((Item != nullptr) && Item->isSelected()) ? CustomCommandsView->indexOfTopLevelItem(Item) : -1;
  bool CommandSelected = (Index >= 0);
  EditCommandButton->setEnabled(CommandSelected);
  RemoveCommandButton->setEnabled(CommandSelected);
  UpCommandButton->setEnabled(CommandSelected && (Index > 0));
  DownCommandButton->setEnabled(CommandSelected && (Index < FCustomCommandList->Count - 1));
}
//---------------------------------------------------------------------------
TShortCuts TCustomCommandsDialog::GetShortCuts()
{
  TShortCuts ShortCuts;
  if (AppConfiguration->SharedBookmarks != NULL)
  {
    AppConfiguration->SharedBookmarks->ShortCuts(ShortCuts);
  }
  FCustomCommandList->ShortCuts(ShortCuts);
  return ShortCuts;
}
//---------------------------------------------------------------------------
void TCustomCommandsDialog::AddEditCommand(bool Edit)
{
  TCustomCommandType Command;
  QTreeWidgetItem * Item = CustomCommandsView->currentItem();
  int Index = ((Item != nullptr) && Item->isSelected()) ? CustomCommandsView->indexOfTopLevelItem(Item) : -1;

  if (Edit)
  {
    Command = *FCustomCommandList->Commands[Index];
  }

  TShortCuts ShortCuts = GetShortCuts();

  if (DoCustomCommandDialog(Command, FCustomCommandList.get(), (Edit ? ccmEdit : ccmAdd), 0, NULL, &ShortCuts))
  {
    TCustomCommandType * ACommand = new TCustomCommandType(Command);
    if (Edit)
    {
      FCustomCommandList->Change(Index, ACommand);
    }
    else if (Index >= 0)
    {
      FCustomCommandList->Insert(Index, ACommand);
    }
    else
    {
      FCustomCommandList->Add(ACommand);
      Index = FCustomCommandList->Count - 1;
    }

    UpdateCustomCommandsView();
    CustomCommandsView->setCurrentItem(CustomCommandsView->topLevelItem(Index));
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TCustomCommandsDialog::RemoveCommand()
{
  int Index = CustomCommandsView->indexOfTopLevelItem(CustomCommandsView->currentItem());
  FCustomCommandList->Delete(Index);
  UpdateCustomCommandsView();
  UpdateControls();
}
//---------------------------------------------------------------------------
void TCustomCommandsDialog::CustomCommandMove(int Source, int Dest)
{
  FCustomCommandList->Move(Source, Dest);
  UpdateCustomCommandsView();
  CustomCommandsView->setCurrentItem(CustomCommandsView->topLevelItem(Dest));
  UpdateControls();
}
//---------------------------------------------------------------------------
bool TCustomCommandsDialog::eventFilter(QObject * Object, QEvent * Event)
{
  if ((Object == CustomCommandsView) && (Event->type() == QEvent::KeyPress))
  {
    int Key = static_cast<QKeyEvent *>(Event)->key();
    if (RemoveCommandButton->isEnabled() && (Key == Qt::Key_Delete))
    {
      RemoveCommand();
      return true;
    }
    if (AddCommandButton->isEnabled() && (Key == Qt::Key_Insert))
    {
      AddEditCommand(false);
      return true;
    }
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoCustomCommandDialog(TCustomCommandType & Command, const TCustomCommandList * CustomCommandList,
  TCustomCommandsMode Mode, int Options, TCustomCommandValidate OnValidate, const TShortCuts * ShortCuts)
{
  TCustomCommandDialog Dialog(CustomCommandList, Mode, Options, OnValidate, ShortCuts);
  return Dialog.Execute(Command);
}
//---------------------------------------------------------------------------
bool DoCustomCommandsDialog()
{
  TCustomCommandsDialog Dialog;
  return Dialog.Execute();
}
//---------------------------------------------------------------------------
TWinInteractiveCustomCommand::TWinInteractiveCustomCommand(
  TCustomCommand * ChildCustomCommand, const UnicodeString & CustomCommandName) :
  TInteractiveCustomCommand(ChildCustomCommand)
{
  FCustomCommandName = StripEllipsis(StripHotkey(CustomCommandName));
}
//---------------------------------------------------------------------------
void __fastcall TWinInteractiveCustomCommand::PatternHint(int Index, const UnicodeString & Pattern)
{
  if (IsPromptPattern(Pattern))
  {
    UnicodeString Prompt;
    UnicodeString Default;
    bool Delimit = false;
    ParsePromptPattern(Pattern, Prompt, Default, Delimit);
    FIndexes.insert(std::make_pair(Index, FPrompts.size()));
    FPrompts.push_back(Prompt);
    FDefaults.push_back(Default);
  }
}
//---------------------------------------------------------------------------
void __fastcall TWinInteractiveCustomCommand::Prompt(
  int Index, const UnicodeString & /*Prompt*/, UnicodeString & Value)
{
  if (DebugAlwaysTrue(FIndexes.find(Index) != FIndexes.end()))
  {
    size_t PromptIndex = FIndexes[Index];
    if (FValues.empty())
    {
      TCustomCommandPromptsDialog Dialog(FCustomCommandName, FPrompts, FDefaults);
      if (!Dialog.Execute(FValues))
      {
        Abort();
      }
    }

    if (DebugAlwaysTrue(FValues.size() == FPrompts.size()) &&
        DebugAlwaysTrue(PromptIndex < FValues.size()))
    {
      Value = FValues[PromptIndex];
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TWinInteractiveCustomCommand::Execute(
  const UnicodeString & Command, UnicodeString & Value)
{
  int DummyExitCode;
  ExecuteProcessAndReadOutput(Command, Value, DummyExitCode, false);
  // trim trailing cr/lf
  Value = TrimRight(Value);
}
//---------------------------------------------------------------------------
