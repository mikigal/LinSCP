// The models of the file panels.
//---------------------------------------------------------------------------
#include "DirModel.h"

#include <RemoteFiles.h>
#include <Terminal.h>

#include <QApplication>
#include <QHash>
#include <QMimeDatabase>
#include <QStyle>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
//---------------------------------------------------------------------------
static QString ColumnCaption(TDirViewColumn Column)
{
  switch (Column)
  {
    case dvName: return QObject::tr("Name");
    case dvSize: return QObject::tr("Size");
    case dvType: return QObject::tr("Type");
    case dvChanged: return QObject::tr("Changed");
    case dvRights: return QObject::tr("Rights");
    case dvOwner: return QObject::tr("Owner");
    case dvGroup: return QObject::tr("Group");
    case dvLinkTarget: return QObject::tr("Link target");
  }
  return QString();
}
//---------------------------------------------------------------------------
void FileTypeAndIcon(const QString & FileName, bool Directory, QString & Type, QIcon & Icon)
{
  static QMimeDatabase MimeDatabase;
  static QHash<QString, QIcon> Icons;
  QMimeType MimeType =
    Directory ?
      MimeDatabase.mimeTypeForName(QStringLiteral("inode/directory")) :
      MimeDatabase.mimeTypeForFile(FileName, QMimeDatabase::MatchExtension);
  Type = MimeType.comment();
  QHash<QString, QIcon>::const_iterator I = Icons.constFind(MimeType.name());
  if (I != Icons.constEnd())
  {
    Icon = I.value();
  }
  else
  {
    QStyle * Style = QApplication::style();
    QIcon Fallback = Style->standardIcon(Directory ? QStyle::SP_DirIcon : QStyle::SP_FileIcon);
    Icon = QIcon::fromTheme(MimeType.iconName(), QIcon::fromTheme(MimeType.genericIconName(), Fallback));
    Icons.insert(MimeType.name(), Icon);
  }
}
//---------------------------------------------------------------------------
static TDirViewItem ParentDirectoryItem()
{
  TDirViewItem Item;
  Item.FileName = PARENTDIRECTORY;
  Item.IsParentDirectory = true;
  Item.IsDirectory = true;
  Item.Icon = QIcon::fromTheme(QStringLiteral("go-up"), QApplication::style()->standardIcon(QStyle::SP_FileDialogToParent));
  return Item;
}
//---------------------------------------------------------------------------
TDirModel::TDirModel(const std::vector<TDirViewColumn> & Columns, QObject * Parent) :
  QAbstractTableModel(Parent),
  FColumns(Columns),
  FSortColumn(0),
  FSortOrder(Qt::AscendingOrder),
  FEditableRow(-1),
  FFormatSizeBytes(fbKilobytes),
  FNaturalOrderNumericalSorting(true),
  FAlwaysSortDirectoriesByName(false),
  FHiddenCount(0),
  FFilteredCount(0)
{
}
//---------------------------------------------------------------------------
Qt::ItemFlags TDirModel::flags(const QModelIndex & Index) const
{
  Qt::ItemFlags Result = QAbstractTableModel::flags(Index);
  if (Index.isValid() && (Index.row() == FEditableRow) && (FColumns[Index.column()] == dvName))
  {
    Result |= Qt::ItemIsEditable;
  }
  // The files can be dragged (the view drags the selected files), the
  // parent directory cannot
  if (Index.isValid() && (Index.row() < Count()) && !FItems[Index.row()].IsParentDirectory)
  {
    Result |= Qt::ItemIsDragEnabled;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TDirModel::setData(const QModelIndex & Index, const QVariant & Value, int Role)
{
  if (Index.isValid() && (Role == Qt::EditRole) && (Index.row() == FEditableRow) && (Index.row() < Count()))
  {
    FEditableRow = -1;
    UnicodeString FileName = FItems[Index.row()].FileName;
    UnicodeString NewName = ToUnicodeString(Value.toString());
    if (!NewName.IsEmpty() && (NewName != FileName) && OnRename)
    {
      OnRename(FileName, NewName);
    }
  }
  // The name changes when the file is renamed and its directory reloaded
  return false;
}
//---------------------------------------------------------------------------
int TDirModel::rowCount(const QModelIndex & Parent) const
{
  return Parent.isValid() ? 0 : Count();
}
//---------------------------------------------------------------------------
int TDirModel::columnCount(const QModelIndex & Parent) const
{
  return Parent.isValid() ? 0 : static_cast<int>(FColumns.size());
}
//---------------------------------------------------------------------------
QVariant TDirModel::data(const QModelIndex & Index, int Role) const
{
  QVariant Result;
  if (Index.isValid() && (Index.row() < Count()))
  {
    const TDirViewItem & Item = FItems[Index.row()];
    TDirViewColumn Column = FColumns[Index.column()];
    if ((Role == Qt::EditRole) && (Column == dvName))
    {
      Result = ToQString(Item.FileName);
    }
    else if (Role == Qt::DisplayRole)
    {
      switch (Column)
      {
        case dvName:
          Result = ToQString(Item.FileName);
          break;
        case dvSize:
          if (!Item.IsDirectory)
          {
            Result = ToQString(FormatPanelBytes(Item.Size, FFormatSizeBytes));
          }
          break;
        case dvType:
          Result = Item.TypeText;
          break;
        case dvChanged:
          Result = Item.ModificationText;
          break;
        case dvRights:
          Result = Item.RightsText;
          break;
        case dvOwner:
          Result = Item.OwnerText;
          break;
        case dvGroup:
          Result = Item.GroupText;
          break;
        case dvLinkTarget:
          Result = Item.LinkTarget;
          break;
      }
    }
    else if ((Role == Qt::DecorationRole) && (Column == dvName))
    {
      Result = Item.Icon;
    }
    else if ((Role == Qt::ForegroundRole) && Item.Color.isValid())
    {
      Result = QBrush(Item.Color);
    }
    else if ((Role == Qt::TextAlignmentRole) && (Column == dvSize))
    {
      Result = QVariant::fromValue(Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
QVariant TDirModel::headerData(int Section, Qt::Orientation Orientation, int Role) const
{
  QVariant Result;
  if ((Orientation == Qt::Horizontal) && (Section >= 0) && (Section < static_cast<int>(FColumns.size())))
  {
    if (Role == Qt::DisplayRole)
    {
      Result = ColumnCaption(FColumns[Section]);
    }
    else if ((Role == Qt::TextAlignmentRole) && (FColumns[Section] == dvSize))
    {
      Result = QVariant::fromValue(Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TDirModel::SortItems()
{
  TDirViewColumn Column = FColumns[FSortColumn];
  bool Ascending = (FSortOrder == Qt::AscendingOrder);
  bool NaturalOrderNumericalSorting = FNaturalOrderNumericalSorting;
  bool AlwaysSortDirectoriesByName = FAlwaysSortDirectoriesByName;
  std::stable_sort(FItems.begin(), FItems.end(),
    [Column, Ascending, NaturalOrderNumericalSorting, AlwaysSortDirectoriesByName](const TDirViewItem & Item1, const TDirViewItem & Item2)
  {
    // The parent directory first, then the directories, in any order
    if (Item1.IsParentDirectory != Item2.IsParentDirectory)
    {
      return Item1.IsParentDirectory;
    }
    if (Item1.IsDirectory != Item2.IsDirectory)
    {
      return Item1.IsDirectory;
    }
    int Result = 0;
    // The directories by their names, in the order of the column
    TDirViewColumn AColumn = (AlwaysSortDirectoriesByName && Item1.IsDirectory) ? dvName : Column;
    switch (AColumn)
    {
      case dvSize:
        Result = CompareNumber(Item1.Size, Item2.Size);
        break;
      case dvChanged:
        Result = (Item1.Modification < Item2.Modification) ? -1 : ((Item1.Modification > Item2.Modification) ? 1 : 0);
        break;
      case dvType:
        Result = Item1.TypeText.compare(Item2.TypeText, Qt::CaseInsensitive);
        break;
      case dvRights:
        Result = Item1.RightsText.compare(Item2.RightsText);
        break;
      case dvOwner:
        Result = Item1.OwnerText.compare(Item2.OwnerText, Qt::CaseInsensitive);
        break;
      case dvGroup:
        Result = Item1.GroupText.compare(Item2.GroupText, Qt::CaseInsensitive);
        break;
      case dvLinkTarget:
        Result = Item1.LinkTarget.compare(Item2.LinkTarget);
        break;
      case dvName:
        break;
    }
    if (Result == 0)
    {
      Result = CompareLogicalText(Item1.FileName, Item2.FileName, NaturalOrderNumericalSorting);
    }
    return Ascending ? (Result < 0) : (Result > 0);
  });
}
//---------------------------------------------------------------------------
void TDirModel::sort(int Column, Qt::SortOrder Order)
{
  if ((Column >= 0) && (Column < static_cast<int>(FColumns.size())))
  {
    Q_EMIT layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);
    QModelIndexList Persistent = persistentIndexList();
    std::vector<UnicodeString> PersistentNames;
    for (const QModelIndex & Index : Persistent)
    {
      PersistentNames.push_back(FItems[Index.row()].FileName);
    }
    FSortColumn = Column;
    FSortOrder = Order;
    SortItems();
    QModelIndexList Updated;
    for (int Index = 0; Index < Persistent.size(); Index++)
    {
      int Row = FindFile(PersistentNames[Index]);
      Updated.append((Row >= 0) ? index(Row, Persistent[Index].column()) : QModelIndex());
    }
    changePersistentIndexList(Persistent, Updated);
    Q_EMIT layoutChanged({}, QAbstractItemModel::VerticalSortHint);
  }
}
//---------------------------------------------------------------------------
void TDirModel::SetDisplayOptions(
  TFormatBytesStyle FormatSizeBytes, bool NaturalOrderNumericalSorting, bool AlwaysSortDirectoriesByName)
{
  bool Resort =
    (FNaturalOrderNumericalSorting != NaturalOrderNumericalSorting) ||
    (FAlwaysSortDirectoriesByName != AlwaysSortDirectoriesByName);
  bool Redisplay = (FFormatSizeBytes != FormatSizeBytes);
  FFormatSizeBytes = FormatSizeBytes;
  FNaturalOrderNumericalSorting = NaturalOrderNumericalSorting;
  FAlwaysSortDirectoriesByName = AlwaysSortDirectoriesByName;
  if (Resort)
  {
    sort(FSortColumn, FSortOrder);
  }
  if (Redisplay && (Count() > 0))
  {
    Q_EMIT dataChanged(index(0, 0), index(Count() - 1, columnCount() - 1), { Qt::DisplayRole });
  }
}
//---------------------------------------------------------------------------
void TDirModel::SetMask(const UnicodeString & Mask)
{
  FMask = Mask;
}
//---------------------------------------------------------------------------
QColor TDirModel::ItemColor(const TDirViewItem & Item) const
{
  // upstream's DirViewGetItemColor
  QColor Result;
  if (!Item.IsParentDirectory)
  {
    TFileMasks::TParams MaskParams;
    MaskParams.Size = Item.Size;
    MaskParams.Modification = Item.Modification;
    for (const TFileColorData & FileColor : FFileColors)
    {
      bool ImplicitMatch;
      if (FileColor.FileMask.Matches(Item.FileName, IsLocal(), Item.IsDirectory, &MaskParams, false, ImplicitMatch) &&
          !ImplicitMatch)
      {
        Result = TColorToQColor(FileColor.Color);
        break;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TDirModel::SetFileColors(const TFileColorData::TList & FileColors)
{
  FFileColors = FileColors;
  for (TDirViewItem & Item : FItems)
  {
    Item.Color = ItemColor(Item);
  }
  if (Count() > 0)
  {
    Q_EMIT dataChanged(index(0, 0), index(Count() - 1, columnCount() - 1), { Qt::ForegroundRole });
  }
}
//---------------------------------------------------------------------------
void TDirModel::SetItems(std::vector<TDirViewItem> && Items, int HiddenCount)
{
  beginResetModel();
  ClearItems();
  FHiddenCount = HiddenCount;
  FFilteredCount = 0;
  bool Filter = !FMask.Masks.IsEmpty();
  FItems.clear();
  FItems.reserve(Items.size());
  for (TDirViewItem & Item : Items)
  {
    // upstream's DirViewMatchMask with implicit matches (the directories
    // match masks of files)
    if (Filter && !Item.IsParentDirectory)
    {
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Item.Size;
      MaskParams.Modification = Item.Modification;
      bool ImplicitMatch;
      if (!FMask.Matches(Item.FileName, false, Item.IsDirectory, &MaskParams, true, ImplicitMatch))
      {
        FFilteredCount++;
        continue;
      }
    }
    Item.Color = ItemColor(Item);
    FItems.push_back(std::move(Item));
  }
  SortItems();
  endResetModel();
}
//---------------------------------------------------------------------------
void TDirModel::ClearItems()
{
  FItems.clear();
}
//---------------------------------------------------------------------------
int TDirModel::FindFile(const UnicodeString & FileName) const
{
  for (int Index = 0; Index < Count(); Index++)
  {
    if (FItems[Index].FileName == FileName)
    {
      return Index;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
void TDirModel::Totals(__int64 & Size, int & Count) const
{
  Size = 0;
  Count = 0;
  for (const TDirViewItem & Item : FItems)
  {
    if (!Item.IsParentDirectory)
    {
      Size += Item.Size;
      Count++;
    }
  }
}
//---------------------------------------------------------------------------
TLocalDirModel::TLocalDirModel(QObject * Parent) :
  // Upstream's default columns of the local panel, with the rights in
  // place of the Windows attributes
  TDirModel({ dvName, dvSize, dvType, dvChanged, dvRights }, Parent)
{
}
//---------------------------------------------------------------------------
static QString UserName(uid_t Uid)
{
  static QHash<uid_t, QString> Names;
  QHash<uid_t, QString>::const_iterator I = Names.constFind(Uid);
  if (I != Names.constEnd())
  {
    return I.value();
  }
  QString Result;
  struct passwd Passwd;
  struct passwd * Found = NULL;
  char Buffer[4096];
  if ((getpwuid_r(Uid, &Passwd, Buffer, sizeof(Buffer), &Found) == 0) && (Found != NULL))
  {
    Result = QString::fromLocal8Bit(Found->pw_name);
  }
  else
  {
    Result = QString::number(Uid);
  }
  Names.insert(Uid, Result);
  return Result;
}
//---------------------------------------------------------------------------
void TLocalDirModel::Load(const UnicodeString & Path, bool ShowHiddenFiles)
{
  UTF8String DirectoryBytes = UTF8Encode(IncludeTrailingBackslash(Path));
  DIR * Directory = opendir(DirectoryBytes.c_str());
  if (Directory == NULL)
  {
    int Error = errno;
    throw ExtException(FMTLOAD(LIST_DIR_ERROR, (Path)), UnicodeString(strerror(Error)));
  }

  std::vector<TDirViewItem> Items;
  int HiddenCount = 0;
  struct dirent * Entry;
  while ((Entry = readdir(Directory)) != NULL)
  {
    const char * Name = Entry->d_name;
    if (strcmp(Name, ".") == 0)
    {
      continue;
    }
    if (strcmp(Name, "..") == 0)
    {
      if (Path != L"/")
      {
        Items.push_back(ParentDirectoryItem());
      }
      continue;
    }
    if ((Name[0] == '.') && !ShowHiddenFiles)
    {
      HiddenCount++;
      continue;
    }

    std::string FullName = std::string(DirectoryBytes.c_str()) + Name;
    struct stat Stat;
    if (lstat(FullName.c_str(), &Stat) != 0)
    {
      continue;
    }
    TDirViewItem Item;
    Item.FileName = UTF8ToString(Name);
    Item.IsSymLink = S_ISLNK(Stat.st_mode);
    if (Item.IsSymLink)
    {
      char Target[PATH_MAX];
      ssize_t Length = readlink(FullName.c_str(), Target, sizeof(Target) - 1);
      if (Length >= 0)
      {
        Target[Length] = '\0';
        Item.LinkTarget = ToQString(UTF8ToString(Target));
      }
      // The properties of the target, as with the remote files
      struct stat TargetStat;
      if (stat(FullName.c_str(), &TargetStat) == 0)
      {
        Stat = TargetStat;
      }
    }
    Item.IsDirectory = S_ISDIR(Stat.st_mode);
    Item.Size = Item.IsDirectory ? 0 : Stat.st_size;
    Item.Modification = UnixToDateTime(Stat.st_mtime, dstmUnix);
    Item.ModificationText = ToQString(UserModificationStr(Item.Modification, mfFull));
    TRights Rights(static_cast<unsigned short>(Stat.st_mode & 07777));
    Item.RightsText = ToQString(Rights.Text);
    Item.OwnerText = UserName(Stat.st_uid);
    FileTypeAndIcon(ToQString(Item.FileName), Item.IsDirectory, Item.TypeText, Item.Icon);
    Items.push_back(Item);
  }
  closedir(Directory);

  FPath = Path;
  SetItems(std::move(Items), HiddenCount);
}
//---------------------------------------------------------------------------
TRemoteDirModel::TRemoteDirModel(QObject * Parent) :
  // Upstream's default columns of the remote panel
  TDirModel({ dvName, dvSize, dvChanged, dvRights, dvOwner }, Parent)
{
  FFiles = NULL;
}
//---------------------------------------------------------------------------
TRemoteDirModel::~TRemoteDirModel()
{
  delete FFiles;
}
//---------------------------------------------------------------------------
void TRemoteDirModel::ClearItems()
{
  TDirModel::ClearItems();
  delete FFiles;
  FFiles = NULL;
}
//---------------------------------------------------------------------------
void TRemoteDirModel::Clear()
{
  SetItems(std::vector<TDirViewItem>(), 0);
}
//---------------------------------------------------------------------------
void TRemoteDirModel::Load(TRemoteFileList * Files, bool ShowHiddenFiles, bool ShowInaccesibleDirectories)
{
  // The terminal reuses its list, the model has its own copy
  TRemoteFileList * Copy = new TRemoteFileList();
  Files->DuplicateTo(Copy);

  std::vector<TDirViewItem> Items;
  int HiddenCount = 0;
  for (int Index = 0; Index < Copy->Count; Index++)
  {
    TRemoteFile * File = Copy->Files[Index];
    if (File->IsThisDirectory)
    {
      continue;
    }
    if ((!ShowHiddenFiles && File->IsHidden) ||
        (!ShowInaccesibleDirectories && File->IsInaccesibleDirectory))
    {
      HiddenCount++;
      continue;
    }
    TDirViewItem Item;
    Item.File = File;
    Item.FileName = File->FileName;
    Item.IsParentDirectory = File->IsParentDirectory;
    Item.IsDirectory = File->IsDirectory;
    Item.IsSymLink = File->IsSymLink;
    Item.Size = File->Size;
    if (!Item.IsParentDirectory)
    {
      Item.Modification = File->Modification;
      Item.ModificationFmt = File->ModificationFmt;
      Item.ModificationText = ToQString(File->UserModificationStr);
      Item.RightsText = ToQString(File->RightsStr);
      Item.OwnerText = ToQString(File->Owner.DisplayText);
      Item.GroupText = ToQString(File->Group.DisplayText);
      Item.LinkTarget = ToQString(File->LinkTo);
      FileTypeAndIcon(ToQString(Item.FileName), Item.IsDirectory, Item.TypeText, Item.Icon);
    }
    else
    {
      Item.Icon = ParentDirectoryItem().Icon;
    }
    Items.push_back(Item);
  }

  SetItems(std::move(Items), HiddenCount);
  // After SetItems, which frees the previous copy
  FFiles = Copy;
}
//---------------------------------------------------------------------------
