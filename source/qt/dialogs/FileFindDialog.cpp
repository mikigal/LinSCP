// The find of remote files (upstream's TFileFindDialog, forms/FileFind.dfm):
// a modeless window that lists the files matching a mask in a directory
// tree, and the operations on the files found (focus, download, delete,
// edit, copy of the list).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <FileMasks.h>
#include <Terminal.h>

#include "CustomDialog.h"
#include "DirModel.h"
#include "HistoryComboBox.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <map>
//---------------------------------------------------------------------------
class TFileFindDialog : public QDialog
{
public:
  TFileFindDialog();
  virtual ~TFileFindDialog();

  void Init(
    TTerminal * Terminal, UnicodeString Directory, TFindEvent OnFind, TFocusFileEvent OnFocusFile,
    TFileListOperationEvent OnDeleteFiles, TFileListOperationEvent OnDownloadFiles,
    TFileListOperationEvent OnEditFiles);

protected:
  virtual void closeEvent(QCloseEvent * Event) override;
  virtual void keyPressEvent(QKeyEvent * Event) override;
  virtual void reject() override;

private:
  enum { ffInit, ffFinding, ffAborting, ffAborted, ffDone } FState;
  TTerminal * FTerminal;
  UnicodeString FTerminalName;
  UnicodeString FFindingInDirectory;
  UnicodeString FDirectory;
  TFindEvent FOnFind;
  TFocusFileEvent FOnFocusFile;
  TFileListOperationEvent FOnDeleteFiles;
  TFileListOperationEvent FOnDownloadFiles;
  TFileListOperationEvent FOnEditFiles;
  UnicodeString FFocusPath;
  typedef std::map<UnicodeString, QTreeWidgetItem *> TFileItemMap;
  TFileItemMap FFileItemMap;
  bool FClosePending;

  QGroupBox * FilterGroup;
  THistoryComboBox * MaskEdit;
  THistoryComboBox * RemoteDirectoryEdit;
  QPushButton * StartStopButton;
  QTreeWidget * FileView2;
  QStatusBar * StatusBar;
  QPushButton * FocusButton;
  QPushButton * EditButton;
  QPushButton * DownloadButton;
  QPushButton * DeleteButton;
  QPushButton * CopyButton;
  QAction * FocusAction;
  QAction * EditAction;
  QAction * DownloadAction;
  QAction * DeleteAction;
  QAction * SelectAllAction;
  QAction * CopyAction;

