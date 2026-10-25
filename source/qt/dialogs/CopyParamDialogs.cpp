// The dialogs of the transfer settings: the custom settings (upstream's
// TCopyParamCustomDialog, forms/CopyParamCustom.dfm), the presets
// (TCopyParamPresetDialog, forms/CopyParamPreset.dfm) and the menu of the
// presets (CopyParamListPopup of windows/WinInterface.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>

#include "CopyParamsFrame.h"
#include "CustomDialog.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
class TCopyParamCustomDialog : public QDialog
{
public:
  explicit TCopyParamCustomDialog(int CopyParamAttrs);

  bool Execute(TCopyParamType & CopyParam);

protected:
  virtual void accept() override;
  virtual void done(int Result) override;

private:
  TCopyParamsFrame * CopyParamsFrame;
};
//---------------------------------------------------------------------------
TCopyParamCustomDialog::TCopyParamCustomDialog(int CopyParamAttrs) :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("CopyParamCustomDialog"));
  setWindowTitle(tr("Transfer settings"));
  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  CopyParamsFrame = new TCopyParamsFrame(this);
  CopyParamsFrame->SetCopyParamAttrs(CopyParamAttrs);
  MainLayout->addWidget(CopyParamsFrame);
  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  Buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("OkButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
}
//---------------------------------------------------------------------------
bool TCopyParamCustomDialog::Execute(TCopyParamType & CopyParam)
{
  CopyParamsFrame->SetParams(CopyParam);
  CopyParamsFrame->BeforeExecute();
  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    CopyParamsFrame->AfterExecute();
    CopyParam = CopyParamsFrame->GetParams();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCopyParamCustomDialog::accept()
{
  try
  {
    CopyParamsFrame->Validate();
  }
  catch (EAbort &)
  {
    return;
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
void TCopyParamCustomDialog::done(int Result)
{
  CopyParamsFrame->Closing();
  QDialog::done(Result);
}
//---------------------------------------------------------------------------
bool DoCopyParamCustomDialog(TCopyParamType & CopyParam, int CopyParamAttrs)
{
  TCopyParamCustomDialog Dialog(CopyParamAttrs);
  return Dialog.Execute(CopyParam);
}
//---------------------------------------------------------------------------
class TCopyParamPresetDialog : public QDialog
{
public:
  TCopyParamPresetDialog(TCopyParamPresetMode Mode, TCopyParamRuleData * CurrentRuleData);

  bool Execute(TCopyParamList * CopyParamList, int & Index, const TCopyParamType & DefaultCopyParams);

protected:
  virtual void accept() override;
  virtual void done(int Result) override;

private:
  TCopyParamPresetMode FMode;
  TCopyParamRuleData * FCurrentRuleData;
  TCopyParamList * FCopyParamList;
  int FIndex;
  QLineEdit * DescriptionEdit;
  TCopyParamsFrame * CopyParamsFrame;
  QCheckBox * HasRuleCheck;
  QGroupBox * RuleGroup;
  QLineEdit * HostNameEdit;
  QLineEdit * UserNameEdit;
  QLineEdit * RemoteDirectoryEdit;
  QLineEdit * LocalDirectoryEdit;
  QPushButton * CurrentRuleButton;
  QPushButton * OkButton;

  void UpdateControls();
  void SetRuleData(const TCopyParamRuleData & Data);
  TCopyParamRule * GetRule();
};
//---------------------------------------------------------------------------
TCopyParamPresetDialog::TCopyParamPresetDialog(TCopyParamPresetMode Mode, TCopyParamRuleData * CurrentRuleData) :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("CopyParamPresetDialog"));
  FMode = Mode;
  FCurrentRuleData = CurrentRuleData;
  FCopyParamList = NULL;
  FIndex = -1;
  setWindowTitle(ToQString(LoadStr(Mode == cpmEdit ? COPY_PARAM_EDIT : COPY_PARAM_ADD)));

  QGridLayout * MainLayout = new QGridLayout(this);
  QLabel * Label1 = new QLabel(tr("Preset &description:"), this);
  Label1->setObjectName(QStringLiteral("Label1"));
  MainLayout->addWidget(Label1, 0, 0);
  DescriptionEdit = new QLineEdit(this);
  DescriptionEdit->setObjectName(QStringLiteral("DescriptionEdit"));
  Label1->setBuddy(DescriptionEdit);
  connect(DescriptionEdit, &QLineEdit::textChanged, this, [this]() { UpdateControls(); });
  MainLayout->addWidget(DescriptionEdit, 1, 0);

  CopyParamsFrame = new TCopyParamsFrame(this);
  MainLayout->addWidget(CopyParamsFrame, 2, 0, 3, 1);

  HasRuleCheck = new QCheckBox(tr("Automatically select this preset when"), this);
  HasRuleCheck->setObjectName(QStringLiteral("HasRuleCheck"));
  connect(HasRuleCheck, &QCheckBox::toggled, this, [this]() { UpdateControls(); });
  MainLayout->addWidget(HasRuleCheck, 2, 1);
  RuleGroup = new QGroupBox(tr("Autoselection rule"), this);
  RuleGroup->setObjectName(QStringLiteral("RuleGroup"));
  QVBoxLayout * RuleLayout = new QVBoxLayout(RuleGroup);
  struct TRuleEdit
  {
    const char * LabelName;
    QString Caption;
    const char * Name;
    QLineEdit ** Edit;
  };
  const TRuleEdit RuleEdits[] =
  {
    { "Label2", tr("Hostna&me mask:"), "HostNameEdit", &HostNameEdit },
    { "Label3", tr("Us&ername mask:"), "UserNameEdit", &UserNameEdit },
    { "Label4", tr("Remote director&y mask:"), "RemoteDirectoryEdit", &RemoteDirectoryEdit },
    { "Label5", tr("&Local directory mask:"), "LocalDirectoryEdit", &LocalDirectoryEdit },
  };
  for (const TRuleEdit & RuleEdit : RuleEdits)
  {
    QLabel * Label = new QLabel(RuleEdit.Caption, RuleGroup);
    Label->setObjectName(QString::fromLatin1(RuleEdit.LabelName));
    RuleLayout->addWidget(Label);
    QLineEdit * Edit = new QLineEdit(RuleGroup);
    Edit->setObjectName(QString::fromLatin1(RuleEdit.Name));
    Label->setBuddy(Edit);
    RuleLayout->addWidget(Edit);
    *RuleEdit.Edit = Edit;
  }
  CurrentRuleButton = new QPushButton(tr("Current"), RuleGroup);
  CurrentRuleButton->setObjectName(QStringLiteral("CurrentRuleButton"));
  CurrentRuleButton->setAutoDefault(false);
  connect(CurrentRuleButton, &QPushButton::clicked, this, [this]() { SetRuleData(*FCurrentRuleData); });
  RuleLayout->addWidget(CurrentRuleButton, 0, Qt::AlignLeft);
  QLabel * RuleMaskHintText = new QLabel(QStringLiteral("<a href=\"#\">%1</a>").arg(tr("mask hints")), RuleGroup);
  RuleMaskHintText->setObjectName(QStringLiteral("RuleMaskHintText"));
  RuleMaskHintText->setToolTip(ToQString(FORMAT(L"%s\n \n%s", (LoadStr(MASK_HINT2), LoadStr(COMBINING_MASKS_HINT)))));
  RuleMaskHintText->setAlignment(Qt::AlignRight);
  RuleLayout->addWidget(RuleMaskHintText);
  MainLayout->addWidget(RuleGroup, 3, 1);
  MainLayout->setRowStretch(4, 1);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  OkButton = Buttons->button(QDialogButtonBox::Ok);
  OkButton->setObjectName(QStringLiteral("OkButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons, 5, 0, 1, 2);
}
//---------------------------------------------------------------------------
void TCopyParamPresetDialog::UpdateControls()
{
  OkButton->setEnabled(!DescriptionEdit->text().isEmpty());
  RuleGroup->setEnabled(HasRuleCheck->isChecked());
  CurrentRuleButton->setVisible(
    (FCurrentRuleData != NULL) &&
    // current rule data are loaded implicitly
    (FMode != cpmAddCurrent));
}
//---------------------------------------------------------------------------
bool TCopyParamPresetDialog::Execute(TCopyParamList * CopyParamList, int & Index, const TCopyParamType & DefaultCopyParams)
{
  FCopyParamList = CopyParamList;
  if ((FMode == cpmEdit) || (FMode == cpmDuplicate))
  {
    const TCopyParamRule * Rule;
    if (Index >= 0)
    {
      CopyParamsFrame->SetParams(*CopyParamList->CopyParams[Index]);
      Rule = CopyParamList->Rules[Index];
    }
    else
    {
      CopyParamsFrame->SetParams(DefaultCopyParams);
      Rule = NULL;
    }

    if (FMode == cpmEdit)
    {
      DescriptionEdit->setText(ToQString(CopyParamList->Names[Index]));
      FIndex = Index;
    }
    else
    {
      DescriptionEdit->clear();
      FIndex = -1; // never used
      Index = FCopyParamList->Count;
    }
    HasRuleCheck->setChecked(Rule != NULL);

    if (Rule != NULL)
    {
      SetRuleData(Rule->Data);
    }
  }
  else
  {
    DescriptionEdit->clear();
    CopyParamsFrame->SetParams(DefaultCopyParams);
    if (FMode == cpmAddCurrent)
    {
      SetRuleData(*FCurrentRuleData);
      HasRuleCheck->setChecked(true);
    }
    else
    {
      HasRuleCheck->setChecked(false);
    }
    FIndex = -1; // never used
    if (Index < 0)
    {
      Index = FCopyParamList->Count;
    }
  }

  UpdateControls();
  CopyParamsFrame->BeforeExecute();
  bool Result = (exec() == QDialog::Accepted);

  if (Result)
  {
    CopyParamsFrame->AfterExecute();

    UnicodeString Name = ToUnicodeString(DescriptionEdit->text());
    std::unique_ptr<TCopyParamType> CopyParam(new TCopyParamType(CopyParamsFrame->GetParams()));
    std::unique_ptr<TCopyParamRule> Rule(GetRule());

    if (FMode == cpmEdit)
    {
      FCopyParamList->Change(Index, Name, CopyParam.release(), Rule.release());
    }
    else
    {
      FCopyParamList->Insert(Index, Name, CopyParam.release(), Rule.release());
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TCopyParamPresetDialog::SetRuleData(const TCopyParamRuleData & Data)
{
  HostNameEdit->setText(ToQString(Data.HostName));
  UserNameEdit->setText(ToQString(Data.UserName));
  RemoteDirectoryEdit->setText(ToQString(Data.RemoteDirectory));
  LocalDirectoryEdit->setText(ToQString(Data.LocalDirectory));
}
//---------------------------------------------------------------------------
TCopyParamRule * TCopyParamPresetDialog::GetRule()
{
  TCopyParamRule * Rule = NULL;
  if (HasRuleCheck->isChecked())
  {
    TCopyParamRuleData Data;
    Data.HostName = ToUnicodeString(HostNameEdit->text());
    Data.UserName = ToUnicodeString(UserNameEdit->text());
    Data.RemoteDirectory = ToUnicodeString(RemoteDirectoryEdit->text());
    Data.LocalDirectory = ToUnicodeString(LocalDirectoryEdit->text());
    Rule = new TCopyParamRule(Data);
  }
  return Rule;
}
//---------------------------------------------------------------------------
void TCopyParamPresetDialog::accept()
{
  try
  {
    CopyParamsFrame->Validate();
    UnicodeString Description = ToUnicodeString(DescriptionEdit->text());
    TCopyParamList::ValidateName(Description);

    if (HasRuleCheck->isChecked())
    {
      for (QLineEdit * Edit : { HostNameEdit, UserNameEdit, RemoteDirectoryEdit, LocalDirectoryEdit })
      {
        ValidateMaskEdit(Edit);
      }
      std::unique_ptr<TCopyParamRule> Rule(GetRule());
      if (Rule->IsEmpty)
      {
        throw Exception(LoadStr(COPY_PARAM_NO_RULE));
      }
    }

    int Index = FCopyParamList->IndexOfName(Description);
    if (((FMode == cpmEdit) && (Index >= 0) && (Index != FIndex)) ||
        (((FMode == cpmAdd) || (FMode == cpmAddCurrent) || (FMode == cpmDuplicate)) && (Index >= 0)))
    {
      DescriptionEdit->setFocus();
      throw Exception(FMTLOAD(COPY_PARAM_DUPLICATE, (Description)));
    }
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
    return;
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
void TCopyParamPresetDialog::done(int Result)
{
  CopyParamsFrame->Closing();
  QDialog::done(Result);
}
//---------------------------------------------------------------------------
bool DoCopyParamPresetDialog(TCopyParamList * CopyParamList,
  int & Index, TCopyParamPresetMode Mode, TCopyParamRuleData * CurrentRuleData,
  const TCopyParamType & DefaultCopyParams)
{
  TCopyParamPresetDialog Dialog(Mode, CurrentRuleData);
  return Dialog.Execute(CopyParamList, Index, DefaultCopyParams);
}
//---------------------------------------------------------------------------
const int cpiDefault = -1;
const int cpiCustom = -3;
const int cpiSaveSettings = -4;
const int cpiSavePreset = -6;
//---------------------------------------------------------------------------
int CopyParamListPopup(const QPoint & Pos, TCopyParamType & Param, UnicodeString & Preset,
  int Options, int CopyParamAttrs, bool * SaveSettings)
{
  QMenu Menu(QApplication::activeWindow());
  Menu.setObjectName(QStringLiteral("CopyParamListMenu"));

  auto AddItem = [&](const QString & Caption, int Tag, const char * Name)
  {
    QAction * Item = Menu.addAction(Caption);
    Item->setObjectName(QString::fromLatin1(Name));
    Item->setData(Tag);
    return Item;
  };

  QAction * CustomizeItem = AddItem(ToQString(LoadStr(COPY_PARAM_CUSTOM)), cpiCustom, "CustomItem");
  CustomizeItem->setCheckable(true);
  if (FLAGSET(Options, cplCustomizeDefault))
  {
    Menu.setDefaultAction(CustomizeItem);
  }

  if (FLAGSET(Options, cplSaveSettings))
  {
    QAction * Item = AddItem(ToQString(LoadStr(COPY_PARAM_SAVE_SETTINGS)), cpiSaveSettings, "SaveSettingsItem");
    Item->setCheckable(true);
    Item->setChecked(*SaveSettings);
  }

  AddItem(ToQString(LoadStr(COPY_PARAM_SAVE_PRESET)), cpiSavePreset, "SavePresetItem");

  Menu.addSection(ToQString(LoadStr(COPY_PARAM_PRESET_HEADER)));

  bool AnyChecked = false;
  QAction * Item = AddItem(ToQString(LoadStr(COPY_PARAM_DEFAULT)), cpiDefault, "DefaultItem");
  Item->setCheckable(true);
  Item->setChecked(Preset.IsEmpty() && (GUIConfiguration->CopyParamPreset[L""] == Param));
  AnyChecked = AnyChecked || Item->isChecked();

  TCopyParamType DefaultParam;
  const TCopyParamList * CopyParamList = GUIConfiguration->CopyParamList;
  for (int Index = 0; Index < CopyParamList->Count; Index++)
  {
    UnicodeString Name = CopyParamList->Names[Index];
    TCopyParamType AParam = GUIConfiguration->CopyParamPreset[Name];
    if (AParam.AnyUsableCopyParam(CopyParamAttrs) ||
        // This makes "Binary" preset visible,
        // as long as we care about transfer mode
        ((AParam == DefaultParam) &&
         FLAGCLEAR(CopyParamAttrs, cpaIncludeMaskOnly) &&
         FLAGCLEAR(CopyParamAttrs, cpaNoTransferMode)))
    {
      Item = AddItem(ToQString(Name), Index, "");
      Item->setObjectName(QStringLiteral("PresetItem%1").arg(Index));
      Item->setCheckable(true);
      Item->setChecked((Preset == Name) && (AParam == Param));
      AnyChecked = AnyChecked || Item->isChecked();
    }
  }

  CustomizeItem->setChecked(!AnyChecked);
  // The preferences (the presets) are not ported yet: no "Configure..."

  QAction * Chosen = Menu.exec(Pos);
  int Result = 0;
  if (Chosen != nullptr)
  {
    int Tag = Chosen->data().toInt();
    if (Tag == cpiCustom)
    {
      Result = DoCopyParamCustomDialog(Param, CopyParamAttrs) ? 1 : 0;
    }
    else if (Tag == cpiSaveSettings)
    {
      if (DebugAlwaysTrue(SaveSettings != NULL))
      {
        *SaveSettings = !*SaveSettings;
      }
    }
    else if (Tag == cpiSavePreset)
    {
      std::unique_ptr<TCopyParamList> NewCopyParamList(new TCopyParamList());
      *NewCopyParamList = *GUIConfiguration->CopyParamList;
      int Index = -1;
      if (DoCopyParamPresetDialog(NewCopyParamList.get(), Index, cpmAdd, NULL, Param))
      {
        GUIConfiguration->CopyParamList = NewCopyParamList.get();
        // If saved unmodified, then make this the selected preset
        if (*NewCopyParamList->CopyParams[Index] == Param)
        {
          Preset = NewCopyParamList->Names[Index];
        }
      }
    }
    else
    {
      Preset = (Tag >= 0) ? GUIConfiguration->CopyParamList->Names[Tag] : UnicodeString();
      // The cast strips away the "queue" properties of the TGUICopyParamType
      // that are not configurable in presets
      Param = TCopyParamType(GUIConfiguration->CopyParamPreset[Preset]);
      Result = 1;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
