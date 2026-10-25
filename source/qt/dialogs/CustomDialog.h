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
class QPlainTextEdit;
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
// Validates a file mask; an error is shown, the edit focused with the
// error selected, and EAbort thrown (upstream's ValidateMaskEdit)
void ValidateMaskEdit(QComboBox * Edit, int ForceDirectoryMasks = -1);
void ValidateMaskEdit(QLineEdit * Edit, int ForceDirectoryMasks = -1);
void ValidateMaskEdit(QPlainTextEdit * Edit, bool Directory);
// Selects the program of the command (keeping its parameters; with
// FileNameCommand, the file name pattern is added to them)
void BrowseForExecutable(QComboBox * Control, const UnicodeString & Title, bool FileNameCommand);
//---------------------------------------------------------------------------
#endif
