// The directory trees of the file panels (upstream's TDriveView and
// TUnixDriveView): the directories of the local file system, the remote
// directories known to the session (the visited ones and those in its
// directory cache).
//---------------------------------------------------------------------------
#ifndef DirTreeH
#define DirTreeH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <QTreeView>
#include <QTreeWidget>

#include <map>
//---------------------------------------------------------------------------
class QFileSystemModel;
class TRemoteFileList;
class TTerminal;
//---------------------------------------------------------------------------
class TLocalDirTree : public QTreeView
{
  Q_OBJECT

public:
  explicit TLocalDirTree(QWidget * Parent);

  // The directory of the panel is selected
  void SetPath(const UnicodeString & Path);
  void SetShowHiddenFiles(bool Value);

Q_SIGNALS:
  void DirectorySelected(const UnicodeString & Path);

private:
  QFileSystemModel * FModel;
  bool FSettingPath;
};
//---------------------------------------------------------------------------
class TRemoteDirTree : public QTreeWidget
{
  Q_OBJECT

public:
  explicit TRemoteDirTree(QWidget * Parent);
  virtual ~TRemoteDirTree();

  // The session whose directories are shown, or NULL
  void SetTerminal(TTerminal * Terminal);
  // The current directory of the session was read: its path is shown and
  // selected, the directories known are updated
  void LoadDirectory();
  void SetShowHiddenFiles(bool Value);
  void SetShowInaccesibleDirectories(bool Value);

Q_SIGNALS:
  void DirectorySelected(const UnicodeString & Path);

private:
  TTerminal * FTerminal;
  bool FShowHiddenFiles;
  bool FShowInaccesibleDirectories;
  bool FIgnoreChange;
  // The listings of the directories of the nodes (owned)
  std::map<QTreeWidgetItem *, TRemoteFileList *> FFileLists;

  QTreeWidgetItem * LoadPath(const UnicodeString & Path);
  QTreeWidgetItem * LoadPathEasy(QTreeWidgetItem * Parent, const UnicodeString & Path);
  void UpdatePath(QTreeWidgetItem * Node, bool Force);
  QTreeWidgetItem * FindNodeToPath(const UnicodeString & Path);
  static UnicodeString NodePath(QTreeWidgetItem * Node);
  void DeleteNode(QTreeWidgetItem * Node);
  void Clear();
  void Rebuild();
};
//---------------------------------------------------------------------------
#endif
