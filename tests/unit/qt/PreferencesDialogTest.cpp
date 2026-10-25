// L3 tests of the preferences (dialogs/PreferencesDialog.cpp,
// EditorPreferencesDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <EditorPreferences.h>
#include <QtInterface.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTreeWidget>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// The configuration of the tests is restored after them
class PreferencesDialogTest : public testing::Test
{
protected:
  void SetUp() override
  {
    FCopyParamList = *GUIConfiguration->CopyParamList;
    FEditorList = *AppConfiguration->EditorList;
  }

  void TearDown() override
  {
    AppConfiguration->ConfirmDeleting = true;
    AppConfiguration->PathInCaption = picShort;
    AppConfiguration->FormatSizeBytes = fbKilobytes;
    AppConfiguration->DoubleClickAction = dcaEdit;
    AppConfiguration->PanelSearch = isNameStartOnly;
    AppConfiguration->DDTemporaryDirectory = L"";
    AppConfiguration->DDTransferConfirmation = asAuto;
    AppConfiguration->NortonLikeMode = nlKeyboard;
    AppConfiguration->SwappedPanels = false;
    AppConfiguration->ExplorerKeyboardShortcuts = false;
    AppConfiguration->FileColors = L"";
    Configuration->QueueTransfersLimit = 2;
    GUIConfiguration->CopyParamList = &FCopyParamList;
    AppConfiguration->EditorList = &FEditorList;
  }

  // The page shown
  static QString CurrentSheet(QWidget * Dialog)
  {
    QStackedWidget * PageControl = Child<QStackedWidget>(Dialog, "PageControl");
    QScrollArea * Scroll = qobject_cast<QScrollArea *>(PageControl->currentWidget());
    return Scroll->widget()->objectName();
  }

  static void SelectPage(QWidget * Dialog, const QString & Caption)
  {
    QTreeWidget * Tree = Child<QTreeWidget>(Dialog, "NavigationTree");
    QList<QTreeWidgetItem *> Items = Tree->findItems(Caption, Qt::MatchExactly | Qt::MatchRecursive);
    ASSERT_EQ(Items.size(), 1);
    Tree->setCurrentItem(Items[0]);
  }

private:
  TCopyParamList FCopyParamList;
  TEditorList FEditorList;
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST_F(PreferencesDialogTest, LoadsAndSavesTheConfiguration)
{
  AppConfiguration->ConfirmDeleting = true;
  AppConfiguration->PathInCaption = picShort;
  AppConfiguration->FormatSizeBytes = fbKilobytes;
  AppConfiguration->DoubleClickAction = dcaEdit;
  AppConfiguration->DDTransferConfirmation = asAuto;
  Configuration->QueueTransfersLimit = 2;

  bool Checked = false;
  WhenModal([&](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("PreferencesDialog"));
    EXPECT_EQ(CurrentSheet(Dialog), QStringLiteral("PreferencesSheet"));
    QCheckBox * ConfirmDeletingCheck = Child<QCheckBox>(Dialog, "ConfirmDeletingCheck");
    EXPECT_TRUE(ConfirmDeletingCheck->isChecked());
    // The transfer confirmation of drag and drop, undecided yet
    EXPECT_EQ(Child<QCheckBox>(Dialog, "DDTransferConfirmationCheck2")->checkState(), Qt::PartiallyChecked);
    EXPECT_TRUE(Child<QRadioButton>(Dialog, "PathInCaptionShortButton")->isChecked());
    EXPECT_EQ(Child<QComboBox>(Dialog, "FormatSizeBytesCombo")->currentIndex(), 1);
    EXPECT_EQ(Child<QComboBox>(Dialog, "DoubleClickActionCombo")->currentIndex(), 2);
    EXPECT_EQ(Child<QSpinBox>(Dialog, "QueueTransferLimitEdit")->value(), 2);
    // The copy on double-click is confirmed only when copying
    EXPECT_FALSE(Child<QCheckBox>(Dialog, "CopyOnDoubleClickConfirmationCheck")->isEnabled());

    ConfirmDeletingCheck->setChecked(false);
    Child<QCheckBox>(Dialog, "DDTransferConfirmationCheck2")->setCheckState(Qt::Unchecked);
    SelectPage(Dialog, QStringLiteral("Window"));
    EXPECT_EQ(CurrentSheet(Dialog), QStringLiteral("WindowSheet"));
    Child<QRadioButton>(Dialog, "PathInCaptionFullButton")->setChecked(true);
    Child<QComboBox>(Dialog, "FormatSizeBytesCombo")->setCurrentIndex(0);
    Child<QComboBox>(Dialog, "DoubleClickActionCombo")->setCurrentIndex(1);
    EXPECT_TRUE(Child<QCheckBox>(Dialog, "CopyOnDoubleClickConfirmationCheck")->isEnabled());
    Child<QSpinBox>(Dialog, "QueueTransferLimitEdit")->setValue(5);
    Child<QRadioButton>(Dialog, "DDCustomTemporaryDirectoryButton")->setChecked(true);
    Child<QLineEdit>(Dialog, "DDTemporaryDirectoryEdit")->setText(QStringLiteral("/var/tmp"));
    Checked = true;
    Child<QPushButton>(Dialog, "OKButton")->click();
  });
  EXPECT_TRUE(DoPreferencesDialog(pmDefault));
  EXPECT_TRUE(Checked);

