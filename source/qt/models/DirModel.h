// The models of the file panels (upstream's TDirView and TUnixDirView,
// components/UnixDirView.cpp and packages/filemng/DirView.pas): the files
// of a local or a remote directory, sorted with directories first.
//---------------------------------------------------------------------------
#ifndef DirModelH
#define DirModelH
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <QAbstractTableModel>
#include <QIcon>

#include <functional>
#include <vector>
//---------------------------------------------------------------------------
class TRemoteFile;
class TRemoteFileList;
class TTerminal;
//---------------------------------------------------------------------------
enum TDirViewColumn { dvName, dvSize, dvType, dvChanged, dvRights, dvOwner, dvGroup, dvLinkTarget };
//---------------------------------------------------------------------------
struct TDirViewItem
{
  // The name as the core has it (QString does not keep names that are not
  // valid UTF-8)
  UnicodeString FileName;
  bool IsParentDirectory = false;
  bool IsDirectory = false;
  bool IsSymLink = false;
  __int64 Size = 0;
  TDateTime Modification;
  QString ModificationText;
  QString RightsText;
  QString OwnerText;
  QString GroupText;
  QString TypeText;
  QString LinkTarget;
  QIcon Icon;
  // The remote file (owned by the model)
  TRemoteFile * File = nullptr;
};
//---------------------------------------------------------------------------
class TDirModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  TDirModel(const std::vector<TDirViewColumn> & Columns, QObject * Parent);

  virtual int rowCount(const QModelIndex & Parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex & Parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex & Index, int Role = Qt::DisplayRole) const override;
  virtual QVariant headerData(int Section, Qt::Orientation Orientation, int Role = Qt::DisplayRole) const override;
  virtual void sort(int Column, Qt::SortOrder Order = Qt::AscendingOrder) override;
  virtual Qt::ItemFlags flags(const QModelIndex & Index) const override;
  virtual bool setData(const QModelIndex & Index, const QVariant & Value, int Role = Qt::EditRole) override;

  // The name of the row can be edited (renamed), until the edit ends
  void SetEditableRow(int Row) { FEditableRow = Row; }
  // A new name was entered (the model does not rename the file)
  std::function<void(const UnicodeString & FileName, const UnicodeString & NewName)> OnRename;

  const TDirViewItem & Item(int Row) const { return FItems[Row]; }
  int Count() const { return static_cast<int>(FItems.size()); }
  // The row of the file, or -1
  int FindFile(const UnicodeString & FileName) const;
  // The size and count of the files, without the parent directory
  void Totals(__int64 & Size, int & Count) const;
  TDirViewColumn Column(int Index) const { return FColumns[Index]; }

protected:
  void SetItems(std::vector<TDirViewItem> && Items);
  virtual void ClearItems();

private:
  std::vector<TDirViewColumn> FColumns;
  std::vector<TDirViewItem> FItems;
  int FSortColumn;
  Qt::SortOrder FSortOrder;
  int FEditableRow;

  void SortItems();
};
//---------------------------------------------------------------------------
class TLocalDirModel : public TDirModel
{
  Q_OBJECT

public:
  explicit TLocalDirModel(QObject * Parent);

  // Lists the directory; throws when it cannot be read
  void Load(const UnicodeString & Path, bool ShowHiddenFiles);
  const UnicodeString & GetPath() const { return FPath; }

private:
  UnicodeString FPath;
};
//---------------------------------------------------------------------------
class TRemoteDirModel : public TDirModel
{
  Q_OBJECT

public:
  explicit TRemoteDirModel(QObject * Parent);
  virtual ~TRemoteDirModel();

  // A copy of the files of the terminal's current directory
  void Load(TRemoteFileList * Files, bool ShowHiddenFiles);
  void Clear();

protected:
  virtual void ClearItems() override;

private:
  TRemoteFileList * FFiles;
};
//---------------------------------------------------------------------------
#endif
