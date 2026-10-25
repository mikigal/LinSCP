// The transfer settings (upstream's forms/CopyParams.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>

#include "CopyParamsFrame.h"
#include "CustomDialog.h"
#include "HistoryComboBox.h"
#include "RightsFrame.h"

#include <QAction>
#include <QCheckBox>
#include <QEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStyle>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
TCopyParamsFrame::TCopyParamsFrame(QWidget * Parent) :
  QWidget(Parent)
{
  setObjectName(QStringLiteral("CopyParamsFrame"));
  FCopyParamAttrs = 0;
  FRemoveCtrlZ = false;
  FRightsModified = false;
  FUpdating = 0;
  FParams = new TCopyParamType();

  CreateControls();

  FRightsFrame = new TRightsFrame(this);
  FRightsFrame->SetPopup(true);
  FRightsFrame->SetAllowAddXToDirectories(true);
  FRightsFrame->OnChange = [this]() { RightsFrameChange(); };

  TCopyParamType DefParams;
  SetParams(DefParams);
}
//---------------------------------------------------------------------------
TCopyParamsFrame::~TCopyParamsFrame()
{
  delete FParams;
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
static QRadioButton * CreateRadio(QWidget * Parent, QBoxLayout * Layout, const char * Name, const QString & Caption)
{
  QRadioButton * Button = new QRadioButton(Caption, Parent);
  Button->setObjectName(QString::fromLatin1(Name));
  Layout->addWidget(Button);
  return Button;
}
//---------------------------------------------------------------------------
static QLabel * CreateHint(QWidget * Parent, const QString & Hint)
{
  QLabel * Label = new QLabel(QStringLiteral("<a href=\"#\">%1</a>").arg(QObject::tr("mask hints")), Parent);
  Label->setTextFormat(Qt::RichText);
  Label->setToolTip(Hint);
  Label->setAlignment(Qt::AlignRight);
  return Label;
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::CreateControls()
{
  QGridLayout * Grid = new QGridLayout(this);
  Grid->setContentsMargins(0, 0, 0, 0);

  TransferModeGroup = new QGroupBox(tr("Transfer mode"), this);
  TransferModeGroup->setObjectName(QStringLiteral("TransferModeGroup"));
  QVBoxLayout * TransferModeLayout = new QVBoxLayout(TransferModeGroup);
  TMTextButton = CreateRadio(TransferModeGroup, TransferModeLayout, "TMTextButton", tr("&Text (plain text, html, scripts, ...)"));
  TMBinaryButton = CreateRadio(TransferModeGroup, TransferModeLayout, "TMBinaryButton", tr("&Binary (archives, doc, ...)"));
  TMAutomaticButton = CreateRadio(TransferModeGroup, TransferModeLayout, "TMAutomaticButton", tr("&Automatic"));
  AsciiFileMaskLabel = new QLabel(tr("Transfer following &files in text mode:"), TransferModeGroup);
  AsciiFileMaskLabel->setObjectName(QStringLiteral("AsciiFileMaskLabel"));
  TransferModeLayout->addWidget(AsciiFileMaskLabel);
  AsciiFileMaskCombo = new THistoryComboBox(TransferModeGroup);
  AsciiFileMaskCombo->setObjectName(QStringLiteral("AsciiFileMaskCombo"));
  AsciiFileMaskCombo->lineEdit()->setMaxLength(1000);
  AsciiFileMaskLabel->setBuddy(AsciiFileMaskCombo);
  TransferModeLayout->addWidget(AsciiFileMaskCombo);
  TransferModeLayout->addStretch(1);
  Grid->addWidget(TransferModeGroup, 0, 0);

  ChangeCaseGroup = new QGroupBox(tr("Filename modification"), this);
  ChangeCaseGroup->setObjectName(QStringLiteral("ChangeCaseGroup"));
  QVBoxLayout * ChangeCaseLayout = new QVBoxLayout(ChangeCaseGroup);
  CCNoChangeButton = CreateRadio(ChangeCaseGroup, ChangeCaseLayout, "CCNoChangeButton", tr("No chan&ge"));
  CCUpperCaseButton = CreateRadio(ChangeCaseGroup, ChangeCaseLayout, "CCUpperCaseButton", tr("&Upper case"));
  CCLowerCaseButton = CreateRadio(ChangeCaseGroup, ChangeCaseLayout, "CCLowerCaseButton", tr("Lo&wer case"));
  CCLowerCaseShortButton = CreateRadio(ChangeCaseGroup, ChangeCaseLayout, "CCLowerCaseShortButton", tr("Lower case &8.3"));
  // Upstream lists the characters invalid on Windows; on Linux only the
  // slash is
  ReplaceInvalidCharsCheck =
    CreateCheck(ChangeCaseGroup, ChangeCaseLayout, "ReplaceInvalidCharsCheck", tr("Rep&lace invalid characters"));
  ChangeCaseLayout->addStretch(1);
  Grid->addWidget(ChangeCaseGroup, 0, 1);

  RemotePropertiesGroup = new QGroupBox(tr("Upload options"), this);
  RemotePropertiesGroup->setObjectName(QStringLiteral("RemotePropertiesGroup"));
  QVBoxLayout * RemoteLayout = new QVBoxLayout(RemotePropertiesGroup);
  PreserveRightsCheck = CreateCheck(RemotePropertiesGroup, RemoteLayout, "PreserveRightsCheck", tr("Set pe&rmissions:"));
  RightsEdit = new QLineEdit(RemotePropertiesGroup);
  RightsEdit->setObjectName(QStringLiteral("RightsEdit"));
  RightsEdit->setMaximumWidth(fontMetrics().averageCharWidth() * 24);
  RightsEditButton = RightsEdit->addAction(style()->standardIcon(QStyle::SP_ArrowDown), QLineEdit::TrailingPosition);
  RightsEditButton->setObjectName(QStringLiteral("RightsEditButton"));
  RightsEditButton->setToolTip(tr("Configure permissions"));
  connect(RightsEditButton, &QAction::triggered, this, [this]() { RightsEditButtonClick(); });
  connect(RightsEdit, &QLineEdit::textEdited, this, [this]() { FRightsModified = true; });
  connect(RightsEdit, &QLineEdit::editingFinished, this, [this]() { RightsEditFinished(); });
  QHBoxLayout * RightsLayout = new QHBoxLayout();
  RightsLayout->setContentsMargins(fontMetrics().averageCharWidth() * 2, 0, 0, 0);
  RightsLayout->addWidget(RightsEdit);
  RemoteLayout->addLayout(RightsLayout);
  IgnorePermErrorsCheck =
    CreateCheck(RemotePropertiesGroup, RemoteLayout, "IgnorePermErrorsCheck", tr("Ign&ore permission errors"));
  // There is no archive attribute on Linux
  ClearArchiveCheck = CreateCheck(RemotePropertiesGroup, RemoteLayout, "ClearArchiveCheck", tr("Clear 'Arc&hive' attribute"));
  ClearArchiveCheck->setVisible(false);
  RemoveCtrlZAndBOMCheck =
    CreateCheck(RemotePropertiesGroup, RemoteLayout, "RemoveCtrlZAndBOMCheck", ToQString(LoadStr(COPY_PARAM_REMOVE_BOM_EOF)));
  EncryptNewFilesCheck = CreateCheck(RemotePropertiesGroup, RemoteLayout, "EncryptNewFilesCheck", tr("&Encrypt new files"));
  RemoteLayout->addStretch(1);
  Grid->addWidget(RemotePropertiesGroup, 1, 0);

  QVBoxLayout * RightColumn = new QVBoxLayout();
  CommonPropertiesGroup = new QGroupBox(tr("Common options"), this);
  CommonPropertiesGroup->setObjectName(QStringLiteral("CommonPropertiesGroup"));
  QVBoxLayout * CommonLayout = new QVBoxLayout(CommonPropertiesGroup);
  PreserveTimeCheck = CreateCheck(CommonPropertiesGroup, CommonLayout, "PreserveTimeCheck", tr("&Preserve timestamp"));
  PreserveTimeDirsCheck = new QCheckBox(tr("Including directories"), CommonPropertiesGroup);
  PreserveTimeDirsCheck->setObjectName(QStringLiteral("PreserveTimeDirsCheck"));
  PreserveTimeDirsCheck->setToolTip(
    tr("When preserving directory timestamps is enabled, using multiple connections for transfer is not possible."));
  QHBoxLayout * TimeDirsLayout = new QHBoxLayout();
  TimeDirsLayout->setContentsMargins(fontMetrics().averageCharWidth() * 2, 0, 0, 0);
  TimeDirsLayout->addWidget(PreserveTimeDirsCheck);
  CommonLayout->addLayout(TimeDirsLayout);
  CommonCalculateSizeCheck =
    CreateCheck(CommonPropertiesGroup, CommonLayout, "CommonCalculateSizeCheck", tr("&Calculate total size"));
  QHBoxLayout * SpeedLayout = new QHBoxLayout();
  SpeedLabel3 = new QLabel(tr("&Speed (KB/s):"), CommonPropertiesGroup);
  SpeedLabel3->setObjectName(QStringLiteral("SpeedLabel3"));
  SpeedCombo = new THistoryComboBox(CommonPropertiesGroup);
  SpeedCombo->setObjectName(QStringLiteral("SpeedCombo"));
  SpeedLabel3->setBuddy(SpeedCombo);
  SpeedLayout->addWidget(SpeedLabel3);
  SpeedLayout->addWidget(SpeedCombo, 1);
  CommonLayout->addLayout(SpeedLayout);
  RightColumn->addWidget(CommonPropertiesGroup);

  LocalPropertiesGroup = new QGroupBox(tr("Download options"), this);
  LocalPropertiesGroup->setObjectName(QStringLiteral("LocalPropertiesGroup"));
  QVBoxLayout * LocalLayout = new QVBoxLayout(LocalPropertiesGroup);
  PreserveReadOnlyCheck = CreateCheck(LocalPropertiesGroup, LocalLayout, "PreserveReadOnlyCheck", tr("Preserve rea&d-only"));
  RightColumn->addWidget(LocalPropertiesGroup);
  RightColumn->addStretch(1);
  Grid->addLayout(RightColumn, 1, 1);

  OtherGroup = new QGroupBox(tr("Other"), this);
  OtherGroup->setObjectName(QStringLiteral("OtherGroup"));
  QGridLayout * OtherLayout = new QGridLayout(OtherGroup);
  IncludeFileMaskLabel = new QLabel(tr("File &mask:"), OtherGroup);
  IncludeFileMaskLabel->setObjectName(QStringLiteral("IncludeFileMaskLabel"));
  OtherLayout->addWidget(IncludeFileMaskLabel, 0, 0, 1, 2);
  IncludeFileMaskCombo = new THistoryComboBox(OtherGroup);
  IncludeFileMaskCombo->setObjectName(QStringLiteral("IncludeFileMaskCombo"));
  IncludeFileMaskCombo->lineEdit()->setMaxLength(3000);
  IncludeFileMaskLabel->setBuddy(IncludeFileMaskCombo);
  OtherLayout->addWidget(IncludeFileMaskCombo, 1, 0, 1, 2);
  IncludeFileMaskButton = new QPushButton(tr("&Edit..."), OtherGroup);
  IncludeFileMaskButton->setObjectName(QStringLiteral("IncludeFileMaskButton"));
  IncludeFileMaskButton->setAutoDefault(false);
  connect(IncludeFileMaskButton, &QPushButton::clicked, this, [this]() { IncludeFileMaskButtonClick(); });
  OtherLayout->addWidget(IncludeFileMaskButton, 1, 2);
  QLabel * IncludeFileMaskHintText = CreateHint(OtherGroup,
    ToQString(FORMAT(L"%s\n \n%s\n \n%s\n \n%s\n \n%s\n \n%s", (LoadStr(MASK_HINT2),
      LoadStr(FILE_MASK_EX_HINT), LoadStr(COMBINING_MASKS_HINT),
      LoadStr(PATH_MASK_HINT2),
      LoadStr(DIRECTORY_MASK_HINT), LoadStr(MASK_HELP)))));
  IncludeFileMaskHintText->setObjectName(QStringLiteral("IncludeFileMaskHintText"));
  OtherLayout->addWidget(IncludeFileMaskHintText, 2, 1);
  NewerOnlyCheck = new QCheckBox(tr("&New and updated files only"), OtherGroup);
  NewerOnlyCheck->setObjectName(QStringLiteral("NewerOnlyCheck"));
  OtherLayout->addWidget(NewerOnlyCheck, 3, 0);
  ExcludeHiddenFilesCheck = new QCheckBox(tr("Exclude h&idden files"), OtherGroup);
  ExcludeHiddenFilesCheck->setObjectName(QStringLiteral("ExcludeHiddenFilesCheck"));
  OtherLayout->addWidget(ExcludeHiddenFilesCheck, 3, 1, 1, 2);
  ExcludeEmptyDirectoriesCheck = new QCheckBox(tr("E&xclude empty directories"), OtherGroup);
  ExcludeEmptyDirectoriesCheck->setObjectName(QStringLiteral("ExcludeEmptyDirectoriesCheck"));
  OtherLayout->addWidget(ExcludeEmptyDirectoriesCheck, 4, 0);
  OtherLayout->setColumnStretch(0, 1);
  OtherLayout->setColumnStretch(1, 1);
  Grid->addWidget(OtherGroup, 2, 0, 1, 2);

  for (QRadioButton * Button : findChildren<QRadioButton *>())
  {
    connect(Button, &QRadioButton::toggled, this, [this]() { UpdateControls(); });
  }
  for (QCheckBox * Check : findChildren<QCheckBox *>())
  {
    connect(Check, &QCheckBox::toggled, this, [this]() { UpdateControls(); });
  }
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::SetParams(const TCopyParamType & Value)
{
  SetParamsControls(Value);
  *FParams = Value;
  UpdateControls();
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::SetParamsControls(const TCopyParamType & Value)
{
  TAutoNestingCounter UpdatingCounter(FUpdating);
  DebugAssert((Value.TransferMode == tmAscii) ||
    (Value.TransferMode == tmBinary) || (Value.TransferMode == tmAutomatic));
  switch (Value.TransferMode)
  {
    case tmAscii: TMTextButton->setChecked(true); break;
    case tmBinary: TMBinaryButton->setChecked(true); break;
    default: TMAutomaticButton->setChecked(true); break;
  }
  AsciiFileMaskCombo->SetText(Value.AsciiFileMask.Masks);

  switch (Value.FileNameCase)
  {
    case ncNoChange: CCNoChangeButton->setChecked(true); break;
    case ncLowerCase: CCLowerCaseButton->setChecked(true); break;
    case ncUpperCase: CCUpperCaseButton->setChecked(true); break;
    case ncFirstUpperCase: CCNoChangeButton->setChecked(true); break; // unsupported
    case ncLowerCaseShort: CCLowerCaseShortButton->setChecked(true); break;
  }

  ReplaceInvalidCharsCheck->setChecked(Value.ReplaceInvalidChars);

  FRightsFrame->SetAddXToDirectories(Value.AddXToDirectories);
  FRightsFrame->SetRights(Value.Rights);
  RightsFrameChange();
  PreserveRightsCheck->setChecked(Value.PreserveRights);
  IgnorePermErrorsCheck->setChecked(Value.IgnorePermErrors);
  PreserveReadOnlyCheck->setChecked(Value.PreserveReadOnly);

  PreserveTimeCheck->setChecked(Value.PreserveTime);
  PreserveTimeDirsCheck->setChecked(Value.PreserveTimeDirs);

  CommonCalculateSizeCheck->setChecked(Value.CalculateSize);

  IncludeFileMaskCombo->SetText(Value.IncludeFileMask.Masks);
  ClearArchiveCheck->setChecked(Value.ClearArchive);
  if (FLAGSET(FCopyParamAttrs, cpaNoRemoveCtrlZ))
  {
    FRemoveCtrlZ = Value.RemoveCtrlZ;
    RemoveCtrlZAndBOMCheck->setChecked(Value.RemoveBOM);
  }
  else
  {
    DebugAssert(FLAGCLEAR(FCopyParamAttrs, cpaNoRemoveBOM));
    RemoveCtrlZAndBOMCheck->setChecked(Value.RemoveCtrlZ && Value.RemoveBOM);
  }

  SpeedCombo->SetText(SetSpeedLimit(Value.CPSLimit));

  NewerOnlyCheck->setChecked(Value.NewerOnly);
  EncryptNewFilesCheck->setChecked(Value.EncryptNewFiles);
  ExcludeHiddenFilesCheck->setChecked(Value.ExcludeHiddenFiles);
  ExcludeEmptyDirectoriesCheck->setChecked(Value.ExcludeEmptyDirectories);
}
//---------------------------------------------------------------------------
TCopyParamType TCopyParamsFrame::GetParams()
{
  TCopyParamType Result = *FParams;

  DebugAssert(TMTextButton->isChecked() || TMBinaryButton->isChecked() || TMAutomaticButton->isChecked());
  if (TMTextButton->isChecked()) Result.TransferMode = tmAscii;
    else
  if (TMBinaryButton->isChecked()) Result.TransferMode = tmBinary;
    else Result.TransferMode = tmAutomatic;

  Result.AsciiFileMask.Masks = AsciiFileMaskCombo->GetText();

  if (CCLowerCaseButton->isChecked()) Result.FileNameCase = ncLowerCase;
    else
  if (CCUpperCaseButton->isChecked()) Result.FileNameCase = ncUpperCase;
    else
  if (CCLowerCaseShortButton->isChecked()) Result.FileNameCase = ncLowerCaseShort;
    else Result.FileNameCase = ncNoChange;

  Result.ReplaceInvalidChars = ReplaceInvalidCharsCheck->isChecked();

  Result.AddXToDirectories = FRightsFrame->GetAddXToDirectories();
  Result.Rights = FRightsFrame->GetRights();
  Result.PreserveRights = PreserveRightsCheck->isChecked();
  Result.IgnorePermErrors = IgnorePermErrorsCheck->isChecked();
  Result.PreserveReadOnly = PreserveReadOnlyCheck->isChecked();

  Result.PreserveTime = PreserveTimeCheck->isChecked();
  Result.PreserveTimeDirs = PreserveTimeDirsCheck->isChecked();

  Result.CalculateSize = CommonCalculateSizeCheck->isChecked();

  Result.IncludeFileMask.Masks = IncludeFileMaskCombo->GetText();

  Result.ClearArchive = ClearArchiveCheck->isChecked();
  if (FLAGSET(FCopyParamAttrs, cpaNoRemoveCtrlZ))
  {
    Result.RemoveCtrlZ = FRemoveCtrlZ;
  }
  else
  {
    Result.RemoveCtrlZ = RemoveCtrlZAndBOMCheck->isChecked();
  }
  Result.RemoveBOM = RemoveCtrlZAndBOMCheck->isChecked();

  Result.CPSLimit = GetSpeedLimit(SpeedCombo->GetText());

  Result.NewerOnly = NewerOnlyCheck->isChecked();
  Result.EncryptNewFiles = EncryptNewFilesCheck->isChecked();
  Result.ExcludeHiddenFiles = ExcludeHiddenFilesCheck->isChecked();
  Result.ExcludeEmptyDirectories = ExcludeEmptyDirectoriesCheck->isChecked();

  return Result;
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::UpdateControls()
{
  if (FUpdating > 0)
  {
    return;
  }
  bool Enabled = isEnabled();
  CommonPropertiesGroup->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) && Enabled);
  LocalPropertiesGroup->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) && Enabled);
  RemotePropertiesGroup->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) && Enabled);
  TransferModeGroup->setEnabled(
    FLAGCLEAR(FCopyParamAttrs, cpaNoTransferMode) &&
    FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) && Enabled);
  AsciiFileMaskLabel->setEnabled(TransferModeGroup->isEnabled() && TMAutomaticButton->isChecked());
  AsciiFileMaskCombo->setEnabled(TransferModeGroup->isEnabled() && TMAutomaticButton->isChecked());
  PreserveRightsCheck->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) &&
    FLAGCLEAR(FCopyParamAttrs, cpaNoRights) && Enabled);
  RightsEdit->setEnabled(PreserveRightsCheck->isChecked() && PreserveRightsCheck->isEnabled());
  FRightsFrame->setEnabled(RightsEdit->isEnabled());
  PreserveReadOnlyCheck->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) &&
    FLAGCLEAR(FCopyParamAttrs, cpaNoPreserveReadOnly) && Enabled);
  IncludeFileMaskCombo->setEnabled(
    (FLAGCLEAR(FCopyParamAttrs, cpaNoIncludeMask) ||
     FLAGSET(FCopyParamAttrs, cpaIncludeMaskOnly)) &&
    Enabled);
  IncludeFileMaskLabel->setEnabled(IncludeFileMaskCombo->isEnabled());
  IncludeFileMaskButton->setEnabled(IncludeFileMaskCombo->isEnabled());
  ClearArchiveCheck->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaNoClearArchive) &&
    FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) && Enabled);
  // Do not mention EOF when not supported, but only when at least BOM is supported.
  // This is not ideal as it shows both BOM and EOF for FTP downloads
  // (when EOF is never supported for FTP).
  // But as the option is disabled for FTP download, it's not that bad.
  // We do not have enough information here to implement this correctly.
  RemoveCtrlZAndBOMCheck->setText(ToQString(
    LoadStr(
      FLAGSET(FCopyParamAttrs, cpaNoRemoveCtrlZ) && FLAGCLEAR(FCopyParamAttrs, cpaNoRemoveBOM) ?
        COPY_PARAM_REMOVE_BOM : COPY_PARAM_REMOVE_BOM_EOF)));
  RemoveCtrlZAndBOMCheck->setEnabled(
    (FLAGCLEAR(FCopyParamAttrs, cpaNoRemoveCtrlZ) || FLAGCLEAR(FCopyParamAttrs, cpaNoRemoveBOM)) &&
    FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) &&
    (TMTextButton->isChecked() || TMAutomaticButton->isChecked()) &&
    Enabled);
  PreserveTimeCheck->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaNoPreserveTime) &&
    FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) && Enabled);
  // The checkbox can be enabled even then "preserve time" checkbox is not (such as on the Synchronize dialog);
  PreserveTimeDirsCheck->setEnabled(
    FLAGCLEAR(FCopyParamAttrs, cpaNoPreserveTimeDirs) &&
    FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) && Enabled &&
    PreserveTimeCheck->isChecked());
  CommonCalculateSizeCheck->setEnabled(
    FLAGCLEAR(FCopyParamAttrs, cpaNoCalculateSize) && FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) && Enabled);
  ChangeCaseGroup->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) && Enabled);
  IgnorePermErrorsCheck->setEnabled(
    ((PreserveRightsCheck->isEnabled() && PreserveRightsCheck->isChecked()) ||
     (PreserveTimeCheck->isEnabled() && PreserveTimeCheck->isChecked())) &&
    FLAGCLEAR(FCopyParamAttrs, cpaNoIgnorePermErrors) &&
    FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly));
  NewerOnlyCheck->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) &&
    FLAGCLEAR(FCopyParamAttrs, cpaNoNewerOnly) && Enabled);
  EncryptNewFilesCheck->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly) &&
    FLAGCLEAR(FCopyParamAttrs, cpaNoEncryptNewFiles) && Enabled);
  ExcludeHiddenFilesCheck->setEnabled(IncludeFileMaskCombo->isEnabled());
  ExcludeEmptyDirectoriesCheck->setEnabled(FLAGCLEAR(FCopyParamAttrs, cpaIncludeMaskOnly));
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::changeEvent(QEvent * Event)
{
  QWidget::changeEvent(Event);
  if (Event->type() == QEvent::EnabledChange)
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::BeforeExecute()
{
  AsciiFileMaskCombo->SetHistoryKey(L"Mask");
  IncludeFileMaskCombo->SetHistoryKey(L"IncludeMask");
  SpeedCombo->SetHistoryKey(L"SpeedLimit");
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::AfterExecute()
{
  AsciiFileMaskCombo->SaveToHistory();
  IncludeFileMaskCombo->SaveToHistory();
  SpeedCombo->SaveToHistory();
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::Validate()
{
  if (FRightsModified)
  {
    RightsEditFinished();
  }
  ValidateMaskEdit(AsciiFileMaskCombo);
  ValidateMaskEdit(IncludeFileMaskCombo);
  try
  {
    SpeedCombo->SetText(SetSpeedLimit(GetSpeedLimit(SpeedCombo->GetText())));
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
    SpeedCombo->setFocus();
    SpeedCombo->lineEdit()->selectAll();
    Abort();
  }
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::SetCopyParamAttrs(int Value)
{
  FCopyParamAttrs = Value;
  UpdateControls();
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::RightsEditButtonClick()
{
  if (FRightsFrame->isHidden())
  {
    // validate input before showing the popup
    RightsEditFinished();
    FRightsFrame->DropDown(RightsEdit);
  }
  else
  {
    FRightsFrame->CloseUp();
  }
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::RightsFrameChange()
{
  RightsEdit->setText(ToQString(FRightsFrame->GetText()));
  FRightsModified = false;
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::UpdateRightsByStr()
{
  if (!RightsEdit->text().isEmpty())
  {
    FRightsFrame->SetText(ToUnicodeString(RightsEdit->text()));
    // change handler may not be called if the rights were not actually changed,
    // but we want to normalize the user-entered information anyway
    RightsFrameChange();
  }
  UpdateControls();
  FRightsModified = false;
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::RightsEditFinished()
{
  if (FRightsModified)
  {
    try
    {
      UpdateRightsByStr();
    }
    catch (Exception & E)
    {
      ShowExtendedException(&E);
      RightsEdit->selectAll();
      RightsEdit->setFocus();
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::IncludeFileMaskButtonClick()
{
  TFileMasks Masks = IncludeFileMaskCombo->GetText();
  if (DoEditMaskDialog(Masks))
  {
    IncludeFileMaskCombo->SetText(Masks.Masks);
  }
}
//---------------------------------------------------------------------------
void TCopyParamsFrame::Closing()
{
  if (!FRightsFrame->isHidden())
  {
    FRightsFrame->CloseUp();
  }
}
//---------------------------------------------------------------------------
