// A push button with a drop-down menu by its arrow (the VCL's
// bsSplitButton): the button runs its action, the arrow shows the menu; it
// can be the default button of its dialog.
//---------------------------------------------------------------------------
#ifndef SplitButtonH
#define SplitButtonH
//---------------------------------------------------------------------------
#include <QPushButton>
//---------------------------------------------------------------------------
class QMenu;
//---------------------------------------------------------------------------
class TSplitButton : public QPushButton
{
  Q_OBJECT

public:
  explicit TSplitButton(QWidget * Parent);

  void SetDropDownMenu(QMenu * Menu) { FMenu = Menu; }
  QMenu * GetDropDownMenu() const { return FMenu; }
  // Shows the menu below the button
  void ShowDropDownMenu();

  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

protected:
  virtual void paintEvent(QPaintEvent * Event) override;
  virtual void mousePressEvent(QMouseEvent * Event) override;
  virtual void keyPressEvent(QKeyEvent * Event) override;

private:
  QMenu * FMenu;

  int ArrowWidth() const;
};
//---------------------------------------------------------------------------
#endif
