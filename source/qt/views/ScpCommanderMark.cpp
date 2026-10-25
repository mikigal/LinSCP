// The selection of the files of the main window: the Mark menu (upstream's
// Selection actions of forms/NonVisual.dfm) and the comparison of the
// directories of the panels.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>

#include "DirPanel.h"
#include "ScpCommander.h"

#include <QAction>
#include <QMenu>
#include <QTreeView>
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateMarkActions()
{
  // Insert, as upstream's Commander (CommanderShortcuts)
  SelectOneAction = CreateAction("SelectOneAction", tr("&Select/Unselect"), QString(),
    { QKeySequence(Qt::Key_Insert) }, &TScpCommanderForm::SelectOneActionExecute);
  SelectOneAction->setToolTip(tr("Select/unselect focused file"));
  // The keys of the numeric keypad, as upstream's (EventFilterPanels)
  SelectAction = CreateAction("SelectAction", tr("Sele&ct Files..."), QStringLiteral("edit-select"),
    { QKeySequence(Qt::KeypadModifier | Qt::Key_Plus) }, &TScpCommanderForm::SelectActionExecute);
  SelectAction->setToolTip(tr("Select files by mask"));
  UnselectAction = CreateAction("UnselectAction", tr("&Unselect Files..."), QString(),
    { QKeySequence(Qt::KeypadModifier | Qt::Key_Minus) }, &TScpCommanderForm::UnselectActionExecute);
  UnselectAction->setToolTip(tr("Unselect files by mask"));
  SelectAllAction = CreateAction("SelectAllAction", tr("Select &All"), QStringLiteral("edit-select-all"),
    { QKeySequence(Qt::CTRL | Qt::Key_A) }, &TScpCommanderForm::SelectAllActionExecute);
  SelectAllAction->setToolTip(tr("Select all files"));
  InvertSelectionAction = CreateAction("InvertSelectionAction", tr("&Invert Selection"), QStringLiteral("edit-select-invert"),
    { QKeySequence(Qt::KeypadModifier | Qt::Key_Asterisk) }, &TScpCommanderForm::InvertSelectionActionExecute);
  InvertSelectionAction->setToolTip(tr("Invert selection"));
  ClearSelectionAction = CreateAction("ClearSelectionAction", tr("C&lear Selection"), QStringLiteral("edit-select-none"),
    { QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L) }, &TScpCommanderForm::ClearSelectionActionExecute);
  ClearSelectionAction->setToolTip(tr("Clear selection"));
  RestoreSelectionAction = CreateAction("RestoreSelectionAction", tr("&Restore Selection"), QString(),
    { QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S) }, &TScpCommanderForm::RestoreSelectionActionExecute);
  RestoreSelectionAction->setToolTip(tr("Restore previous selection"));
  SelectSameExtAction = CreateAction("SelectSameExtAction", tr("Select Files with Same &Extension"), QString(),
    { QKeySequence(QKeyCombination(Qt::AltModifier | Qt::KeypadModifier, Qt::Key_Plus)) }, &TScpCommanderForm::SelectSameExtActionExecute);
  SelectSameExtAction->setToolTip(tr("Select all files with the same extension as currently focused file"));
  UnselectSameExtAction = CreateAction("UnselectSameExtAction", tr("Unselect Files with Same E&xtension"), QString(),
    { QKeySequence(QKeyCombination(Qt::AltModifier | Qt::KeypadModifier, Qt::Key_Minus)) }, &TScpCommanderForm::UnselectSameExtActionExecute);
  UnselectSameExtAction->setToolTip(tr("Unselect all files with the same extension as currently focused file"));
  CompareDirectoriesAction2 = CreateAction("CompareDirectoriesAction2", tr("&Compare Directories"), QString(),
    { QKeySequence(Qt::SHIFT | Qt::Key_F2) }, &TScpCommanderForm::CompareDirectories);
  CompareDirectoriesAction2->setToolTip(tr("Mark different files between files panels"));
}
//---------------------------------------------------------------------------
QMenu * TScpCommanderForm::CreateMarkMenu()
{
  QMenu * Menu = new QMenu(tr("&Mark"), this);
  Menu->setObjectName(QStringLiteral("MarkMenu"));
  Menu->addAction(SelectOneAction);
  Menu->addAction(SelectAction);
  Menu->addAction(UnselectAction);
  Menu->addAction(SelectAllAction);
  Menu->addSeparator();
  Menu->addAction(InvertSelectionAction);
  Menu->addAction(ClearSelectionAction);
  Menu->addAction(RestoreSelectionAction);
  Menu->addSeparator();
  Menu->addAction(SelectSameExtAction);
  Menu->addAction(UnselectSameExtAction);
  return Menu;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateMarkActions()
{
  bool FilesCount = (FCurrentPanel->GetFilesCount() > 0);
  bool SelCount = (FCurrentPanel->GetSelCount() > 0);
  SelectOneAction->setEnabled(FilesCount);
  SelectAction->setEnabled(FilesCount);
  UnselectAction->setEnabled(SelCount);
  SelectAllAction->setEnabled(FilesCount);
  InvertSelectionAction->setEnabled(FilesCount);
  ClearSelectionAction->setEnabled(SelCount);
  RestoreSelectionAction->setEnabled(FCurrentPanel->IsReady() && FCurrentPanel->GetSelectedNamesSaved());
  bool FocusedFile = !IsBusy() && FCurrentPanel->HasFocusedFile();
  SelectSameExtAction->setEnabled(FocusedFile);
  UnselectSameExtAction->setEnabled(FocusedFile);
  CompareDirectoriesAction2->setEnabled(FLocalPanel->IsReady() && FRemotePanel->IsReady());
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SelectOneActionExecute()
{
  if (FCurrentPanel->IsReady())
  {
    FCurrentPanel->SelectCurrentItem();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SelectAllActionExecute()
{
  if (FCurrentPanel->IsReady())
  {
    FCurrentPanel->SelectAll(smAll);
    FCurrentPanel->GetDirView()->setFocus();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::InvertSelectionActionExecute()
{
  if (FCurrentPanel->IsReady())
  {
    FCurrentPanel->SelectAll(smInvert);
    FCurrentPanel->GetDirView()->setFocus();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ClearSelectionActionExecute()
{
  if (FCurrentPanel->IsReady())
  {
    FCurrentPanel->SelectAll(smNone);
    FCurrentPanel->GetDirView()->setFocus();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RestoreSelectionActionExecute()
{
  if (FCurrentPanel->IsReady())
  {
    FCurrentPanel->RestoreSelectedNames();
    FCurrentPanel->GetDirView()->setFocus();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SelectByMask(bool Select)
{
  TDirPanel * Panel = FCurrentPanel;
  if (Panel->IsReady())
  {
    TFileFilter Filter;
    if (DoSelectMaskDialog(this, Select, Filter))
    {
      Configuration->Usage->Inc(L"MaskSelections");
      Panel->SelectFiles(Filter, Select);
      Panel->GetDirView()->setFocus();
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SelectSameExt(bool Select)
{
  if (FCurrentPanel->HasFocusedFile())
  {
    UnicodeString FileName = FCurrentPanel->GetFocusedFileName();
    UnicodeString Ext = UnixExtractFileExt(FileName);
    if (Ext.IsEmpty())
    {
      Ext = L".";
    }
    TFileFilter Filter;
    Filter.Masks = FORMAT(L"*%s", (Ext));
    Filter.Directories = false;
    FCurrentPanel->SelectFiles(Filter, Select);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CompareDirectories()
{
  if (FLocalPanel->IsReady() && FRemotePanel->IsReady())
  {
    bool CompareByTime = AppConfiguration->CompareByTime;
    bool CompareBySize = AppConfiguration->CompareBySize;
    FLocalPanel->CompareFiles(FRemotePanel, CompareByTime, CompareBySize);
    FRemotePanel->CompareFiles(FLocalPanel, CompareByTime, CompareBySize);
    if (FLocalPanel->GetSelCount() + FRemotePanel->GetSelCount() == 0)
    {
      UnicodeString Message = MainInstructions(LoadStr(COMPARE_NO_DIFFERENCES));
      MessageDialog(Message, qtInformation, qaOK);
    }
  }
}
//---------------------------------------------------------------------------
