// A push button with a drop-down menu by its arrow.
//---------------------------------------------------------------------------
#include "SplitButton.h"

#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QStylePainter>
//---------------------------------------------------------------------------
TSplitButton::TSplitButton(QWidget * Parent) :
  QPushButton(Parent)
{
  FMenu = nullptr;
}
//---------------------------------------------------------------------------
int TSplitButton::ArrowWidth() const
{
  return style()->pixelMetric(QStyle::PM_MenuButtonIndicator, nullptr, this) + 4;
}
//---------------------------------------------------------------------------
QSize TSplitButton::sizeHint() const
{
  return QPushButton::sizeHint() + QSize(ArrowWidth(), 0);
}
//---------------------------------------------------------------------------
QSize TSplitButton::minimumSizeHint() const
{
  return QPushButton::minimumSizeHint() + QSize(ArrowWidth(), 0);
}
//---------------------------------------------------------------------------
void TSplitButton::paintEvent(QPaintEvent * /*Event*/)
{
  // The style's button with a menu, its arrow, and the line that splits it
  QStylePainter Painter(this);
  QStyleOptionButton Option;
  initStyleOption(&Option);
  Option.features |= QStyleOptionButton::HasMenu;
  Painter.drawControl(QStyle::CE_PushButton, Option);
  int X = (layoutDirection() == Qt::RightToLeft) ? ArrowWidth() : (width() - ArrowWidth());
  QColor Line = palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::ButtonText);
  Line.setAlphaF(0.3);
  Painter.setPen(Line);
  Painter.drawLine(X, 5, X, height() - 6);
}
//---------------------------------------------------------------------------
void TSplitButton::ShowDropDownMenu()
{
  if ((FMenu != nullptr) && isEnabled())
  {
    setDown(true);
    FMenu->exec(mapToGlobal(QPoint(0, height())));
    setDown(false);
  }
}
//---------------------------------------------------------------------------
void TSplitButton::mousePressEvent(QMouseEvent * Event)
{
  int X = Event->position().toPoint().x();
  bool OnArrow = (layoutDirection() == Qt::RightToLeft) ? (X < ArrowWidth()) : (X >= width() - ArrowWidth());
  if ((Event->button() == Qt::LeftButton) && OnArrow && (FMenu != nullptr))
  {
    ShowDropDownMenu();
    Event->accept();
  }
  else
  {
    QPushButton::mousePressEvent(Event);
  }
}
//---------------------------------------------------------------------------
void TSplitButton::keyPressEvent(QKeyEvent * Event)
{
  // As a split button of Windows: Alt+Down shows the menu
  if ((Event->key() == Qt::Key_Down) && (Event->modifiers() & Qt::AltModifier))
  {
    ShowDropDownMenu();
  }
  else
  {
    QPushButton::keyPressEvent(Event);
  }
}
//---------------------------------------------------------------------------
