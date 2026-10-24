// The base of simple dialogs and the input dialog (upstream's
// forms/Custom.cpp and forms/InputDlg.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

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
