// An editable combo box with a history of the values entered (upstream's
// THistoryComboBox, packages/my/HistoryComboBox.pas), kept in the
// configuration under its history key.
//---------------------------------------------------------------------------
#ifndef HistoryComboBoxH
#define HistoryComboBoxH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <QComboBox>
//---------------------------------------------------------------------------
class THistoryComboBox : public QComboBox
{
  Q_OBJECT

public:
  explicit THistoryComboBox(QWidget * Parent);

  // Loads the history of the key (the text is kept)
  void SetHistoryKey(const UnicodeString & Key);
  const UnicodeString & GetHistoryKey() const { return FHistoryKey; }
  UnicodeString GetText() const;
  void SetText(const UnicodeString & Value);
  // The text goes first in the history, which is stored
  void SaveToHistory();

private:
  UnicodeString FHistoryKey;
};
//---------------------------------------------------------------------------
#endif
