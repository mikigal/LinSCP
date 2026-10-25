// The dialogs of pages chosen in a navigation tree.
//---------------------------------------------------------------------------
#include "TreePagesDialog.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolButton>
#include <QTreeWidget>
//---------------------------------------------------------------------------
TTreePagesDialog::TTreePagesDialog(QWidget * Parent) :
  QDialog(Parent)
{
  FMainLayout = new QVBoxLayout(this);
  QSplitter * MainPanel = new QSplitter(Qt::Horizontal, this);
  MainPanel->setObjectName(QStringLiteral("MainPanel"));
  MainPanel->setChildrenCollapsible(false);
  NavigationTree = new QTreeWidget(MainPanel);
  NavigationTree->setObjectName(QStringLiteral("NavigationTree"));
  NavigationTree->setHeaderHidden(true);
  NavigationTree->setRootIsDecorated(false);
  NavigationTree->setItemsExpandable(false);
  NavigationTree->setMinimumWidth(fontMetrics().averageCharWidth() * 22);
  PageControl = new QStackedWidget(MainPanel);
  PageControl->setObjectName(QStringLiteral("PageControl"));
  MainPanel->addWidget(NavigationTree);
  MainPanel->addWidget(PageControl);
  MainPanel->setStretchFactor(1, 1);
  MainPanel->setSizes({ fontMetrics().averageCharWidth() * 24, fontMetrics().averageCharWidth() * 96 });
  FMainLayout->addWidget(MainPanel, 1);

  connect(NavigationTree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem * Item)
  {
    if (Item != nullptr)
    {
      PageControl->setCurrentWidget(FPages[Item->data(0, Qt::UserRole).toInt()]);
    }
  });
}
//---------------------------------------------------------------------------
QWidget * TTreePagesDialog::AddSheet(const char * Name, const QString & Caption, int Tag, QTreeWidgetItem * ParentNode)
{
  QScrollArea * Scroll = new QScrollArea(PageControl);
  Scroll->setWidgetResizable(true);
  Scroll->setFrameShape(QFrame::NoFrame);
  QWidget * Sheet = new QWidget(Scroll);
  Sheet->setObjectName(QString::fromLatin1(Name));
  QVBoxLayout * Layout = new QVBoxLayout(Sheet);
  Layout->setContentsMargins(0, 0, 0, 0);
  Scroll->setWidget(Sheet);
  PageControl->addWidget(Scroll);
  FPages[Tag] = Scroll;

  QTreeWidgetItem * Node =
    (ParentNode != nullptr) ? new QTreeWidgetItem(ParentNode) : new QTreeWidgetItem(NavigationTree);
  Node->setText(0, Caption);
  Node->setData(0, Qt::UserRole, Tag);
  return Sheet;
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TTreePagesDialog::NodeOf(int Tag)
{
  for (QTreeWidgetItemIterator It(NavigationTree); *It != nullptr; ++It)
  {
    if ((*It)->data(0, Qt::UserRole).toInt() == Tag)
    {
      return *It;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
void TTreePagesDialog::SetActivePage(int Tag)
{
  QTreeWidgetItem * Node = NodeOf(Tag);
  if (Node != nullptr)
  {
    NavigationTree->setCurrentItem(Node);
  }
}
//---------------------------------------------------------------------------
int TTreePagesDialog::ActivePage()
{
  QTreeWidgetItem * Node = NavigationTree->currentItem();
  return (Node != nullptr) ? Node->data(0, Qt::UserRole).toInt() : -1;
}
//---------------------------------------------------------------------------
void TTreePagesDialog::SetPageVisible(int Tag, bool Visible)
{
  QTreeWidgetItem * Node = NodeOf(Tag);
  if ((Node != nullptr) && (Node->isHidden() == Visible))
  {
    Node->setHidden(!Visible);
  }
}
//---------------------------------------------------------------------------
void TTreePagesDialog::ConnectControls()
{
  for (QAbstractButton * Button : findChildren<QAbstractButton *>())
  {
    if (Button->isCheckable())
    {
      connect(Button, &QAbstractButton::toggled, this, [this]() { UpdateControls(); });
    }
  }
  for (QComboBox * Combo : findChildren<QComboBox *>())
  {
    connect(Combo, &QComboBox::currentTextChanged, this, [this]() { UpdateControls(); });
  }
  for (QLineEdit * Edit : findChildren<QLineEdit *>())
  {
    connect(Edit, &QLineEdit::textChanged, this, [this]() { UpdateControls(); });
  }
  for (QSpinBox * Spin : findChildren<QSpinBox *>())
  {
    connect(Spin, &QSpinBox::valueChanged, this, [this]() { UpdateControls(); });
  }
}
//---------------------------------------------------------------------------
QGroupBox * TTreePagesDialog::AddGroup(QWidget * Page, const char * Name, const QString & Caption)
{
  QGroupBox * Group = new QGroupBox(Caption, Page);
  Group->setObjectName(QString::fromLatin1(Name));
  new QVBoxLayout(Group);
  static_cast<QVBoxLayout *>(Page->layout())->addWidget(Group);
  return Group;
}
//---------------------------------------------------------------------------
QCheckBox * TTreePagesDialog::AddCheck(QWidget * Parent, const char * Name, const QString & Caption)
{
  QCheckBox * Check = new QCheckBox(Caption, Parent);
  Check->setObjectName(QString::fromLatin1(Name));
  Parent->layout()->addWidget(Check);
  return Check;
}
//---------------------------------------------------------------------------
QRadioButton * TTreePagesDialog::AddRadio(QWidget * Parent, const char * Name, const QString & Caption)
{
  QRadioButton * Radio = new QRadioButton(Caption, Parent);
  Radio->setObjectName(QString::fromLatin1(Name));
  Parent->layout()->addWidget(Radio);
  return Radio;
}
//---------------------------------------------------------------------------
QSpinBox * TTreePagesDialog::CreateSpin(QWidget * Parent, const char * Name, int Min, int Max, int Step)
{
  QSpinBox * Spin = new QSpinBox(Parent);
  Spin->setObjectName(QString::fromLatin1(Name));
  Spin->setRange(Min, Max);
  Spin->setSingleStep(Step);
  return Spin;
}
//---------------------------------------------------------------------------
QComboBox * TTreePagesDialog::CreateCombo(QWidget * Parent, const char * Name, const QStringList & Items, bool Editable)
{
  QComboBox * Combo = new QComboBox(Parent);
  Combo->setObjectName(QString::fromLatin1(Name));
  Combo->addItems(Items);
  Combo->setEditable(Editable);
  if (Editable)
  {
    Combo->setInsertPolicy(QComboBox::NoInsert);
  }
  return Combo;
}
//---------------------------------------------------------------------------
QPushButton * TTreePagesDialog::CreateButton(QWidget * Parent, const char * Name, const QString & Caption)
{
  QPushButton * Button = new QPushButton(Caption, Parent);
  Button->setObjectName(QString::fromLatin1(Name));
  Button->setAutoDefault(false);
  return Button;
}
//---------------------------------------------------------------------------
QHBoxLayout * TTreePagesDialog::AddRow(QWidget * Parent)
{
  QHBoxLayout * Row = new QHBoxLayout();
  static_cast<QBoxLayout *>(Parent->layout())->addLayout(Row);
  return Row;
}
//---------------------------------------------------------------------------
QLineEdit * TTreePagesDialog::CreatePathEdit(
  QWidget * Parent, QHBoxLayout * Row, const char * Name, bool Directory, const QString & Title,
  const QString & OpenFilter)
{
  QLineEdit * Edit = new QLineEdit(Parent);
  Edit->setObjectName(QString::fromLatin1(Name));
  Row->addWidget(Edit, 1);
  QToolButton * Browse = new QToolButton(Parent);
  Browse->setObjectName(QString::fromLatin1(Name) + QStringLiteral("Browse"));
  Browse->setText(QStringLiteral("..."));
  Browse->setToolTip(tr("Browse"));
  Row->addWidget(Browse);
  connect(Browse, &QToolButton::clicked, this, [this, Edit, Directory, Title, OpenFilter]()
  {
    QString Path;
    if (Directory)
    {
      Path = QFileDialog::getExistingDirectory(this, Title, Edit->text());
    }
    else if (!OpenFilter.isEmpty())
    {
      Path = QFileDialog::getOpenFileName(this, Title, Edit->text(), OpenFilter);
    }
    else
    {
      Path = QFileDialog::getSaveFileName(this, Title, Edit->text(), QString(), nullptr, QFileDialog::DontConfirmOverwrite);
    }
    if (!Path.isEmpty())
    {
      Edit->setText(Path);
    }
  });
  return Edit;
}
//---------------------------------------------------------------------------
QTreeWidget * TTreePagesDialog::CreateListView(QWidget * Parent, const char * Name, const QStringList & Columns)
{
  QTreeWidget * View = new QTreeWidget(Parent);
  View->setObjectName(QString::fromLatin1(Name));
  View->setRootIsDecorated(false);
  View->setUniformRowHeights(true);
  View->setHeaderLabels(Columns);
  View->setMinimumHeight(fontMetrics().height() * 10);
  View->installEventFilter(this);
  connect(View, &QTreeWidget::itemSelectionChanged, this, [this]() { UpdateControls(); });
  return View;
}
//---------------------------------------------------------------------------
void TTreePagesDialog::AddListButtons(
  QWidget * Parent, QHBoxLayout * Layout, QTreeWidget * View, const std::vector<QPushButton *> & Buttons)
{
  Layout->addWidget(View, 1);
  QVBoxLayout * ButtonsLayout = new QVBoxLayout();
  for (QPushButton * Button : Buttons)
  {
    Button->setParent(Parent);
    ButtonsLayout->addWidget(Button);
  }
  ButtonsLayout->addStretch(1);
  Layout->addLayout(ButtonsLayout);
}
//---------------------------------------------------------------------------
int TTreePagesDialog::ListIndex(QTreeWidget * View)
{
  QTreeWidgetItem * Item = View->currentItem();
  return ((Item != nullptr) && Item->isSelected()) ? View->indexOfTopLevelItem(Item) : -1;
}
//---------------------------------------------------------------------------
void TTreePagesDialog::SetListIndex(QTreeWidget * View, int Index)
{
  if ((Index >= 0) && (Index < View->topLevelItemCount()))
  {
    View->setCurrentItem(View->topLevelItem(Index));
  }
}
//---------------------------------------------------------------------------
