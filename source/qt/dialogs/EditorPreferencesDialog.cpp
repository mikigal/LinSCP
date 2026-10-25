// The dialog of an editor of the list of editors (upstream's
// forms/EditorPreferences.dfm): the internal editor, an external program
// or the application of the desktop, for the files of a mask.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <EditorPreferences.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
class TEditorPreferencesDialog : public QDialog
{
public:
  TEditorPreferencesDialog(TEditorPreferencesMode Mode, bool MayRemote);

  bool Execute(TEditorData * Editor, bool & Remember);

protected:
  virtual void accept() override;

private:
  bool FMayRemote;
  QRadioButton * EditorInternalButton;
  QRadioButton * EditorExternalButton;
  QRadioButton * EditorOpenButton;
  THistoryComboBox * ExternalEditorEdit;
  QPushButton * ExternalEditorBrowseButton;
  QGroupBox * MaskGroup;
  THistoryComboBox * MaskEdit;
  QGroupBox * ExternalEditorGroup;
  QCheckBox * ExternalEditorTextCheck;
  QCheckBox * SDIExternalEditorCheck;
  QCheckBox * RememberCheck;
  QPushButton * OkButton;

  void UpdateControls();
  void ExternalEditorBrowseButtonClick();
};
//---------------------------------------------------------------------------
TEditorPreferencesDialog::TEditorPreferencesDialog(TEditorPreferencesMode Mode, bool MayRemote) :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("EditorPreferencesDialog"));
  int CaptionId;
  switch (Mode)
  {
    case epmEdit:
      CaptionId = EDITOR_EDIT;
      break;

    case epmAdd:
      CaptionId = EDITOR_ADD;
      break;

    default:
      CaptionId = EDITOR_AD_HOC;
      break;
  }
  setWindowTitle(ToQString(LoadStr(CaptionId)));
  FMayRemote = MayRemote;
  bool AdHoc = (Mode == epmAdHoc);

  QVBoxLayout * MainLayout = new QVBoxLayout(this);

  QGroupBox * EditorGroup2 = new QGroupBox(tr("Editor"), this);
  EditorGroup2->setObjectName(QStringLiteral("EditorGroup2"));
  QVBoxLayout * EditorLayout = new QVBoxLayout(EditorGroup2);
  EditorInternalButton = new QRadioButton(tr("&Internal editor"), EditorGroup2);
  EditorInternalButton->setObjectName(QStringLiteral("EditorInternalButton"));
  EditorLayout->addWidget(EditorInternalButton);
  EditorExternalButton = new QRadioButton(tr("&External editor:"), EditorGroup2);
  EditorExternalButton->setObjectName(QStringLiteral("EditorExternalButton"));
  EditorLayout->addWidget(EditorExternalButton);
  QHBoxLayout * ExternalEditorLayout = new QHBoxLayout();
  ExternalEditorEdit = new THistoryComboBox(EditorGroup2);
  ExternalEditorEdit->setObjectName(QStringLiteral("ExternalEditorEdit"));
  ExternalEditorEdit->SetHistoryKey(L"ExternalEditor");
  ExternalEditorLayout->addWidget(ExternalEditorEdit, 1);
  ExternalEditorBrowseButton = new QPushButton(tr("B&rowse..."), EditorGroup2);
  ExternalEditorBrowseButton->setObjectName(QStringLiteral("ExternalEditorBrowseButton"));
  ExternalEditorBrowseButton->setAutoDefault(false);
  ExternalEditorLayout->addWidget(ExternalEditorBrowseButton);
  EditorLayout->addLayout(ExternalEditorLayout);
  EditorOpenButton = new QRadioButton(tr("&Associated application"), EditorGroup2);
  EditorOpenButton->setObjectName(QStringLiteral("EditorOpenButton"));
  EditorLayout->addWidget(EditorOpenButton);
  MainLayout->addWidget(EditorGroup2);
  // An ad hoc editor is an external one
  if (AdHoc)
  {
    EditorGroup2->setTitle(EditorExternalButton->text().remove(QLatin1Char('&')));
    EditorInternalButton->setVisible(false);
    EditorExternalButton->setVisible(false);
    EditorOpenButton->setVisible(false);
  }

  MaskGroup = new QGroupBox(tr("Editor autoselection"), this);
  MaskGroup->setObjectName(QStringLiteral("MaskGroup"));
  QVBoxLayout * MaskLayout = new QVBoxLayout(MaskGroup);
  QLabel * MaskLabel = new QLabel(tr("Use this editor for &following files:"), MaskGroup);
  MaskLabel->setObjectName(QStringLiteral("MaskLabel"));
  MaskLayout->addWidget(MaskLabel);
  MaskEdit = new THistoryComboBox(MaskGroup);
  MaskEdit->setObjectName(QStringLiteral("MaskEdit"));
  MaskEdit->SetHistoryKey(L"Mask");
  MaskLabel->setBuddy(MaskEdit);
  MaskLayout->addWidget(MaskEdit);
  MainLayout->addWidget(MaskGroup);
  MaskGroup->setVisible(!AdHoc);

  ExternalEditorGroup = new QGroupBox(tr("External editor options (affects editing remote files only)"), this);
  ExternalEditorGroup->setObjectName(QStringLiteral("ExternalEditorGroup"));
  QVBoxLayout * ExternalLayout = new QVBoxLayout(ExternalEditorGroup);
  ExternalEditorTextCheck = new QCheckBox(tr("Force &text transfer mode for files edited in external editor"), ExternalEditorGroup);
  ExternalEditorTextCheck->setObjectName(QStringLiteral("ExternalEditorTextCheck"));
  ExternalLayout->addWidget(ExternalEditorTextCheck);
  SDIExternalEditorCheck = new QCheckBox(tr("E&xternal editor opens each file in separate window (process)"), ExternalEditorGroup);
  SDIExternalEditorCheck->setObjectName(QStringLiteral("SDIExternalEditorCheck"));
  ExternalLayout->addWidget(SDIExternalEditorCheck);
  MainLayout->addWidget(ExternalEditorGroup);

  RememberCheck = new QCheckBox(tr("&Remember this editor"), this);
  RememberCheck->setObjectName(QStringLiteral("RememberCheck"));
  RememberCheck->setVisible(AdHoc);
  MainLayout->addWidget(RememberCheck);
  MainLayout->addStretch(1);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  OkButton = Buttons->button(QDialogButtonBox::Ok);
  OkButton->setObjectName(QStringLiteral("OkButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
  AddHelpButton(Buttons, FormHelpKeyword(L"EditorPreferences.dfm"));

  for (QRadioButton * Button : { EditorInternalButton, EditorExternalButton, EditorOpenButton })
  {
    connect(Button, &QRadioButton::toggled, this, [this]() { UpdateControls(); });
  }
  connect(ExternalEditorEdit, &QComboBox::editTextChanged, this, [this]() { UpdateControls(); });
  connect(ExternalEditorBrowseButton, &QPushButton::clicked, this,
    [this]() { ExecuteHandler([this]() { ExternalEditorBrowseButtonClick(); }); });
  setMinimumWidth(fontMetrics().averageCharWidth() * 70);
}
//---------------------------------------------------------------------------
bool TEditorPreferencesDialog::Execute(TEditorData * Editor, bool & Remember)
{
  EditorInternalButton->setChecked(Editor->Editor == edInternal);
  // An ad hoc editor is an external one
  EditorExternalButton->setChecked((Editor->Editor == edExternal) || RememberCheck->isVisible());
  EditorOpenButton->setChecked(Editor->Editor == edOpen);
  UnicodeString ExternalEditor = Editor->ExternalEditor;
  if (!ExternalEditor.IsEmpty())
  {
    ReformatFileNameCommand(ExternalEditor);
  }
  ExternalEditorEdit->SetText(ExternalEditor);
  MaskEdit->SetText(Editor->FileMask.Masks);
  ExternalEditorTextCheck->setChecked(Editor->ExternalEditorText);
  SDIExternalEditorCheck->setChecked(Editor->SDIExternalEditor);
  RememberCheck->setChecked(Remember);
  UpdateControls();

  bool Result = (exec() == QDialog::Accepted);

  if (Result)
  {
    if (EditorExternalButton->isChecked())
    {
      Editor->Editor = edExternal;
    }
    else if (EditorOpenButton->isChecked())
    {
      Editor->Editor = edOpen;
    }
    else
    {
      Editor->Editor = edInternal;
    }
    Editor->ExternalEditor = ExternalEditorEdit->GetText();
    ExternalEditorEdit->SaveToHistory();
    Editor->FileMask = MaskEdit->GetText();
    MaskEdit->SaveToHistory();
    Editor->ExternalEditorText = ExternalEditorTextCheck->isChecked();
    Editor->SDIExternalEditor = SDIExternalEditorCheck->isChecked();
    Remember = RememberCheck->isChecked();
  }

  return Result;
}
//---------------------------------------------------------------------------
void TEditorPreferencesDialog::accept()
{
  try
  {
    UnicodeString FileName = ExternalEditorEdit->GetText();
    if (!FileName.IsEmpty())
    {
      ReformatFileNameCommand(FileName);
      ExternalEditorEdit->SetText(FileName);
    }
    ValidateMaskEdit(MaskEdit);
  }
  catch (EAbort &)
  {
    return;
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
    return;
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
void TEditorPreferencesDialog::UpdateControls()
{
  OkButton->setEnabled(
    EditorInternalButton->isChecked() || EditorOpenButton->isChecked() ||
    !ExternalEditorEdit->currentText().isEmpty());
  ExternalEditorEdit->setEnabled(EditorExternalButton->isChecked());
  ExternalEditorBrowseButton->setEnabled(EditorExternalButton->isChecked());
  ExternalEditorGroup->setEnabled(EditorExternalButton->isChecked() && FMayRemote);
}
//---------------------------------------------------------------------------
void TEditorPreferencesDialog::ExternalEditorBrowseButtonClick()
{
  BrowseForExecutable(ExternalEditorEdit, LoadStr(PREFERENCES_SELECT_EXTERNAL_EDITOR), true);
  UpdateControls();
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoEditorPreferencesDialog(TEditorData * Editor, bool & Remember, TEditorPreferencesMode Mode, bool MayRemote)
{
  TEditorPreferencesDialog Dialog(Mode, MayRemote);
  return Dialog.Execute(Editor, Remember);
}
//---------------------------------------------------------------------------
