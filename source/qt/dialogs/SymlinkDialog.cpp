// The link to add or edit (upstream's forms/Symlink.dfm): the name of the
// link, its target and whether it is a hard link.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <QtInterface.h>
#include <TextsWin.h>

#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
class TSymlinkDialog : public QDialog
{
public:
  TSymlinkDialog(bool Edit, bool AllowHardLink);

  bool Execute(UnicodeString & FileName, UnicodeString & PointTo, bool & SymbolicLink);

private:
  bool FEdit;
  bool FAllowHardLink;
  QLineEdit * FileNameEdit;
  QLineEdit * PointToEdit;
  QCheckBox * HardLinkCheck;
  QPushButton * OkButton;

  void UpdateControls();
};
//---------------------------------------------------------------------------
TSymlinkDialog::TSymlinkDialog(bool Edit, bool AllowHardLink) :
  QDialog(QApplication::activeWindow()),
  FEdit(Edit),
  FAllowHardLink(AllowHardLink)
{
  setObjectName(QStringLiteral("SymlinkDialog"));
  setWindowTitle(ToQString(LoadStr(Edit ? LINK_EDIT_CAPTION : LINK_ADD_CAPTION)));

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QGroupBox * SymlinkGroup = new QGroupBox(this);
  SymlinkGroup->setObjectName(QStringLiteral("SymlinkGroup"));
  QVBoxLayout * GroupLayout = new QVBoxLayout(SymlinkGroup);
  QLabel * FileNameLabel = new QLabel(tr("&Link/shortcut file:"), SymlinkGroup);
  FileNameLabel->setObjectName(QStringLiteral("FileNameLabel"));
  GroupLayout->addWidget(FileNameLabel);
  FileNameEdit = new QLineEdit(SymlinkGroup);
  FileNameEdit->setObjectName(QStringLiteral("FileNameEdit"));
  // The name of an edited link is not changed
  FileNameEdit->setReadOnly(Edit);
  if (Edit)
  {
    FileNameEdit->setFocusPolicy(Qt::NoFocus);
  }
  FileNameLabel->setBuddy(FileNameEdit);
  GroupLayout->addWidget(FileNameEdit);
  QLabel * Label1 = new QLabel(tr("&Point link/shortcut to:"), SymlinkGroup);
  GroupLayout->addWidget(Label1);
  PointToEdit = new QLineEdit(SymlinkGroup);
  PointToEdit->setObjectName(QStringLiteral("PointToEdit"));
  Label1->setBuddy(PointToEdit);
  GroupLayout->addWidget(PointToEdit);
  HardLinkCheck = new QCheckBox(tr("&Hard link"), SymlinkGroup);
  HardLinkCheck->setObjectName(QStringLiteral("HardLinkCheck"));
  GroupLayout->addWidget(HardLinkCheck);
  MainLayout->addWidget(SymlinkGroup);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  OkButton = Buttons->button(QDialogButtonBox::Ok);
  OkButton->setObjectName(QStringLiteral("OkButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
  AddHelpButton(Buttons, FormHelpKeyword(L"Symlink.dfm"));

  connect(FileNameEdit, &QLineEdit::textChanged, this, [this]() { UpdateControls(); });
  connect(PointToEdit, &QLineEdit::textChanged, this, [this]() { UpdateControls(); });
  resize(fontMetrics().averageCharWidth() * 60, sizeHint().height());
}
//---------------------------------------------------------------------------
void TSymlinkDialog::UpdateControls()
{
  // Upstream allows the hard links of the remote side only (the local
  // links are Windows shortcuts there)
  HardLinkCheck->setEnabled(!FEdit && FAllowHardLink);
  OkButton->setEnabled(!FileNameEdit->text().isEmpty() && !PointToEdit->text().isEmpty());
}
//---------------------------------------------------------------------------
bool TSymlinkDialog::Execute(UnicodeString & FileName, UnicodeString & PointTo, bool & SymbolicLink)
{
  FileNameEdit->setText(ToQString(FileName));
  PointToEdit->setText(ToQString(PointTo));
  HardLinkCheck->setChecked(!SymbolicLink);
  UpdateControls();
  (FEdit ? PointToEdit : FileNameEdit)->setFocus();
  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    FileName = ToUnicodeString(FileNameEdit->text());
    PointTo = ToUnicodeString(PointToEdit->text());
    SymbolicLink = !HardLinkCheck->isChecked();
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoSymlinkDialog(UnicodeString & FileName, UnicodeString & PointTo,
  bool & SymbolicLink, bool Edit, bool AllowHardLink)
{
  TSymlinkDialog Dialog(Edit, AllowHardLink);
  return Dialog.Execute(FileName, PointTo, SymbolicLink);
}
//---------------------------------------------------------------------------
