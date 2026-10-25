// The transfer settings (upstream's TCopyParamsFrame, forms/CopyParams.dfm):
// the transfer mode, the file name modification, the options of uploads,
// downloads and both, the file mask and the speed limit. Used by the
// transfer settings dialog, the presets and the synchronization.
//---------------------------------------------------------------------------
#ifndef CopyParamsFrameH
#define CopyParamsFrameH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <CopyParam.h>

#include <QWidget>
//---------------------------------------------------------------------------
class QAction;
class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class THistoryComboBox;
class TRightsFrame;
//---------------------------------------------------------------------------
class TCopyParamsFrame : public QWidget
{
  Q_OBJECT

public:
  explicit TCopyParamsFrame(QWidget * Parent);
  virtual ~TCopyParamsFrame();

  void SetParams(const TCopyParamType & Value);
  TCopyParamType GetParams();
  int GetCopyParamAttrs() const { return FCopyParamAttrs; }
  void SetCopyParamAttrs(int Value);
  // Loads the histories of the combo boxes
  void BeforeExecute();
  // Saves the histories of the combo boxes
  void AfterExecute();
  // Validates the masks and the speed (throws, the control is focused)
  void Validate();
  void Closing();

protected:
  virtual void changeEvent(QEvent * Event) override;

private:
  QGroupBox * TransferModeGroup;
  QRadioButton * TMTextButton;
  QRadioButton * TMBinaryButton;
  QRadioButton * TMAutomaticButton;
  QLabel * AsciiFileMaskLabel;
  THistoryComboBox * AsciiFileMaskCombo;
  QGroupBox * ChangeCaseGroup;
  QRadioButton * CCNoChangeButton;
  QRadioButton * CCUpperCaseButton;
  QRadioButton * CCLowerCaseButton;
  QRadioButton * CCLowerCaseShortButton;
  QCheckBox * ReplaceInvalidCharsCheck;
  QGroupBox * RemotePropertiesGroup;
  QCheckBox * PreserveRightsCheck;
  QLineEdit * RightsEdit;
  QAction * RightsEditButton;
  QCheckBox * IgnorePermErrorsCheck;
  QCheckBox * ClearArchiveCheck;
  QCheckBox * RemoveCtrlZAndBOMCheck;
  QCheckBox * EncryptNewFilesCheck;
  QGroupBox * CommonPropertiesGroup;
  QCheckBox * PreserveTimeCheck;
  QCheckBox * PreserveTimeDirsCheck;
  QCheckBox * CommonCalculateSizeCheck;
  QLabel * SpeedLabel3;
  THistoryComboBox * SpeedCombo;
  QGroupBox * LocalPropertiesGroup;
  QCheckBox * PreserveReadOnlyCheck;
  QGroupBox * OtherGroup;
  QLabel * IncludeFileMaskLabel;
  THistoryComboBox * IncludeFileMaskCombo;
  QPushButton * IncludeFileMaskButton;
  QCheckBox * NewerOnlyCheck;
  QCheckBox * ExcludeHiddenFilesCheck;
  QCheckBox * ExcludeEmptyDirectoriesCheck;
  TRightsFrame * FRightsFrame;

  TCopyParamType * FParams;
  int FCopyParamAttrs;
  bool FRemoveCtrlZ;
  bool FRightsModified;
  int FUpdating;

  void CreateControls();
  void SetParamsControls(const TCopyParamType & Value);
  void UpdateControls();
  void RightsEditButtonClick();
  void RightsFrameChange();
  void UpdateRightsByStr();
  void RightsEditFinished();
  void IncludeFileMaskButtonClick();
};
//---------------------------------------------------------------------------
#endif