  QAction * CreateAction(const char * Name, const QString & Caption, const QKeySequence & ShortCut,
    std::function<void()> Execute);
  QPushButton * CreateButton(const char * Name, QAction * Action);
  void Clear();
  void Start();
  void Stop();
  bool StopIfFinding();
  void UpdateControls();
  bool IsFinding();
  void StartStopButtonClick();
  void FileFound(TTerminal * Terminal, const UnicodeString FileName, const TRemoteFile * File, bool & Cancel);
  void FindingFile(TTerminal * Terminal, const UnicodeString Directory, bool & Cancel);
  void CopyToClipboard();
  void FocusFile();
  void DoFocusFile(const UnicodeString & Path);
  static TRemoteFile * ItemFile(QTreeWidgetItem * Item);
  void ClearItem(QTreeWidgetItem * Item);
  void FileDeleteFinished(TOperationSide Side, const UnicodeString & FileName, bool Success, bool NotCancelled);
  void FileDownloadFinished(TOperationSide Side, const UnicodeString & FileName, bool Success, bool NotCancelled);
  QTreeWidgetItem * FileOperationFinished(const UnicodeString & FileName);
  void FileListOperation(TFileListOperationEvent Operation, TFileOperationFinishedEvent OnFileOperationFinished);
};
//---------------------------------------------------------------------------
static TFileFindDialog * FileFindDialog = NULL;
//---------------------------------------------------------------------------
void ShowFileFindDialog(
  TTerminal * Terminal, UnicodeString Directory, TFindEvent OnFind, TFocusFileEvent OnFocusFile,
  TFileListOperationEvent OnDeleteFiles, TFileListOperationEvent OnDownloadFiles,
  TFileListOperationEvent OnEditFiles)
{
  if (FileFindDialog == NULL)
  {
    FileFindDialog = new TFileFindDialog();
  }
  FileFindDialog->Init(Terminal, Directory, OnFind, OnFocusFile, OnDeleteFiles, OnDownloadFiles, OnEditFiles);
  FileFindDialog->show();
  FileFindDialog->raise();
  FileFindDialog->activateWindow();
}
//---------------------------------------------------------------------------
void HideFileFindDialog()
{
  delete FileFindDialog;
}
//---------------------------------------------------------------------------
TFileFindDialog::TFileFindDialog() :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("FileFindDialog"));
  setAttribute(Qt::WA_DeleteOnClose);
  setSizeGripEnabled(true);
  FState = ffInit;
  FTerminal = NULL;
  FClosePending = false;

  FocusAction = CreateAction("FocusAction", tr("Fo&cus"), QKeySequence(), [this]() { FocusFile(); });
  EditAction = CreateAction("EditAction", tr("Edi&t"), QKeySequence(),
    [this]() { FileListOperation(FOnEditFiles, [this](TOperationSide Side, const UnicodeString & FileName, bool Success, bool NotCancelled) { FileDownloadFinished(Side, FileName, Success, NotCancelled); }); });
  DownloadAction = CreateAction("DownloadAction", tr("Down&load..."), QKeySequence(),
    [this]() { FileListOperation(FOnDownloadFiles, [this](TOperationSide Side, const UnicodeString & FileName, bool Success, bool NotCancelled) { FileDownloadFinished(Side, FileName, Success, NotCancelled); }); });
  DeleteAction = CreateAction("DeleteAction", tr("&Delete"), QKeySequence(Qt::Key_Delete),
    [this]() { FileListOperation(FOnDeleteFiles, [this](TOperationSide Side, const UnicodeString & FileName, bool Success, bool NotCancelled) { FileDeleteFinished(Side, FileName, Success, NotCancelled); }); });
  SelectAllAction = CreateAction("SelectAllAction", tr("Select &All"), QKeySequence(Qt::CTRL | Qt::Key_A), [this]() { FileView2->selectAll(); });
  CopyAction = CreateAction("CopyAction", tr("&Copy Results"), QKeySequence(Qt::CTRL | Qt::Key_C), [this]() { CopyToClipboard(); });

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QHBoxLayout * TopLayout = new QHBoxLayout();
  FilterGroup = new QGroupBox(tr("Filter"), this);
  FilterGroup->setObjectName(QStringLiteral("FilterGroup"));
  QGridLayout * FilterLayout = new QGridLayout(FilterGroup);
  QLabel * MaskLabel = new QLabel(tr("&File mask:"), FilterGroup);
  MaskLabel->setObjectName(QStringLiteral("MaskLabel"));
  FilterLayout->addWidget(MaskLabel, 0, 0);
  MaskEdit = new THistoryComboBox(FilterGroup);
  MaskEdit->setObjectName(QStringLiteral("MaskEdit"));
  MaskLabel->setBuddy(MaskEdit);
  FilterLayout->addWidget(MaskEdit, 0, 1);
  QPushButton * MaskButton = new QPushButton(tr("&Edit..."), FilterGroup);
  MaskButton->setObjectName(QStringLiteral("MaskButton"));
  MaskButton->setAutoDefault(false);
  connect(MaskButton, &QPushButton::clicked, this, [this]()
  {
    TFileMasks Masks = MaskEdit->GetText();
    if (DoEditMaskDialog(Masks))
    {
      MaskEdit->SetText(Masks.Masks);
    }
  });
  FilterLayout->addWidget(MaskButton, 0, 2);
  QLabel * MaskHintText = new QLabel(QStringLiteral("<a href=\"#\">%1</a>").arg(tr("mask hints")), FilterGroup);
  MaskHintText->setObjectName(QStringLiteral("MaskHintText"));
  MaskHintText->setToolTip(ToQString(
    FORMAT(L"%s\n \n%s\n \n%s\n \n%s\n \n%s\n \n%s", (LoadStr(MASK_HINT2),
      LoadStr(FILE_MASK_EX_HINT), LoadStr(COMBINING_MASKS_HINT),
      LoadStr(PATH_MASK_HINT2), LoadStr(DIRECTORY_MASK_HINT),
      LoadStr(MASK_HELP)))));
  MaskHintText->setAlignment(Qt::AlignRight);
  FilterLayout->addWidget(MaskHintText, 1, 1);
  QLabel * RemoteDirectoryLabel = new QLabel(tr("Sear&ch in:"), FilterGroup);
  RemoteDirectoryLabel->setObjectName(QStringLiteral("RemoteDirectoryLabel"));
  FilterLayout->addWidget(RemoteDirectoryLabel, 2, 0);
  RemoteDirectoryEdit = new THistoryComboBox(FilterGroup);
  RemoteDirectoryEdit->setObjectName(QStringLiteral("RemoteDirectoryEdit"));
  RemoteDirectoryLabel->setBuddy(RemoteDirectoryEdit);
  connect(RemoteDirectoryEdit, &QComboBox::currentTextChanged, this, [this]() { UpdateControls(); });
  FilterLayout->addWidget(RemoteDirectoryEdit, 2, 1, 1, 2);
  FilterLayout->setColumnStretch(1, 1);
  TopLayout->addWidget(FilterGroup, 1);

  QVBoxLayout * StartLayout = new QVBoxLayout();
  StartStopButton = new QPushButton(this);
  StartStopButton->setObjectName(QStringLiteral("StartStopButton"));
  StartStopButton->setDefault(true);
  connect(StartStopButton, &QPushButton::clicked, this, [this]() { StartStopButtonClick(); });
  StartLayout->addWidget(StartStopButton);
  QPushButton * HelpButton = new QPushButton(tr("&Help"), this);
  HelpButton->setObjectName(QStringLiteral("HelpButton"));
  HelpButton->setAutoDefault(false);
  connect(HelpButton, &QPushButton::clicked, this, []() { ShowHelp(FormHelpKeyword(L"FileFind.dfm")); });
  StartLayout->addWidget(HelpButton);
  StartLayout->addStretch(1);
  TopLayout->addLayout(StartLayout);
  MainLayout->addLayout(TopLayout);

  QHBoxLayout * FilesLayout = new QHBoxLayout();
  FileView2 = new QTreeWidget(this);
  FileView2->setObjectName(QStringLiteral("FileView2"));
  FileView2->setRootIsDecorated(false);
  FileView2->setUniformRowHeights(true);
  FileView2->setSelectionMode(QAbstractItemView::ExtendedSelection);
  FileView2->setHeaderLabels({ tr("Name"), tr("Directory"), tr("Size"), tr("Date modified") });
  FileView2->setColumnWidth(0, 130);
  FileView2->setColumnWidth(1, 200);
  FileView2->setContextMenuPolicy(Qt::ActionsContextMenu);
  for (QAction * Action : { FocusAction, EditAction, DownloadAction, DeleteAction, SelectAllAction, CopyAction })
  {
    FileView2->addAction(Action);
  }
  connect(FileView2, &QTreeWidget::itemSelectionChanged, this, [this]() { UpdateControls(); });
  connect(FileView2, &QTreeWidget::currentItemChanged, this, [this]() { UpdateControls(); });
  connect(FileView2, &QTreeWidget::itemActivated, this, [this]() { FocusFile(); });
  FilesLayout->addWidget(FileView2, 1);
  QVBoxLayout * ButtonsLayout = new QVBoxLayout();
  FocusButton = CreateButton("FocusButton", FocusAction);
  EditButton = CreateButton("EditButton", EditAction);
  DownloadButton = CreateButton("DownloadButton", DownloadAction);
  DeleteButton = CreateButton("DeleteButton", DeleteAction);
  CopyButton = CreateButton("CopyButton", CopyAction);
  for (QPushButton * Button : { FocusButton, EditButton, DownloadButton, DeleteButton, CopyButton })
  {
    ButtonsLayout->addWidget(Button);
  }
  ButtonsLayout->addStretch(1);
  FilesLayout->addLayout(ButtonsLayout);
  MainLayout->addLayout(FilesLayout, 1);

  StatusBar = new QStatusBar(this);
  StatusBar->setObjectName(QStringLiteral("StatusBar"));
  StatusBar->setSizeGripEnabled(false);
  MainLayout->addWidget(StatusBar);
  resize(fontMetrics().averageCharWidth() * 100, fontMetrics().lineSpacing() * 30);
}
//---------------------------------------------------------------------------
TFileFindDialog::~TFileFindDialog()
{
  Clear();
  DebugAssert(FileFindDialog == this);
  FileFindDialog = NULL;
}
//---------------------------------------------------------------------------
QAction * TFileFindDialog::CreateAction(const char * Name, const QString & Caption, const QKeySequence & ShortCut,
  std::function<void()> Execute)
{
  QAction * Action = new QAction(Caption, this);
  Action->setObjectName(QString::fromLatin1(Name));
  Action->setShortcut(ShortCut);
  Action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  connect(Action, &QAction::triggered, this, Execute);
  return Action;
}
//---------------------------------------------------------------------------
QPushButton * TFileFindDialog::CreateButton(const char * Name, QAction * Action)
{
  QPushButton * Button = new QPushButton(Action->text(), this);
  Button->setObjectName(QString::fromLatin1(Name));
  Button->setAutoDefault(false);
  connect(Button, &QPushButton::clicked, Action, &QAction::trigger);
  connect(Action, &QAction::enabledChanged, Button, &QPushButton::setEnabled);
  connect(Action, &QAction::visibleChanged, Button, [Button, Action]() { Button->setVisible(Action->isVisible()); });
  return Button;
}
//---------------------------------------------------------------------------
bool TFileFindDialog::IsFinding()
{
  return (FState == ffFinding) || (FState == ffAborting);
}
//---------------------------------------------------------------------------
void TFileFindDialog::UpdateControls()
{
  bool Finding = IsFinding();
  setWindowTitle(ToQString(LoadStr(Finding ? FIND_FILE_FINDING : FIND_FILE_TITLE) + TitleSeparator + FTerminalName));
  UnicodeString StartStopCaption;
  if (Finding)
  {
    StartStopButton->setEnabled(true);
    StartStopCaption = LoadStr(FIND_FILE_STOP);
  }
  else
  {
    StartStopButton->setEnabled(!RemoteDirectoryEdit->currentText().isEmpty());
    StartStopCaption = LoadStr(FIND_FILE_START);
  }
  StartStopButton->setText(ToQString(StartStopCaption));
  FilterGroup->setEnabled(!Finding);
  FocusAction->setEnabled(FileView2->currentItem() != NULL);
  int SelCount = FileView2->selectedItems().count();
  bool EnableFileOperations = !Finding && (SelCount > 0);
  DeleteAction->setEnabled(EnableFileOperations && static_cast<bool>(FOnDeleteFiles));
  DownloadAction->setEnabled(EnableFileOperations && static_cast<bool>(FOnDownloadFiles));
  EditAction->setVisible(static_cast<bool>(FOnEditFiles));
  EditAction->setEnabled(EnableFileOperations && static_cast<bool>(FOnEditFiles));
  CopyAction->setEnabled(FileView2->topLevelItemCount() > 0);
  SelectAllAction->setEnabled(SelCount < FileView2->topLevelItemCount());

  switch (FState)
  {
    case ffInit:
      StatusBar->clearMessage();
      break;

    case ffFinding:
    case ffAborting:
      if (!FFindingInDirectory.IsEmpty())
      {
        StatusBar->showMessage(ToQString(FMTLOAD(FIND_FILE_IN_DIRECTORY, (FFindingInDirectory))));
      }
      else
      {
        StatusBar->clearMessage();
      }
      break;

    case ffAborted:
      StatusBar->showMessage(ToQString(LoadStr(FIND_FILE_ABORTED)));
      break;

    case ffDone:
      StatusBar->showMessage(ToQString(LoadStr(FIND_FILE_DONE)));
      break;
  }

  bool FocusDefault = FileView2->hasFocus() && (FState != ffInit);
  FocusButton->setDefault(FocusDefault);
  StartStopButton->setDefault(!FocusDefault);
}
//---------------------------------------------------------------------------
void TFileFindDialog::Init(
  TTerminal * Terminal, UnicodeString Directory, TFindEvent OnFind, TFocusFileEvent OnFocusFile,
  TFileListOperationEvent OnDeleteFiles, TFileListOperationEvent OnDownloadFiles,
  TFileListOperationEvent OnEditFiles)
{
  if (FTerminal != Terminal)
  {
    FTerminal = Terminal;
    FTerminalName = Terminal->SessionData->SessionName;
    Clear();
    FState = ffInit;
    MaskEdit->setFocus();
  }

  FOnFind = OnFind;
  FOnFocusFile = OnFocusFile;
  FOnDeleteFiles = OnDeleteFiles;
  FOnDownloadFiles = OnDownloadFiles;
  FOnEditFiles = OnEditFiles;

  MaskEdit->SetHistoryKey(L"Mask");
  RemoteDirectoryEdit->SetHistoryKey(L"RemoteDirectory");
  MaskEdit->SetText(AppConfiguration->SelectMask);
  RemoteDirectoryEdit->SetText(UnixExcludeTrailingBackslash(Directory));
  UpdateControls();
}
//---------------------------------------------------------------------------
TRemoteFile * TFileFindDialog::ItemFile(QTreeWidgetItem * Item)
{
  return static_cast<TRemoteFile *>(Item->data(0, Qt::UserRole).value<void *>());
}
//---------------------------------------------------------------------------
void TFileFindDialog::ClearItem(QTreeWidgetItem * Item)
{
  TRemoteFile * File = ItemFile(Item);
  Item->setData(0, Qt::UserRole, QVariant());
  delete File;
}
//---------------------------------------------------------------------------
void TFileFindDialog::Clear()
{
  for (int Index = 0; Index < FileView2->topLevelItemCount(); Index++)
  {
    ClearItem(FileView2->topLevelItem(Index));
  }

  FileView2->clear();
}
//---------------------------------------------------------------------------
void TFileFindDialog::Start()
{
  try
  {
    ValidateMaskEdit(MaskEdit);
  }
  catch (EAbort &)
  {
    return;
  }

  RemoteDirectoryEdit->SaveToHistory();
  MaskEdit->SaveToHistory();
  AppConfiguration->SelectMask = MaskEdit->GetText();

  DebugAssert(FState != ffFinding);

  FState = ffFinding;
  FClosePending = false;
  {
    ::rtl::TFinally Finally([&]()
    {
      FFindingInDirectory = L"";
      if (FState == ffFinding)
      {
        FState = ffDone;
      }
      if (FState == ffAborting)
      {
        FState = ffAborted;
      }

      if (!FFocusPath.IsEmpty())
      {
        UnicodeString FocusPath = FFocusPath;
        FFocusPath = L"";
        DoFocusFile(FocusPath);
      }

      UpdateControls();
    });
    UpdateControls();

    DebugAssert(FOnFind);
    UnicodeString Directory = UnixExcludeTrailingBackslash(RemoteDirectoryEdit->GetText());

    FDirectory = Directory;
    if (FDirectory == ROOTDIRECTORY)
    {
      FDirectory = UnicodeString();
    }

    FOnFind(FTerminal, Directory, MaskEdit->GetText(),
      ::rtl::MethodPtr(this, &TFileFindDialog::FileFound), ::rtl::MethodPtr(this, &TFileFindDialog::FindingFile));
  }

  if (FClosePending)
  {
    FClosePending = false;
    close();
  }
}
//---------------------------------------------------------------------------
void TFileFindDialog::FileFound(TTerminal * /*Terminal*/,
  const UnicodeString /*FileName*/, const TRemoteFile * AFile, bool & Cancel)
{
  QTreeWidgetItem * Item = new QTreeWidgetItem();
  TRemoteFile * File = AFile->Duplicate(true);
  Item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void *>(File)));

  UnicodeString Caption = File->FileName;
  if (File->IsDirectory)
  {
    Caption = UnixIncludeTrailingBackslash(Caption);
  }
  Item->setText(0, ToQString(Caption));
  QString Type;
  QIcon Icon;
  FileTypeAndIcon(ToQString(File->FileName), File->IsDirectory, Type, Icon);
  Item->setIcon(0, Icon);

  UnicodeString Directory = UnixExtractFilePath(File->FullFileName);
  if (AnsiSameText(FDirectory, Directory.SubString(1, FDirectory.Length())))
  {
    Directory = L"." + Directory.SubString(FDirectory.Length() + 1, Directory.Length() - FDirectory.Length());
  }
  else
  {
    DebugFail();
  }
  Item->setText(1, ToQString(Directory));

  if (!File->IsDirectory)
  {
    Item->setText(2, ToQString(FormatPanelBytes(File->Size, fbKilobytes)));
    Item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
  }
  Item->setText(3, ToQString(UserModificationStr(File->Modification, File->ModificationFmt)));
  FileView2->addTopLevelItem(Item);

  UpdateControls();
  Cancel = (FState == ffAborting);
  QCoreApplication::processEvents();
}
//---------------------------------------------------------------------------
void TFileFindDialog::FindingFile(TTerminal * /*Terminal*/, const UnicodeString Directory, bool & Cancel)
{
  if (!Directory.IsEmpty() && (FFindingInDirectory != Directory))
  {
    FFindingInDirectory = Directory;
    UpdateControls();
  }

  Cancel = (FState == ffAborting);
  QCoreApplication::processEvents();
}
//---------------------------------------------------------------------------
void TFileFindDialog::StartStopButtonClick()
{
  if (IsFinding())
  {
    Stop();
  }
  else
  {
    Clear();
    if (FilterGroup->isAncestorOf(focusWidget()))
    {
      FileView2->setFocus();
    }
    Start();
  }
}
//---------------------------------------------------------------------------
void TFileFindDialog::Stop()
{
  FState = ffAborting;
  UpdateControls();
}
//---------------------------------------------------------------------------
bool TFileFindDialog::StopIfFinding()
{
  bool Result = IsFinding();
  if (Result)
  {
    Stop();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFileFindDialog::closeEvent(QCloseEvent * Event)
{
  if (StopIfFinding())
  {
    // Closed once the find returns
    FClosePending = true;
    Event->ignore();
  }
  else
  {
    QDialog::closeEvent(Event);
  }
}
//---------------------------------------------------------------------------
void TFileFindDialog::reject()
{
  // Escape stops the find first
  if (!StopIfFinding())
  {
    QDialog::reject();
  }
}
//---------------------------------------------------------------------------
void TFileFindDialog::keyPressEvent(QKeyEvent * Event)
{
  if ((Event->key() == Qt::Key_F10) && (Event->modifiers() == Qt::NoModifier))
  {
    StopIfFinding();
    close();
  }
  else
  {
    QDialog::keyPressEvent(Event);
  }
}
//---------------------------------------------------------------------------
void TFileFindDialog::DoFocusFile(const UnicodeString & Path)
{
  FOnFocusFile(FTerminal, Path);
}
//---------------------------------------------------------------------------
void TFileFindDialog::FocusFile()
{
  if (FileView2->currentItem() == NULL)
  {
    return;
  }
  UnicodeString Path = ItemFile(FileView2->currentItem())->FullFileName;
  // To make focussing directories work,
  // otherwise it would try to focus "empty-named file" in the Path
  Path = UnixExcludeTrailingBackslash(Path);

  if (StopIfFinding())
  {
    FFocusPath = Path;
  }
  else
  {
    DoFocusFile(Path);
  }
}
//---------------------------------------------------------------------------
void TFileFindDialog::CopyToClipboard()
{
  QStringList Lines;
  for (int Index = 0; Index < FileView2->topLevelItemCount(); Index++)
  {
    Lines.append(ToQString(ItemFile(FileView2->topLevelItem(Index))->FullFileName));
  }
  QApplication::clipboard()->setText(Lines.join(QLatin1Char('\n')) + QLatin1Char('\n'));
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TFileFindDialog::FileOperationFinished(const UnicodeString & FileName)
{
  TFileItemMap::iterator I = FFileItemMap.find(FileName);

  QTreeWidgetItem * Result = NULL;
  if (DebugAlwaysTrue(I != FFileItemMap.end()))
  {
    Result = I->second;
    FileView2->scrollToItem(Result);
    FFileItemMap.erase(I);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFileFindDialog::FileDeleteFinished(
  TOperationSide, const UnicodeString & FileName, bool Success, bool NotCancelled)
{
  DebugUsedParam(NotCancelled);
  if (FileName.IsEmpty())
  {
    DebugAssert(Success && NotCancelled);
    FileView2->clearSelection();
  }
  else
  {
    QTreeWidgetItem * Item = FileOperationFinished(FileName);
    if ((Item != NULL) && Success)
    {
      ClearItem(Item);
      delete Item;
    }
  }
}
//---------------------------------------------------------------------------
void TFileFindDialog::FileDownloadFinished(
  TOperationSide, const UnicodeString & FileName, bool Success, bool NotCancelled)
{
  if (FileName.IsEmpty())
  {
    DebugAssert(Success && NotCancelled);
    // Moved to queue, see call in TScpCommanderForm::CopyParamDialog
    FileView2->clearSelection();
  }
  else
  {
    QTreeWidgetItem * Item = FileOperationFinished(FileName);
    if ((Item != NULL) && Success && NotCancelled)
    {
      Item->setSelected(false);
    }
  }
}
//---------------------------------------------------------------------------
void TFileFindDialog::FileListOperation(
  TFileListOperationEvent Operation, TFileOperationFinishedEvent OnFileOperationFinished)
{
  std::unique_ptr<TStrings> FileList(new TStringList());

  DebugAssert(FFileItemMap.empty());
  for (int Index = 0; Index < FileView2->topLevelItemCount(); Index++)
  {
    QTreeWidgetItem * Item = FileView2->topLevelItem(Index);
    if (Item->isSelected())
    {
      TRemoteFile * File = ItemFile(Item);
      FileList->AddObject(File->FullFileName, File);
      FFileItemMap.insert(std::make_pair(File->FullFileName, Item));
    }
  }

  {
    // can be non-empty only when not all files were processed
    ::rtl::TFinally Finally([&]() { FFileItemMap.clear(); });
    Operation(FTerminal, FileList.get(), OnFileOperationFinished);
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
