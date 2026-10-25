// The internal editor (upstream's TEditorForm, forms/Editor.dfm): a text
// editor of local files, also of the temporary copies of remote files,
// whose saves the editor manager uploads.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <AppInterface.h>
#include <QtInterface.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QFile>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStatusBar>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QTextBlock>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// The encodings: UTF-8 and the other one (upstream's ANSI code page; here
// ISO-8859-1, which decodes any file)
enum TEncodingKind { ekNone, ekUtf8, ekOther };
const QString OtherEncodingName(QStringLiteral("ISO-8859-1"));
const QByteArray Utf8Preamble("\xEF\xBB\xBF");
//---------------------------------------------------------------------------
class TEditorForm;
//---------------------------------------------------------------------------
// The find and replace dialog (upstream uses the dialogs of Windows)
class TFindDialog : public QDialog
{
public:
  TFindDialog(TEditorForm * Editor, bool Replace);

  QString FindText() const { return FindTextEdit->text(); }
  void SetFindText(const QString & Text) { FindTextEdit->setText(Text); }
  QString ReplaceText() const { return (ReplaceTextEdit != nullptr) ? ReplaceTextEdit->text() : QString(); }
  void SetReplaceText(const QString & Text);
  bool MatchCase() const { return MatchCaseCheck->isChecked(); }
  bool WholeWord() const { return WholeWordCheck->isChecked(); }
  bool Down() const { return DownButton->isChecked(); }
  void SetOptions(bool AMatchCase, bool AWholeWord, bool ADown);
  bool IsReplace() const { return (ReplaceTextEdit != nullptr); }

  // What the last button asked for (upstream's frReplace and frReplaceAll)
  bool FReplaceRequested;
  bool FReplaceAllRequested;

private:
  TEditorForm * FEditor;
  QLineEdit * FindTextEdit;
  QLineEdit * ReplaceTextEdit;
  QCheckBox * MatchCaseCheck;
  QCheckBox * WholeWordCheck;
  QRadioButton * UpButton;
  QRadioButton * DownButton;
  QPushButton * FindNextButton;

  void Submit(bool Replace, bool ReplaceAll);
};
//---------------------------------------------------------------------------
class TEditorForm : public QMainWindow
{
public:
  TEditorForm();
  virtual ~TEditorForm();

  void SetFileName(const UnicodeString & Value) { FFileName = Value; }
  void LoadFile();
  void SaveFile();
  bool IsFileModified();
  void FileUploadComplete();
  void Find();

  TEditorEvent FOnFileChanged;
  TEditorEvent FOnFileReload;
  TEditorEvent FOnSaveAll;
  TAnyModifiedEvent FOnAnyModified;
  TEditorClosedEvent FOnWindowClose;
  bool FNewFile;

protected:
  virtual void closeEvent(QCloseEvent * Event) override;
  virtual void keyPressEvent(QKeyEvent * Event) override;

private:
  UnicodeString FFileName;
  TEncodingKind FEncoding;
  QString FEncodingName;
  bool FLoadedWithPreamble;
  bool FCrLf;
  bool FSaving;
  bool FReloading;
  bool FCloseAnnounced;
  TFindDialog * FFindDialog;
  TFindDialog * FReplaceDialog;
  TFindDialog * FLastFindDialog;

  QPlainTextEdit * EditorMemo;
  QAction * SaveAction;
  QAction * SaveAllAction2;
  QAction * ReloadAction;
  QAction * EditUndo;
  QAction * EditRedo;
  QAction * EditCut;
  QAction * EditCopy;
  QAction * EditPaste;
  QAction * EditDelete;
  QAction * EditSelectAll;
  QAction * FindAction;
  QAction * ReplaceAction;
  QAction * FindNextAction;
  QAction * GoToLineAction;
  QAction * DefaultEncodingAction;
  QAction * UTF8EncodingAction;
  QMenu * EncodingMenu;
  QLabel * LineStatus;
  QLabel * ColumnStatus;
  QLabel * CharacterStatus;
  QLabel * EncodingStatus;
  QLabel * ModifiedStatus;

