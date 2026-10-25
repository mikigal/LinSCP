// The remote files offered to other applications.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include "ExportedFiles.h"

#include <QDBusConnection>
#include <QStandardPaths>
#include <QUrl>
//---------------------------------------------------------------------------
// The data of a drag are the files of OpenSCP's panel, nothing to read
static const QString RemoteFilesMimeType = QStringLiteral("application/x-openscp-remote-files");
static const QString UriListMimeType = QStringLiteral("text/uri-list");
static const QString ExtractServiceMimeType = QStringLiteral("application/x-kde-ark-dndextract-service");
static const QString ExtractPathMimeType = QStringLiteral("application/x-kde-ark-dndextract-path");
// Klipper does not keep data with the hint in its history (a clipboard
// manager would read, and download, the files)
static const QString PasswordManagerHintMimeType = QStringLiteral("x-kde-passwordManagerHint");
//---------------------------------------------------------------------------
TRemoteFilesMimeData::TRemoteFilesMimeData() :
  QMimeData(),
  FDownloaded(false)
{
}
//---------------------------------------------------------------------------
void TRemoteFilesMimeData::SetExtractObject(const QString & Path)
{
  FExtractPath = Path;
}
//---------------------------------------------------------------------------
void TRemoteFilesMimeData::SetFakeDirectory(const QString & Directory)
{
  FFakeDirectory = Directory;
}
//---------------------------------------------------------------------------
void TRemoteFilesMimeData::SetDownload(std::function<QList<QUrl>()> OnDownload)
{
  FOnDownload = OnDownload;
}
//---------------------------------------------------------------------------
QStringList TRemoteFilesMimeData::formats() const
{
  QStringList Result;
  Result.append(RemoteFilesMimeType);
  if (!FExtractPath.isEmpty())
  {
    Result.append(ExtractServiceMimeType);
    Result.append(ExtractPathMimeType);
  }
  if (!FFakeDirectory.isEmpty() || FOnDownload)
  {
    Result.append(UriListMimeType);
  }
  Result.append(PasswordManagerHintMimeType);
  return Result;
}
//---------------------------------------------------------------------------
bool TRemoteFilesMimeData::hasFormat(const QString & MimeType) const
{
  // Qt reads the URLs of the data it offers to other applications
  // (hasUrls), to pass the files through the document portal; the files to
  // download are not read before a target asks for them
  if ((MimeType == UriListMimeType) && FOnDownload)
  {
    return false;
  }
  return formats().contains(MimeType);
}
//---------------------------------------------------------------------------
QVariant TRemoteFilesMimeData::retrieveData(const QString & MimeType, QMetaType Type) const
{
  QVariant Result;
  if (MimeType == ExtractServiceMimeType)
  {
    if (!FExtractPath.isEmpty())
    {
      Result = QDBusConnection::sessionBus().baseService().toUtf8();
    }
  }
  else if (MimeType == ExtractPathMimeType)
  {
    if (!FExtractPath.isEmpty())
    {
      Result = FExtractPath.toUtf8();
    }
  }
  else if (MimeType == PasswordManagerHintMimeType)
  {
    Result = QByteArray("secret");
  }
  else if (MimeType == RemoteFilesMimeType)
  {
    Result = QByteArray();
  }
  else if (MimeType == UriListMimeType)
  {
    QList<QUrl> Urls;
    if (!FFakeDirectory.isEmpty())
    {
      Urls.append(QUrl::fromLocalFile(FFakeDirectory));
    }
    else if (FOnDownload)
    {
      // Once for the data, the target may ask several times
      if (!FDownloaded)
      {
        FDownloaded = true;
        FDownloadedUrls = FOnDownload();
      }
      Urls = FDownloadedUrls;
    }
    if (!Urls.isEmpty())
    {
      if (Type.id() == QMetaType::QByteArray)
      {
        QByteArray List;
        for (const QUrl & Url : Urls)
        {
          List.append(Url.toEncoded());
          List.append("\r\n");
        }
        Result = List;
      }
      else
      {
        QList<QVariant> List;
        for (const QUrl & Url : Urls)
        {
          List.append(Url);
        }
        Result = List;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool IsRemoteFilesMimeData(const QMimeData * Data)
{
  return (dynamic_cast<const TRemoteFilesMimeData *>(Data) != nullptr);
}
//---------------------------------------------------------------------------
UnicodeString LocalTargetDirectory(const QString & Directory)
{
  UnicodeString Result;
  QUrl Url = Directory.startsWith(QLatin1Char('/')) ? QUrl::fromLocalFile(Directory) : QUrl(Directory);
  if (Url.isLocalFile())
  {
    Result = UrlToLocalPath(Url);
  }
  // The desktop of Plasma
  else if ((Url.scheme() == QLatin1String("desktop")) && ((Url.path() == QLatin1String("/")) || Url.path().isEmpty()))
  {
    Result = ToUnicodeString(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
  }
  return Result;
}
//---------------------------------------------------------------------------
TDragExtractObject::TDragExtractObject(QObject * Parent) :
  QObject(Parent),
  FRegistered(false)
{
  static int Counter = 0;
  FPath = QStringLiteral("/org/openscp/DndExtract/%1").arg(++Counter);
}
//---------------------------------------------------------------------------
TDragExtractObject::~TDragExtractObject()
{
  if (FRegistered)
  {
    QDBusConnection::sessionBus().unregisterObject(FPath);
  }
}
//---------------------------------------------------------------------------
bool TDragExtractObject::Register()
{
  QDBusConnection Bus = QDBusConnection::sessionBus();
  FRegistered = Bus.isConnected() && Bus.registerObject(FPath, this, QDBusConnection::ExportScriptableSlots);
  return FRegistered;
}
//---------------------------------------------------------------------------
void TDragExtractObject::extractSelectedFilesTo(const QString & Directory)
{
  // The caller waits for the reply, the files are downloaded later
  if (OnExtract)
  {
    OnExtract(Directory);
  }
}
//---------------------------------------------------------------------------
TPasteWatcher::TPasteWatcher(const UnicodeString & FakeDirectory, QObject * Parent) :
  QObject(Parent),
  FFakeDirectory(FakeDirectory)
{
  QDBusConnection::sessionBus().connect(
    QString(), QString(), QStringLiteral("org.kde.KDirNotify"), QStringLiteral("FilesAdded"),
    this, SLOT(FilesAdded(QString)));
}
//---------------------------------------------------------------------------
TPasteWatcher::~TPasteWatcher()
{
  QDBusConnection::sessionBus().disconnect(
    QString(), QString(), QStringLiteral("org.kde.KDirNotify"), QStringLiteral("FilesAdded"),
    this, SLOT(FilesAdded(QString)));
}
//---------------------------------------------------------------------------
void TPasteWatcher::FilesAdded(const QString & Directory)
{
  UnicodeString Path = LocalTargetDirectory(Directory);
  if (!Path.IsEmpty())
  {
    UnicodeString Pasted = UnixIncludeTrailingBackslash(Path) + ExtractFileName(FFakeDirectory);
    if ((Pasted != FFakeDirectory) && DirectoryExists(ApiPath(Pasted)))
    {
      RemoveDir(ApiPath(Pasted));
      if (OnPasted)
      {
        OnPasted(Path);
      }
    }
  }
}
//---------------------------------------------------------------------------
