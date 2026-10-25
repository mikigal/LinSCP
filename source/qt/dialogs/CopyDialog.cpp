// The transfer confirmation (upstream's TCopyDialog, forms/Copy.dfm) and the
// directory creation dialog (TCreateDirectoryDialog, forms/CreateDirectory.dfm),
// without the transfer settings and the attributes yet.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include "CustomDialog.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
//---------------------------------------------------------------------------
class TCopyDialog : public TCustomDialog
{
public:
  TCopyDialog(bool ToRemote, TStrings * FileList);

  bool Execute(UnicodeString & TargetDirectory);

protected:
  virtual void DoChange(bool & CanSubmit) override;

private:
  QComboBox * FDirectoryEdit;
};
//---------------------------------------------------------------------------
TCopyDialog::TCopyDialog(bool ToRemote, TStrings * FileList)
{
  setObjectName(QStringLiteral("CopyDialog"));
  OKButton->setObjectName(QStringLiteral("OkButton"));
  setWindowTitle(ToQString(LoadStr(ToRemote ? COPY_COPY_TOREMOTE_CAPTION : COPY_COPY_TOLOCAL_CAPTION)));

  UnicodeString TransferStr = LoadStr(ToRemote ? COPY_COPY_TOREMOTE : COPY_COPY_TOLOCAL);
  UnicodeString DirectionStr = LoadStr(ToRemote ? COPY_TOREMOTE : COPY_TOLOCAL);
  UnicodeString Caption;
  if (FileList->Count == 1)
  {
    UnicodeString FileName =
      ToRemote ? ExtractFileName(FileList->Strings[0]) : UnixExtractFileName(FileList->Strings[0]);
    Caption = FMTLOAD(COPY_FILE, (TransferStr, FileName, DirectionStr));
  }
  else
  {
    Caption = FMTLOAD(COPY_FILES, (TransferStr, FileList->Count, DirectionStr));
  }
  QLabel * DirectoryLabel = CreateLabel(Caption);
  DirectoryLabel->setObjectName(QStringLiteral("DirectoryLabel"));
  DirectoryLabel->setTextFormat(Qt::PlainText);

  FDirectoryEdit = new QComboBox(this);
  FDirectoryEdit->setObjectName(ToRemote ? QStringLiteral("RemoteDirectoryEdit") : QStringLiteral("LocalDirectoryEdit"));
  FDirectoryEdit->setEditable(true);
  FDirectoryEdit->setInsertPolicy(QComboBox::NoInsert);
  AddEditLikeControl(FDirectoryEdit, DirectoryLabel);
  setMinimumWidth(fontMetrics().averageCharWidth() * 60);
}
//---------------------------------------------------------------------------
void TCopyDialog::DoChange(bool & CanSubmit)
{
  CanSubmit = !FDirectoryEdit->currentText().trimmed().isEmpty();
}
//---------------------------------------------------------------------------
bool TCopyDialog::Execute(UnicodeString & TargetDirectory)
{
  FDirectoryEdit->setEditText(ToQString(TargetDirectory));
  FDirectoryEdit->lineEdit()->selectAll();
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    TargetDirectory = ToUnicodeString(FDirectoryEdit->currentText()).Trim();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool DoCopyDialog(bool ToRemote, TStrings * FileList, UnicodeString & TargetDirectory)
{
  TCopyDialog Dialog(ToRemote, FileList);
  return Dialog.Execute(TargetDirectory);
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
