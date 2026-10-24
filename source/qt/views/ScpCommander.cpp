// The main window with the Commander interface.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include "ScpCommander.h"

#include <QLabel>
//---------------------------------------------------------------------------
TScpCommanderForm::TScpCommanderForm(QWidget * Parent) :
  QMainWindow(Parent)
{
  setObjectName(QStringLiteral("ScpCommanderForm"));
  resize(1000, 700);

  QLabel * Placeholder = new QLabel(this);
  Placeholder->setObjectName(QStringLiteral("NoSessionLabel"));
  Placeholder->setAlignment(Qt::AlignCenter);
  Placeholder->setText(tr("Not connected."));
  setCentralWidget(Placeholder);

  UpdateCaption();
}
//---------------------------------------------------------------------------
TScpCommanderForm::~TScpCommanderForm()
{
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateCaption()
{
  setWindowTitle(ToQString(FormatMainFormCaption(UnicodeString())));
}
//---------------------------------------------------------------------------
