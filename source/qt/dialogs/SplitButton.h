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
  // Without the drop-down part, a plain push button
  void SetDropDownVisible(bool Visible);
  bool IsDropDownVisible() const { return FDropDownVisible; }
  QMenu * GetDropDownMenu() const { return FMenu; }
  // Shows the menu below the button
  void ShowDropDownMenu();

  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

Q_SIGNALS:
  // The arrow was clicked, without a menu of the button (the menu is
  // created when shown)
  void DropDownRequested();

protected:
  virtual void paintEvent(QPaintEvent * Event) override;
  virtual void mousePressEvent(QMouseEvent * Event) override;
  virtual void keyPressEvent(QKeyEvent * Event) override;

private:
  QMenu * FMenu;
  bool FDropDownVisible;

  int ArrowWidth() const;
};
//---------------------------------------------------------------------------
#endif
