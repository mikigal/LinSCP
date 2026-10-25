// The dialogs of pages chosen in a navigation tree (upstream's Preferences
// and SiteAdvanced dialogs): the tree, the pages, and the helpers creating
// the controls of the pages, named as upstream's components.
//---------------------------------------------------------------------------
#ifndef TreePagesDialogH
#define TreePagesDialogH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QStringList>

#include <map>
#include <vector>
//---------------------------------------------------------------------------
class QCheckBox;
class QComboBox;
class QGroupBox;
class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
//---------------------------------------------------------------------------
class TTreePagesDialog : public QDialog
{
public:
  explicit TTreePagesDialog(QWidget * Parent);

protected:
  QTreeWidget * NavigationTree;
  QStackedWidget * PageControl;

  // The layout of the dialog, for its buttons below the pages
  QVBoxLayout * MainLayout() const { return FMainLayout; }
  // A page (the scrolled widget of the name, with a vertical layout) and
  // its node, of the tag, under the parent node
  QWidget * AddSheet(const char * Name, const QString & Caption, int Tag, QTreeWidgetItem * ParentNode);
  QTreeWidgetItem * NodeOf(int Tag);
  void SetActivePage(int Tag);
  int ActivePage();
  // Shown in the tree (the pages that do not apply are not)
  void SetPageVisible(int Tag, bool Visible);
  // The changes of the controls update the others (UpdateControls)
  void ConnectControls();
  virtual void UpdateControls() = 0;

  QGroupBox * AddGroup(QWidget * Page, const char * Name, const QString & Caption);
  QCheckBox * AddCheck(QWidget * Parent, const char * Name, const QString & Caption);
  QRadioButton * AddRadio(QWidget * Parent, const char * Name, const QString & Caption);
  QSpinBox * CreateSpin(QWidget * Parent, const char * Name, int Min, int Max, int Step = 1);
  QComboBox * CreateCombo(QWidget * Parent, const char * Name, const QStringList & Items, bool Editable = false);
  QPushButton * CreateButton(QWidget * Parent, const char * Name, const QString & Caption);
  QHBoxLayout * AddRow(QWidget * Parent);
  // An edit with a browse button in the row; a directory, or a file of the
  // filter to open (else to save)
  QLineEdit * CreatePathEdit(
    QWidget * Parent, QHBoxLayout * Row, const char * Name, bool Directory, const QString & Title,
    const QString & OpenFilter = QString());
  // A list of the columns, its selection updates the controls (its keys
  // go to the event filter of the dialog)
  QTreeWidget * CreateListView(QWidget * Parent, const char * Name, const QStringList & Columns);
  void AddListButtons(QWidget * Parent, QHBoxLayout * Layout, QTreeWidget * View, const std::vector<QPushButton *> & Buttons);
  static int ListIndex(QTreeWidget * View);
  static void SetListIndex(QTreeWidget * View, int Index);

private:
  QVBoxLayout * FMainLayout;
  std::map<int, QWidget *> FPages;
};
//---------------------------------------------------------------------------
#endif