  EXPECT_FALSE(AppConfiguration->ConfirmDeleting);
  EXPECT_EQ(AppConfiguration->DDTransferConfirmation, asOff);
  EXPECT_EQ(AppConfiguration->PathInCaption, picFull);
  EXPECT_EQ(AppConfiguration->FormatSizeBytes, fbNone);
  EXPECT_EQ(AppConfiguration->DoubleClickAction, dcaCopy);
  EXPECT_EQ(Configuration->QueueTransfersLimit, 5);
  EXPECT_EQ(AppConfiguration->DDTemporaryDirectory, UnicodeString(L"/var/tmp"));
  EXPECT_EQ(AppConfiguration->ExpandedTemporaryDirectory(), UnicodeString(L"/var/tmp"));
}
//---------------------------------------------------------------------------
TEST_F(PreferencesDialogTest, CancelKeepsTheConfiguration)
{
  AppConfiguration->ConfirmDeleting = true;
  WhenModal([&](QWidget * Dialog)
  {
    Child<QCheckBox>(Dialog, "ConfirmDeletingCheck")->setChecked(false);
    Child<QPushButton>(Dialog, "CloseButton")->click();
  });
  EXPECT_FALSE(DoPreferencesDialog(pmDefault));
  EXPECT_TRUE(AppConfiguration->ConfirmDeleting);
}
//---------------------------------------------------------------------------
TEST_F(PreferencesDialogTest, ModesOpenTheirPages)
{
  const std::pair<TPreferencesMode, const char *> Modes[] =
  {
    { pmCustomCommands, "CustomCommandsSheet" },
    { pmPresets, "CopyParamListSheet" },
    { pmQueue, "QueueSheet" },
    { pmLogging, "LogSheet" },
    { pmEditorInternal, "EditorInternalSheet" },
    { pmCommander, "CommanderSheet" },
    { pmFileColors, "FileColorsSheet" },
  };
  for (const auto & Mode : Modes)
  {
    QString Sheet;
    WhenModal([&](QWidget * Dialog)
    {
      Sheet = CurrentSheet(Dialog);
      Child<QPushButton>(Dialog, "CloseButton")->click();
    });
    DoPreferencesDialog(Mode.first);
    EXPECT_EQ(Sheet, QString::fromLatin1(Mode.second));
  }
}
//---------------------------------------------------------------------------
TEST_F(PreferencesDialogTest, PresetsAndEditors)
{
  std::unique_ptr<TCopyParamList> CopyParamList(new TCopyParamList());
  TCopyParamType Binary;
  Binary.TransferMode = tmBinary;
  CopyParamList->Add(L"Binary", new TCopyParamType(Binary), NULL);
  CopyParamList->Add(L"Text", new TCopyParamType(), NULL);
  GUIConfiguration->CopyParamList = CopyParamList.get();
  std::unique_ptr<TEditorList> EditorList(new TEditorList());
  TEditorData External;
  External.Editor = edExternal;
  External.ExternalEditor = L"kate !.!";
  External.FileMask = L"*.txt";
  EditorList->Add(new TEditorPreferences(External));
  EditorList->Add(new TEditorPreferences());
  AppConfiguration->EditorList = EditorList.get();

  WhenModal([&](QWidget * Dialog)
  {
    // The default settings first, which cannot be removed nor moved
    QTreeWidget * CopyParamListView = Child<QTreeWidget>(Dialog, "CopyParamListView");
    ASSERT_EQ(CopyParamListView->topLevelItemCount(), 3);
    EXPECT_EQ(CopyParamListView->topLevelItem(1)->text(0), QStringLiteral("Binary"));
    CopyParamListView->setCurrentItem(CopyParamListView->topLevelItem(0));
    EXPECT_FALSE(Child<QPushButton>(Dialog, "RemoveCopyParamButton")->isEnabled());
    EXPECT_FALSE(Child<QPushButton>(Dialog, "UpCopyParamButton")->isEnabled());
    CopyParamListView->setCurrentItem(CopyParamListView->topLevelItem(2));
    EXPECT_TRUE(Child<QPushButton>(Dialog, "UpCopyParamButton")->isEnabled());
    Child<QPushButton>(Dialog, "UpCopyParamButton")->click();
    EXPECT_EQ(CopyParamListView->topLevelItem(1)->text(0), QStringLiteral("Text"));
    CopyParamListView->setCurrentItem(CopyParamListView->topLevelItem(2));
    Child<QPushButton>(Dialog, "RemoveCopyParamButton")->click();
    EXPECT_EQ(CopyParamListView->topLevelItemCount(), 2);

    QTreeWidget * EditorListView = Child<QTreeWidget>(Dialog, "EditorListView3");
    ASSERT_EQ(EditorListView->topLevelItemCount(), 2);
    EXPECT_EQ(EditorListView->topLevelItem(0)->text(0), QStringLiteral("Kate"));
    EXPECT_EQ(EditorListView->topLevelItem(0)->text(1), QStringLiteral("*.txt"));
    EditorListView->setCurrentItem(EditorListView->topLevelItem(1));
    Child<QPushButton>(Dialog, "UpEditorButton")->click();
    EXPECT_EQ(EditorListView->topLevelItem(1)->text(0), QStringLiteral("Kate"));
    Child<QPushButton>(Dialog, "OKButton")->click();
  });
  EXPECT_TRUE(DoPreferencesDialog(pmPresets));

  ASSERT_EQ(GUIConfiguration->CopyParamList->Count, 1);
  EXPECT_EQ(GUIConfiguration->CopyParamList->Names[0], UnicodeString(L"Text"));
  ASSERT_EQ(AppConfiguration->EditorList->Count, 2);
  EXPECT_EQ(AppConfiguration->EditorList->Editors[1]->Data->Editor, edExternal);
}
//---------------------------------------------------------------------------
TEST_F(PreferencesDialogTest, EditorPreferencesDialog)
{
  TEditorData Editor;
  Editor.Editor = edInternal;
  Editor.FileMask = L"*.*";
  bool Remember = false;
  WhenModal([&](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("EditorPreferencesDialog"));
    EXPECT_TRUE(Child<QRadioButton>(Dialog, "EditorInternalButton")->isChecked());
    EXPECT_FALSE(Child<QComboBox>(Dialog, "ExternalEditorEdit")->isEnabled());
    Child<QRadioButton>(Dialog, "EditorExternalButton")->setChecked(true);
    // An external editor needs its command
    EXPECT_FALSE(Child<QPushButton>(Dialog, "OkButton")->isEnabled());
    Child<QComboBox>(Dialog, "ExternalEditorEdit")->setEditText(QStringLiteral("/usr/bin/kate"));
    Child<QComboBox>(Dialog, "MaskEdit")->setEditText(QStringLiteral("*.c"));
    Child<QCheckBox>(Dialog, "ExternalEditorTextCheck")->setChecked(true);
    Child<QPushButton>(Dialog, "OkButton")->click();
  });
  EXPECT_TRUE(DoEditorPreferencesDialog(&Editor, Remember, epmEdit, true));
  EXPECT_EQ(Editor.Editor, edExternal);
  // The file name pattern is added
  EXPECT_EQ(Editor.ExternalEditor, UnicodeString(L"/usr/bin/kate !.!"));
  EXPECT_EQ(Editor.FileMask.Masks, UnicodeString(L"*.c"));
  EXPECT_TRUE(Editor.ExternalEditorText);
}
//---------------------------------------------------------------------------
TEST_F(PreferencesDialogTest, CommanderAndFileColors)
{
  AppConfiguration->NortonLikeMode = nlKeyboard;
  AppConfiguration->FileColors = L"0000FF:*.txt";
  WhenModal([&](QWidget * Dialog)
  {
    QTreeWidget * FileColorsView = Child<QTreeWidget>(Dialog, "FileColorsView");
    ASSERT_EQ(FileColorsView->topLevelItemCount(), 1);
    EXPECT_EQ(FileColorsView->topLevelItem(0)->text(0), QStringLiteral("*.txt"));
    EXPECT_EQ(FileColorsView->topLevelItem(0)->foreground(0).color(), QColor(Qt::red));
    EXPECT_FALSE(Child<QPushButton>(Dialog, "EditFileColorButton")->isEnabled());
    WhenModal("SelectMaskDialog", [](QWidget * MaskDialog)
    {
      Child<QComboBox>(MaskDialog, "MaskEdit")->setEditText(QStringLiteral("*.log"));
      Child<QPushButton>(MaskDialog, "OKBtn")->click();
      WhenModal("PreferencesDialog", [](QWidget * ADialog)
      {
        QTreeWidget * FileColorsView = Child<QTreeWidget>(ADialog, "FileColorsView");
        ASSERT_EQ(FileColorsView->topLevelItemCount(), 2);
        // Moved to the top
        EXPECT_TRUE(Child<QPushButton>(ADialog, "UpFileColorButton")->isEnabled());
        Child<QPushButton>(ADialog, "UpFileColorButton")->click();
        EXPECT_EQ(FileColorsView->topLevelItem(0)->text(0), QStringLiteral("*.log"));

        SelectPage(ADialog, QStringLiteral("Commander"));
        EXPECT_EQ(CurrentSheet(ADialog), QStringLiteral("CommanderSheet"));
        QComboBox * NortonLikeModeCombo = Child<QComboBox>(ADialog, "NortonLikeModeCombo");
        EXPECT_EQ(NortonLikeModeCombo->currentIndex(), 1);
        NortonLikeModeCombo->setCurrentIndex(2);
        Child<QCheckBox>(ADialog, "SwappedPanelsCheck")->setChecked(true);
        Child<QComboBox>(ADialog, "ExplorerKeyboardShortcutsCombo")->setCurrentIndex(1);
        Child<QPushButton>(ADialog, "OKButton")->click();
      });
    });
    Child<QPushButton>(Dialog, "AddFileColorButton")->click();
  });
  EXPECT_TRUE(DoPreferencesDialog(pmFileColors));
  EXPECT_EQ(AppConfiguration->FileColors, UnicodeString(L"000000:*.log,0000FF:*.txt"));
  EXPECT_EQ(AppConfiguration->NortonLikeMode, nlOff);
  EXPECT_TRUE(AppConfiguration->SwappedPanels);
  EXPECT_TRUE(AppConfiguration->ExplorerKeyboardShortcuts);
}
//---------------------------------------------------------------------------
TEST_F(PreferencesDialogTest, Languages)
{
  AppConfiguration->Language = L"";
  WhenModal([](QWidget * Dialog)
  {
    SelectPage(Dialog, QStringLiteral("Languages"));
    EXPECT_EQ(CurrentSheet(Dialog), QStringLiteral("LanguagesSheet"));
    QTreeWidget * LanguagesView = Child<QTreeWidget>(Dialog, "LanguagesView");
    ASSERT_GT(LanguagesView->topLevelItemCount(), 50);
    // The language of the system first, English, the translations
    EXPECT_EQ(LanguagesView->topLevelItem(0)->text(0), QStringLiteral("Automatic"));
    EXPECT_EQ(LanguagesView->currentItem(), LanguagesView->topLevelItem(0));
    EXPECT_EQ(LanguagesView->topLevelItem(1)->text(0), QStringLiteral("English"));
    QList<QTreeWidgetItem *> Polish = LanguagesView->findItems(QStringLiteral("Polski"), Qt::MatchExactly);
    ASSERT_EQ(Polish.size(), 1);
    QList<QTreeWidgetItem *> Brazilian = LanguagesView->findItems(QStringLiteral("Português (Brasil)"), Qt::MatchExactly);
    EXPECT_EQ(Brazilian.size(), 1);
    // The change applies on the next start
    QLabel * LanguageChangeLabel = Child<QLabel>(Dialog, "LanguageChangeLabel");
    EXPECT_TRUE(LanguageChangeLabel->isHidden());
    LanguagesView->setCurrentItem(Polish[0]);
    EXPECT_FALSE(LanguageChangeLabel->isHidden());
    Child<QPushButton>(Dialog, "OKButton")->click();
  });
  EXPECT_TRUE(DoPreferencesDialog(pmDefault));
  EXPECT_EQ(AppConfiguration->Language, UnicodeString(L"PL"));
  AppConfiguration->Language = L"";
}
//---------------------------------------------------------------------------