  QAction * CreateAction(const char * Name, const QString & Caption, const QString & IconName,
    const QKeySequence & ShortCut);
  void ApplyConfiguration();
  void UpdateControls();
  void CheckFileSize();
  void LoadFromFile(bool PrimaryEncoding);
  bool Decode(const QByteArray & Data, TEncodingKind Encoding, QString & Text);
  void SaveToFile();
  void BackupSave();
  void ChangeEncoding(TEncodingKind Encoding);
  void Reload();
  void StartFind(bool AFind);
  void GoToLine();
  void DoWindowClose(bool Forced);
};
//---------------------------------------------------------------------------
TFindDialog::TFindDialog(TEditorForm * Editor, bool Replace) :
  QDialog(Editor)
{
  FEditor = Editor;
  FReplaceRequested = false;
  FReplaceAllRequested = false;
  setObjectName(Replace ? QStringLiteral("ReplaceDialog") : QStringLiteral("FindDialog"));
  setWindowTitle(Replace ? tr("Replace") : tr("Find"));
  setModal(false);

  QHBoxLayout * MainLayout = new QHBoxLayout(this);
  QVBoxLayout * FieldsLayout = new QVBoxLayout();
  QGridLayout * Grid = new QGridLayout();
  QLabel * FindLabel = new QLabel(tr("Fi&nd what:"), this);
  Grid->addWidget(FindLabel, 0, 0);
  FindTextEdit = new QLineEdit(this);
  FindTextEdit->setObjectName(QStringLiteral("FindTextEdit"));
  FindTextEdit->setMinimumWidth(fontMetrics().averageCharWidth() * 35);
  FindLabel->setBuddy(FindTextEdit);
  Grid->addWidget(FindTextEdit, 0, 1);
  ReplaceTextEdit = nullptr;
  if (Replace)
  {
    QLabel * ReplaceLabel = new QLabel(tr("Re&place with:"), this);
    Grid->addWidget(ReplaceLabel, 1, 0);
    ReplaceTextEdit = new QLineEdit(this);
    ReplaceTextEdit->setObjectName(QStringLiteral("ReplaceTextEdit"));
    ReplaceLabel->setBuddy(ReplaceTextEdit);
    Grid->addWidget(ReplaceTextEdit, 1, 1);
  }
  FieldsLayout->addLayout(Grid);
  QHBoxLayout * OptionsLayout = new QHBoxLayout();
  QVBoxLayout * ChecksLayout = new QVBoxLayout();
  WholeWordCheck = new QCheckBox(tr("Match &whole word only"), this);
  WholeWordCheck->setObjectName(QStringLiteral("WholeWordCheck"));
  ChecksLayout->addWidget(WholeWordCheck);
  MatchCaseCheck = new QCheckBox(tr("Match &case"), this);
  MatchCaseCheck->setObjectName(QStringLiteral("MatchCaseCheck"));
  ChecksLayout->addWidget(MatchCaseCheck);
  OptionsLayout->addLayout(ChecksLayout);
  QGroupBox * DirectionGroup = new QGroupBox(tr("Direction"), this);
  QHBoxLayout * DirectionLayout = new QHBoxLayout(DirectionGroup);
  UpButton = new QRadioButton(tr("&Up"), DirectionGroup);
  UpButton->setObjectName(QStringLiteral("UpButton"));
  DirectionLayout->addWidget(UpButton);
  DownButton = new QRadioButton(tr("&Down"), DirectionGroup);
  DownButton->setObjectName(QStringLiteral("DownButton"));
  DownButton->setChecked(true);
  DirectionLayout->addWidget(DownButton);
  DirectionGroup->setVisible(!Replace);
  OptionsLayout->addWidget(DirectionGroup);
  FieldsLayout->addLayout(OptionsLayout);
  MainLayout->addLayout(FieldsLayout, 1);

  QVBoxLayout * ButtonsLayout = new QVBoxLayout();
  FindNextButton = new QPushButton(tr("&Find Next"), this);
  FindNextButton->setObjectName(QStringLiteral("FindNextButton"));
  FindNextButton->setDefault(true);
  connect(FindNextButton, &QPushButton::clicked, this, [this]() { Submit(false, false); });
  ButtonsLayout->addWidget(FindNextButton);
  if (Replace)
  {
    QPushButton * ReplaceButton = new QPushButton(tr("&Replace"), this);
    ReplaceButton->setObjectName(QStringLiteral("ReplaceButton"));
    ReplaceButton->setAutoDefault(false);
    connect(ReplaceButton, &QPushButton::clicked, this, [this]() { Submit(true, false); });
    ButtonsLayout->addWidget(ReplaceButton);
    QPushButton * ReplaceAllButton = new QPushButton(tr("Replace &All"), this);
    ReplaceAllButton->setObjectName(QStringLiteral("ReplaceAllButton"));
    ReplaceAllButton->setAutoDefault(false);
    connect(ReplaceAllButton, &QPushButton::clicked, this, [this]() { Submit(false, true); });
    ButtonsLayout->addWidget(ReplaceAllButton);
  }
  QPushButton * CancelButton = new QPushButton(tr("Cancel"), this);
  CancelButton->setObjectName(QStringLiteral("CancelButton"));
  CancelButton->setAutoDefault(false);
  connect(CancelButton, &QPushButton::clicked, this, &QDialog::close);
  ButtonsLayout->addWidget(CancelButton);
  ButtonsLayout->addStretch(1);
  MainLayout->addLayout(ButtonsLayout);

  connect(FindTextEdit, &QLineEdit::textChanged, this, [this]() { FindNextButton->setEnabled(!FindTextEdit->text().isEmpty()); });
  FindNextButton->setEnabled(false);
}
//---------------------------------------------------------------------------
void TFindDialog::SetReplaceText(const QString & Text)
{
  if (ReplaceTextEdit != nullptr)
  {
    ReplaceTextEdit->setText(Text);
  }
}
//---------------------------------------------------------------------------
void TFindDialog::SetOptions(bool AMatchCase, bool AWholeWord, bool ADown)
{
  MatchCaseCheck->setChecked(AMatchCase);
  WholeWordCheck->setChecked(AWholeWord);
  (ADown ? DownButton : UpButton)->setChecked(true);
}
//---------------------------------------------------------------------------
void TFindDialog::Submit(bool Replace, bool ReplaceAll)
{
  FReplaceRequested = Replace;
  FReplaceAllRequested = ReplaceAll;
  FEditor->Find();
}
//---------------------------------------------------------------------------
TEditorForm::TEditorForm() :
  QMainWindow(nullptr)
{
  setObjectName(QStringLiteral("EditorForm"));
  setAttribute(Qt::WA_DeleteOnClose);
  FEncoding = ekNone;
  FLoadedWithPreamble = false;
  FCrLf = false;
  FSaving = false;
  FReloading = false;
  FCloseAnnounced = false;
  FNewFile = false;
  FFindDialog = nullptr;
  FReplaceDialog = nullptr;
  FLastFindDialog = nullptr;

  EditorMemo = new QPlainTextEdit(this);
  EditorMemo->setObjectName(QStringLiteral("EditorMemo"));
  setCentralWidget(EditorMemo);

  SaveAction = CreateAction("SaveAction", tr("&Save"), QStringLiteral("document-save"), QKeySequence(Qt::CTRL | Qt::Key_S));
  connect(SaveAction, &QAction::triggered, this, [this]() { SaveFile(); });
  SaveAllAction2 = CreateAction("SaveAllAction2", tr("Save &All"), QStringLiteral("document-save-all"),
    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
  connect(SaveAllAction2, &QAction::triggered, this, [this]() { FOnSaveAll(this); });
  ReloadAction = CreateAction("ReloadAction", tr("&Reload"), QStringLiteral("view-refresh"), QKeySequence(Qt::CTRL | Qt::Key_R));
  connect(ReloadAction, &QAction::triggered, this, [this]() { Reload(); });
  EditCut = CreateAction("EditCut", tr("Cu&t"), QStringLiteral("edit-cut"), QKeySequence::Cut);
  connect(EditCut, &QAction::triggered, EditorMemo, &QPlainTextEdit::cut);
  EditCopy = CreateAction("EditCopy", tr("&Copy"), QStringLiteral("edit-copy"), QKeySequence::Copy);
  connect(EditCopy, &QAction::triggered, EditorMemo, &QPlainTextEdit::copy);
  EditPaste = CreateAction("EditPaste", tr("&Paste"), QStringLiteral("edit-paste"), QKeySequence::Paste);
  connect(EditPaste, &QAction::triggered, EditorMemo, &QPlainTextEdit::paste);
  EditDelete = CreateAction("EditDelete", tr("&Delete"), QStringLiteral("edit-delete"), QKeySequence());
  connect(EditDelete, &QAction::triggered, this, [this]() { EditorMemo->textCursor().removeSelectedText(); });
  EditUndo = CreateAction("EditUndo", tr("&Undo"), QStringLiteral("edit-undo"), QKeySequence::Undo);
  connect(EditUndo, &QAction::triggered, EditorMemo, &QPlainTextEdit::undo);
  EditRedo = CreateAction("EditRedo", tr("Re&do"), QStringLiteral("edit-redo"), QKeySequence(Qt::CTRL | Qt::Key_Y));
  connect(EditRedo, &QAction::triggered, EditorMemo, &QPlainTextEdit::redo);
  EditSelectAll = CreateAction("EditSelectAll", tr("Select &All"), QStringLiteral("edit-select-all"), QKeySequence::SelectAll);
  connect(EditSelectAll, &QAction::triggered, EditorMemo, &QPlainTextEdit::selectAll);
  FindAction = CreateAction("FindAction", tr("&Find..."), QStringLiteral("edit-find"), QKeySequence(Qt::CTRL | Qt::Key_F));
  connect(FindAction, &QAction::triggered, this, [this]() { StartFind(true); });
  ReplaceAction = CreateAction("ReplaceAction", tr("&Replace..."), QStringLiteral("edit-find-replace"),
    QKeySequence(Qt::CTRL | Qt::Key_H));
  connect(ReplaceAction, &QAction::triggered, this, [this]() { StartFind(false); });
  FindNextAction = CreateAction("FindNextAction", tr("Find &Next"), QStringLiteral("go-down-search"), QKeySequence(Qt::Key_F3));
  connect(FindNextAction, &QAction::triggered, this, [this]()
  {
    if (FLastFindDialog == nullptr)
    {
      StartFind(true);
    }
    else
    {
      FLastFindDialog->FReplaceRequested = false;
      FLastFindDialog->FReplaceAllRequested = false;
      Find();
    }
  });
  GoToLineAction = CreateAction("GoToLineAction", tr("&Go to Line Number..."), QStringLiteral("go-jump"),
    QKeySequence(Qt::CTRL | Qt::Key_G));
  connect(GoToLineAction, &QAction::triggered, this, [this]() { GoToLine(); });
  DefaultEncodingAction = CreateAction("DefaultEncodingAction", OtherEncodingName, QString(), QKeySequence());
  DefaultEncodingAction->setCheckable(true);
  connect(DefaultEncodingAction, &QAction::triggered, this, [this]() { ChangeEncoding(ekOther); });
  UTF8EncodingAction = CreateAction("UTF8EncodingAction", QStringLiteral("UTF-8"), QString(), QKeySequence());
  UTF8EncodingAction->setCheckable(true);
  connect(UTF8EncodingAction, &QAction::triggered, this, [this]() { ChangeEncoding(ekUtf8); });

  QToolBar * ToolBar = addToolBar(tr("ToolBar"));
  ToolBar->setObjectName(QStringLiteral("ToolBar"));
  ToolBar->setMovable(false);
  ToolBar->addActions({ SaveAction, SaveAllAction2, ReloadAction });
  ToolBar->addSeparator();
  ToolBar->addActions({ EditCut, EditCopy, EditPaste, EditDelete, EditUndo, EditRedo });
  ToolBar->addSeparator();
  ToolBar->addActions({ FindAction, ReplaceAction, FindNextAction, GoToLineAction });
  ToolBar->addSeparator();
  EncodingMenu = new QMenu(tr("Encoding"), this);
  EncodingMenu->setObjectName(QStringLiteral("Encoding"));
  EncodingMenu->addActions({ DefaultEncodingAction, UTF8EncodingAction });
  QToolButton * EncodingButton = new QToolButton(ToolBar);
  EncodingButton->setObjectName(QStringLiteral("EncodingButton"));
  EncodingButton->setText(EncodingMenu->title());
  EncodingButton->setMenu(EncodingMenu);
  EncodingButton->setPopupMode(QToolButton::InstantPopup);
  ToolBar->addWidget(EncodingButton);

  EditorMemo->setContextMenuPolicy(Qt::ActionsContextMenu);
  EditorMemo->addActions({ EditUndo, EditRedo });
  QAction * Separator = new QAction(EditorMemo);
  Separator->setSeparator(true);
  EditorMemo->addAction(Separator);
  EditorMemo->addActions({ EditCut, EditCopy, EditPaste, EditDelete, EditSelectAll });

  for (QLabel ** Label : { &LineStatus, &ColumnStatus, &CharacterStatus, &EncodingStatus, &ModifiedStatus })
  {
    *Label = new QLabel(statusBar());
    statusBar()->addWidget(*Label, (Label == &ModifiedStatus) ? 1 : 0);
  }
  LineStatus->setObjectName(QStringLiteral("LineStatus"));
  ColumnStatus->setObjectName(QStringLiteral("ColumnStatus"));
  CharacterStatus->setObjectName(QStringLiteral("CharacterStatus"));
  EncodingStatus->setObjectName(QStringLiteral("EncodingStatus"));
  ModifiedStatus->setObjectName(QStringLiteral("ModifiedStatus"));

  connect(EditorMemo, &QPlainTextEdit::cursorPositionChanged, this, [this]() { UpdateControls(); });
  connect(EditorMemo, &QPlainTextEdit::modificationChanged, this, [this]() { UpdateControls(); });
  connect(EditorMemo, &QPlainTextEdit::undoAvailable, this, [this]() { UpdateControls(); });
  connect(EditorMemo, &QPlainTextEdit::redoAvailable, this, [this]() { UpdateControls(); });
  connect(EditorMemo, &QPlainTextEdit::copyAvailable, this, [this]() { UpdateControls(); });

  ApplyConfiguration();
}
//---------------------------------------------------------------------------
TEditorForm::~TEditorForm()
{
  // see closeEvent for explanation
  if (!FCloseAnnounced)
  {
    BackupSave();
    DoWindowClose(true);
  }
}
//---------------------------------------------------------------------------
QAction * TEditorForm::CreateAction(const char * Name, const QString & Caption, const QString & IconName,
  const QKeySequence & ShortCut)
{
  QAction * Action = new QAction(Caption, this);
  Action->setObjectName(QString::fromLatin1(Name));
  if (!IconName.isEmpty())
  {
    Action->setIcon(QIcon::fromTheme(IconName));
  }
  Action->setShortcut(ShortCut);
  addAction(Action);
  return Action;
}
//---------------------------------------------------------------------------
void TEditorForm::ApplyConfiguration()
{
  const TEditorConfiguration & Editor = AppConfiguration->Editor;
  QFont Font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  if (!Editor.FontName.IsEmpty())
  {
    Font.setFamily(ToQString(Editor.FontName));
  }
  if (Editor.FontSize > 0)
  {
    Font.setPointSize(Editor.FontSize);
  }
  EditorMemo->setFont(Font);
  EditorMemo->setTabStopDistance(QFontMetricsF(Font).horizontalAdvance(QLatin1Char(' ')) * std::max(Editor.TabSize, 1));
  EditorMemo->setLineWrapMode(Editor.WordWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
  UpdateControls();
}
//---------------------------------------------------------------------------
void TEditorForm::UpdateControls()
{
  // To disable saving, e.g. in encryption mode (no queue support) - both OnFileChanged and OnSaveAll are NULL.
  // But internal editors have (OnFileChanged == NULL), but OnSaveAll set, so we use OnSaveAll for dicision
  EditorMemo->setReadOnly(!FOnSaveAll);

  QTextCursor Cursor = EditorMemo->textCursor();
  int Line = Cursor.blockNumber();
  int Count = EditorMemo->document()->blockCount();
  LineStatus->setText(ToQString(FMTLOAD(EDITOR_LINE_STATUS, (Line + 1, Count))));
  QString Text = Cursor.block().text();
  int Position = Cursor.positionInBlock();
  int TabSize = AppConfiguration->Editor.TabSize;
  int Column = 0;
  for (int Index = 0; Index <= Position; Index++)
  {
    if ((Index >= 1) && (Index - 1 < Text.length()) && (Text[Index - 1] == QLatin1Char('\t')) &&
        (TabSize > 0)) // sanity check
    {
      Column = (((Column / TabSize) + 1) * TabSize) + 1;
    }
    else
    {
      Column++;
    }
  }
  ColumnStatus->setText(ToQString(FMTLOAD(EDITOR_COLUMN_STATUS, (Column))));
  UnicodeString Character;
  if (Position < Text.length())
  {
    uint Code = Text.at(Position).unicode();
    if (Text.at(Position).isHighSurrogate() && (Position + 1 < Text.length()))
    {
      Code = QChar::surrogateToUcs4(Text.at(Position), Text.at(Position + 1));
    }
    if ((FEncoding != ekOther) || (Code <= 0xFF))
    {
      Character = FMTLOAD(EDITOR_CHARACTER_STATUS2, (static_cast<int>(Code), static_cast<int>(Code)));
    }
  }
  CharacterStatus->setText(ToQString(Character));
  EncodingStatus->setText(ToQString(FMTLOAD(EDITOR_ENCODING_STATUS, (ToUnicodeString(FEncodingName)))));
  UnicodeString Status;
  if (EditorMemo->isReadOnly())
  {
    Status = LoadStr(EDITOR_READONLY);
  }
  else if (FSaving)
  {
    Status = LoadStr(EDITOR_SAVING);
  }
  else if (FNewFile)
  {
    Status = LoadStr(EDITOR_NEW);
  }
  else if (IsFileModified())
  {
    Status = LoadStr(EDITOR_MODIFIED);
  }
  ModifiedStatus->setText(ToQString(Status));

  bool Writable = !EditorMemo->isReadOnly();
  SaveAction->setEnabled(IsFileModified());
  bool SaveAllEnabled = Writable && IsFileModified();
  if (Writable && !SaveAllEnabled && FOnAnyModified)
  {
    FOnAnyModified(this, SaveAllEnabled);
  }
  SaveAllAction2->setEnabled(SaveAllEnabled);
  ReloadAction->setEnabled(!FReloading && !FNewFile);
  bool HasSelection = Cursor.hasSelection();
  EditCut->setEnabled(Writable && HasSelection);
  EditCopy->setEnabled(HasSelection);
  EditPaste->setEnabled(Writable);
  EditDelete->setEnabled(Writable && HasSelection);
  EditUndo->setEnabled(Writable && EditorMemo->document()->isUndoAvailable());
  EditRedo->setEnabled(Writable && EditorMemo->document()->isRedoAvailable());
  ReplaceAction->setEnabled(Writable);
  FindNextAction->setEnabled(
    (FLastFindDialog != nullptr) || !AppConfiguration->Editor.FindText.IsEmpty());
  DefaultEncodingAction->setChecked(FEncoding == ekOther);
  UTF8EncodingAction->setChecked(FEncoding == ekUtf8);
  EncodingMenu->setEnabled(!FNewFile);
}
//---------------------------------------------------------------------------
bool TEditorForm::IsFileModified()
{
  return EditorMemo->document()->isModified();
}
//---------------------------------------------------------------------------
void TEditorForm::CheckFileSize()
{
  TEditorConfiguration EditorConfiguration = AppConfiguration->Editor;

  qint64 Size = QFile(ToQString(FFileName)).size();
  const qint64 MaxSize = static_cast<qint64>(EditorConfiguration.LargeFileSize) * 1024;
  if ((Size >= 0) && (Size > MaxSize) && EditorConfiguration.WarnOrLargeFileSize)
  {
    TMessageParams Params(mpNeverAskAgainCheck);
    unsigned int Answer =
      MoreMessageDialog(
        FMTLOAD(INTERNAL_EDITOR_LARGE_FILE2, (FormatBytes(Size))), NULL,
        qtConfirmation, qaOK | qaCancel, &Params);
    switch (Answer)
    {
      case qaOK:
        // noop;
        break;

      case qaCancel:
        Abort();
        break;

      case qaNeverAskAgain:
        EditorConfiguration.WarnOrLargeFileSize = false;
        AppConfiguration->Editor = EditorConfiguration;
        break;

      default:
        DebugFail();
    }
  }
}
//---------------------------------------------------------------------------
bool TEditorForm::Decode(const QByteArray & Data, TEncodingKind Encoding, QString & Text)
{
  if (Encoding == ekUtf8)
  {
    QStringDecoder Decoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    Text = Decoder.decode(Data);
    return !Decoder.hasError();
  }
  else
  {
    Text = QString::fromLatin1(Data);
    return true;
  }
}
//---------------------------------------------------------------------------
void TEditorForm::LoadFromFile(bool PrimaryEncoding)
{
  QFile File(ToQString(FFileName));
  if (!File.open(QIODevice::ReadOnly))
  {
    throw Exception(MainInstructions(FMTLOAD(EDITOR_LOAD_ERROR, (FFileName))));
  }
  QByteArray Data = File.readAll();

  bool CanTrySecondary;
  if (FEncoding == ekNone)
  {
    int Encoding;
    if (Data.startsWith(Utf8Preamble))
    {
      Encoding = CP_UTF8;
      CanTrySecondary = false;
    }
    else
    {
      Encoding = AppConfiguration->Editor.Encoding;
      CanTrySecondary = true;
    }

    if (Encoding == CP_UTF8)
    {
      FEncoding = PrimaryEncoding ? ekUtf8 : ekOther;
    }
    else
    {
      FEncoding = PrimaryEncoding ? ekOther : ekUtf8;
    }
  }
  else
  {
    CanTrySecondary = false;
  }

  FEncodingName = (FEncoding == ekUtf8) ? ToQString(LoadStr(UTF8_NAME)) : OtherEncodingName;

  FLoadedWithPreamble = (FEncoding == ekUtf8) && Data.startsWith(Utf8Preamble);
  QString Text;
  if (!Decode(FLoadedWithPreamble ? Data.mid(Utf8Preamble.size()) : Data, FEncoding, Text))
  {
    UnicodeString Message = FMTLOAD(EDITOR_ENCODING_ERROR, (FFileName, ToUnicodeString(FEncodingName)));

    if (PrimaryEncoding && CanTrySecondary)
    {
      FEncoding = ekNone;
      LoadFromFile(false);
      TEditorConfiguration EditorConfiguration = AppConfiguration->Editor;
      if (EditorConfiguration.WarnOnEncodingFallback)
      {
        // by now the FEncodingName is the secondary encoding
        Message = Message + L" " + FMTLOAD(EDITOR_ENCODING_REVERTED, (ToUnicodeString(FEncodingName)));
        TMessageParams Params(mpNeverAskAgainCheck);
        unsigned int Answer =
          MessageDialog(MainInstructions(Message), qtInformation, qaOK, &Params);
        if (Answer == qaNeverAskAgain)
        {
          EditorConfiguration.WarnOnEncodingFallback = false;
          AppConfiguration->Editor = EditorConfiguration;
        }
      }
      return;
    }
    else
    {
      throw Exception(Message);
    }
  }

  // The line endings of the file are kept
  FCrLf = Text.contains(QStringLiteral("\r\n"));
  if (FCrLf)
  {
    Text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  }
  EditorMemo->setPlainText(Text);
}
//---------------------------------------------------------------------------
void TEditorForm::LoadFile()
{
  CheckFileSize();
  LoadFromFile(true);
  EditorMemo->document()->setModified(false);
  // this is important particularly after reload
  UpdateControls();
}
//---------------------------------------------------------------------------
void TEditorForm::SaveToFile()
{
  QString Text = EditorMemo->toPlainText();
  if (FCrLf)
  {
    Text.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));
  }
  QByteArray Data;
  if (FEncoding == ekUtf8)
  {
    if (FLoadedWithPreamble)
    {
      Data = Utf8Preamble;
    }
    Data += Text.toUtf8();
  }
  else
  {
    QStringEncoder Encoder(QStringConverter::Latin1);
    Data = Encoder.encode(Text);
  }
  QFile File(ToQString(FFileName));
  if (!File.open(QIODevice::WriteOnly | QIODevice::Truncate) || (File.write(Data) != Data.size()))
  {
    throw EOSExtException(FMTLOAD(WRITE_ERROR, (FFileName)));
  }
}
//---------------------------------------------------------------------------
void TEditorForm::SaveFile()
{
  // Test is needed for "Save all" and is redundant for "Save"
  if (IsFileModified())
  {
    DebugAssert(!FFileName.IsEmpty());
    FSaving = true;
    UpdateControls();
    bool Direct = !FOnFileChanged;
    ::rtl::TFinally Finally([&]()
    {
      if (Direct)
      {
        FSaving = false;
      }
      UpdateControls();
    });
    SaveToFile();
    if (!Direct)
    {
      FOnFileChanged(this);
    }
    EditorMemo->document()->setModified(false);
    FNewFile = false;
  }
}
//---------------------------------------------------------------------------
void TEditorForm::BackupSave()
{
  if (IsFileModified())
  {
    int Uniq = 0;
    while (true)
    {
      UnicodeString FileName = FFileName + L".bak" + ((Uniq == 0) ? UnicodeString() : IntToStr(Uniq));
      if (!FileExists(ApiPath(FileName)))
      {
        UnicodeString PrevFileName = FFileName;
        FFileName = FileName;
        ::rtl::TFinally Finally([&]() { FFileName = PrevFileName; });
        SaveToFile();
        break;
      }
      Uniq++;
    }
  }
}
//---------------------------------------------------------------------------
void TEditorForm::ChangeEncoding(TEncodingKind Encoding)
{
  if (FEncoding != Encoding)
  {
    if (!IsFileModified() ||
        (MessageDialog(MainInstructions(LoadStr(EDITOR_MODIFIED_ENCODING)), qtConfirmation,
          qaOK | qaCancel) != qaCancel))
    {
      TEncodingKind PrevEncoding = FEncoding;
      try
      {
        FEncoding = Encoding;
        LoadFile();
      }
      catch (...)
      {
        // try to restore
        FEncoding = PrevEncoding;
        LoadFile();
        throw;
      }
    }
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TEditorForm::FileUploadComplete()
{
  DebugAssert(FSaving);
  FSaving = false;
  UpdateControls();
}
//---------------------------------------------------------------------------
void TEditorForm::Reload()
{
  TAutoFlag ReloadingFlag(FReloading);
  if (!IsFileModified() ||
      (MessageDialog(MainInstructions(LoadStr(EDITOR_MODIFIED_RELOAD)), qtConfirmation,
        qaOK | qaCancel) != qaCancel))
  {
    if (FOnFileReload)
    {
      FOnFileReload(this);
    }
    LoadFile();
  }
}
//---------------------------------------------------------------------------
void TEditorForm::Find()
{
  bool Found;
  int Replacements = 0;

  do
  {
    DebugAssert(FLastFindDialog != nullptr);
    TFindDialog * Dialog = FLastFindDialog;
    QTextCursor Cursor = EditorMemo->textCursor();
    Qt::CaseSensitivity Sensitivity = Dialog->MatchCase() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    // length condition is there to improve performance when large
    // block is selected in editor
    if (Dialog->IsReplace() &&
        (Dialog->FReplaceRequested || Dialog->FReplaceAllRequested) &&
        (Dialog->FindText().length() == Cursor.selectedText().length()) &&
        (Cursor.selectedText().compare(Dialog->FindText(), Sensitivity) == 0))
    {
      Cursor.insertText(Dialog->ReplaceText());
      EditorMemo->setTextCursor(Cursor);
      Replacements++;
    }

    TEditorConfiguration EditorConfiguration = AppConfiguration->Editor;
    EditorConfiguration.FindText = ToUnicodeString(Dialog->FindText());
    if (FReplaceDialog != nullptr)
    {
      EditorConfiguration.ReplaceText = ToUnicodeString(FReplaceDialog->ReplaceText());
    }
    EditorConfiguration.FindMatchCase = Dialog->MatchCase();
    EditorConfiguration.FindWholeWord = Dialog->WholeWord();
    // The replace dialog has no direction
    EditorConfiguration.FindDown = !Dialog->IsReplace() ? Dialog->Down() : true;
    AppConfiguration->Editor = EditorConfiguration;

    QTextDocument::FindFlags Flags;
    if (EditorConfiguration.FindMatchCase)
    {
      Flags |= QTextDocument::FindCaseSensitively;
    }
    if (EditorConfiguration.FindWholeWord)
    {
      Flags |= QTextDocument::FindWholeWords;
    }
    if (!EditorConfiguration.FindDown)
    {
      Flags |= QTextDocument::FindBackward;
    }

    Found = EditorMemo->find(ToQString(EditorConfiguration.FindText), Flags);

    if (!Found)
    {
      if ((Replacements == 0) || Dialog->FReplaceAllRequested)
      {
        if (Replacements == 0)
        {
          UnicodeString Message = MainInstructions(FMTLOAD(EDITOR_FIND_END, (EditorConfiguration.FindText)));
          MessageDialog(Message, qtInformation, qaOK);
        }
        else if (Dialog->FReplaceAllRequested)
        {
          UnicodeString Message = MainInstructions(FMTLOAD(EDITOR_REPLACE_END, (Replacements)));
          MessageDialog(Message, qtInformation, qaOK);
        }
      }
    }
  }
  while (Found && FLastFindDialog->IsReplace() && FLastFindDialog->FReplaceAllRequested);
  UpdateControls();
}
//---------------------------------------------------------------------------
void TEditorForm::StartFind(bool AFind)
{
  QString Text = EditorMemo->textCursor().selectedText();
  if (Text.isEmpty())
  {
    Text = ToQString(AppConfiguration->Editor.FindText);
  }
  TFindDialog *& Dialog = AFind ? FFindDialog : FReplaceDialog;
  if (Dialog == nullptr)
  {
    Dialog = new TFindDialog(this, !AFind);
  }
  if ((FLastFindDialog != nullptr) && (Dialog != FLastFindDialog))
  {
    FLastFindDialog->close();
  }
  FLastFindDialog = Dialog;
  if (!Text.isEmpty())
  {
    FLastFindDialog->SetFindText(Text);
  }
  if (FReplaceDialog != nullptr)
  {
    FReplaceDialog->SetReplaceText(ToQString(AppConfiguration->Editor.ReplaceText));
  }
  const TEditorConfiguration & Editor = AppConfiguration->Editor;
  FLastFindDialog->SetOptions(Editor.FindMatchCase, Editor.FindWholeWord, Editor.FindDown);
  FLastFindDialog->show();
  FLastFindDialog->raise();
  FLastFindDialog->activateWindow();
}
//---------------------------------------------------------------------------
void TEditorForm::GoToLine()
{
  UnicodeString Str;
  if (InputDialog(LoadStr(EDITOR_GO_TO_LINE), LoadStr(EDITOR_LINE_NUMBER), Str))
  {
    int Line = StrToIntDef(Str, -1);
    if ((Line <= 0) || (Line > EditorMemo->document()->blockCount()))
    {
      throw Exception(MainInstructions(LoadStr(EDITOR_INVALID_LINE)));
    }
    else
    {
      QTextCursor Cursor(EditorMemo->document()->findBlockByNumber(Line - 1));
      EditorMemo->setTextCursor(Cursor);
      EditorMemo->setFocus();
    }
  }
}
//---------------------------------------------------------------------------
void TEditorForm::closeEvent(QCloseEvent * Event)
{
  if (IsFileModified())
  {
    raise();
    activateWindow();
    UnicodeString Message = MainInstructions(LoadStr(SAVE_CHANGES));
    unsigned int Answer = MessageDialog(Message, qtConfirmation, qaYes | qaNo | qaCancel);
    if (Answer == qaCancel)
    {
      Event->ignore();
      return;
    }
    if (Answer == qaYes)
    {
      SaveFile();
    }
    else
    {
      // Not to back the changes up when closing
      EditorMemo->document()->setModified(false);
    }
  }
  // Announce closure here, synchronously (the editor manager thus considers
  // the form to be really closed); the destructor is the last resort
  DoWindowClose(false);
  FCloseAnnounced = true;
  QMainWindow::closeEvent(Event);
}
//---------------------------------------------------------------------------
void TEditorForm::DoWindowClose(bool Forced)
{
  if (FOnWindowClose)
  {
    try
    {
      FOnWindowClose(this, Forced);
    }
    catch (Exception & E)
    {
      ShowExtendedException(&E);
    }
  }
}
//---------------------------------------------------------------------------
void TEditorForm::keyPressEvent(QKeyEvent * Event)
{
  if (((Event->key() == Qt::Key_Escape) || (Event->key() == Qt::Key_F10)) && (Event->modifiers() == Qt::NoModifier))
  {
    Event->accept();
    close();
  }
  else
  {
    QMainWindow::keyPressEvent(Event);
  }
}
//---------------------------------------------------------------------------
TEditorForm * EditorForm(QWidget * Form)
{
  return DebugNotNull(dynamic_cast<TEditorForm *>(Form));
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
QWidget * ShowEditorForm(const UnicodeString & FileName, QWidget * ParentForm,
  TEditorEvent OnFileChanged, TEditorEvent OnFileReload, TEditorClosedEvent OnClose,
  TEditorEvent OnSaveAll, TAnyModifiedEvent OnAnyModified,
  const UnicodeString & Caption, bool NewFile)
{
  std::unique_ptr<TEditorForm> Dialog(new TEditorForm());
  Dialog->SetFileName(FileName);
  UnicodeString ACaption = Caption.IsEmpty() ? FileName : Caption;
  Dialog->setWindowTitle(ToQString(ACaption + TitleSeparator + LoadStr(EDITOR_CAPTION) + TitleSeparator + AppName));
  Dialog->FOnFileChanged = OnFileChanged;
  Dialog->FOnFileReload = OnFileReload;
  Dialog->FOnSaveAll = OnSaveAll;
  Dialog->FOnAnyModified = OnAnyModified;
  Dialog->FNewFile = NewFile;
  if (ParentForm != nullptr)
  {
    Dialog->resize(ParentForm->size());
  }
  // load before showing, so when loading failes,
  // we do not show an empty editor
  Dialog->LoadFile();
  Dialog->show();
  // make sure editor closing is announced only if it was ever successfully shown
  Dialog->FOnWindowClose = OnClose;
  return Dialog.release();
}
//---------------------------------------------------------------------------
void EditorFormFileUploadComplete(QWidget * Form)
{
  EditorForm(Form)->FileUploadComplete();
}
//---------------------------------------------------------------------------
void EditorFormFileSave(QWidget * Form)
{
  EditorForm(Form)->SaveFile();
}
//---------------------------------------------------------------------------
bool IsEditorFormModified(QWidget * Form)
{
  return EditorForm(Form)->IsFileModified();
}
//---------------------------------------------------------------------------
