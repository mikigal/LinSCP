// The base of simple dialogs: a column of labelled controls with OK and
// Cancel buttons (upstream's TCustomDialog, forms/Custom.cpp).
//---------------------------------------------------------------------------
#ifndef CustomDialogH
#define CustomDialogH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <QDialog>
//---------------------------------------------------------------------------
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
//---------------------------------------------------------------------------
class TCustomDialog : public QDialog
{
  Q_OBJECT

public:
  explicit TCustomDialog(QWidget * Parent = nullptr);

  bool Execute();

protected:
  QPushButton * OKButton;
  QPushButton * CancelButton;

  QLabel * CreateLabel(const UnicodeString & Caption);
  void AddEditLikeControl(QWidget * Edit, QLabel * Label);
  QCheckBox * CreateAndAddCheckBox(const UnicodeString & Caption);
  void AddWidget(QWidget * Widget);
  // Updates the state of the controls (the OK button with CanSubmit)
  void Change();

  virtual void DoChange(bool & CanSubmit);
  // Throws when the dialog cannot be submitted
  virtual void DoValidate();
  virtual void accept() override;

private:
  QVBoxLayout * FControlsLayout;
};
//---------------------------------------------------------------------------
// The history of a combo box: the value goes first, without duplicates
// (upstream's THistoryComboBox::SaveToHistory)
void SaveToHistory(QComboBox * Combo);
//---------------------------------------------------------------------------
#endif
