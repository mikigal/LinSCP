// The console: commands executed in the session's shell (upstream's
// TConsoleDialog, forms/Console.dfm), also showing the results of custom
// commands.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <Terminal.h>
#include <TerminalManager.h>

#include "HistoryComboBox.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
class TConsoleDialog : public QDialog
{
public:
  TConsoleDialog();
  virtual ~TConsoleDialog();

  void SetTerminal(TTerminal * Value);
  bool Execute(const UnicodeString & Command, const TStrings * Log);

protected:
  virtual void closeEvent(QCloseEvent * Event) override;
  virtual void reject() override;

private:
  TTerminal * FTerminal;
  TTerminal * FLastTerminal;
  bool FClearExceptionOnFail;
  bool FDirectoryChanged;
  bool FExecuting;
  TNotifyEvent FOldChangeDirectory;
  TNotifyEvent FPrevTerminalClose;

  THistoryComboBox * CommandEdit;
  QPushButton * ExecuteButton;
  QLabel * DirectoryLabel;
  QPlainTextEdit * OutputMemo;
  QPushButton * CancelBtn;

  void UpdateControls();
  void DoChangeDirectory(TObject * Sender);
  void TerminalClose(TObject * Sender);
  void ExecuteCommand();
  void DoExecuteCommand();
  void AddLine(const UnicodeString & Line, TCaptureOutputType OutputType);
  void DoAdjustWindow();
};
//---------------------------------------------------------------------------
TConsoleDialog::TConsoleDialog() :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("ConsoleDialog"));
  setWindowTitle(tr("Console"));
  FTerminal = NULL;
  FLastTerminal = NULL;
  FClearExceptionOnFail = false;
  FDirectoryChanged = false;
  FExecuting = false;
  FOldChangeDirectory = NULL;
  FPrevTerminalClose = NULL;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  // upstream's layout: the image, the command, the note and the directory
  // in rows, Close and Help on the right
  QGridLayout * Grid = new QGridLayout();
  Grid->addWidget(CreateDialogImage(this, L"Open console window"), 0, 0, 3, 1, Qt::AlignTop);
  QLabel * Label1 = new QLabel(tr("Enter &command:"), this);
  Label1->setObjectName(QStringLiteral("Label1"));
  Grid->addWidget(Label1, 0, 1);
  CommandEdit = new THistoryComboBox(this);
  CommandEdit->setObjectName(QStringLiteral("CommandEdit"));
  CommandEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  Label1->setBuddy(CommandEdit);
  connect(CommandEdit, &QComboBox::currentTextChanged, this, [this]() { UpdateControls(); });
  Grid->addWidget(CommandEdit, 0, 2);
  ExecuteButton = new QPushButton(tr("&Execute"), this);
  ExecuteButton->setObjectName(QStringLiteral("ExecuteButton"));
  ExecuteButton->setDefault(true);
  connect(ExecuteButton, &QPushButton::clicked, this, [this]()
  {
    CommandEdit->SaveToHistory();
    ExecuteCommand();
  });
  Grid->addWidget(ExecuteButton, 0, 3);
  CancelBtn = new QPushButton(tr("Close"), this);
  CancelBtn->setObjectName(QStringLiteral("CancelBtn"));
  CancelBtn->setAutoDefault(false);
  connect(CancelBtn, &QPushButton::clicked, this, &QDialog::reject);
  Grid->addWidget(CancelBtn, 0, 4);
  QLabel * Label4 = new QLabel(tr("Do not execute commands that require user-input or data transfer"), this);
  Label4->setObjectName(QStringLiteral("Label4"));
  Grid->addWidget(Label4, 1, 1, 1, 3);
  QPushButton * HelpButton = new QPushButton(tr("&Help"), this);
  HelpButton->setObjectName(QStringLiteral("HelpButton"));
  HelpButton->setAutoDefault(false);
  connect(HelpButton, &QPushButton::clicked, this, []() { ShowHelp(FormHelpKeyword(L"Console.dfm")); });
  Grid->addWidget(HelpButton, 1, 4);
  QLabel * Label2 = new QLabel(tr("Current directory:"), this);
  Label2->setObjectName(QStringLiteral("Label2"));
  Grid->addWidget(Label2, 2, 1);
  DirectoryLabel = new QLabel(this);
  DirectoryLabel->setObjectName(QStringLiteral("DirectoryLabel"));
  DirectoryLabel->setTextFormat(Qt::PlainText);
  Grid->addWidget(DirectoryLabel, 2, 2, 1, 2);
  Grid->setColumnStretch(2, 1);
  MainLayout->addLayout(Grid);

  OutputMemo = new QPlainTextEdit(this);
  OutputMemo->setObjectName(QStringLiteral("OutputMemo"));
  OutputMemo->setReadOnly(true);
  OutputMemo->setLineWrapMode(QPlainTextEdit::NoWrap);
  OutputMemo->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  // As upstream, a terminal-like look
  QPalette Palette = OutputMemo->palette();
  Palette.setColor(QPalette::Base, Qt::black);
  Palette.setColor(QPalette::Text, QColor(0xBB, 0xBB, 0xBB));
  OutputMemo->setPalette(Palette);
  OutputMemo->setContextMenuPolicy(Qt::ActionsContextMenu);
  QAction * EditCopy = new QAction(tr("&Copy"), OutputMemo);
  EditCopy->setObjectName(QStringLiteral("EditCopy"));
  EditCopy->setShortcut(QKeySequence::Copy);
  EditCopy->setShortcutContext(Qt::WidgetShortcut);
  connect(EditCopy, &QAction::triggered, OutputMemo, &QPlainTextEdit::copy);
  QAction * EditSelectAll = new QAction(tr("Select &All"), OutputMemo);
  EditSelectAll->setObjectName(QStringLiteral("EditSelectAll"));
  EditSelectAll->setShortcut(QKeySequence::SelectAll);
  EditSelectAll->setShortcutContext(Qt::WidgetShortcut);
  connect(EditSelectAll, &QAction::triggered, OutputMemo, &QPlainTextEdit::selectAll);
  QAction * Separator = new QAction(OutputMemo);
  Separator->setSeparator(true);
  QAction * AdjustWindow = new QAction(tr("Adjust &Window"), this);
  AdjustWindow->setObjectName(QStringLiteral("AdjustWindow"));
  AdjustWindow->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
  connect(AdjustWindow, &QAction::triggered, this, [this]() { DoAdjustWindow(); });
  addAction(AdjustWindow);
  OutputMemo->addActions({ EditCopy, EditSelectAll, Separator, AdjustWindow });
  MainLayout->addWidget(OutputMemo, 1);

  resize(fontMetrics().averageCharWidth() * 100, fontMetrics().height() * 30);
}
//---------------------------------------------------------------------------
TConsoleDialog::~TConsoleDialog()
{
  SetTerminal(NULL);
}
//---------------------------------------------------------------------------
void TConsoleDialog::SetTerminal(TTerminal * Value)
{
  if (FTerminal != Value)
  {
    // LinSCP: the session may be freed meanwhile (once its connection is
    // lost and it was never connected)
    TTerminalManager * Manager = TTerminalManager::Instance(false);
    if ((FTerminal != NULL) && (Manager != NULL) && (Manager->IndexOf(FTerminal) < 0))
    {
      FTerminal = NULL;
    }
    if (FTerminal != NULL)
    {
      if (FClearExceptionOnFail)
      {
        FTerminal->ExceptionOnFail = false;
        FClearExceptionOnFail = false;
      }
      FTerminal->OnClose = FPrevTerminalClose;
      FTerminal->OnChangeDirectory = FOldChangeDirectory;
      FOldChangeDirectory = NULL;
      if (FDirectoryChanged)
      {
        FDirectoryChanged = false;
        if (FTerminal->Active)
        {
          // directory would be read from EndTransaction anyway,
          // but with reload only flag set, what prevents
          // recording path in history, what we want if the path was
          // changed by "cd" command in console
          FTerminal->ReadDirectory(false);
        }
      }
      FTerminal->EndTransaction();
    }
    FTerminal = Value;
    if (FTerminal != NULL)
    {
      OutputMemo->clear();
      // The remote panel listens to the changes of the directory too
      FOldChangeDirectory = FTerminal->OnChangeDirectory;
      FTerminal->OnChangeDirectory = ::rtl::MethodPtr(this, &TConsoleDialog::DoChangeDirectory);
      // avoid reloading directory after each change of current directory from console
      FTerminal->BeginTransaction();
      FLastTerminal = FTerminal;
      FPrevTerminalClose = FTerminal->OnClose;
      FTerminal->OnClose = ::rtl::MethodPtr(this, &TConsoleDialog::TerminalClose);
    }
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TConsoleDialog::DoChangeDirectory(TObject * Sender)
{
  if (FOldChangeDirectory != NULL)
  {
    FOldChangeDirectory(Sender);
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TConsoleDialog::UpdateControls()
{
  DirectoryLabel->setText((FTerminal != NULL) ? ToQString(FTerminal->CurrentDirectory) : QString());
  // Disabling buttons while executing to prevent recursive execution,
  // now that message queue is processed while command is executing.
  ExecuteButton->setEnabled(
    ((FTerminal != NULL) ? FTerminal->AllowedAnyCommand(CommandEdit->GetText()) : false) &&
    !FExecuting);
  CancelBtn->setEnabled(!FExecuting);
}
//---------------------------------------------------------------------------
bool TConsoleDialog::Execute(const UnicodeString & Command, const TStrings * Log)
{
  CommandEdit->SetHistoryKey(L"Commands");
  ::rtl::TFinally Finally([&]()
  {
    if (FTerminal != NULL)
    {
      CommandEdit->SaveToHistory();
    }
  });
  if (Log != NULL)
  {
    TStrings * ALog = const_cast<TStrings *>(Log);
    for (int Index = 0; Index < ALog->Count; Index++)
    {
      AddLine(ALog->Strings[Index], cotOutput);
    }
  }

  if (!Command.IsEmpty())
  {
    CommandEdit->SetText(Command);
    DoExecuteCommand();
  }
  UpdateControls();
  CommandEdit->setFocus();
  exec();
  return true;
}
//---------------------------------------------------------------------------
void TConsoleDialog::TerminalClose(TObject * Sender)
{
  // Not deassociating terminal here, leaving it to the destructor.
  // Because at this point, we could be in the midddle of a reconnect and the event handlers can be set to TTerminalThread.
  QDialog::reject();
  if (FPrevTerminalClose != NULL)
  {
    FPrevTerminalClose(Sender);
  }
}
//---------------------------------------------------------------------------
void TConsoleDialog::DoExecuteCommand()
{
  CommandEdit->lineEdit()->selectAll();
  FTerminal->ExceptionOnFail = true;
  FClearExceptionOnFail = true;
  UnicodeString CurrentDirectory = FTerminal->CurrentDirectory;
  {
    ::rtl::TFinally Finally([&]()
    {
      if (FTerminal != NULL)
      {
        FTerminal->ExceptionOnFail = false;
        DebugAssert(FClearExceptionOnFail);
        FClearExceptionOnFail = false;
        if (FTerminal->Active)
        {
          FTerminal->ReadCurrentDirectory();
        }
      }
      UpdateControls();
    });
    UnicodeString Command = CommandEdit->GetText();
    OutputMemo->appendPlainText(ToQString(FORMAT(L"%s$ %s", (CurrentDirectory, Command))));
    Configuration->Usage->Inc(L"RemoteCommandExecutions");
    TAutoFlag ExecutingFlag(FExecuting);
    UpdateControls();
    // give a chance for disabled buttons to redraw
    QCoreApplication::processEvents();
    FTerminal->AnyCommand(Command, ::rtl::MethodPtr(this, &TConsoleDialog::AddLine));
  }

  if ((FTerminal != NULL) && (CurrentDirectory != FTerminal->CurrentDirectory))
  {
    FDirectoryChanged = true;
  }
}
//---------------------------------------------------------------------------
void TConsoleDialog::ExecuteCommand()
{
  try
  {
    DoExecuteCommand();
  }
  catch (Exception & E)
  {
    DebugAssert(FLastTerminal != NULL);
    // Should use the command session, if there's one, not to close the main session
    FLastTerminal->ShowExtendedException(&E);
  }
}
//---------------------------------------------------------------------------
void TConsoleDialog::AddLine(const UnicodeString & Line, TCaptureOutputType OutputType)
{
  if ((OutputType == cotOutput) || (OutputType == cotError))
  {
    OutputMemo->appendPlainText(ToQString(Line));
    OutputMemo->verticalScrollBar()->setValue(OutputMemo->verticalScrollBar()->maximum());
  }
}
//---------------------------------------------------------------------------
void TConsoleDialog::DoAdjustWindow()
{
  QFontMetrics Metrics(OutputMemo->font());
  QStringList Lines = OutputMemo->toPlainText().split(QLatin1Char('\n'));
  int Columns = 0;
  for (const QString & Line : Lines)
  {
    Columns = std::max(Columns, static_cast<int>(Line.length()));
  }
  // there is always one line more
  int RequiredWidth = Metrics.averageCharWidth() * Columns + 2 * OutputMemo->frameWidth() + 10;
  int RequiredHeight = Metrics.lineSpacing() * (Lines.count() + 1) + 2 * OutputMemo->frameWidth() + 10;
  QSize Size = size() + QSize(RequiredWidth - OutputMemo->width(), RequiredHeight - OutputMemo->height());
  resize(Size.boundedTo(screen()->availableGeometry().size()));
}
//---------------------------------------------------------------------------
void TConsoleDialog::closeEvent(QCloseEvent * Event)
{
  if (FExecuting)
  {
    Event->ignore();
  }
  else
  {
    QDialog::closeEvent(Event);
  }
}
//---------------------------------------------------------------------------
void TConsoleDialog::reject()
{
  if (!FExecuting)
  {
    QDialog::reject();
  }
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
void DoConsoleDialog(TTerminal * Terminal, const UnicodeString & Command, const TStrings * Log)
{
  TConsoleDialog Dialog;
  Dialog.SetTerminal(Terminal);
  Dialog.Execute(Command, Log);
}
//---------------------------------------------------------------------------
