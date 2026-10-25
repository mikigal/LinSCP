// The directory trees of the file panels.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <RemoteFiles.h>
#include <Terminal.h>

#include "DirTree.h"

#include <QApplication>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QStyle>

#include <map>
#include <set>
//---------------------------------------------------------------------------
TLocalDirTree::TLocalDirTree(QWidget * Parent) :
  QTreeView(Parent),
  FSettingPath(false)
{
  FModel = new QFileSystemModel(this);
  FModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives);
  FModel->setRootPath(QStringLiteral("/"));
  setModel(FModel);
  setRootIndex(FModel->index(QStringLiteral("/")).parent());
  setHeaderHidden(true);
  for (int Column = 1; Column < FModel->columnCount(); Column++)
  {
    setColumnHidden(Column, true);
  }
  setUniformRowHeights(true);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex & Index)
  {
    if (!FSettingPath && Index.isValid())
    {
      Q_EMIT DirectorySelected(ToUnicodeString(FModel->filePath(Index)));
    }
  });
}
//---------------------------------------------------------------------------
void TLocalDirTree::SetPath(const UnicodeString & Path)
{
  QModelIndex Index = FModel->index(ToQString(Path));
  if (Index.isValid() && (Index != currentIndex()))
  {
    TAutoFlag SettingPathFlag(FSettingPath);
    setCurrentIndex(Index);
    scrollTo(Index);
  }
}
//---------------------------------------------------------------------------
void TLocalDirTree::SetShowHiddenFiles(bool Value)
{
  QDir::Filters Filter = QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives;
  if (Value)
  {
    Filter |= QDir::Hidden;
  }
  if (FModel->filter() != Filter)
  {
    FModel->setFilter(Filter);
  }
}
//---------------------------------------------------------------------------
TRemoteDirTree::TRemoteDirTree(QWidget * Parent) :
  QTreeWidget(Parent),
  FTerminal(NULL),
  FShowHiddenFiles(false),
  FShowInaccesibleDirectories(true),
  FIgnoreChange(false)
{
  setHeaderHidden(true);
  setColumnCount(1);
  setUniformRowHeights(true);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  connect(this, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem * Current)
  {
    if (!FIgnoreChange && (Current != nullptr))
    {
      Q_EMIT DirectorySelected(NodePath(Current));
    }
  });
  connect(this, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem * Node)
  {
    Node->setIcon(0, QIcon::fromTheme(QStringLiteral("folder-open"), style()->standardIcon(QStyle::SP_DirOpenIcon)));
  });
  connect(this, &QTreeWidget::itemCollapsed, this, [this](QTreeWidgetItem * Node)
  {
    Node->setIcon(0, QIcon::fromTheme(QStringLiteral("folder"), style()->standardIcon(QStyle::SP_DirClosedIcon)));
  });
}
//---------------------------------------------------------------------------
TRemoteDirTree::~TRemoteDirTree()
{
  Clear();
}
//---------------------------------------------------------------------------
void TRemoteDirTree::Clear()
{
  for (auto & FileList : FFileLists)
  {
    delete FileList.second;
  }
  FFileLists.clear();
  TAutoFlag IgnoreChangeFlag(FIgnoreChange);
  clear();
}
//---------------------------------------------------------------------------
void TRemoteDirTree::SetTerminal(TTerminal * Terminal)
{
  if (FTerminal != Terminal)
  {
    Clear();
    FTerminal = Terminal;
  }
}
//---------------------------------------------------------------------------
UnicodeString TRemoteDirTree::NodePath(QTreeWidgetItem * Node)
{
  return ToUnicodeString(Node->data(0, Qt::UserRole).toString());
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TRemoteDirTree::FindNodeToPath(const UnicodeString & Path)
{
  UnicodeString APath = UnixExcludeTrailingBackslash(Path);
  if (APath.IsEmpty())
  {
    APath = ROOTDIRECTORY;
  }
  QString QPath = ToQString(APath);
  for (QTreeWidgetItemIterator It(this); *It != nullptr; ++It)
  {
    if ((*It)->data(0, Qt::UserRole).toString() == QPath)
    {
      return *It;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
void TRemoteDirTree::DeleteNode(QTreeWidgetItem * Node)
{
  // With the listings of the nodes under it
  while (Node->childCount() > 0)
  {
    DeleteNode(Node->child(0));
  }
  auto I = FFileLists.find(Node);
  if (I != FFileLists.end())
  {
    delete I->second;
    FFileLists.erase(I);
  }
  delete Node;
}
//---------------------------------------------------------------------------
void TRemoteDirTree::UpdatePath(QTreeWidgetItem * Node, bool Force)
{
  // upstream's TCustomUnixDriveView::UpdatePath: the subdirectories of the
  // directory from its newer listing (of the cache)
  UnicodeString Path = NodePath(Node);
  auto I = FFileLists.find(Node);
  TRemoteFileList * OldFileList = (I != FFileLists.end()) ? I->second : NULL;
  TDateTime Timestamp = (OldFileList != NULL) ? OldFileList->Timestamp : TDateTime();
  TRemoteFileList * FileList = FTerminal->DirectoryFileList(Path, Timestamp, false);
  if ((FileList != NULL) || ((OldFileList != NULL) && Force))
  {
    if (FileList != NULL)
    {
      FFileLists[Node] = FileList;
    }
    else
    {
      FileList = OldFileList;
    }

    std::map<UnicodeString, QTreeWidgetItem *> Children;
    for (int Index = 0; Index < Node->childCount(); Index++)
    {
      QTreeWidgetItem * Child = Node->child(Index);
      Children[UnixExtractFileName(NodePath(Child))] = Child;
    }

    std::set<UnicodeString> Found;
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      TRemoteFile * File = FileList->Files[Index];
      if (File->IsDirectory && IsRealFile(File->FileName) &&
          (FShowHiddenFiles || !File->IsHidden) &&
          (FShowInaccesibleDirectories || !File->IsInaccesibleDirectory))
      {
        Found.insert(File->FileName);
        auto Child = Children.find(File->FileName);
        if (Child != Children.end())
        {
          UpdatePath(Child->second, Force);
        }
        else
        {
          LoadPathEasy(Node, UnixIncludeTrailingBackslash(Path) + File->FileName);
        }
      }
    }

    for (auto & Child : Children)
    {
      if (Found.count(Child.first) == 0)
      {
        DeleteNode(Child.second);
      }
    }

    Node->sortChildren(0, Qt::AscendingOrder);

    // Released only now, once they are no longer referenced
    if ((OldFileList != NULL) && (OldFileList != FileList))
    {
      delete OldFileList;
    }
  }
  else if (Force)
  {
    for (int Index = 0; Index < Node->childCount(); Index++)
    {
      UpdatePath(Node->child(Index), true);
    }
  }
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TRemoteDirTree::LoadPathEasy(QTreeWidgetItem * Parent, const UnicodeString & Path)
{
  QTreeWidgetItem * Node = (Parent != nullptr) ? new QTreeWidgetItem(Parent) : new QTreeWidgetItem(this);
  Node->setText(0, IsUnixRootPath(Path) ? QStringLiteral("/") : ToQString(UnixExtractFileName(Path)));
  Node->setData(0, Qt::UserRole, ToQString(Path));
  Node->setIcon(0, QIcon::fromTheme(QStringLiteral("folder"), style()->standardIcon(QStyle::SP_DirClosedIcon)));
  UpdatePath(Node, true);
  return Node;
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TRemoteDirTree::LoadPath(const UnicodeString & APath)
{
  UnicodeString Path = UnixExcludeTrailingBackslash(APath);
  if (Path.IsEmpty())
  {
    Path = ROOTDIRECTORY;
  }

  QTreeWidgetItem * Node = FindNodeToPath(Path);
  if (Node == nullptr)
  {
    QTreeWidgetItem * Parent = nullptr;
    if (!IsUnixRootPath(Path))
    {
      Parent = LoadPath(UnixExtractFileDir(Path));
    }

    // When the parent directory is not in the cache, its node has not the
    // directory yet
    Node = FindNodeToPath(Path);
    if (Node == nullptr)
    {
      Node = LoadPathEasy(Parent, Path);
      if (Parent != nullptr)
      {
        Parent->sortChildren(0, Qt::AscendingOrder);
      }
    }
    else
    {
      UpdatePath(Node, false);
    }
  }
  else
  {
    UpdatePath(Node, false);
  }

  return Node;
}
//---------------------------------------------------------------------------
void TRemoteDirTree::LoadDirectory()
{
  if ((FTerminal != NULL) && FTerminal->Active)
  {
    TAutoFlag IgnoreChangeFlag(FIgnoreChange);
    QTreeWidgetItem * Node = LoadPath(FTerminal->CurrentDirectory);
    // The directories above are shown
    for (QTreeWidgetItem * Parent = Node->parent(); Parent != nullptr; Parent = Parent->parent())
    {
      Parent->setExpanded(true);
    }
    setCurrentItem(Node);
    scrollToItem(Node);
  }
  else
  {
    Clear();
  }
}
//---------------------------------------------------------------------------
void TRemoteDirTree::Rebuild()
{
  Clear();
  LoadDirectory();
}
//---------------------------------------------------------------------------
void TRemoteDirTree::SetShowHiddenFiles(bool Value)
{
  if (FShowHiddenFiles != Value)
  {
    FShowHiddenFiles = Value;
    Rebuild();
  }
}
//---------------------------------------------------------------------------
void TRemoteDirTree::SetShowInaccesibleDirectories(bool Value)
{
  if (FShowInaccesibleDirectories != Value)
  {
    FShowInaccesibleDirectories = Value;
    Rebuild();
  }
}
//---------------------------------------------------------------------------
