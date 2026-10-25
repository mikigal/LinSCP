// The progress of the operations on the button of the application in the
// task manager (upstream's ITaskbarList3 of forms/CustomScpExplorer.cpp),
// through the com.canonical.Unity.LauncherEntry signals of the session bus,
// which Plasma's task manager and the docks of GNOME follow.
//---------------------------------------------------------------------------
#ifndef TaskbarProgressH
#define TaskbarProgressH
//---------------------------------------------------------------------------
class TTaskbarProgress
{
public:
  TTaskbarProgress();
  ~TTaskbarProgress();

  // In percents; a negative progress is indeterminate (not shown, as the
  // protocol has no such state)
  void SetProgress(int Progress);
  void SetNoProgress();
  // The application wants the attention of the user (upstream flashes the
  // button of the task bar)
  void SetUrgent(bool Urgent);

private:
  // As last sent
  int FProgress;
  bool FUrgent;

  void Update(bool ProgressVisible, int Progress, bool Urgent);
};
//---------------------------------------------------------------------------
#endif
