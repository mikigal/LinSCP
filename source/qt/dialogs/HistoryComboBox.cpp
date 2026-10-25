// An editable combo box with a history.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"

#include <QLineEdit>
#include <QSignalBlocker>
//---------------------------------------------------------------------------
THistoryComboBox::THistoryComboBox(QWidget * Parent) :
  QComboBox(Parent)
{
  setEditable(true);
  setInsertPolicy(QComboBox::NoInsert);
  // The history is not completed into the text as it is typed
  setCompleter(nullptr);
}
//---------------------------------------------------------------------------
void THistoryComboBox::SetHistoryKey(const UnicodeString & Key)
{
  FHistoryKey = Key;
  QString Text = currentText();
  {
    QSignalBlocker Blocker(this);
    clear();
    TStrings * History = AppConfiguration->History[Key];
    for (int Index = 0; Index < History->Count; Index++)
    {
      addItem(ToQString(History->Strings[Index]));
    }
    setCurrentIndex(-1);
  }
  setEditText(Text);
}
//---------------------------------------------------------------------------
UnicodeString THistoryComboBox::GetText() const
{
  return ToUnicodeString(currentText());
}
//---------------------------------------------------------------------------
void THistoryComboBox::SetText(const UnicodeString & Value)
{
  setEditText(ToQString(Value));
}
//---------------------------------------------------------------------------
void THistoryComboBox::SaveToHistory()
{
  ::SaveToHistory(this);
  if (!FHistoryKey.IsEmpty())
  {
    std::unique_ptr<TStrings> History(new TStringList());
    for (int Index = 0; Index < count(); Index++)
    {
      History->Add(ToUnicodeString(itemText(Index)));
    }
    AppConfiguration->History[FHistoryKey] = History.get();
  }
}
//---------------------------------------------------------------------------
