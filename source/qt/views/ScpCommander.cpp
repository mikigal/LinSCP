// The main window with the Commander interface.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <TerminalManager.h>

#include "DirPanel.h"
#include "ScpCommander.h"

#include <QDir>
#include <QSplitter>
//---------------------------------------------------------------------------
TScpCommanderForm::TScpCommanderForm(QWidget * Parent) :
  QMainWindow(Parent)
{
  setObjectName(QStringLiteral("ScpCommanderForm"));
  resize(1000, 700);

  FPanelSplitter = new QSplitter(Qt::Horizontal, this);
  FPanelSplitter->setObjectName(QStringLiteral("PanelSplitter"));
  FLocalPanel = new TLocalDirPanel(FPanelSplitter);
  FRemotePanel = new TRemoteDirPanel(FPanelSplitter);
  FPanelSplitter->addWidget(FLocalPanel);
  FPanelSplitter->addWidget(FRemotePanel);
  setCentralWidget(FPanelSplitter);
  connect(FRemotePanel, &TRemoteDirPanel::NewSessionRequested, this, [this]() { NewSession(); });
  FLocalPanel->ChangeDirectory(ToUnicodeString(QDir::homePath()));

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
  FRemotePanel->SetTerminal((Manager != NULL) ? Manager->ActiveTerminal : NULL);
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
