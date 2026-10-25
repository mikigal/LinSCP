// wlh-probe: a minimal Qt window used to verify the test harness and to act as
// a drag & drop / clipboard peer in end-to-end tests. Every observed event is
// written as one JSON object per line to the log (stdout by default).
//
// Options:
//   --title TITLE        window title (default "wlh-probe")
//   --log FILE           append events to FILE instead of stdout
//   --drag-source FILE   offer FILE (repeatable) as a drag source: pressing the
//                        mouse and moving it starts a drag with the files' URIs
//
// Typing "v" in the window reads the clipboard and logs its content ("paste").

#include <QApplication>
#include <QClipboard>
#include <QCommandLineParser>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QUrl>
#include <QWidget>

namespace
{

class TEventLog
{
public:
  explicit TEventLog(const QString & FileName)
  {
    bool Opened;
    if (FileName.isEmpty())
    {
      Opened = FFile.open(stdout, QIODevice::WriteOnly | QIODevice::Unbuffered);
    }
    else
    {
      FFile.setFileName(FileName);
      Opened = FFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
    }
    if (!Opened)
    {
      qFatal("cannot open event log: %s", qPrintable(FFile.errorString()));
    }
  }

  void Write(const QString & Event, QJsonObject Data = QJsonObject())
  {
    Data.insert(QStringLiteral("event"), Event);
    FFile.write(QJsonDocument(Data).toJson(QJsonDocument::Compact));
    FFile.write("\n");
    FFile.flush();
  }

private:
  QFile FFile;
};

QJsonArray FormatsToJson(const QMimeData * Data)
{
  QJsonArray Result;
  for (const QString & Format : Data->formats())
  {
    Result.append(Format);
  }
  return Result;
}

QJsonArray UrlsToJson(const QMimeData * Data)
{
  QJsonArray Result;
  for (const QUrl & Url : Data->urls())
  {
    Result.append(Url.toString());
  }
  return Result;
}

QString ButtonName(Qt::MouseButton Button)
{
  switch (Button)
  {
    case Qt::LeftButton: return QStringLiteral("left");
    case Qt::RightButton: return QStringLiteral("right");
    case Qt::MiddleButton: return QStringLiteral("middle");
    default: return QString::number(static_cast<int>(Button));
  }
}

class TProbeWindow : public QWidget
{
public:
  TProbeWindow(TEventLog & Log, const QStringList & DragFiles) :
    FLog(Log),
    FDragFiles(DragFiles)
  {
    setObjectName(QStringLiteral("ProbeWindow"));
    setAcceptDrops(true);
    setMouseTracking(false);
    resize(480, 320);
  }

protected:
  void showEvent(QShowEvent * Event) override
  {
    QWidget::showEvent(Event);
    if (!FShown)
    {
      FShown = true;
      FLog.Write(QStringLiteral("shown"), { { QStringLiteral("width"), width() }, { QStringLiteral("height"), height() } });
    }
  }

  void paintEvent(QPaintEvent *) override
  {
    QPainter Painter(this);
    // A distinctive, flat colour that screenshot checks can look for
    Painter.fillRect(rect(), QColor(0x20, 0xA0, 0x60));
    Painter.setPen(Qt::white);
    QString Text = FDragFiles.isEmpty() ? windowTitle() : windowTitle() + QStringLiteral(" (drag source)");
    Painter.drawText(rect(), Qt::AlignCenter, Text);
  }

  void mousePressEvent(QMouseEvent * Event) override
  {
    FPressPosition = Event->position().toPoint();
    FLog.Write(QStringLiteral("mouse_press"), MouseData(Event));
  }

  void mouseReleaseEvent(QMouseEvent * Event) override
  {
    FLog.Write(QStringLiteral("mouse_release"), MouseData(Event));
  }

