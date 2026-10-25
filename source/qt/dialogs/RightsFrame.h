// The permissions editor (upstream's TRightsFrame, forms/Rights.dfm): the
// read, write and execute checks of the owner, group and others, the
// special bits, the octal value and "add X to directories". Embedded in
// the properties dialog; the transfer settings drop it down from an edit
// box (the popup mode).
//---------------------------------------------------------------------------
#ifndef RightsFrameH
#define RightsFrameH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <RemoteFiles.h>

#include <QCheckBox>
#include <QFrame>

#include <functional>
//---------------------------------------------------------------------------
class QAction;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;
//---------------------------------------------------------------------------
// A check box that cycles unchecked, checked and, when it is tristate,
// grayed (upstream's TGrayedCheckBox; Qt goes through the grayed state
// between unchecked and checked)
class TGrayedCheckBox : public QCheckBox
{
  Q_OBJECT

public:
  TGrayedCheckBox(const QString & Text, QWidget * Parent);

protected:
  virtual void nextCheckState() override;
};
//---------------------------------------------------------------------------
class TRightsFrame : public QFrame
{
  Q_OBJECT

public:
  explicit TRightsFrame(QWidget * Parent);

  TRights GetRights();
  void SetRights(const TRights & Value);
  bool GetAddXToDirectories() const;
  void SetAddXToDirectories(bool Value);
  bool GetAllowAddXToDirectories() const { return FAllowAddXToDirectories; }
  void SetAllowAddXToDirectories(bool Value);
  bool GetAllowUndef() const;
  void SetAllowUndef(bool Value);
  // The rights as text ("rw-r--r--"), with "(+x)" when X is added to
  // directories; the text or the octal value can be set
  UnicodeString GetText();
  void SetText(const UnicodeString & Value);
  QCheckBox * GetCheck(TRights::TRight Right) const { return FChecks[Right]; }
  // The S3 access control list (read, read ACL, write ACL)
  void DisplayAsAcl();
  // The frame is a popup window, shown under its parent edit box
  void SetPopup(bool Value);
  void DropDown(QWidget * PopupParent);
  void CloseUp();

  std::function<void()> OnChange;
  // The popup was closed
  std::function<void()> OnCloseUp;

protected:
  virtual void changeEvent(QEvent * Event) override;
  virtual void contextMenuEvent(QContextMenuEvent * Event) override;
  virtual void hideEvent(QHideEvent * Event) override;
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  QCheckBox * FChecks[TRights::rrLast + 1];
  QToolButton * OwnerButton;
  QToolButton * GroupButton;
  QToolButton * OthersButton;
  QLabel * OctalLabel;
  QLineEdit * OctalEdit;
  QCheckBox * DirectoriesXCheck;
  QPushButton * CloseButton;
  QAction * NoRightsAction;
  QAction * DefaultRightsAction;
  QAction * AllRightsAction;
  QAction * LeaveRightsAsIsAction;
  QAction * CopyTextAction;
  QAction * CopyOctalAction;
  QAction * PasteAction;
  bool FAllowAddXToDirectories;
  bool FInitialized;
  bool FAcl;
  bool FPopup;
  bool FOctalModified;
  int FUpdating;
  int FSettingOctal;

  TRights::TState GetState(TRights::TRight Right) const;
  void SetState(TRights::TRight Right, TRights::TState Value);
  QAction * CreateAction(const char * Name, const QString & Caption, const QString & IconName,
    const QKeySequence & ShortCut);
  void CycleRights(TRights::TRightGroup RightGroup);
  void ActionExecute(QAction * Action);
  void UpdateActions();
  void UpdateControls();
  void DoChange();
  bool DirectoriesXEffective();
  void UpdateOctalEdit();
  void UpdateByOctal();
  void OctalEditChanged();
  void OctalEditFinished();
  void DisplayAsAcl(TRights::TRight ReadRight, TRights::TRight WriteRight,
    TRights::TRight ExecRight, TRights::TRight SpecialRight);
};
//---------------------------------------------------------------------------
#endif
