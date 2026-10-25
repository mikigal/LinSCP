// The bookmarks and location profiles of the main window (upstream has them
// in forms/CustomScpExplorer.cpp and forms/ScpCommander.cpp): opening a
// directory or a bookmark, adding bookmarks, their menus and shortcuts.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Bookmarks.h>
#include <ShortCuts.h>

#include "DirPanel.h"
#include "ScpCommander.h"

#include <QAction>
#include <QMenu>
//---------------------------------------------------------------------------
TDirPanel * TScpCommanderForm::Panel(TOperationSide Side) const
{
  return (Side == osLocal) ? static_cast<TDirPanel *>(FLocalPanel) : static_cast<TDirPanel *>(FRemotePanel);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OpenDirectory(TOperationSide Side)
{
  if (!IsBusy() && Panel(Side)->IsReady())
  {
    DoOpenDirectoryDialog(odBrowse, Side);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::AddBookmark(TOperationSide Side)
{
  if (!IsBusy() && Panel(Side)->IsReady())
  {
    DoOpenDirectoryDialog(odAddBookmark, Side);
  }
}
//---------------------------------------------------------------------------
TStrings * TScpCommanderForm::CreateVisitedDirectories(TOperationSide Side)
{
  std::unique_ptr<TStringList> VisitedDirectories(CreateSortedStringList(Side == osRemote));
  for (const UnicodeString & Path : Panel(Side)->GetHistory())
  {
    if (VisitedDirectories->IndexOf(Path) < 0)
    {
      VisitedDirectories->Add(Path);
    }
  }
  return VisitedDirectories.release();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoOpenDirectoryDialog(TOpenDirectoryMode Mode, TOperationSide Side)
{
  bool UseLocationProfiles;
  do
  {
    UseLocationProfiles = AppConfiguration->UseLocationProfiles;
    TTerminal * Terminal = GetTerminal();
    // Location profiles dialog is not ready to be run without terminal
    if (UseLocationProfiles && (Terminal != NULL) && Terminal->Active)
    {
      std::unique_ptr<TStrings> LocalDirectories(CreateVisitedDirectories(osLocal));
      std::unique_ptr<TStrings> RemoteDirectories(CreateVisitedDirectories(osRemote));

      UnicodeString Local = FLocalPanel->GetPath();
      UnicodeString Remote = FRemotePanel->GetPath();

      if (LocationProfilesDialog(Mode, Side, Local, Remote, LocalDirectories.get(),
            RemoteDirectories.get(), Terminal))
      {
        DoOpenBookmark(Local, Remote);
      }
    }
    else
    {
      DoOpenDirectoryDialogBookmarks(Mode, Side);
    }

    // for second and further rounds, always do browse only
    Mode = odBrowse;
  }
  while (UseLocationProfiles != AppConfiguration->UseLocationProfiles);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoOpenDirectoryDialogBookmarks(TOpenDirectoryMode Mode, TOperationSide Side)
{
  bool Continue = true;
  if (Mode == odAddBookmark)
  {
    TMessageParams Params(mpNeverAskAgainCheck);
    Params.NeverAskAgainTitle = LoadStr(ADD_BOOKMARK_SHARED);
    Params.NeverAskAgainCheckedInitially = AppConfiguration->UseSharedBookmarks;

    UnicodeString Message = MainInstructions(FMTLOAD(ADD_BOOKMARK_CONFIRM, (Panel(Side)->GetPath())));
    unsigned int Answer = MessageDialog(Message, qtConfirmation, qaOK | qaCancel, &Params);
    if (Answer == qaNeverAskAgain)
    {
      Continue = true;
      AppConfiguration->UseSharedBookmarks = true;
    }
    else if (Answer == qaOK)
    {
      Continue = true;
      AppConfiguration->UseSharedBookmarks = false;
    }
    else
    {
      Continue = false;
    }
  }

  if (Continue)
  {
    std::unique_ptr<TStrings> VisitedDirectories(CreateVisitedDirectories(Side));
    UnicodeString Name = Panel(Side)->GetPath();
    TTerminal * Terminal = GetTerminal();
    bool Repeat;
    do
    {
      Repeat = false;
      if (::DoOpenDirectoryDialog(Mode, Side, Name, VisitedDirectories.get(), Terminal,
            // do not allow switching to location profiles,
            // if we are not connected
            (Terminal != NULL) && Terminal->Active))
      {
        if (!TryOpenDirectory(Side, Name))
        {
          Repeat = true;
          Mode = odBrowse;
        }
      }
    }
    while (Repeat);
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::TryOpenDirectory(TOperationSide Side, const UnicodeString & Path)
{
  bool Result = true;
  bool Remote = (Side == osRemote);
  TTerminal * Terminal = GetTerminal();
  try
  {
    UnicodeString APath = Path;
    if (Remote)
    {
      Terminal->ExceptionOnFail = true;
    }
    else
    {
      APath = ExpandEnvironmentVariables(Path);
    }

    ::rtl::TFinally Finally([&]()
    {
      if (Remote)
      {
        Terminal->ExceptionOnFail = false;
      }
    });
    Panel(Side)->ChangeDirectory(APath);
  }
  catch (Exception & E)
  {
    if (!Remote || Terminal->Active)
    {
      ShowExtendedExceptionEx(Terminal, &E);
      Result = false;
    }
    else
    {
      throw;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoOpenBookmark(UnicodeString Local, UnicodeString Remote)
{
  // make sure that whatever path is valid it is opened first and only
  // after that an eventual error is reported
  ::rtl::TFinally Finally([&]()
  {
    if (!Remote.IsEmpty())
    {
      // While we might get here when the session is closed (from location profiles),
      // it's not a problem as the panel does nothing then
      FRemotePanel->ChangeDirectory(Remote);
    }
  });
  if (!Local.IsEmpty())
  {
    FLocalPanel->ChangeDirectory(ExpandEnvironmentVariables(Local));
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::OpenBookmark(TOperationSide Side, TBookmark * Bookmark)
{
  bool Result;
  if (AppConfiguration->UseLocationProfiles)
  {
    DoOpenBookmark(Bookmark->Local, Bookmark->Remote);
    Result = true;
  }
  else
  {
    UnicodeString Path = Bookmark->GetSideDirectory(Side);
    Result = !Path.IsEmpty();
    if (Result)
    {
      TryOpenDirectory(Side, Path);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OpenBookmarkByKey(TOperationSide Side, bool Shared, const UnicodeString & Node,
  const UnicodeString & Name)
{
  // The bookmark is looked up again, the lists may have been replaced
  TTerminal * Terminal = GetTerminal();
  TBookmarkList * BookmarkList =
    Shared ? AppConfiguration->SharedBookmarks :
      ((Terminal != NULL) ? AppConfiguration->Bookmarks[Terminal->SessionData->SessionKey] : NULL);
  TBookmark * Bookmark = (BookmarkList != NULL) ? BookmarkList->FindByName(Node, Name) : NULL;
  if ((Bookmark != NULL) && !IsBusy() && Panel(Side)->IsReady())
  {
    OpenBookmark(Side, Bookmark);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateOpenDirMenuList(QMenu * Menu, TOperationSide Side, TBookmarkList * BookmarkList, bool Shared)
{
  if (BookmarkList == NULL)
  {
    return;
  }

  auto AddItem = [&](QMenu * Parent, const UnicodeString & Caption, TBookmark * Bookmark)
  {
    QAction * Item = Parent->addAction(ToQString(EscapeHotkey(Caption)));
    Item->setShortcut(ShortCutToKeySequence(Bookmark->ShortCut));
    // Shown only, the window handles the shortcuts itself
    Item->setShortcutContext(Qt::WidgetShortcut);
    UnicodeString Node = Bookmark->Node;
    UnicodeString Name = Bookmark->Name;
    connect(Item, &QAction::triggered, this, [this, Side, Shared, Node, Name]()
    {
      OpenBookmarkByKey(Side, Shared, Node, Name);
    });
    return Item;
  };

  if (!AppConfiguration->UseLocationProfiles)
  {
    std::unique_ptr<TStringList> Directories(new TStringList());
    Directories->CaseSensitive = (Side == osRemote);
    for (int Index = 0; Index < BookmarkList->Count; Index++)
    {
      TBookmark * Bookmark = BookmarkList->Bookmarks[Index];
      UnicodeString Directory = Bookmark->GetSideDirectory(Side);
      if (!Directory.IsEmpty() && (Directories->IndexOf(Directory) < 0))
      {
        AddItem(Menu, Directory, Bookmark);
        Directories->Add(Directory);
      }
    }
  }
  else
  {
    std::unique_ptr<TStringList> Folders(CreateSortedStringList());
    for (int Index = 0; Index < BookmarkList->Count; Index++)
    {
      TBookmark * Bookmark = BookmarkList->Bookmarks[Index];
      if (!Bookmark->Node.IsEmpty() && (Folders->IndexOf(Bookmark->Node) < 0))
      {
        Folders->Add(Bookmark->Node);
      }
    }

    for (int Index = 0; Index < Folders->Count; Index++)
    {
      QMenu * Folder = Menu->addMenu(QIcon::fromTheme(QStringLiteral("folder")), ToQString(Folders->Strings[Index]));
      Folders->Objects[Index] = reinterpret_cast<TObject *>(Folder);
    }

    for (int Index = 0; Index < BookmarkList->Count; Index++)
    {
      TBookmark * Bookmark = BookmarkList->Bookmarks[Index];
      QMenu * Parent =
        Bookmark->Node.IsEmpty() ? Menu : reinterpret_cast<QMenu *>(Folders->Objects[Folders->IndexOf(Bookmark->Node)]);
      QAction * Item = AddItem(Parent, Bookmark->Name, Bookmark);

      if (((Bookmark->Name == Bookmark->Local) || (Bookmark->Name == Bookmark->Remote)) &&
          (Bookmark->Local.IsEmpty() || Bookmark->Remote.IsEmpty()))
      {
        // No hint for location profiles that are actually mere "bookmarks"
      }
      else
      {
        UnicodeString Hint = FORMAT(LoadStrPart(LOCATION_PROFILE_HINT, 1), (Bookmark->Name));
        if (!Bookmark->Local.IsEmpty())
        {
          AddToList(Hint, FORMAT(LoadStrPart(LOCATION_PROFILE_HINT, 2), (Bookmark->Local)), L"\n");
        }
        if (!Bookmark->Remote.IsEmpty())
        {
          AddToList(Hint, FORMAT(LoadStrPart(LOCATION_PROFILE_HINT, 3), (Bookmark->Remote)), L"\n");
        }
        Item->setToolTip(ToQString(Hint));
      }
    }
    Menu->setToolTipsVisible(true);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateOpenDirMenu(TDirPanel * APanel, QMenu * Menu)
{
  TOperationSide Side = APanel->GetSide();
  TTerminal * Terminal = GetTerminal();
  if ((Terminal != NULL) && Terminal->Active)
  {
    CreateOpenDirMenuList(Menu, Side, AppConfiguration->Bookmarks[Terminal->SessionData->SessionKey], false);
  }
  int Count = Menu->actions().count();
  CreateOpenDirMenuList(Menu, Side, AppConfiguration->SharedBookmarks, true);
  if ((Count > 0) && (Menu->actions().count() > Count))
  {
    Menu->insertSeparator(Menu->actions().at(Count));
  }
  if (Menu->isEmpty())
  {
    Menu->addAction(tr("(No bookmarks)"))->setEnabled(false);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateBookmarkShortCuts()
{
  // The shortcuts of the shared bookmarks open them in the current panel
  // (upstream's KeyDown)
  qDeleteAll(FBookmarkShortCutActions);
  FBookmarkShortCutActions.clear();
  TBookmarkList * BookmarkList = AppConfiguration->SharedBookmarks;
  if (BookmarkList != NULL)
  {
    for (int Index = 0; Index < BookmarkList->Count; Index++)
    {
      TBookmark * Bookmark = BookmarkList->Bookmarks[Index];
      QKeySequence Sequence = ShortCutToKeySequence(Bookmark->ShortCut);
      if (IsCustomShortCut(Bookmark->ShortCut) && !Sequence.isEmpty())
      {
        QAction * Action = new QAction(this);
        Action->setObjectName(QStringLiteral("BookmarkShortCutAction"));
        Action->setShortcut(Sequence);
        addAction(Action);
        UnicodeString Node = Bookmark->Node;
        UnicodeString Name = Bookmark->Name;
        connect(Action, &QAction::triggered, this, [this, Node, Name]()
        {
          OpenBookmarkByKey(FCurrentPanel->GetSide(), true, Node, Name);
        });
        FBookmarkShortCutActions.push_back(Action);
      }
    }
  }
}
//---------------------------------------------------------------------------
