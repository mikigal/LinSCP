// Keyboard shortcuts as the configuration stores them (the VCL's TShortCut:
// a virtual key code with the scShift, scCtrl and scAlt flags) and the
// shortcuts that can be assigned to bookmarks and custom commands
// (upstream's windows/UserInterface.cpp).
//---------------------------------------------------------------------------
#ifndef ShortCutsH
#define ShortCutsH
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include <QKeySequence>
//---------------------------------------------------------------------------
class QComboBox;
class TShortCuts;
//---------------------------------------------------------------------------
const TShortCut scShift = 0x2000;
const TShortCut scCtrl = 0x4000;
const TShortCut scAlt = 0x8000;
//---------------------------------------------------------------------------
TShortCut ShortCut(Word Key, TShiftState Shift);
QKeySequence ShortCutToKeySequence(TShortCut ShortCut);
// As Qt shows the shortcut (empty for none or an unknown key)
UnicodeString ShortCutToText(TShortCut ShortCut);
//---------------------------------------------------------------------------
// Ctrl+0..9 and Shift+Ctrl+Alt+A..Z, except those already used
void InitializeShortCutCombo(QComboBox * ComboBox, const TShortCuts & ShortCuts);
void SetShortCutCombo(QComboBox * ComboBox, TShortCut Value);
TShortCut GetShortCutCombo(QComboBox * ComboBox);
bool IsCustomShortCut(TShortCut ShortCut);
//---------------------------------------------------------------------------
#endif