  void mouseMoveEvent(QMouseEvent * Event) override
  {
    if (FDragFiles.isEmpty() || !(Event->buttons() & Qt::LeftButton) ||
        ((Event->position().toPoint() - FPressPosition).manhattanLength() < QApplication::startDragDistance()))
    {
      return;
    }
    auto * Data = new QMimeData();
    QList<QUrl> Urls;
    for (const QString & FileName : FDragFiles)
    {
      Urls.append(QUrl::fromLocalFile(FileName));
    }
    Data->setUrls(Urls);
    auto * Drag = new QDrag(this);
    Drag->setMimeData(Data);
    FLog.Write(QStringLiteral("drag_start"), { { QStringLiteral("urls"), UrlsToJson(Data) } });
    Qt::DropAction Result = Drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
    FLog.Write(QStringLiteral("drag_finished"), { { QStringLiteral("action"), static_cast<int>(Result) } });
  }

  void keyPressEvent(QKeyEvent * Event) override
  {
    FLog.Write(QStringLiteral("key_press"), {
      { QStringLiteral("key"), Event->key() },
      { QStringLiteral("text"), Event->text() },
      { QStringLiteral("modifiers"), static_cast<int>(Event->modifiers()) } });
    // "v" pastes: the content of the clipboard is read and logged
    if (Event->text() == QLatin1String("v"))
    {
      const QMimeData * Data = QApplication::clipboard()->mimeData();
      FLog.Write(QStringLiteral("paste"), {
        { QStringLiteral("formats"), FormatsToJson(Data) },
        { QStringLiteral("urls"), UrlsToJson(Data) },
        { QStringLiteral("text"), (Data != nullptr) ? Data->text() : QString() } });
    }
  }

  void dragEnterEvent(QDragEnterEvent * Event) override
  {
    FLog.Write(QStringLiteral("drag_enter"), { { QStringLiteral("formats"), FormatsToJson(Event->mimeData()) } });
    Event->acceptProposedAction();
  }

  void dropEvent(QDropEvent * Event) override
  {
    const QMimeData * Data = Event->mimeData();
    FLog.Write(QStringLiteral("drop"), {
      { QStringLiteral("formats"), FormatsToJson(Data) },
      { QStringLiteral("urls"), UrlsToJson(Data) },
      // Raw list, to tell apart a failed portal file transfer from a missing payload
      { QStringLiteral("uri_list"), QString::fromUtf8(Data->data(QStringLiteral("text/uri-list"))) },
      { QStringLiteral("x"), Event->position().x() },
      { QStringLiteral("y"), Event->position().y() } });
    Event->acceptProposedAction();
  }

private:
  QJsonObject MouseData(const QMouseEvent * Event) const
  {
    return {
      { QStringLiteral("x"), Event->position().x() },
      { QStringLiteral("y"), Event->position().y() },
      { QStringLiteral("button"), ButtonName(Event->button()) } };
  }

  TEventLog & FLog;
  QStringList FDragFiles;
  QPoint FPressPosition;
  bool FShown = false;
};

} // namespace

int main(int Argc, char ** Argv)
{
  QApplication Application(Argc, Argv);
  Application.setApplicationName(QStringLiteral("wlh-probe"));
  Application.setDesktopFileName(QStringLiteral("wlh-probe"));

  QCommandLineParser Parser;
  Parser.addHelpOption();
  QCommandLineOption TitleOption(QStringLiteral("title"), QStringLiteral("Window title."), QStringLiteral("title"), QStringLiteral("wlh-probe"));
  QCommandLineOption LogOption(QStringLiteral("log"), QStringLiteral("Event log file."), QStringLiteral("file"));
  QCommandLineOption DragSourceOption(QStringLiteral("drag-source"), QStringLiteral("File to offer as a drag source."), QStringLiteral("file"));
  Parser.addOption(TitleOption);
  Parser.addOption(LogOption);
  Parser.addOption(DragSourceOption);
  Parser.process(Application);

  TEventLog Log(Parser.value(LogOption));
  TProbeWindow Window(Log, Parser.values(DragSourceOption));
  Window.setWindowTitle(Parser.value(TitleOption));

  QObject::connect(QApplication::clipboard(), &QClipboard::dataChanged, [&Log]()
  {
    Log.Write(QStringLiteral("clipboard_changed"), { { QStringLiteral("formats"), FormatsToJson(QApplication::clipboard()->mimeData()) } });
  });

  Window.show();
  return Application.exec();
}
