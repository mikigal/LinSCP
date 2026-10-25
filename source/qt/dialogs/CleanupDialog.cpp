// The cleanup of the application data (upstream's forms/Cleanup.dfm): the
// configuration file (with the sites and the caches, as all is in the INI
// file on Linux), the random seed file, the temporary directories.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <PuttyTools.h>
#include <QtInterface.h>
#include <TextsWin.h>

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <functional>
#include <vector>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
class TCleanupDialog : public QDialog
{
public:
  TCleanupDialog();

  bool Execute();

private:
  typedef std::function<void()> TCleanupEvent;
  std::vector<TCleanupEvent> FCleanupEvents;
  std::unique_ptr<TStrings> FTemporaryFolders;
  QTreeWidget * DataListView;
  QPushButton * OKButton;

  void AddLocation(int CaptionId, const UnicodeString & Location, TCleanupEvent Event);
  void FindData();
  void UpdateControls();
};
//---------------------------------------------------------------------------
TCleanupDialog::TCleanupDialog() :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("CleanupDialog"));
  setWindowTitle(tr("Cleanup application data"));

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QLabel * Label1 = new QLabel(tr(
    "Following list contains all application data possibly stored on this computer. "
    "Check those you want to remove.\n\n"
    "If another instance of application is running, please close it before cleaning data.\n\n"
    "Notice: Opening session and/or next execution of application will recreate some of data."), this);
  Label1->setWordWrap(true);
  MainLayout->addWidget(Label1);
  DataListView = new QTreeWidget(this);
  DataListView->setObjectName(QStringLiteral("DataListView"));
  DataListView->setRootIsDecorated(false);
  DataListView->setHeaderLabels({ tr("Data"), tr("Location") });
  connect(DataListView, &QTreeWidget::itemChanged, this, [this]() { UpdateControls(); });
  MainLayout->addWidget(DataListView, 1);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Close, this);
  OKButton = Buttons->button(QDialogButtonBox::Ok);
  OKButton->setObjectName(QStringLiteral("OKButton"));
  Buttons->button(QDialogButtonBox::Close)->setObjectName(QStringLiteral("CancelButton"));
  QPushButton * CheckAllButton = Buttons->addButton(tr("Un/check &all"), QDialogButtonBox::ResetRole);
  CheckAllButton->setObjectName(QStringLiteral("CheckAllButton"));
  connect(CheckAllButton, &QPushButton::clicked, this, [this]()
  {
    // All checked unless all are checked already (upstream's caToggle)
    bool AllChecked = true;
    for (int Index = 0; Index < DataListView->topLevelItemCount(); Index++)
    {
      AllChecked = AllChecked && (DataListView->topLevelItem(Index)->checkState(0) == Qt::Checked);
    }
    for (int Index = 0; Index < DataListView->topLevelItemCount(); Index++)
    {
      DataListView->topLevelItem(Index)->setCheckState(0, AllChecked ? Qt::Unchecked : Qt::Checked);
    }
  });
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
  AddHelpButton(Buttons, FormHelpKeyword(L"Cleanup.dfm"));

  FindData();
  for (int Column = 0; Column < DataListView->columnCount(); Column++)
  {
    DataListView->resizeColumnToContents(Column);
  }
  UpdateControls();
  resize(fontMetrics().averageCharWidth() * 80, fontMetrics().height() * 22);
}
//---------------------------------------------------------------------------
void TCleanupDialog::AddLocation(int CaptionId, const UnicodeString & Location, TCleanupEvent Event)
{
  QTreeWidgetItem * Item = new QTreeWidgetItem(DataListView);
  Item->setText(0, ToQString(LoadStr(CaptionId)));
  Item->setText(1, ToQString(Location));
  Item->setToolTip(0, ToQString(FORMAT(L"%s\nLocation: %s", (LoadStr(CaptionId), Location))));
  Item->setFlags(Item->flags() | Qt::ItemIsUserCheckable);
  Item->setCheckState(0, Qt::Unchecked);
  FCleanupEvents.push_back(Event);
}
//---------------------------------------------------------------------------
void TCleanupDialog::FindData()
{
  // The INI file is the only storage (upstream's configuration, sites and
  // caches of the registry are in it); added unconditionally, as cleaning
  // it up stops the configuration from being saved
  UnicodeString IniFilePath = ExpandEnvironmentVariables(Configuration->IniFileStorageNameForReading);
  AddLocation(CLEANUP_INIFILE, IniFilePath, []() { Configuration->CleanupIniFile(); });
  if (RandomSeedExists())
  {
    AddLocation(CLEANUP_SEEDFILE, Configuration->RandomSeedFileName, []() { Configuration->CleanupRandomSeedFile(); });
  }
  FTemporaryFolders.reset(AppConfiguration->FindTemporaryFolders());
  if ((FTemporaryFolders.get() != NULL) && (FTemporaryFolders->Count > 0))
  {
    AddLocation(CLEANUP_TEMP_FOLDERS, AppConfiguration->TemporaryDir(true),
      [this]() { AppConfiguration->CleanupTemporaryFolders(FTemporaryFolders.get()); });
  }
}
//---------------------------------------------------------------------------
void TCleanupDialog::UpdateControls()
{
  bool AnyChecked = false;
  for (int Index = 0; Index < DataListView->topLevelItemCount(); Index++)
  {
    AnyChecked = AnyChecked || (DataListView->topLevelItem(Index)->checkState(0) == Qt::Checked);
  }
  OKButton->setEnabled(AnyChecked);
}
//---------------------------------------------------------------------------
bool TCleanupDialog::Execute()
{
  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    Configuration->Usage->Inc(L"Cleanups");
    for (int Index = 0; Index < DataListView->topLevelItemCount(); Index++)
    {
      if (DataListView->topLevelItem(Index)->checkState(0) == Qt::Checked)
      {
        try
        {
          FCleanupEvents[Index]();
        }
        catch (Exception & E)
        {
          ShowExtendedException(&E);
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoCleanupDialog()
{
  TCleanupDialog Dialog;
  return Dialog.Execute();
}
//---------------------------------------------------------------------------
