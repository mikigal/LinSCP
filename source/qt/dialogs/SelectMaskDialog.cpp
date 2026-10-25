// The file mask dialog (upstream's forms/SelectMask.dfm): files selected or
// unselected by a mask, the filter of a panel, the mask of a file color.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <FileMasks.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
class TSelectMaskDialog : public QDialog
{
public:
  enum TMode { smSelect, smDeselect, smFilter, smFileColor };

  TSelectMaskDialog(QWidget * Parent, TMode Mode);

  bool Execute(TFileFilter & FileFilter, TColor & Color);
  bool Execute(UnicodeString & Mask, TColor & Color);

protected:
  virtual void accept() override;

private:
  TColor FColor;
  THistoryComboBox * MaskEdit;
  QCheckBox * ApplyToDirectoriesCheck;
  QLabel * ColorFileNamesLabel;
  QLabel * ColorSizesLabel;
  QPushButton * ColorButton;
  QPushButton * OKBtn;

  void UpdateControls();
};
//---------------------------------------------------------------------------
TSelectMaskDialog::TSelectMaskDialog(QWidget * Parent, TMode Mode) :
  QDialog(Parent != nullptr ? Parent : QApplication::activeWindow())
{
  setObjectName(QStringLiteral("SelectMaskDialog"));
  FColor = TColor(0);
  int CaptionStr;
  switch (Mode)
  {
    case smSelect:
      CaptionStr = SELECT_MASK_SELECT_CAPTION;
      break;

    case smDeselect:
      CaptionStr = SELECT_MASK_DESELECT_CAPTION;
      break;

    case smFilter:
      CaptionStr = FILTER_MASK_CAPTION;
      break;

    default:
      CaptionStr = FILE_COLOR_CAPTION;
      break;
  }
  setWindowTitle(ToQString(LoadStr(CaptionStr)));

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QGroupBox * MaskGroup = new QGroupBox(this);
  MaskGroup->setObjectName(QStringLiteral("MaskGroup"));
  QVBoxLayout * GroupLayout = new QVBoxLayout(MaskGroup);
  QHBoxLayout * LabelRow = new QHBoxLayout();
  QLabel * Label3 = new QLabel(tr("File &mask:"), MaskGroup);
  LabelRow->addWidget(Label3);
  LabelRow->addStretch(1);
  QLabel * HintText = new QLabel(QStringLiteral("<a href=\"#\">%1</a>").arg(tr("mask hi&nts").remove(QLatin1Char('&'))), MaskGroup);
  HintText->setObjectName(QStringLiteral("HintText"));
  HintText->setToolTip(ToQString(
    FORMAT(L"%s\n \n%s\n \n%s\n \n%s", (LoadStr(MASK_HINT2), LoadStr(FILE_MASK_EX_HINT),
      LoadStr(COMBINING_MASKS_HINT), LoadStr(MASK_HELP)))));
  LabelRow->addWidget(HintText);
  GroupLayout->addLayout(LabelRow);
  QHBoxLayout * MaskRow = new QHBoxLayout();
  MaskEdit = new THistoryComboBox(MaskGroup);
  MaskEdit->setObjectName(QStringLiteral("MaskEdit"));
  MaskEdit->SetHistoryKey(L"Mask");
  Label3->setBuddy(MaskEdit);
  MaskRow->addWidget(MaskEdit, 1);
  QPushButton * MaskButton = new QPushButton(tr("&Edit..."), MaskGroup);
  MaskButton->setObjectName(QStringLiteral("MaskButton"));
  MaskButton->setAutoDefault(false);
  MaskRow->addWidget(MaskButton);
  GroupLayout->addLayout(MaskRow);
  connect(MaskButton, &QPushButton::clicked, this, [this]()
  {
    ExecuteHandler([this]()
    {
      TFileMasks Masks = MaskEdit->GetText();
      if (DoEditMaskDialog(Masks))
      {
        MaskEdit->SetText(Masks.Masks);
        UpdateControls();
      }
    });
  });
  connect(MaskEdit, &QComboBox::editTextChanged, this, [this]() { UpdateControls(); });

  ApplyToDirectoriesCheck = new QCheckBox(tr("Apply to &directories"), MaskGroup);
  ApplyToDirectoriesCheck->setObjectName(QStringLiteral("ApplyToDirectoriesCheck"));
  GroupLayout->addWidget(ApplyToDirectoriesCheck);
  ApplyToDirectoriesCheck->setVisible((Mode == smSelect) || (Mode == smDeselect));

  // A sample of the files in the color
  QHBoxLayout * ColorRow = new QHBoxLayout();
  ColorFileNamesLabel = new QLabel(QStringLiteral("about.html\nindex.html\nphoto.jpg"), MaskGroup);
  ColorFileNamesLabel->setObjectName(QStringLiteral("ColorFileNamesLabel"));
  ColorRow->addWidget(ColorFileNamesLabel);
  ColorSizesLabel = new QLabel(MaskGroup);
  ColorSizesLabel->setObjectName(QStringLiteral("ColorSizesLabel"));
  ColorSizesLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
  ColorSizesLabel->setText(
    ToQString(
      FormatPanelBytes(10723, AppConfiguration->FormatSizeBytes) + L"\n" +
      FormatPanelBytes(25835, AppConfiguration->FormatSizeBytes) + L"\n" +
      FormatPanelBytes(276445, AppConfiguration->FormatSizeBytes)));
  ColorRow->addWidget(ColorSizesLabel);
  ColorRow->addStretch(1);
  ColorButton = new QPushButton(tr("&Color"), MaskGroup);
  ColorButton->setObjectName(QStringLiteral("ColorButton"));
  ColorButton->setAutoDefault(false);
  ColorRow->addWidget(ColorButton, 0, Qt::AlignTop);
  GroupLayout->addLayout(ColorRow);
  connect(ColorButton, &QPushButton::clicked, this, [this]()
  {
    if (ColorMenuPopup(ColorButton->mapToGlobal(QPoint(0, ColorButton->height())), FColor, false))
    {
      UpdateControls();
    }
  });
  bool Color = (Mode == smFileColor);
  ColorFileNamesLabel->setVisible(Color);
  ColorSizesLabel->setVisible(Color);
  ColorButton->setVisible(Color);
  MainLayout->addWidget(MaskGroup);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  OKBtn = Buttons->button(QDialogButtonBox::Ok);
  OKBtn->setObjectName(QStringLiteral("OKBtn"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelBtn"));
  if (Mode == smFilter)
  {
    QPushButton * ClearButton = Buttons->addButton(tr("&Clear"), QDialogButtonBox::ResetRole);
    ClearButton->setObjectName(QStringLiteral("ClearButton"));
    connect(ClearButton, &QPushButton::clicked, this, [this]() { MaskEdit->SetText(UnicodeString()); });
  }
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
  setMinimumWidth(fontMetrics().averageCharWidth() * 60);
}
//---------------------------------------------------------------------------
bool TSelectMaskDialog::Execute(TFileFilter & FileFilter, TColor & Color)
{
  ApplyToDirectoriesCheck->setChecked(FileFilter.Directories);
  MaskEdit->SetText(FileFilter.Masks);
  MaskEdit->setFocus();
  FColor = Color;
  UpdateControls();
  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    MaskEdit->SaveToHistory();
    FileFilter.Directories = ApplyToDirectoriesCheck->isChecked();
    FileFilter.Masks = MaskEdit->GetText();
    Color = FColor;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TSelectMaskDialog::Execute(UnicodeString & Mask, TColor & Color)
{
  TFileFilter Filter;
  Filter.Masks = Mask;
  bool Result = Execute(Filter, Color);
  if (Result)
  {
    Mask = Filter.Masks;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSelectMaskDialog::accept()
{
  try
  {
    ValidateMaskEdit(MaskEdit);
  }
  catch (EAbort &)
  {
    return;
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
void TSelectMaskDialog::UpdateControls()
{
  OKBtn->setEnabled(!ColorButton->isVisible() || !MaskEdit->currentText().isEmpty());
  QPalette Palette = ColorFileNamesLabel->palette();
  Palette.setColor(QPalette::WindowText,
    (FColor != TColor(0)) ? TColorToQColor(FColor) : QApplication::palette().color(QPalette::WindowText));
  ColorFileNamesLabel->setPalette(Palette);
  ColorSizesLabel->setPalette(Palette);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoSelectMaskDialog(QWidget * Parent, bool Select, TFileFilter & Filter)
{
  TSelectMaskDialog Dialog(Parent, (Select ? TSelectMaskDialog::smSelect : TSelectMaskDialog::smDeselect));
  Filter.Masks = AppConfiguration->SelectMask;
  Filter.Directories = AppConfiguration->SelectDirectories;
  TColor Color = TColor(0);
  bool Result = Dialog.Execute(Filter, Color);
  if (Result)
  {
    AppConfiguration->SelectMask = Filter.Masks;
    AppConfiguration->SelectDirectories = Filter.Directories;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool DoFilterMaskDialog(QWidget * Parent, UnicodeString & Mask)
{
  TSelectMaskDialog Dialog(Parent, TSelectMaskDialog::smFilter);
  TColor Color = TColor(0);
  return Dialog.Execute(Mask, Color);
}
//---------------------------------------------------------------------------
bool DoFileColorDialog(TFileColorData & FileColorData)
{
  TSelectMaskDialog Dialog(nullptr, TSelectMaskDialog::smFileColor);
  UnicodeString Mask = FileColorData.FileMask.Masks;
  bool Result = Dialog.Execute(Mask, FileColorData.Color);
  if (Result)
  {
    FileColorData.FileMask.Masks = Mask;
  }
  return Result;
}
//---------------------------------------------------------------------------
