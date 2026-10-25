// The remote files offered to other applications, by drag and drop and by
// the clipboard (upstream's fake file and temporary directory modes,
// forms/CustomScpExplorer.cpp). A file manager either tells the local
// directory the files are to be downloaded to (DDFakeFile: Ark's drop
// protocol and KIO's notifications of KDE), or asks for local files, which
// are downloaded to a temporary directory then.
//---------------------------------------------------------------------------
#ifndef ExportedFilesH
#define ExportedFilesH
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <QMimeData>
#include <QObject>

#include <functional>
//---------------------------------------------------------------------------
class TRemoteFilesMimeData : public QMimeData
{
public:
  TRemoteFilesMimeData();

  // The directory is told by a call of Ark's drop protocol to the object
  // (TDragExtractObject)
  void SetExtractObject(const QString & Path);
  // A fake directory, whose copy is found where it was pasted
  // (TPasteWatcher)
  void SetFakeDirectory(const QString & Directory);
  // The files are downloaded once asked for, OnDownload returns their URLs
  void SetDownload(std::function<QList<QUrl>()> OnDownload);

  virtual QStringList formats() const override;
  virtual bool hasFormat(const QString & MimeType) const override;

protected:
  virtual QVariant retrieveData(const QString & MimeType, QMetaType Type) const override;

private:
  QString FExtractPath;
  QString FFakeDirectory;
  std::function<QList<QUrl>()> FOnDownload;
  mutable bool FDownloaded;
  mutable QList<QUrl> FDownloadedUrls;
};
//---------------------------------------------------------------------------
// The data are remote files of LinSCP (of this process)
bool IsRemoteFilesMimeData(const QMimeData * Data);
// The local directory of a directory of a file manager (a path or a URL),
// or an empty string when it is not local
UnicodeString LocalTargetDirectory(const QString & Directory);
//---------------------------------------------------------------------------
// Ark's drop protocol (org.kde.ark.DndExtract), which Dolphin and the other
// KIO applications speak: the target of the drop calls the object of the
// drag with the directory the files were dropped to
class TDragExtractObject : public QObject
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.ark.DndExtract")

public:
  explicit TDragExtractObject(QObject * Parent);
  virtual ~TDragExtractObject();

  // On the session bus; false when there is none
  bool Register();
  const QString & GetPath() const { return FPath; }

  std::function<void(const QString & Directory)> OnExtract;

public Q_SLOTS:
  Q_SCRIPTABLE void extractSelectedFilesTo(const QString & Directory);

private:
  QString FPath;
  bool FRegistered;
};
//---------------------------------------------------------------------------
// Finds where a fake directory was pasted: KIO announces the directories it
// adds files to (org.kde.KDirNotify.FilesAdded) to other applications
class TPasteWatcher : public QObject
{
  Q_OBJECT

public:
  TPasteWatcher(const UnicodeString & FakeDirectory, QObject * Parent);
  virtual ~TPasteWatcher();

  // The copy of the fake directory was found (and removed) in the
  // directory
  std::function<void(const UnicodeString & Directory)> OnPasted;

private Q_SLOTS:
  void FilesAdded(const QString & Directory);

private:
  UnicodeString FFakeDirectory;
};
//---------------------------------------------------------------------------
#endif
