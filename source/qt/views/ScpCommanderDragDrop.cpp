// Drag and drop of the Commander (upstream's drag and drop handlers of
// TCustomScpExplorerForm, forms/CustomScpExplorer.cpp): files dragged
// between the panels, from other applications to the remote panel and from
// the panels to other applications.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <TerminalManager.h>

#include "DirModel.h"
#include "DirPanel.h"
#include "ScpCommander.h"

#include <QDrag>
#include <QMimeData>
#include <QTimer>
#include <QUrl>
//---------------------------------------------------------------------------
// A drag from the remote panel carries no data, the panel has the files
static const QString RemoteFilesMimeType = QStringLiteral("application/x-openscp-remote-files");
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

  QMimeData * Data = new QMimeData();
  if (APanel == FLocalPanel)
  {
    QList<QUrl> Urls;
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      Urls.append(LocalPathToUrl(FileList->Strings[Index]));
    }
    Data->setUrls(Urls);
  }
  else
  {
    Data->setData(RemoteFilesMimeType, QByteArray());
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
  {
    TAutoFlag DraggingFlag(FDragging);
    Drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
  }
  FDragSourcePanel = NULL;
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
