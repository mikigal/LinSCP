// The automation hook of test builds.
//---------------------------------------------------------------------------
#include "Automation.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QJsonArray>
#include <QKeyEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProgressBar>
#include <QSpinBox>
#include <QTabBar>
#include <QTimer>
#include <QTreeView>

#include <deque>
#include <stdexcept>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// The number of rows of an item view that "get" describes at most
const int MaxRows = 2000;
//---------------------------------------------------------------------------
class EAutomation : public std::runtime_error
{
public:
  explicit EAutomation(const QString & Message) :
    std::runtime_error(Message.toStdString())
  {
  }
};
//---------------------------------------------------------------------------
typedef std::function<void()> TAction;
//---------------------------------------------------------------------------
class TAutomationServer : public QObject
{
public:
  explicit TAutomationServer(const QString & Path);

  std::function<bool()> BusyCheck;

private:
  struct TRequest
  {
    QPointer<QLocalSocket> Socket;
    QByteArray Line;
  };

  QLocalServer * FServer;
  std::deque<TRequest> FRequests;

  void NewConnection();
  void ReadRequests(QLocalSocket * Socket);
  void ProcessRequests();
  void Process(const TRequest & Request);
  QJsonValue Execute(const QString & Command, const QJsonObject & Request, TAction & Action);
  void WaitIdle(QPointer<QLocalSocket> Socket, const QJsonValue & Id, int Rounds);
  static void Reply(QLocalSocket * Socket, const QJsonObject & Reply);
};
//---------------------------------------------------------------------------
TAutomationServer * Server = nullptr;
//---------------------------------------------------------------------------
QString Name(QObject * Object)
{
  return Object->objectName();
}
//---------------------------------------------------------------------------
QWidget * FindWindow(const QString & Name)
{
  QWidget * Result = nullptr;
  QWidget * Active = QApplication::activeWindow();
  if ((Active != nullptr) && (Active->objectName() == Name))
  {
    Result = Active;
  }
  else
  {
    for (QWidget * Widget : QApplication::topLevelWidgets())
    {
      if (Widget->objectName() == Name)
      {
        if (Widget->isVisible() || (Result == nullptr))
        {
          Result = Widget;
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
QObject * FindTarget(const QJsonObject & Request)
{
  QString Path = Request.value(QStringLiteral("target")).toString();
  QStringList Parts = Path.split(QLatin1Char('/'));
  QObject * Result = FindWindow(Parts.first());
  if (Result == nullptr)
  {
    throw EAutomation(QStringLiteral("No window %1").arg(Parts.first()));
  }
  for (int Index = 1; Index < Parts.size(); Index++)
  {
    QObject * Child = Result->findChild<QObject *>(Parts[Index]);
    if (Child == nullptr)
    {
      throw EAutomation(QStringLiteral("No %1 in %2").arg(Parts[Index], Name(Result)));
    }
    Result = Child;
  }
  return Result;
}
//---------------------------------------------------------------------------
template<class T>
T * FindTargetOf(const QJsonObject & Request)
{
  QObject * Object = FindTarget(Request);
  T * Result = qobject_cast<T *>(Object);
  if (Result == nullptr)
  {
    throw EAutomation(
      QStringLiteral("%1 is a %2").arg(Name(Object), QString::fromLatin1(Object->metaObject()->className())));
  }
  return Result;
}
//---------------------------------------------------------------------------
QStringList ItemPath(const QJsonValue & Value)
{
  QStringList Result;
  if (Value.isArray())
  {
    for (const QJsonValue & Part : Value.toArray())
    {
      Result.append(Part.toString());
    }
  }
  else
  {
    Result = Value.toString().split(QLatin1Char('/'));
  }
  return Result;
}
//---------------------------------------------------------------------------
// An item of a view by the texts of the first column of it and of its
// parents
QModelIndex FindItem(QAbstractItemView * View, const QJsonValue & Value)
{
  QAbstractItemModel * Model = View->model();
  QModelIndex Result = View->rootIndex();
  for (const QString & Text : ItemPath(Value))
  {
    QModelIndex Parent = Result;
    Result = QModelIndex();
    for (int Row = 0; !Result.isValid() && (Row < Model->rowCount(Parent)); Row++)
    {
      QModelIndex Index = Model->index(Row, 0, Parent);
      if (Index.data(Qt::DisplayRole).toString() == Text)
      {
        Result = Index;
      }
    }
    if (!Result.isValid())
    {
      throw EAutomation(QStringLiteral("No item %1 in %2").arg(Text, Name(View)));
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
QJsonArray DescribeRows(QAbstractItemView * View, const QModelIndex & Parent, int & Budget)
{
  QAbstractItemModel * Model = View->model();
  QItemSelectionModel * Selection = View->selectionModel();
  QTreeView * Tree = qobject_cast<QTreeView *>(View);
  QJsonArray Result;
  int Columns = Model->columnCount(Parent);
  for (int Row = 0; (Row < Model->rowCount(Parent)) && (Budget > 0); Row++)
  {
    Budget--;
    QModelIndex Index = Model->index(Row, 0, Parent);
    QJsonArray Cells;
    for (int Column = 0; Column < Columns; Column++)
    {
      Cells.append(Model->index(Row, Column, Parent).data(Qt::DisplayRole).toString());
    }
    QJsonObject Item;
    Item[QStringLiteral("cells")] = Cells;
    Item[QStringLiteral("selected")] = (Selection != nullptr) && Selection->isSelected(Index);
    Item[QStringLiteral("current")] = (Index == View->currentIndex());
    if ((Tree != nullptr) && (Tree->isRowHidden(Row, Parent)))
    {
      Item[QStringLiteral("hidden")] = true;
    }
    if (Model->hasChildren(Index))
    {
      if (Tree != nullptr)
      {
        Item[QStringLiteral("expanded")] = Tree->isExpanded(Index);
      }
      Item[QStringLiteral("children")] = DescribeRows(View, Index, Budget);
    }
    Result.append(Item);
  }
  return Result;
}
//---------------------------------------------------------------------------
QJsonObject Describe(QObject * Object, bool Children)
{
  QJsonObject Result;
  Result[QStringLiteral("name")] = Object->objectName();
  Result[QStringLiteral("class")] = QString::fromLatin1(Object->metaObject()->className());

  if (QWidget * Widget = qobject_cast<QWidget *>(Object))
  {
    Result[QStringLiteral("visible")] = Widget->isVisible();
    Result[QStringLiteral("enabled")] = Widget->isEnabled();
    if (Widget->isWindow())
    {
      Result[QStringLiteral("title")] = Widget->windowTitle();
      Result[QStringLiteral("modal")] = Widget->isModal();
      Result[QStringLiteral("active")] = Widget->isActiveWindow();
    }
    if (Widget->hasFocus())
    {
      Result[QStringLiteral("focus")] = true;
    }
  }

  if (QLineEdit * Edit = qobject_cast<QLineEdit *>(Object))
  {
    Result[QStringLiteral("text")] = Edit->text();
    Result[QStringLiteral("readOnly")] = Edit->isReadOnly();
    Result[QStringLiteral("password")] = (Edit->echoMode() != QLineEdit::Normal);
  }
  else if (QAbstractButton * Button = qobject_cast<QAbstractButton *>(Object))
  {
    Result[QStringLiteral("text")] = Button->text();
    if (Button->isCheckable())
    {
      Result[QStringLiteral("checked")] = Button->isChecked();
    }
  }
  else if (QLabel * Label = qobject_cast<QLabel *>(Object))
  {
    Result[QStringLiteral("text")] = Label->text();
  }
  else if (QComboBox * Combo = qobject_cast<QComboBox *>(Object))
  {
    Result[QStringLiteral("text")] = Combo->currentText();
    Result[QStringLiteral("index")] = Combo->currentIndex();
    QJsonArray Items;
    for (int Index = 0; Index < Combo->count(); Index++)
    {
      Items.append(Combo->itemText(Index));
    }
    Result[QStringLiteral("items")] = Items;
  }
  else if (QPlainTextEdit * Memo = qobject_cast<QPlainTextEdit *>(Object))
  {
    Result[QStringLiteral("text")] = Memo->toPlainText();
  }
  else if (QSpinBox * Spin = qobject_cast<QSpinBox *>(Object))
  {
    Result[QStringLiteral("value")] = Spin->value();
  }
  else if (QProgressBar * Progress = qobject_cast<QProgressBar *>(Object))
  {
    Result[QStringLiteral("value")] = Progress->value();
    Result[QStringLiteral("minimum")] = Progress->minimum();
    Result[QStringLiteral("maximum")] = Progress->maximum();
  }
  else if (QTabBar * Tabs = qobject_cast<QTabBar *>(Object))
  {
    QJsonArray Items;
    for (int Index = 0; Index < Tabs->count(); Index++)
    {
      Items.append(Tabs->tabText(Index));
    }
    Result[QStringLiteral("items")] = Items;
    Result[QStringLiteral("index")] = Tabs->currentIndex();
  }
  else if (QAbstractItemView * View = qobject_cast<QAbstractItemView *>(Object))
  {
    QAbstractItemModel * Model = View->model();
    if (Model != nullptr)
    {
      QJsonArray Columns;
      for (int Column = 0; Column < Model->columnCount(View->rootIndex()); Column++)
      {
        Columns.append(Model->headerData(Column, Qt::Horizontal, Qt::DisplayRole).toString());
      }
      Result[QStringLiteral("columns")] = Columns;
      int Budget = MaxRows;
      Result[QStringLiteral("rows")] = DescribeRows(View, View->rootIndex(), Budget);
    }
  }
  else if (QAction * Action = qobject_cast<QAction *>(Object))
  {
    Result[QStringLiteral("text")] = Action->text();
    Result[QStringLiteral("enabled")] = Action->isEnabled();
    Result[QStringLiteral("visible")] = Action->isVisible();
    if (Action->isCheckable())
    {
      Result[QStringLiteral("checked")] = Action->isChecked();
    }
    Result[QStringLiteral("shortcut")] = Action->shortcut().toString(QKeySequence::PortableText);
  }

  if (Children)
  {
    QJsonArray Items;
    for (QObject * Child : Object->children())
    {
      if ((qobject_cast<QWidget *>(Child) != nullptr) ||
          ((qobject_cast<QAction *>(Child) != nullptr) && !Child->objectName().isEmpty()))
      {
        Items.append(Describe(Child, true));
      }
    }
    Result[QStringLiteral("children")] = Items;
  }
  return Result;
}
//---------------------------------------------------------------------------
void CheckEnabled(QWidget * Widget)
{
  if (!Widget->isVisible() || !Widget->isEnabled())
  {
    throw EAutomation(QStringLiteral("%1 is not visible or not enabled").arg(Name(Widget)));
  }
}
//---------------------------------------------------------------------------
TAutomationServer::TAutomationServer(const QString & Path)
{
  FServer = new QLocalServer(this);
  FServer->setSocketOptions(QLocalServer::UserAccessOption);
  QLocalServer::removeServer(Path);
  if (!FServer->listen(Path))
  {
    qWarning("Automation: cannot listen on %s: %s", qPrintable(Path), qPrintable(FServer->errorString()));
  }
  connect(FServer, &QLocalServer::newConnection, this, [this]() { NewConnection(); });
}
//---------------------------------------------------------------------------
void TAutomationServer::NewConnection()
{
  while (QLocalSocket * Socket = FServer->nextPendingConnection())
  {
    connect(Socket, &QLocalSocket::readyRead, this, [this, Socket]() { ReadRequests(Socket); });
    connect(Socket, &QLocalSocket::disconnected, Socket, &QObject::deleteLater);
  }
}
//---------------------------------------------------------------------------
void TAutomationServer::ReadRequests(QLocalSocket * Socket)
{
  // The requests are processed outside of the readyRead signal, which Qt
  // does not emit again in an event loop nested in its handler (a modal
  // window opened by an action)
  while (Socket->canReadLine())
  {
    TRequest Request;
    Request.Socket = Socket;
    Request.Line = Socket->readLine();
    FRequests.push_back(Request);
  }
  QTimer::singleShot(0, this, [this]() { ProcessRequests(); });
}
//---------------------------------------------------------------------------
void TAutomationServer::ProcessRequests()
{
  while (!FRequests.empty())
  {
    TRequest Request = FRequests.front();
    FRequests.pop_front();
    Process(Request);
  }
}
//---------------------------------------------------------------------------
void TAutomationServer::Reply(QLocalSocket * Socket, const QJsonObject & Reply)
{
  if (Socket != nullptr)
  {
    Socket->write(QJsonDocument(Reply).toJson(QJsonDocument::Compact) + '\n');
    Socket->flush();
  }
}
//---------------------------------------------------------------------------
void TAutomationServer::Process(const TRequest & Request)
{
  QJsonParseError ParseError;
  QJsonDocument Document = QJsonDocument::fromJson(Request.Line, &ParseError);
  QJsonObject Object = Document.object();
  QJsonValue Id = Object.value(QStringLiteral("id"));
  QString Command = Object.value(QStringLiteral("cmd")).toString();
  QJsonObject Result;
  Result[QStringLiteral("id")] = Id;
  TAction Action;
  if (ParseError.error != QJsonParseError::NoError)
  {
    Result[QStringLiteral("ok")] = false;
    Result[QStringLiteral("error")] = ParseError.errorString();
  }
  else if (Command == QLatin1String("wait-idle"))
  {
    WaitIdle(Request.Socket, Id, 3);
    return;
  }
  else
  {
    try
    {
      Result[QStringLiteral("result")] = Execute(Command, Object, Action);
      Result[QStringLiteral("ok")] = true;
    }
    catch (const std::exception & E)
    {
      Result[QStringLiteral("ok")] = false;
      Result[QStringLiteral("error")] = QString::fromStdString(E.what());
      Action = nullptr;
    }
  }
  Reply(Request.Socket, Result);
  if (Action)
  {
    Action();
  }
}
//---------------------------------------------------------------------------
void TAutomationServer::WaitIdle(QPointer<QLocalSocket> Socket, const QJsonValue & Id, int Rounds)
{
  // A few rounds of the event loop let the events that are already posted
  // run, and those they post
  if (BusyCheck && BusyCheck())
  {
    QTimer::singleShot(50, this, [this, Socket, Id]() { WaitIdle(Socket, Id, 3); });
  }
  else if (Rounds > 0)
  {
    QTimer::singleShot(0, this, [this, Socket, Id, Rounds]() { WaitIdle(Socket, Id, Rounds - 1); });
  }
  else
  {
    QJsonObject Result;
    Result[QStringLiteral("id")] = Id;
    Result[QStringLiteral("ok")] = true;
    Reply(Socket, Result);
  }
}
//---------------------------------------------------------------------------
QJsonValue TAutomationServer::Execute(const QString & Command, const QJsonObject & Request, TAction & Action)
{
  QJsonValue Result;
  if (Command == QLatin1String("windows"))
  {
    QJsonArray Windows;
    for (QWidget * Widget : QApplication::topLevelWidgets())
    {
      if (Widget->isVisible())
      {
        Windows.append(Describe(Widget, false));
      }
    }
    Result = Windows;
  }
  else if (Command == QLatin1String("get"))
  {
    Result = Describe(FindTarget(Request), Request.value(QStringLiteral("children")).toBool());
  }
  else if ((Command == QLatin1String("focus")) && Request.contains(QStringLiteral("target")))
  {
    QPointer<QWidget> Pointer(FindTargetOf<QWidget>(Request));
    Action = [Pointer]() { if (Pointer) Pointer->setFocus(Qt::OtherFocusReason); };
  }
  else if (Command == QLatin1String("focus"))
  {
    QWidget * Focus = QApplication::focusWidget();
    if (Focus != nullptr)
    {
      QJsonObject Object = Describe(Focus, false);
      Object[QStringLiteral("window")] = Focus->window()->objectName();
      Result = Object;
    }
  }
  else if (Command == QLatin1String("set"))
  {
    QObject * Target = FindTarget(Request);
    QJsonValue Value = Request.value(QStringLiteral("value"));
    if (QLineEdit * Edit = qobject_cast<QLineEdit *>(Target))
    {
      CheckEnabled(Edit);
      QString Text = Value.toString();
      Action = [Edit, Text]() { Edit->setText(Text); };
    }
    else if (QPlainTextEdit * Memo = qobject_cast<QPlainTextEdit *>(Target))
    {
      CheckEnabled(Memo);
      QString Text = Value.toString();
      Action = [Memo, Text]() { Memo->setPlainText(Text); };
    }
    else if (QComboBox * Combo = qobject_cast<QComboBox *>(Target))
    {
      CheckEnabled(Combo);
      int Index = Value.isDouble() ? Value.toInt() : Combo->findText(Value.toString());
      if ((Index >= 0) && (Index < Combo->count()))
      {
        Action = [Combo, Index]() { Combo->setCurrentIndex(Index); };
      }
      else if (Combo->isEditable())
      {
        QString Text = Value.toString();
        Action = [Combo, Text]() { Combo->setEditText(Text); };
      }
      else
      {
        throw EAutomation(QStringLiteral("No item %1 in %2").arg(Value.toVariant().toString(), Name(Combo)));
      }
    }
    else if (QAbstractButton * Button = qobject_cast<QAbstractButton *>(Target))
    {
      CheckEnabled(Button);
      bool Checked = Value.toBool();
      Action = [Button, Checked]() { Button->setChecked(Checked); };
    }
    else if (QSpinBox * Spin = qobject_cast<QSpinBox *>(Target))
    {
      CheckEnabled(Spin);
      int Number = Value.toInt();
      Action = [Spin, Number]() { Spin->setValue(Number); };
    }
    else
    {
      throw EAutomation(QStringLiteral("Cannot set %1").arg(Name(Target)));
    }
  }
  else if ((Command == QLatin1String("click")) || (Command == QLatin1String("trigger")))
  {
    QObject * Target = FindTarget(Request);
    if (QAbstractButton * Button = qobject_cast<QAbstractButton *>(Target))
    {
      CheckEnabled(Button);
      QPointer<QAbstractButton> Pointer(Button);
      Action = [Pointer]() { if (Pointer) Pointer->click(); };
    }
    else if (QAction * TheAction = qobject_cast<QAction *>(Target))
    {
      if (!TheAction->isEnabled())
      {
        throw EAutomation(QStringLiteral("%1 is not enabled").arg(Name(TheAction)));
      }
      QPointer<QAction> Pointer(TheAction);
      Action = [Pointer]() { if (Pointer) Pointer->trigger(); };
    }
    else
    {
      throw EAutomation(QStringLiteral("Cannot click %1").arg(Name(Target)));
    }
  }
  else if (Command == QLatin1String("select"))
  {
    QAbstractItemView * View = FindTargetOf<QAbstractItemView>(Request);
    CheckEnabled(View);
    QModelIndexList Indexes;
    for (const QJsonValue & Item : Request.value(QStringLiteral("items")).toArray())
    {
      Indexes.append(FindItem(View, Item));
    }
    QPointer<QAbstractItemView> Pointer(View);
    Action = [Pointer, Indexes]()
    {
      if (Pointer)
      {
        QItemSelectionModel * Selection = Pointer->selectionModel();
        Selection->clearSelection();
        if (!Indexes.isEmpty())
        {
          Selection->setCurrentIndex(Indexes.first(), QItemSelectionModel::NoUpdate);
        }
        for (const QModelIndex & Index : Indexes)
        {
          Selection->select(Index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
      }
    };
  }
  else if (Command == QLatin1String("activate"))
  {
    QAbstractItemView * View = FindTargetOf<QAbstractItemView>(Request);
    CheckEnabled(View);
    QModelIndex Index = FindItem(View, Request.value(QStringLiteral("item")));
    QPointer<QAbstractItemView> Pointer(View);
    QPersistentModelIndex Persistent(Index);
    Action = [Pointer, Persistent]()
    {
      if (Pointer && Persistent.isValid())
      {
        Pointer->setCurrentIndex(Persistent);
        Q_EMIT Pointer->activated(Persistent);
      }
    };
  }
  else if (Command == QLatin1String("expand"))
  {
    QTreeView * Tree = FindTargetOf<QTreeView>(Request);
    QModelIndex Index = FindItem(Tree, Request.value(QStringLiteral("item")));
    bool Expanded = Request.value(QStringLiteral("expanded")).toBool(true);
    Tree->setExpanded(Index, Expanded);
  }
  else if (Command == QLatin1String("key"))
  {
    // Synthetic key events, delivered to the widget (or the focused one);
    // they do not activate shortcuts, which need the input of the
    // compositor (wlh-input)
    QWidget * Widget =
      Request.contains(QStringLiteral("target")) ? FindTargetOf<QWidget>(Request) : QApplication::focusWidget();
    if (Widget == nullptr)
    {
      throw EAutomation(QStringLiteral("No widget has the focus"));
    }
    QKeySequence Sequence = QKeySequence::fromString(Request.value(QStringLiteral("key")).toString(), QKeySequence::PortableText);
    if (Sequence.isEmpty())
    {
      throw EAutomation(QStringLiteral("Unknown key %1").arg(Request.value(QStringLiteral("key")).toString()));
    }
    QKeyCombination Combination = Sequence[0];
    QPointer<QWidget> Pointer(Widget);
    Action = [Pointer, Combination]()
    {
      if (Pointer)
      {
        QKeyEvent Press(QEvent::KeyPress, Combination.key(), Combination.keyboardModifiers());
        QCoreApplication::sendEvent(Pointer, &Press);
      }
      if (Pointer)
      {
        QKeyEvent Release(QEvent::KeyRelease, Combination.key(), Combination.keyboardModifiers());
        QCoreApplication::sendEvent(Pointer, &Release);
      }
    };
  }
  else if (Command == QLatin1String("grab"))
  {
    QWidget * Widget = FindTargetOf<QWidget>(Request);
    QString FileName = Request.value(QStringLiteral("file")).toString();
    if (!Widget->grab().save(FileName, "PNG"))
    {
      throw EAutomation(QStringLiteral("Cannot save %1").arg(FileName));
    }
  }
  else if (Command == QLatin1String("close"))
  {
    QPointer<QWidget> Pointer(FindTargetOf<QWidget>(Request));
    Action = [Pointer]() { if (Pointer) Pointer->close(); };
  }
  else if (Command == QLatin1String("quit"))
  {
    // Ends the nested event loops (modal windows) too
    Action = []() { QCoreApplication::exit(0); };
  }
  else
  {
    throw EAutomation(QStringLiteral("Unknown command %1").arg(Command));
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
void StartAutomation()
{
  QByteArray Path = qgetenv("OPENSCP_AUTOMATION_SOCKET");
  if (!Path.isEmpty())
  {
    // Not for the processes the application starts
    qunsetenv("OPENSCP_AUTOMATION_SOCKET");
    Server = new TAutomationServer(QString::fromLocal8Bit(Path));
  }
}
//---------------------------------------------------------------------------
void StopAutomation()
{
  delete Server;
  Server = nullptr;
}
//---------------------------------------------------------------------------
void SetAutomationBusyCheck(std::function<bool()> BusyCheck)
{
  if (Server != nullptr)
  {
    Server->BusyCheck = BusyCheck;
  }
}
//---------------------------------------------------------------------------
