// The properties of files (upstream's TPropertiesDialog,
// forms/Properties.dfm): the size, the owner, the group and the
// permissions, the checksum and the tags (S3).
//---------------------------------------------------------------------------
#ifndef PropertiesDialogH
#define PropertiesDialogH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <RemoteFiles.h>
#include <Terminal.h>

#include <QDialog>
//---------------------------------------------------------------------------
class QCheckBox;
class QComboBox;
class QFrame;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class TRightsFrame;
//---------------------------------------------------------------------------
class TPropertiesDialog : public QDialog
{
  Q_OBJECT

public:
  TPropertiesDialog(QWidget * Parent,
    TStrings * FileList, const UnicodeString & Directory,
    const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
    TStrings * ChecksumAlgs,
    int AllowedChanges, int Options, TCalculateSizeEvent OnCalculateSize,
    TCalculateChecksumEvent OnCalculateChecksum);
  virtual ~TPropertiesDialog();

  bool Execute(TRemoteProperties & Properties);

protected:
  virtual void accept() override;
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  QTabWidget * PageControl;
  QWidget * CommonSheet;
  QWidget * ChecksumSheet;
  QWidget * TagsSheet;
  QLabel * FileIconImage;
  QLineEdit * FileLabel;
  QLineEdit * LocationLabel;
  QLineEdit * SizeLabel;
  QPushButton * CalculateSizeButton;
  QLabel * LinksToLabelLabel;
  QLineEdit * LinksToLabel;
  QFrame * GroupOwnerRightsBevel;
  QLabel * OwnerLabel;
  QComboBox * OwnerComboBox;
  QLineEdit * OwnerView;
  QLabel * GroupLabel;
  QComboBox * GroupComboBox;
  QLineEdit * GroupView;
  QLabel * RightsLabel;
  TRightsFrame * RightsFrame;
  QFrame * RecursiveBevel;
  QCheckBox * RecursiveCheck2;
  QComboBox * ChecksumAlgEdit;
  QPushButton * ChecksumButton;
  QGroupBox * ChecksumGroup;
  QLineEdit * ChecksumEdit;
  QLabel * ChecksumUnknownLabel;
  QTreeWidget * ChecksumView;
  QTreeWidget * TagsView;
  QPushButton * AddTagButton;
  QPushButton * RemoveTagButton;
  QPushButton * EditTagButton;
  QPushButton * OkButton;
  QPushButton * CancelButton;

  int FAllowedChanges;
  int FOptions;
  TStrings * FFileList;
  const TRemoteTokenList * FGroupList;
  const TRemoteTokenList * FUserList;
  TStrings * FChecksumAlgs;
  TRemoteProperties FOrigProperties;
  bool FMultiple;
  bool FAnyDirectories;
  bool FAllowCalculateStats;
  bool FStatsNotCalculated;
  TCalculateSizeEvent FOnCalculateSize;
  TCalculateChecksumEvent FOnCalculateChecksum;
  bool FChecksumLoaded;
  UnicodeString FAlgUsed;
  bool FMultipleChecksum;
  bool FShown;

  void CreateControls();
  void LoadInfo();
  void LoadRemoteTokens(QComboBox * ComboBox, const TRemoteTokenList * List);
  UnicodeString LoadRemoteToken(const TRemoteToken & Token);
  void LoadRemoteToken(
    QComboBox * ComboBox, QLineEdit * View, QLabel * Label, bool Valid, const TRemoteToken & Token, int Change);
  TRemoteToken StoreRemoteToken(const TRemoteToken & Orig,
    UnicodeString Text, int Message, const TRemoteTokenList * List);
  void StoreRemoteToken(QComboBox * ComboBox,
    int ChangeFlag, TValidProperty PropertyFlag, const TRemoteToken & Orig,
    TRemoteToken & Token, int Message, const TRemoteTokenList * List,
    TRemoteProperties & Properties);
  void StoreRemoteToken(unsigned int ID, const UnicodeString & Text,
    const TRemoteTokenList * List, TRemoteToken & Result);
  void ValidateRemoteToken(
    const TRemoteToken & Orig, int Message, QComboBox * ComboBox, const TRemoteTokenList * List);
  void UpdateControls();
  void ControlChange();
  void LoadStats(__int64 FilesSize, const TCalculateSizeStats & Stats);
  void SetFileProperties(const TRemoteProperties & Value);
  TRemoteProperties GetFileProperties();
  void CalculateSizeButtonClick();
  void CalculateChecksum();
  void NeedChecksum();
  bool ChecksumSupported();
  void ResetChecksum();
  void CalculatedChecksum(
    const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash);
  void ChecksumAlgEditChange();
  void PageControlChange();
  void CopyItems(QTreeWidget * View);
  QTreeWidgetItem * AddTag(const UnicodeString & Key, const UnicodeString & Value);
  void AddEditTag(bool Add);
  void RemoveTagButtonClick();
};
//---------------------------------------------------------------------------
#endif
