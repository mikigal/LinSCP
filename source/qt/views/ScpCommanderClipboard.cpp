// The clipboard of the Commander (upstream's CopyFilesToClipboard,
// PasteFromClipBoard and PanelExport of TCustomScpExplorerForm,
// forms/CustomScpExplorer.cpp): files copied to other applications and
// pasted from them, between the panels, and names and paths as text.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <ExportedFiles.h>
#include <Option.h>
#include <TerminalManager.h>

#include "DirPanel.h"
#include "ScpCommander.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QTimer>
#include <QUrl>
//---------------------------------------------------------------------------
// Whether the files of the clipboard are to be moved: KDE's and GNOME's
// file managers mark cut files so
static const QString KdeCutSelectionMimeType = QStringLiteral("application/x-kde-cutselection");
static const QString GnomeCopiedFilesMimeType = QStringLiteral("x-special/gnome-copied-files");
//---------------------------------------------------------------------------
static bool IsCutClipboard(const QMimeData * Data)
{
  if (Data->hasFormat(KdeCutSelectionMimeType))
  {
    return Data->data(KdeCutSelectionMimeType).startsWith('1');
  }
  if (Data->hasFormat(GnomeCopiedFilesMimeType))
  {
    return Data->data(GnomeCopiedFilesMimeType).startsWith("cut");
  }
  return false;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CopyFilesToClipboard()
{
  TDirPanel * APanel = FCurrentPanel;
  if (IsBusy() || !APanel->IsReady())
  {
    return;
  }
  std::unique_ptr<TStrings> FileList(CreateFileList(APanel));
  if (FileList->Count == 0)
  {
    return;
  }

  // The data of the clipboard are replaced
  ClipboardStop();
  QMimeData * Data;
  if (APanel == FLocalPanel)
  {
    Data = new QMimeData();
    QList<QUrl> Urls;
    QByteArray GnomeList("copy");
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      QUrl Url = LocalPathToUrl(FileList->Strings[Index]);
      Urls.append(Url);
      GnomeList.append('\n');
      GnomeList.append(Url.toEncoded());
    }
    Data->setUrls(Urls);
    Data->setData(GnomeCopiedFilesMimeType, GnomeList);
  }
  else
  {
    // Other applications get the files downloaded to the directory the
    // fake directory is pasted to, else to a temporary directory once they
    // ask for them
    TRemoteFilesMimeData * RemoteData = new TRemoteFilesMimeData();
    if (AppConfiguration->DDFakeFile)
    {
      UnicodeString FakeDirectory = ExcludeTrailingBackslash(AppConfiguration->TemporaryDir());
      if (!ForceDirectories(ApiPath(FakeDirectory)))
      {
        delete RemoteData;
        throw EOSExtException(FMTLOAD(CREATE_TEMP_DIR_ERROR, (FakeDirectory)));
      }
      FClipboardFakeDirectory = FakeDirectory;
      RemoteData->SetFakeDirectory(ToQString(FakeDirectory));
      FPasteWatcher.reset(new TPasteWatcher(FakeDirectory, nullptr));
      FPasteWatcher->OnPasted = [this](const UnicodeString & Directory) { ClipboardPasted(Directory); };
      AppLogFmt(L"Created fake clipboard directory \"%s\"", (FakeDirectory));
    }
    else
    {
      RemoteData->SetDownload([this]() { return TemporarilyDownloadForOtherApplication(FClipboardFileList.get()); });
    }
    FClipboardFileList.reset(FileList.release());
    FClipboardTerminal = GetTerminal();
    Data = RemoteData;
  }
  QApplication::clipboard()->setMimeData(Data);
  UpdateActions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ClipboardStop()
{
  FPasteWatcher.reset();
  if (!FClipboardFakeDirectory.IsEmpty())
  {
    RemoveDir(ApiPath(FClipboardFakeDirectory));
    FClipboardFakeDirectory = UnicodeString();
  }
  FClipboardFileList.reset();
  FClipboardTerminal = NULL;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ClipboardChanged()
{
  // Another application took the clipboard
  if (!QApplication::clipboard()->ownsClipboard() && (FClipboardFileList != nullptr))
  {
    ClipboardStop();
  }
  UpdateActions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ClipboardTerminalChanged(TTerminal * ATerminal)
{
  // The files of a session that is gone cannot be pasted
  if ((FClipboardTerminal != NULL) && (FClipboardTerminal != ATerminal))
  {
    ClipboardStop();
    QClipboard * Clipboard = QApplication::clipboard();
    if (Clipboard->ownsClipboard() && IsRemoteFilesMimeData(Clipboard->mimeData()))
    {
      Clipboard->clear();
    }
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::DoesClipboardContainOurFiles()
{
  return
    (FClipboardFileList != nullptr) && (FClipboardTerminal != NULL) && (FClipboardTerminal == GetTerminal()) &&
    IsRemoteFilesMimeData(QApplication::clipboard()->mimeData());
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ClipboardPasted(const UnicodeString & Directory)
{
  AppLogFmt(L"Fake clipboard directory pasted to \"%s\"", (Directory));
  // Not within the signal of the other application
  QTimer::singleShot(0, this, [this, Directory]()
  {
    if (IsBusy())
    {
      AppLog(L"Is busy, cannot paste");
    }
    else if (DoesClipboardContainOurFiles() && FRemotePanel->IsReady())
    {
      ClipboardDownload(Directory, true);
    }
  });
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ClipboardDownload(const UnicodeString & TargetDirectory, bool DragDrop)
{
  // The clipboard may change during the transfer
  std::unique_ptr<TStrings> FileList(TRemoteFileList::CloneStrings(FClipboardFileList.get()));
  TTransferOperationParam Param;
  Param.TargetDirectory = TargetDirectory;
  Param.DragDrop = DragDrop;
  ExecuteCopyMoveFileOperation(foCopy, osRemote, FileList.get(), Param);
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CanPasteFromClipboard()
{
  bool Result = false;
  const QMimeData * Data = QApplication::clipboard()->mimeData();
  if (IsBusy() || (Data == nullptr))
  {
    // noop
  }
  else if (DoesClipboardContainOurFiles())
  {
    Result = FCurrentPanel->IsReady();
  }
  else if (IsRemoteFilesMimeData(Data))
  {
    // Of a session that is gone
  }
  // Files of other applications are uploaded
  else if (Data->hasUrls())
  {
    Result = (FCurrentPanel == FRemotePanel) && FRemotePanel->IsReady();
  }
  else if (Data->hasText())
  {
    UnicodeString Text = ToUnicodeString(Data->text()).Trim();
    Result =
      !Text.IsEmpty() && (Text.Pos(L"\n") == 0) &&
      (FLAGSET(StoredSessions->GetUrlInfo(Text), piProtocolDefined) || FCurrentPanel->IsReady());
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::PasteFromClipboard()
{
  if (!CanPasteFromClipboard())
  {
    return;
  }
  const QMimeData * Data = QApplication::clipboard()->mimeData();
  if (DoesClipboardContainOurFiles())
  {
    if (FCurrentPanel == FLocalPanel)
    {
      ClipboardDownload(FLocalPanel->GetPath(), false);
    }
    else
    {
      std::unique_ptr<TStrings> FileList(TRemoteFileList::CloneStrings(FClipboardFileList.get()));
      RemoteTransferFiles(FileList.get(), false);
    }
  }
  else if (Data->hasUrls())
  {
    std::unique_ptr<TStrings> FileList(LocalFileListFromMimeData(Data));
    if (FileList != nullptr)
    {
      bool Move = IsCutClipboard(Data);
      ExecuteCopyMoveFileOperation(Move ? foMove : foCopy, osLocal, FileList.get());
    }
  }
  else
  {
    UnicodeString Text = ToUnicodeString(Data->text()).Trim();
    int ParsedInfo = StoredSessions->GetUrlInfo(Text);
    // A session URL replaces the session (there are no tabs yet)
    if (FLAGSET(ParsedInfo, piProtocolDefined))
    {
      if (!FLAGSET(ParsedInfo, piUnsafeSettings) ||
          (MessageDialog(LoadStr(UNSAFE_SESSION), qtConfirmation, qaOK | qaCancel) == qaOK))
      {
        TOptions Options;
        UnicodeString DownloadFile;
        OpenCommandLineSession(Text, &Options, false, DownloadFile);
      }
    }
    // A path to open in the current panel
    else
    {
      FCurrentPanel->ChangeDirectory(Text);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CopyTextToClipboard(TStrings * Strings)
{
  QString Text;
  for (int Index = 0; Index < Strings->Count; Index++)
  {
    if (Index > 0)
    {
      Text.append(QLatin1Char('\n'));
    }
    Text.append(ToQString(Strings->Strings[Index]));
  }
  QApplication::clipboard()->setText(Text);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::FileListToClipboard(bool FullPath)
{
  TDirPanel * APanel = FCurrentPanel;
  if (APanel->IsReady())
  {
    std::unique_ptr<TStrings> FileList(CreateFileList(APanel, FullPath));
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      if ((APanel == FLocalPanel) && !FullPath)
      {
        FileList->Strings[Index] = ExtractFileName(FileList->Strings[Index]);
      }
      if (FileList->Strings[Index].Pos(L" ") > 0)
      {
        FileList->Strings[Index] = FORMAT(L"\"%s\"", (FileList->Strings[Index]));
      }
    }
    CopyTextToClipboard(FileList.get());
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::PathToClipboard(TDirPanel * APanel)
{
  if (APanel->IsReady())
  {
    std::unique_ptr<TStrings> Strings(new TStringList());
    Strings->Add(APanel->GetPath());
    CopyTextToClipboard(Strings.get());
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::LocalPathToClipboardActionExecute()
{
  PathToClipboard(FLocalPanel);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RemotePathToClipboardActionExecute()
{
  PathToClipboard(FRemotePanel);
}
//---------------------------------------------------------------------------
