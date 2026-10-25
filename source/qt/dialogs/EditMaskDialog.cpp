// The file mask editor (upstream's TEditMaskDialog, forms/EditMask.dfm): the
// include and exclude masks of files and directories, and the mask they
// compose.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <FileMasks.h>

#include "CustomDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
class TEditMaskDialog : public QDialog
{
public:
  TEditMaskDialog();

  bool Execute(TFileMasks & Mask);

protected:
  virtual void accept() override;

private:
  QPlainTextEdit * IncludeFileMasksMemo;
  QPlainTextEdit * ExcludeFileMasksMemo;
  QPlainTextEdit * IncludeDirectoryMasksMemo;
  QPlainTextEdit * ExcludeDirectoryMasksMemo;
  QCheckBox * ExcludeDirectoryAllCheck;
  QPlainTextEdit * MaskMemo;
  UnicodeString FExcludeDirectoryMasks;
  bool FChanging;

  QPlainTextEdit * CreateMemo(QWidget * Parent, QGridLayout * Layout, int Column, const char * Name,
    const char * LabelName, const QString & Caption);
  void LoadFileMasks(const TFileMasks & Mask);
  void LoadFileMasks(QPlainTextEdit * Memo, TStrings * MasksStr);
  void SaveFileMasks(TFileMasks & Mask);
  void UpdateControls();
  void ClearButtonClick();
  void ExcludeDirectoryAllCheckClick();
  void ExcludeDirectoryMasksMemoChange();
};
//---------------------------------------------------------------------------
TEditMaskDialog::TEditMaskDialog() :
  QDialog(QApplication::activeWindow())
{
  FChanging = false;
  setObjectName(QStringLiteral("EditMaskDialog"));
  setWindowTitle(tr("Edit file mask"));
  QVBoxLayout * MainLayout = new QVBoxLayout(this);

  QGroupBox * FilesGroup = new QGroupBox(tr("Files masks"), this);
  FilesGroup->setObjectName(QStringLiteral("FilesGroup"));
  QGridLayout * FilesLayout = new QGridLayout(FilesGroup);
  IncludeFileMasksMemo = CreateMemo(FilesGroup, FilesLayout, 0, "IncludeFileMasksMemo", "Label3", tr("&Include files:"));
  ExcludeFileMasksMemo = CreateMemo(FilesGroup, FilesLayout, 1, "ExcludeFileMasksMemo", "Label1", tr("&Exclude files:"));
  MainLayout->addWidget(FilesGroup);

  QGroupBox * DirectoriesGroup = new QGroupBox(tr("Directories masks"), this);
  DirectoriesGroup->setObjectName(QStringLiteral("DirectoriesGroup"));
  QGridLayout * DirectoriesLayout = new QGridLayout(DirectoriesGroup);
  IncludeDirectoryMasksMemo =
    CreateMemo(DirectoriesGroup, DirectoriesLayout, 0, "IncludeDirectoryMasksMemo", "Label2", tr("I&nclude directories:"));
  ExcludeDirectoryMasksMemo =
    CreateMemo(DirectoriesGroup, DirectoriesLayout, 1, "ExcludeDirectoryMasksMemo", "Label4", tr("E&xclude directories:"));
  ExcludeDirectoryAllCheck = new QCheckBox(tr("&All (do not recurse)"), DirectoriesGroup);
  ExcludeDirectoryAllCheck->setObjectName(QStringLiteral("ExcludeDirectoryAllCheck"));
  DirectoriesLayout->addWidget(ExcludeDirectoryAllCheck, 2, 1);
  connect(ExcludeDirectoryAllCheck, &QCheckBox::toggled, this, [this]() { ExcludeDirectoryAllCheckClick(); });
  connect(ExcludeDirectoryMasksMemo, &QPlainTextEdit::textChanged, this, [this]() { ExcludeDirectoryMasksMemoChange(); });
  MainLayout->addWidget(DirectoriesGroup);

  QGroupBox * MaskGroup = new QGroupBox(tr("Mask"), this);
  MaskGroup->setObjectName(QStringLiteral("MaskGroup"));
  QVBoxLayout * MaskLayout = new QVBoxLayout(MaskGroup);
  MaskMemo = new QPlainTextEdit(MaskGroup);
  MaskMemo->setObjectName(QStringLiteral("MaskMemo"));
  MaskMemo->setReadOnly(true);
  MaskMemo->setMaximumHeight(fontMetrics().lineSpacing() * 4);
  MaskLayout->addWidget(MaskMemo);
  QLabel * MaskHintText = new QLabel(QStringLiteral("<a href=\"#\">%1</a>").arg(tr("mask hints")), this);
  MaskHintText->setObjectName(QStringLiteral("MaskHintText"));
  MaskHintText->setToolTip(ToQString(
    FORMAT(L"%s\n \n%s\n \n%s\n \n%s", (LoadStr(MASK_HINT2), LoadStr(FILE_MASK_EX_HINT), LoadStr(PATH_MASK_HINT2), LoadStr(MASK_HELP)))));
  MaskHintText->setAlignment(Qt::AlignRight);
  MaskLayout->addWidget(MaskHintText);
  MainLayout->addWidget(MaskGroup);

  QDialogButtonBox * Buttons = new QDialogButtonBox(this);
  QPushButton * ClearButton = Buttons->addButton(tr("&Clear"), QDialogButtonBox::ResetRole);
  ClearButton->setObjectName(QStringLiteral("ClearButton"));
  connect(ClearButton, &QPushButton::clicked, this, [this]() { ClearButtonClick(); });
  QPushButton * OKBtn = Buttons->addButton(QDialogButtonBox::Ok);
  OKBtn->setObjectName(QStringLiteral("OKBtn"));
  QPushButton * CancelBtn = Buttons->addButton(QDialogButtonBox::Cancel);
  CancelBtn->setObjectName(QStringLiteral("CancelBtn"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
  AddHelpButton(Buttons, FormHelpKeyword(L"EditMask.dfm"));
  setMinimumWidth(fontMetrics().averageCharWidth() * 70);
}
//---------------------------------------------------------------------------
QPlainTextEdit * TEditMaskDialog::CreateMemo(QWidget * Parent, QGridLayout * Layout, int Column, const char * Name,
  const char * LabelName, const QString & Caption)
{
  QLabel * Label = new QLabel(Caption, Parent);
  Label->setObjectName(QString::fromLatin1(LabelName));
  Layout->addWidget(Label, 0, Column);
  QPlainTextEdit * Memo = new QPlainTextEdit(Parent);
  Memo->setObjectName(QString::fromLatin1(Name));
  Memo->setTabChangesFocus(true);
  Memo->setMaximumHeight(fontMetrics().lineSpacing() * 7);
  Label->setBuddy(Memo);
  Layout->addWidget(Memo, 1, Column);
  connect(Memo, &QPlainTextEdit::textChanged, this, [this]() { UpdateControls(); });
  return Memo;
}
//---------------------------------------------------------------------------
bool TEditMaskDialog::Execute(TFileMasks & Mask)
{
  LoadFileMasks(Mask);
  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    SaveFileMasks(Mask);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TEditMaskDialog::accept()
{
  try
  {
    ValidateMaskEdit(IncludeFileMasksMemo, false);
    ValidateMaskEdit(ExcludeFileMasksMemo, false);
    ValidateMaskEdit(IncludeDirectoryMasksMemo, true);
    ValidateMaskEdit(ExcludeDirectoryMasksMemo, true);
  }
  catch (EAbort &)
  {
    return;
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
void TEditMaskDialog::LoadFileMasks(const TFileMasks & Mask)
{
  LoadFileMasks(IncludeFileMasksMemo, Mask.IncludeFileMasksStr);
  LoadFileMasks(ExcludeFileMasksMemo, Mask.ExcludeFileMasksStr);
  LoadFileMasks(IncludeDirectoryMasksMemo, Mask.IncludeDirectoryMasksStr);
  LoadFileMasks(ExcludeDirectoryMasksMemo, Mask.ExcludeDirectoryMasksStr);
  UpdateControls();
}
//---------------------------------------------------------------------------
void TEditMaskDialog::LoadFileMasks(QPlainTextEdit * Memo, TStrings * MasksStr)
{
  QStringList Lines;
  for (int Index = 0; Index < MasksStr->Count; Index++)
  {
    Lines.append(ToQString(MasksStr->Strings[Index]));
  }
  Memo->setPlainText(Lines.join(QLatin1Char('\n')));
}
//---------------------------------------------------------------------------
void TEditMaskDialog::SaveFileMasks(TFileMasks & Mask)
{
  std::unique_ptr<TStrings> IncludeFileMasks(TextToStringList(ToUnicodeString(IncludeFileMasksMemo->toPlainText())));
  std::unique_ptr<TStrings> ExcludeFileMasks(TextToStringList(ToUnicodeString(ExcludeFileMasksMemo->toPlainText())));
  std::unique_ptr<TStrings> IncludeDirectoryMasks(
    TextToStringList(ToUnicodeString(IncludeDirectoryMasksMemo->toPlainText())));
  std::unique_ptr<TStrings> ExcludeDirectoryMasks(
    TextToStringList(ToUnicodeString(ExcludeDirectoryMasksMemo->toPlainText())));

  Mask =
    TFileMasks::ComposeMaskStr(
      IncludeFileMasks.get(), ExcludeFileMasks.get(),
      IncludeDirectoryMasks.get(), ExcludeDirectoryMasks.get());
}
//---------------------------------------------------------------------------
void TEditMaskDialog::ClearButtonClick()
{
  IncludeFileMasksMemo->clear();
  ExcludeFileMasksMemo->clear();
  IncludeDirectoryMasksMemo->clear();
  ExcludeDirectoryMasksMemo->clear();
  ExcludeDirectoryAllCheck->setChecked(false);
  UpdateControls();
}
//---------------------------------------------------------------------------
void TEditMaskDialog::UpdateControls()
{
  try
  {
    TFileMasks Mask;
    SaveFileMasks(Mask);
    MaskMemo->setPlainText(ToQString(Mask.Masks));
  }
  catch (EFileMasksException & E)
  {
    MaskMemo->setPlainText(ToQString(E.Message));
  }
}
//---------------------------------------------------------------------------
void TEditMaskDialog::ExcludeDirectoryAllCheckClick()
{
  if (!FChanging)
  {
    TAutoFlag Flag(FChanging);

    if (ExcludeDirectoryAllCheck->isChecked())
    {
      FExcludeDirectoryMasks = ToUnicodeString(ExcludeDirectoryMasksMemo->toPlainText());
      ExcludeDirectoryMasksMemo->setPlainText(QStringLiteral("*"));
    }
    else
    {
      ExcludeDirectoryMasksMemo->setPlainText(ToQString(FExcludeDirectoryMasks));
    }
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
void TEditMaskDialog::ExcludeDirectoryMasksMemoChange()
{
  if (!FChanging)
  {
    TAutoFlag Flag(FChanging);

    UnicodeString Mask = ToUnicodeString(ExcludeDirectoryMasksMemo->toPlainText()).Trim();
    ExcludeDirectoryAllCheck->setChecked((Mask == L"*") || (Mask == L"*/"));
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
bool DoEditMaskDialog(TFileMasks & Mask)
{
  TEditMaskDialog Dialog;
  return Dialog.Execute(Mask);
}
//---------------------------------------------------------------------------
