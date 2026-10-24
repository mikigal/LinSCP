// The main window with the Commander interface.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <TerminalManager.h>

#include "ScpCommander.h"

#include <QLabel>
//---------------------------------------------------------------------------
TScpCommanderForm::TScpCommanderForm(QWidget * Parent) :
  QMainWindow(Parent)
{
  setObjectName(QStringLiteral("ScpCommanderForm"));
  resize(1000, 700);

  FNoSessionLabel = new QLabel(this);
  FNoSessionLabel->setObjectName(QStringLiteral("NoSessionLabel"));
  FNoSessionLabel->setAlignment(Qt::AlignCenter);
  setCentralWidget(FNoSessionLabel);

  TerminalChanged();
}
//---------------------------------------------------------------------------
TScpCommanderForm::~TScpCommanderForm()
{
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateCaption()
{
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  TTerminal * Terminal = (Manager != NULL) ? Manager->ActiveTerminal : NULL;
  UnicodeString SessionName = ((Terminal != NULL) && Terminal->Active) ? Terminal->SessionData->SessionName : UnicodeString();
  setWindowTitle(ToQString(FormatMainFormCaption(UnicodeString(), SessionName)));
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TerminalChanged()
{
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  TTerminal * Terminal = (Manager != NULL) ? Manager->ActiveTerminal : NULL;
  if ((Terminal != NULL) && Terminal->Active)
  {
    FNoSessionLabel->setText(ToQString(Terminal->SessionData->SessionName + L": " + Terminal->CurrentDirectory));
  }
  else
  {
    FNoSessionLabel->setText(tr("Not connected."));
  }
  UpdateCaption();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::NewSession()
{
  std::unique_ptr<TObjectList> DataList(new TObjectList());
  bool Connected = false;
  // As upstream on startup: the site manager again, when the session does
  // not open
  while (!Connected && DoLoginDialog(DataList.get(), this, false))
  {
    TTerminalManager * Manager = TTerminalManager::Instance();
    Manager->NewSession(static_cast<TSessionData *>(DataList->Items[0]));
    Connected = Manager->ConnectActiveTerminal();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OperationProgress(TFileOperationProgressType & /*ProgressData*/)
{
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OperationFinished(TFileOperation /*Operation*/, TOperationSide /*Side*/,
  bool /*Temp*/, const UnicodeString & /*FileName*/, bool /*Success*/, bool /*NotCancelled*/,
  TOnceDoneOperation & /*OnceDoneOperation*/)
{
}
//---------------------------------------------------------------------------
