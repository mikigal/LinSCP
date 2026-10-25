// The parts of the core's front end interface (core/Interface.h) specific
// to the Qt front end (upstream has them in WinInterface.cpp,
// UserInterface.cpp and Tools.cpp).
//---------------------------------------------------------------------------
#include "QtTools.h"
#include "QtInterface.h"
#include "ComputerPower.h"
#include "ScpCommander.h"
#include "TerminalManager.h"
#include "FormTexts.h"

#include <AppConfiguration.h>

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QUrl>

#include <chrono>
//---------------------------------------------------------------------------
static bool NoGUI = false;
static std::chrono::steady_clock::time_point LastGUIUpdate;
//---------------------------------------------------------------------------
void __fastcall ShowExtendedException(Exception * E)
{
  ShowExtendedExceptionEx(NULL, E, false);
}
//---------------------------------------------------------------------------
void ShowExtendedExceptionEx(TTerminal * Terminal, Exception * E, bool WhileIdle)
{
  bool Show = ShouldDisplayException(E);
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  // A fatal error of the session (a lost or failed connection)
  bool ForActiveTerminal =
    (dynamic_cast<EFatal *>(E) != NULL) && (Manager != NULL) && Manager->IsActiveTerminal(Terminal);
  // The session was closed once an operation completed
  ESshTerminate * Terminate = dynamic_cast<ESshTerminate *>(E);

  if (Terminate != NULL)
  {
    if (ForActiveTerminal)
    {
      DebugAssert(!Terminal->Active);
      Manager->DisconnectActiveTerminal();
    }

    if (Terminate->Operation == odoSuspend)
    {
      // suspend, so that exit prompt is shown only after the computer resumes
      try
      {
        SuspendComputer();
      }
      catch (Exception & SuspendE)
      {
        ExceptionMessageDialog(&SuspendE, qtError);
      }
    }

    unsigned int Result = qaNo;
    bool ConfirmExitOnCompletion =
      ((Terminate->Operation == odoDisconnect) || (Terminate->Operation == odoSuspend)) &&
      AppConfiguration->ConfirmExitOnCompletion;
    if (ConfirmExitOnCompletion)
    {
      TMessageParams Params(mpNeverAskAgainCheck);
      if (ForActiveTerminal)
      {
        UnicodeString MessageFormat =
          (Manager->Count > 1) ?
            FMTLOAD(DISCONNECT_ON_COMPLETION, (Manager->Count - 1)) :
            LoadStr(EXIT_ON_COMPLETION);
        // Remove the leading "%s\n\n" (not to change the translation originals - previously the error message was prepended)
        MessageFormat = FORMAT(MessageFormat, (UnicodeString())).Trim();
        MessageFormat = MainInstructions(MessageFormat) + L"\n\n%s";
        Result = FatalExceptionMessageDialog(E, qtInformation, MessageFormat, qaYes | qaNo, &Params);
      }
      else
      {
        Result = ExceptionMessageDialog(E, qtInformation, L"", qaOK, &Params);
      }

      if (Result == qaNeverAskAgain)
      {
        Result = qaYes;
        AppConfiguration->ConfirmExitOnCompletion = false;
      }
    }
    else if (Terminate->Operation != odoIdle)
    {
      Result = qaYes;
    }

    if ((Result == qaYes) && ForActiveTerminal && (Manager->ScpExplorer != NULL))
    {
      // TerminateApplication
      QMetaObject::invokeMethod(Manager->ScpExplorer, "close", Qt::QueuedConnection);
    }
    // qaRetry is the "Reconnect" button of FatalExceptionMessageDialog
    else if ((Result == qaRetry) && ForActiveTerminal)
    {
      Manager->ReconnectActiveTerminal();
    }
    if ((Result == qaYes) && (Terminate->Operation == odoShutDown))
    {
      try
      {
        ShutDownComputer();
      }
      catch (Exception & ShutDownE)
      {
        ExceptionMessageDialog(&ShutDownE, qtError);
      }
    }
  }
  else if (ForActiveTerminal)
  {
    unsigned int Answer = qaOK;
    UnicodeString DisconnectError;
    if (Show)
    {
      EFatal * FatalException = dynamic_cast<EFatal *>(E);
      bool InactiveTerminationMessage = (FatalException != NULL) && FatalException->InactiveTerminationMessage;

      if (!InactiveTerminationMessage && GUIConfiguration->SessionSilentDisconnect && WhileIdle)
      {
        Answer = qaOK;
        DisconnectError = E->Message;
      }
      else
      {
        TMessageParams Params;
        TManagedTerminal * ActiveTerminal = Manager->ActiveTerminal;
        // The reconnect is automatic after a while, unless reconnecting
        // for too long already (upstream also hooks the dialog to retry
        // the upload of an edited file in it)
        if ((Configuration->SessionReopenTimeout == 0) ||
            (ActiveTerminal->ReopenStart == TDateTime()) ||
            (int(double(Now() - ActiveTerminal->ReopenStart) * MSecsPerDay) < Configuration->SessionReopenTimeout))
        {
          Params.Timeout = GUIConfiguration->SessionReopenAutoIdleOn ? GUIConfiguration->SessionReopenAutoIdle : 0;
          Params.TimeoutAnswer = qaRetry;
          Params.TimeoutResponse = Params.TimeoutAnswer;
        }

        bool PermanentTerminal = ActiveTerminal->Permanent;
        if (InactiveTerminationMessage)
        {
          Params.Params |= mpNeverAskAgainCheck;
          Params.NeverAskAgainTitle = LoadStr(ALWAYS_RECONNECT);
          Params.NeverAskAgainAnswer = qaRetry;
        }
        // For now - maybe it is worth doing even for non-permanent sessions
        else if (PermanentTerminal && WhileIdle)
        {
          Params.Params |= mpNeverAskAgainCheck;
          Params.NeverAskAgainTitle = LoadStr(NEVER_POPUP_DISCONNECT);
          Params.NeverAskAgainAnswer = qaOK;
        }

        Answer = FatalExceptionMessageDialog(E, qtError, EmptyStr, qaOK, &Params);

        if (Answer == qaNeverAskAgain)
        {
          Answer = Params.NeverAskAgainAnswer;
          if (InactiveTerminationMessage)
          {
            GUIConfiguration->SessionReopenAutoInactive = true;
          }
          else if (DebugAlwaysTrue(PermanentTerminal))
          {
            GUIConfiguration->SessionSilentDisconnect = true;
          }
        }
      }
    }
    // qaRetry is the "Reconnect" button of FatalExceptionMessageDialog
    if (Answer == qaRetry)
    {
      Manager->ReconnectActiveTerminal();
    }
    else
    {
      Manager->DisconnectActiveTerminalIfPermanentFreeOtherwise(DisconnectError);
    }
  }
  else if (Show)
  {
    ExceptionMessageDialog(E, qtError);
  }
}
//---------------------------------------------------------------------------
void * __fastcall BusyStart()
{
  if (IsMainThread() && !NoGUI)
  {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    return reinterpret_cast<void *>(1);
  }
  return NULL;
}
//---------------------------------------------------------------------------
void __fastcall BusyEnd(void * Token)
{
  if (Token != NULL)
  {
    QApplication::restoreOverrideCursor();
  }
}
//---------------------------------------------------------------------------
void __fastcall SetNoGUI()
{
  NoGUI = true;
}
//---------------------------------------------------------------------------
bool __fastcall ProcessGUI(bool Force)
{
  bool Result = false;
  if (IsMainThread() && !NoGUI)
  {
    std::chrono::steady_clock::time_point Now = std::chrono::steady_clock::now();
    if (Force || (Now - LastGUIUpdate > std::chrono::milliseconds(GUIUpdateInterval)))
    {
      LastGUIUpdate = Now;
      QCoreApplication::processEvents();
      Result = true;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void SystemRequired()
{
  // Upstream keeps the system from sleeping during transfers
  // (SetThreadExecutionState); the counterpart on Linux is the inhibit
  // portal, which holds an inhibition until it is released, rather than a
  // periodic call
}
//---------------------------------------------------------------------------
void OpenBrowser(const UnicodeString & Url)
{
  QDesktopServices::openUrl(QUrl(ToQString(Url)));
}
//---------------------------------------------------------------------------
void ShowHelp(const UnicodeString & HelpKeyword)
{
  // The documentation of the upstream application applies (the keywords
  // are the names of its pages, with their anchors)
  OpenBrowser(L"https://winscp.net/eng/docs/" + HelpKeyword);
}
//---------------------------------------------------------------------------
UnicodeString FormHelpKeyword(const UnicodeString & Form, const QString & Component)
{
  // tools/dfm2cpp keeps the keywords of the forms with their texts
  std::wstring FormName(Form.c_str());
  std::wstring Suffix = Component.isEmpty() ? std::wstring() : (L"." + Component.toStdWString() + L".HelpKeyword");
  UnicodeString Result;
  for (size_t Index = 0; Index < FormTextCount; Index++)
  {
    const TFormText & Text = FormTexts[Index];
    if (FormName == Text.Form)
    {
      std::wstring Key(Text.Key);
      if (Key == L"HelpKeyword")
      {
        if (Result.IsEmpty())
        {
          Result = Text.Text;
        }
      }
      else if (!Suffix.empty() && (Key.size() > Suffix.size()) &&
               (Key.compare(Key.size() - Suffix.size(), Suffix.size(), Suffix) == 0))
      {
        return Text.Text;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
// The layout that has the widget, in the layouts of its parent, and the
// widget's index in it
static QLayout * LayoutOf(QLayout * Layout, QWidget * Widget, int & Index)
{
  for (int I = 0; (Layout != nullptr) && (I < Layout->count()); I++)
  {
    QLayoutItem * Item = Layout->itemAt(I);
    if (Item->widget() == Widget)
    {
      Index = I;
      return Layout;
    }
    if (QLayout * Result = LayoutOf(Item->layout(), Widget, Index))
    {
      return Result;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
// Puts the Help button right of the box: next to it in a row, or in a row
// with it; false when the box is not in a layout of its window yet
static bool PlaceHelpButton(QDialogButtonBox * Buttons, QPushButton * Button)
{
  int Index = -1;
  QWidget * Parent = Buttons->parentWidget();
  QLayout * Layout = (Parent != nullptr) ? LayoutOf(Parent->layout(), Buttons, Index) : nullptr;
  QBoxLayout * Box = qobject_cast<QBoxLayout *>(Layout);
  QGridLayout * Grid = qobject_cast<QGridLayout *>(Layout);
  if ((Box != nullptr) && ((Box->direction() == QBoxLayout::LeftToRight) || (Box->direction() == QBoxLayout::RightToLeft)))
  {
    Box->insertWidget(Index + 1, Button);
  }
  else if ((Box != nullptr) || (Grid != nullptr))
  {
    QHBoxLayout * Row = new QHBoxLayout();
    if (Box != nullptr)
    {
      Box->removeWidget(Buttons);
      Box->insertLayout(Index, Row);
    }
    else
    {
      int RowIndex, Column, RowSpan, ColumnSpan;
      Grid->getItemPosition(Index, &RowIndex, &Column, &RowSpan, &ColumnSpan);
      Grid->removeWidget(Buttons);
      Grid->addLayout(Row, RowIndex, Column, RowSpan, ColumnSpan);
    }
    Row->addWidget(Buttons, 1);
    Row->addWidget(Button);
  }
  else
  {
    return false;
  }
  Button->show();
  return true;
}
//---------------------------------------------------------------------------
namespace
{
// Places the Help button once the window shows (its layouts complete)
class THelpButtonPlacer : public QObject
{
public:
  THelpButtonPlacer(QDialogButtonBox * Buttons, QPushButton * Button) :
    QObject(Button), FButtons(Buttons), FButton(Button)
  {
  }

  virtual bool eventFilter(QObject * Object, QEvent * Event) override
  {
    if (Event->type() == QEvent::Show)
    {
      Object->removeEventFilter(this);
      if (!PlaceHelpButton(FButtons, FButton))
      {
        FButtons->addButton(FButton, QDialogButtonBox::HelpRole);
        FButton->show();
      }
      deleteLater();
    }
    return false;
  }

private:
  QDialogButtonBox * FButtons;
  QPushButton * FButton;
};
}
//---------------------------------------------------------------------------
QPushButton * AddHelpButton(QDialogButtonBox * Buttons, const std::function<UnicodeString()> & HelpKeyword)
{
  // Right of the other buttons, as upstream's (the desktop's button boxes
  // put it to the left)
  QPushButton * Button = new QPushButton(QDialogButtonBox::tr("&Help"), Buttons->parentWidget());
  Button->setObjectName(QStringLiteral("HelpButton"));
  Button->setAutoDefault(false);
  QObject::connect(Button, &QPushButton::clicked, Buttons, [HelpKeyword]() { ShowHelp(HelpKeyword()); });
  if (!PlaceHelpButton(Buttons, Button))
  {
    Button->hide();
    Buttons->window()->installEventFilter(new THelpButtonPlacer(Buttons, Button));
  }
  return Button;
}
//---------------------------------------------------------------------------
QPushButton * AddHelpButton(QDialogButtonBox * Buttons, const UnicodeString & HelpKeyword)
{
  return AddHelpButton(Buttons, [HelpKeyword]() { return HelpKeyword; });
}
//---------------------------------------------------------------------------
QIcon DialogImage(const UnicodeString & ImageName)
{
  QString Name = ToQString(LowerCase(ImageName)).replace(QLatin1Char(' '), QLatin1Char('-'));
  QString FileName = QStringLiteral(":/icons/openscp/dialogs/%1.svg").arg(Name);
  DebugAssert(QFileInfo::exists(FileName));
  return QIcon(FileName);
}
//---------------------------------------------------------------------------
void LoadDialogImage(QLabel * Image, const UnicodeString & ImageName)
{
  const int Size = 32;
  Image->setPixmap(DialogImage(ImageName).pixmap(QSize(Size, Size), Image->devicePixelRatioF()));
  Image->setFixedSize(Size, Size);
  // For the tests (Automation.cpp)
  Image->setProperty("automationImageName", ToQString(ImageName));
}
//---------------------------------------------------------------------------
QLabel * CreateDialogImage(QWidget * Parent, const UnicodeString & ImageName)
{
  QLabel * Image = new QLabel(Parent);
  Image->setObjectName(QStringLiteral("Image"));
  LoadDialogImage(Image, ImageName);
  return Image;
}
//---------------------------------------------------------------------------
void __fastcall CopyToClipboard(UnicodeString Text)
{
  QGuiApplication::clipboard()->setText(ToQString(Text));
}
//---------------------------------------------------------------------------
bool __fastcall TextFromClipboard(UnicodeString & Text, bool Trim)
{
  QClipboard * Clipboard = QGuiApplication::clipboard();
  bool Result = Clipboard->mimeData()->hasText();
  if (Result)
  {
    // As upstream: large enough for all uses (URLs, keys, fingerprints,
    // permissions)
    const int Limit = 64 * 1024;
    Text = ToUnicodeString(Clipboard->text().left(Limit));
    if (Trim)
    {
      Text = Text.Trim();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void MinimizeApplication(bool ToTray)
{
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  if ((Manager != NULL) && (Manager->ScpExplorer != NULL))
  {
    Manager->ScpExplorer->MinimizeApplication(ToTray);
  }
}
//---------------------------------------------------------------------------
void RestoreApplication()
{
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  if ((Manager != NULL) && (Manager->ScpExplorer != NULL))
  {
    Manager->ScpExplorer->RestoreApplication();
  }
}
//---------------------------------------------------------------------------
bool IsSystemTrayAvailable()
{
  return TScpCommanderForm::IsSystemTrayAvailable();
}
//---------------------------------------------------------------------------
