// Keyboard shortcuts as the configuration stores them.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include "ShortCuts.h"

#include <Configuration.h>

#include <QComboBox>
//---------------------------------------------------------------------------
namespace
{
// The virtual key codes of the function keys (Windows' VK_F1..VK_F24)
const Word VKF1 = 0x70;
const Word VKF24 = 0x87;
//---------------------------------------------------------------------------
const TShortCut FirstCtrlNumberShortCut = ShortCut(L'0', TShiftState() << ssCtrl);
const TShortCut LastCtrlNumberShortCut = ShortCut(L'9', TShiftState() << ssCtrl);
const TShortCut FirstShiftCtrlAltLetterShortCut = ShortCut(L'A', TShiftState() << ssShift << ssCtrl << ssAlt);
const TShortCut LastShiftCtrlAltLetterShortCut = ShortCut(L'Z', TShiftState() << ssShift << ssCtrl << ssAlt);
} // namespace
//---------------------------------------------------------------------------
TShortCut ShortCut(Word Key, TShiftState Shift)
{
  return TShortCut(
    (Key & 0xFF) |
    FLAGMASK(Shift.Contains(ssShift), scShift) |
    FLAGMASK(Shift.Contains(ssCtrl), scCtrl) |
    FLAGMASK(Shift.Contains(ssAlt), scAlt));
}
//---------------------------------------------------------------------------
QKeySequence ShortCutToKeySequence(TShortCut ShortCut)
{
  Word Key = (ShortCut & 0xFF);
  int QtKey = 0;
  if (((Key >= L'0') && (Key <= L'9')) || ((Key >= L'A') && (Key <= L'Z')))
  {
    // The same codes
    QtKey = Key;
  }
  else if ((Key >= VKF1) && (Key <= VKF24))
  {
    QtKey = Qt::Key_F1 + (Key - VKF1);
  }

  QKeySequence Result;
  if (QtKey != 0)
  {
    Qt::KeyboardModifiers Modifiers;
    if (FLAGSET(ShortCut, scShift))
    {
      Modifiers |= Qt::ShiftModifier;
    }
    if (FLAGSET(ShortCut, scCtrl))
    {
      Modifiers |= Qt::ControlModifier;
    }
    if (FLAGSET(ShortCut, scAlt))
    {
      Modifiers |= Qt::AltModifier;
    }
    Result = QKeySequence(QKeyCombination(Modifiers, static_cast<Qt::Key>(QtKey)));
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString ShortCutToText(TShortCut ShortCut)
{
  return ToUnicodeString(ShortCutToKeySequence(ShortCut).toString(QKeySequence::NativeText));
}
//---------------------------------------------------------------------------
static void AddShortCut(QComboBox * ComboBox, int Index, TShortCut ShortCut)
{
  ComboBox->insertItem(Index, ToQString(ShortCutToText(ShortCut)), QVariant(int(ShortCut)));
}
//---------------------------------------------------------------------------
void InitializeShortCutCombo(QComboBox * ComboBox, const TShortCuts & ShortCuts)
{
  ComboBox->clear();
  ComboBox->setEditable(false);
  ComboBox->addItem(ToQString(LoadStr(SHORTCUT_NONE)), QVariant(0));

  for (TShortCut AShortCut = FirstCtrlNumberShortCut; AShortCut <= LastCtrlNumberShortCut; AShortCut++)
  {
    if (!ShortCuts.Has(AShortCut))
    {
      AddShortCut(ComboBox, ComboBox->count(), AShortCut);
    }
  }

  for (TShortCut AShortCut = FirstShiftCtrlAltLetterShortCut; AShortCut <= LastShiftCtrlAltLetterShortCut; AShortCut++)
  {
    if (!ShortCuts.Has(AShortCut))
    {
      AddShortCut(ComboBox, ComboBox->count(), AShortCut);
    }
  }
}
//---------------------------------------------------------------------------
void SetShortCutCombo(QComboBox * ComboBox, TShortCut Value)
{
  for (int Index = ComboBox->count() - 1; Index >= 0; Index--)
  {
    TShortCut AShortCut = TShortCut(ComboBox->itemData(Index).toInt());
    if (AShortCut == Value)
    {
      ComboBox->setCurrentIndex(Index);
      break;
    }
    else if (AShortCut < Value)
    {
      // The shortcut of the item itself is not offered (as already used)
      DebugAssert(Value != 0);
      AddShortCut(ComboBox, Index + 1, Value);
      ComboBox->setCurrentIndex(Index + 1);
      break;
    }
    DebugAssert(Index > 0);
  }
}
//---------------------------------------------------------------------------
TShortCut GetShortCutCombo(QComboBox * ComboBox)
{
  return TShortCut(ComboBox->currentData().toInt());
}
//---------------------------------------------------------------------------
bool IsCustomShortCut(TShortCut ShortCut)
{
  return
    ((FirstCtrlNumberShortCut <= ShortCut) && (ShortCut <= LastCtrlNumberShortCut)) ||
    ((FirstShiftCtrlAltLetterShortCut <= ShortCut) && (ShortCut <= LastShiftCtrlAltLetterShortCut));
}
//---------------------------------------------------------------------------
