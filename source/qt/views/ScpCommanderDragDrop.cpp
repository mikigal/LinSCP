// Drag and drop of the Commander (upstream's drag and drop handlers of
// TCustomScpExplorerForm, forms/CustomScpExplorer.cpp): files dragged
// between the panels, from other applications to the remote panel and from
// the panels to other applications.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <ExportedFiles.h>
#include <TerminalManager.h>

#include "DirModel.h"
#include "DirPanel.h"
#include "ScpCommander.h"

#include <QDeadlineTimer>
#include <QDrag>
#include <QEventLoop>
#include <QMimeData>
#include <QTimer>
#include <QUrl>
//---------------------------------------------------------------------------
// How long the target of a drag may take to tell its directory after the
// drop (upstream's DDExtTimeout)
static const int DDExtTimeout = 1000;
//---------------------------------------------------------------------------
void TScpCommanderForm::PanelStartDrag(TDirPanel * APanel)
{
  if (IsBusy() || !APanel->IsReady() || (GetTerminal() == NULL))
  {
    return;
  }
  std::unique_ptr<TStrings> FileList(CreateFileList(APanel));
  if (FileList->Count == 0)
  {
    return;
  }

  QMimeData * Data;
  std::unique_ptr<TDragExtractObject> ExtractObject;
  if (APanel == FLocalPanel)
  {
    Data = new QMimeData();
    QList<QUrl> Urls;
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      Urls.append(LocalPathToUrl(FileList->Strings[Index]));
    }
    Data->setUrls(Urls);
  }
  else
  {
    // Other applications get the files downloaded to the directory of the
    // drop, if they tell it, else downloaded to a temporary directory
    TRemoteFilesMimeData * RemoteData = new TRemoteFilesMimeData();
    if (AppConfiguration->DDFakeFile)
    {
      ExtractObject.reset(new TDragExtractObject(this));
      if (ExtractObject->Register())
      {
        RemoteData->SetExtractObject(ExtractObject->GetPath());
        ExtractObject->OnExtract = [this](const QString & Directory)
        {
          FDragExtractDirectory = Directory;
          FDragExtracted = true;
        };
      }
    }
    else
    {
      RemoteData->SetDownload([this]() { return TemporarilyDownloadForOtherApplication(FDragFileList.get()); });
    }
    Data = RemoteData;
  }

  QDrag * Drag = new QDrag(APanel->GetDirView());
  Drag->setMimeData(Data);
  const TDirModel * Model = APanel->GetModel();
  int Row = APanel->GetSelectedRows().front();
  QIcon Icon = Model->data(Model->index(Row, 0), Qt::DecorationRole).value<QIcon>();
  if (!Icon.isNull())
  {
    Drag->setPixmap(Icon.pixmap(32, 32));
  }

  FDragFileList.reset(FileList.release());
  FDragSourcePanel = APanel;
  FDragExtracted = false;
  Qt::DropAction Result;
  {
    TAutoFlag DraggingFlag(FDragging);
    Result = Drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
  }
  FDragSourcePanel = NULL;

  // Dropped to another application (a drop on a panel takes the files)
  if ((ExtractObject != nullptr) && (FDragFileList != nullptr) &&
      ((Result == Qt::CopyAction) || (Result == Qt::MoveAction)))
  {
    if (!FDragExtracted)
    {
      QEventLoop Loop;
      ExtractObject->OnExtract = [this, &Loop](const QString & Directory)
      {
        FDragExtractDirectory = Directory;
        FDragExtracted = true;
        Loop.quit();
      };
      QTimer::singleShot(DDExtTimeout, &Loop, &QEventLoop::quit);
      Loop.exec();
      ExtractObject->OnExtract = nullptr;
    }
    if (FDragExtracted)
    {
      DragExtracted(LocalTargetDirectory(FDragExtractDirectory), (Result == Qt::MoveAction));
    }
  }
  FDragFileList.reset();

  // A drop on a panel waits until the drag ends
  if (FPendingDropOperation)
  {
    std::function<void()> Operation = std::move(FPendingDropOperation);
    FPendingDropOperation = nullptr;
    QTimer::singleShot(0, this, Operation);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DragExtracted(const UnicodeString & TargetDirectory, bool Move)
{
  if (TargetDirectory.IsEmpty())
  {
    MessageDialog(
      ToUnicodeString(tr("The files can be dropped to local directories only (%1).").arg(FDragExtractDirectory)),
      qtError, qaOK);
  }
  else
  {
    std::shared_ptr<TStrings> FileList(FDragFileList.release());
    QTimer::singleShot(0, this, [this, FileList, TargetDirectory, Move]()
    {
      if (!IsBusy() && FRemotePanel->IsReady())
      {
        Configuration->Usage->Inc(L"DownloadsDragDropExternalExt");
        TTransferOperationParam Param;
        Param.TargetDirectory = TargetDirectory;
        Param.DragDrop = true;
        ExecuteCopyMoveFileOperation(Move ? foMove : foCopy, osRemote, FileList.get(), Param);
      }
    });
  }
}
//---------------------------------------------------------------------------
QList<QUrl> TScpCommanderForm::TemporarilyDownloadForOtherApplication(TStrings * FileList)
{
  QList<QUrl> Result;
  TTerminal * Terminal = GetTerminal();
  if (IsBusy() || (Terminal == NULL) || !Terminal->Active || (FileList == NULL))
  {
    return Result;
  }
  // Called from the event loop of Qt, which does not take exceptions
  try
  {
    UnicodeString TempDir = AppConfiguration->TemporaryDir();
    if (!ForceDirectories(ApiPath(TempDir)))
    {
      throw EOSExtException(FMTLOAD(CREATE_TEMP_DIR_ERROR, (TempDir)));
    }
    // The other application copies the files from it meanwhile
    AddDelayedDirectoryDeletion(TempDir, AppConfiguration->DDDeleteDelay);

    {
      ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
      TAutoNestingCounter BusyCounter(FBusy);
      UpdateActions();
      ::rtl::TFinally Finally([&]() { DestroyProgressForm(); });
      TGUICopyParamType CopyParam = GUIConfiguration->CurrentCopyParam;
      CopyParam.IncludeFileMask.SetRoots(TempDir, FileList);
      Configuration->Usage->Inc(L"DownloadsDragDropExternalTemp");
      Terminal->CopyToLocal(FileList, TempDir, &CopyParam, cpTemporary, NULL);
    }

    // The local names may differ from the remote ones
    TSearchRecOwned SearchRec;
    if (FindFirstUnchecked(TempDir + L"*", faAnyFile, SearchRec) == 0)
    {
      do
      {
        if (SearchRec.IsRealFile())
        {
          Result.append(LocalPathToUrl(TempDir + SearchRec.Name));
        }
      }
      while (FindNextUnchecked(SearchRec) == 0);
    }
  }
  catch (Exception & E)
  {
    TTerminalManager::Instance()->HandleException(&E);
    Result.clear();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::AddDelayedDirectoryDeletion(const UnicodeString & Directory, int SecDelay)
{
  FDelayedDeletionList.push_back(Directory);
  QTimer::singleShot(SecDelay * MSecsPerSec, this, [this, Directory]()
  {
    auto Iterator = std::find(FDelayedDeletionList.begin(), FDelayedDeletionList.end(), Directory);
    if (Iterator != FDelayedDeletionList.end())
    {
      FDelayedDeletionList.erase(Iterator);
      RecursiveDeleteFile(ExcludeTrailingBackslash(Directory));
    }
  });
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoDelayedDeletion()
{
  for (const UnicodeString & Directory : FDelayedDeletionList)
  {
    RecursiveDeleteFile(ExcludeTrailingBackslash(Directory));
  }
  FDelayedDeletionList.clear();
}
//---------------------------------------------------------------------------
static bool IsDirectoryInside(const UnicodeString & Directory, const UnicodeString & Parent)
{
  UnicodeString AParent = UnixExcludeTrailingBackslash(Parent);
  return (Directory == AParent) || StartsStr(UnixIncludeTrailingBackslash(AParent), Directory);
}
//---------------------------------------------------------------------------
// The action of a drop: by the keys pressed in a drag of ours (Shift
// moves, Ctrl copies), else as the compositor proposes (it knows the keys
// of drags of other applications)
static Qt::DropAction ChooseDropAction(const TDropParams & Params, Qt::DropAction Default)
{
  Qt::DropAction Result;
  if (Params.SourcePanel != NULL)
  {
    if (FLAGSET(Params.Modifiers, Qt::ShiftModifier))
    {
      Result = Qt::MoveAction;
    }
    else if (FLAGSET(Params.Modifiers, Qt::ControlModifier))
    {
      Result = Qt::CopyAction;
    }
    else
    {
      Result = Default;
    }
  }
  else
  {
    Result = (Params.ProposedAction == Qt::MoveAction) ? Qt::MoveAction : Qt::CopyAction;
  }
  if (!Params.PossibleActions.testFlag(Result))
  {
    Result =
      Params.PossibleActions.testFlag(Qt::CopyAction) ? Qt::CopyAction :
      Params.PossibleActions.testFlag(Qt::MoveAction) ? Qt::MoveAction : Qt::IgnoreAction;
  }
  return Result;
}
//---------------------------------------------------------------------------

Qt::DropAction TScpCommanderForm::PanelDragOver(TDirPanel * APanel, const TDropParams & Params)
{
  TTerminal * Terminal = GetTerminal();
  if (IsBusy() || (Terminal == NULL) || !Terminal->Active)
  {
    return Qt::IgnoreAction;
  }

  Qt::DropAction Result = Qt::IgnoreAction;
  if (Params.SourcePanel != NULL)
  {
    if ((Params.SourcePanel != FDragSourcePanel) || !FDragFileList)
    {
      // noop
    }
    else if (Params.SourcePanel != APanel)
    {
      // Upload or download
      Result = ChooseDropAction(Params, Qt::CopyAction);
    }
    // Within the remote panel to its directories only
    else if ((APanel == FRemotePanel) && Params.OnDirectory)
    {
      bool Allowed = true;
      for (int Index = 0; Allowed && (Index < FDragFileList->Count); Index++)
      {
        Allowed = !IsDirectoryInside(Params.TargetDirectory, FDragFileList->Strings[Index]);
      }
      if (Allowed)
      {
        // The default effect inside remote panel is move,
        // unless we do not support it, but support copy
        Result = ChooseDropAction(Params, Terminal->IsCapable[fcRemoteMove] ? Qt::MoveAction : Qt::CopyAction);
      }
    }
  }
  // Files of other applications are uploaded; the local panel does not
  // operate on local files
  else if ((APanel == FRemotePanel) && Params.Data->hasUrls())
  {
    Result = ChooseDropAction(Params, Qt::CopyAction);
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * TScpCommanderForm::LocalFileListFromMimeData(const QMimeData * Data)
{
  std::unique_ptr<TStrings> Result;
  if (Data->hasUrls())
  {
    Result.reset(new TStringList());
    for (const QUrl & Url : Data->urls())
    {
      if (!Url.isLocalFile())
      {
        return NULL;
      }
      Result->Add(UrlToLocalPath(Url));
    }
  }
  return ((Result != nullptr) && (Result->Count > 0)) ? Result.release() : NULL;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::PanelDrop(TDirPanel * APanel, const TDropParams & Params, Qt::DropAction Action)
{
  bool Move = (Action == Qt::MoveAction);
  UnicodeString TargetDirectory = Params.TargetDirectory;
  std::shared_ptr<TStrings> FileList;
  TOperationSide Side;
  if (Params.SourcePanel != NULL)
  {
    FileList.reset(FDragFileList.release());
    Side = Params.SourcePanel->GetSide();
  }
  else
  {
    FileList.reset(LocalFileListFromMimeData(Params.Data));
    Side = osLocal;
  }
  if (!FileList)
  {
    return;
  }

  std::function<void()> Operation = [this, APanel, FileList, Side, Move, TargetDirectory]()
  {
    if (IsBusy() || !APanel->IsReady())
    {
      return;
    }
    if ((Side == osRemote) && (APanel == FRemotePanel))
    {
      Configuration->Usage->Inc(Move ? L"RemoteMoveDragDrop" : L"RemoteCopyDragDrop");
      RemoteTransferFiles(FileList.get(), Move, TargetDirectory, (AppConfiguration->DDTransferConfirmation == asOff));
    }
    else
    {
      Configuration->Usage->Inc((Side == osLocal) ? L"UploadsDragDrop" : L"DownloadsDragDropInternal");
      TTransferOperationParam Param;
      Param.TargetDirectory = TargetDirectory;
      Param.DragDrop = true;
      ExecuteCopyMoveFileOperation(Move ? foMove : foCopy, Side, FileList.get(), Param);
    }
  };
  // Not within the drop (the source waits for it to finish), nor within a
  // drag of ours
  if (FDragging)
  {
    FPendingDropOperation = Operation;
  }
  else
  {
    QTimer::singleShot(0, this, Operation);
  }
}
//---------------------------------------------------------------------------
