// The path above the files of a panel (upstream's TPathLabel,
// packages/my/PathLabel.pas): in the colors of an active caption for the
// active panel, the directories of the path clicked to go to them, the
// filter mask (*.* under the mouse when none) clicked to change it,
// shortened with "..." to fit.
//---------------------------------------------------------------------------
#ifndef PathLabelH
#define PathLabelH
//---------------------------------------------------------------------------
#include <QLabel>
//---------------------------------------------------------------------------
class TPathLabel : public QLabel
{
  Q_OBJECT

public:
  explicit TPathLabel(QWidget * Parent);

  // The full path is the text of the label
  void SetPath(const QString & Path);
  QString GetPath() const { return text(); }
  void SetMask(const QString & Mask);
  QString GetMask() const { return FMask; }
  // Of the panel with the focus
  void SetActive(bool Active);
  bool IsActive() const { return FActive; }
  // The path as shown, with its trailing separator (and shortened)
  QString GetDisplayPath() const;

  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

Q_SIGNALS:
  // A directory of the path (the full path of it) was clicked
  void PathClicked(const QString & Path);
  void MaskClicked();
  void DoubleClicked();

protected:
  virtual void paintEvent(QPaintEvent * Event) override;
  virtual void mouseMoveEvent(QMouseEvent * Event) override;
  virtual void mouseReleaseEvent(QMouseEvent * Event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent * Event) override;
  virtual void enterEvent(QEnterEvent * Event) override;
  virtual void leaveEvent(QEvent * Event) override;

private:
  QString FMask;
  bool FActive;
  bool FMouseInView;
  int FMouseX;
  // Of the last layout: what is shown, the beginning of the path shortened
  // (and what shows it instead), the mask shown
  QString FDisplayPath;
  QString FShortenedPath;
  QString FShortenedDisplayPath;
  QString FDisplayMask;
  QString FVirtualMask;
  bool FVirtualMaskHidden;

  // The text to show in the width (upstream's DoDrawTextIntern)
  void LayoutText(int Width);
  // The part of the displayed path under the mouse, or the mask
  // (HotTrackMask)
  QString HotTrackPath() const;
  QColor BackgroundColor() const;
  QColor TextColor() const;
  QColor HotTrackColor() const;
};
//---------------------------------------------------------------------------
#endif
