// Declarations of the Qt front end shared by its windows (upstream has them
// in WinInterface.h).
//---------------------------------------------------------------------------
#ifndef QtInterfaceH
#define QtInterfaceH
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include <SessionInfo.h>

#include <functional>
//---------------------------------------------------------------------------
class TRemoteTokenList;
class TTerminal;
class QWidget;
struct TCalculateSizeStats;
class TRemoteProperties;
//---------------------------------------------------------------------------
const int mpNeverAskAgainCheck =   0x01;
const int mpAllowContinueOnError = 0x02;
//---------------------------------------------------------------------------
struct TMessageParams
{
  TMessageParams(unsigned int AParams = 0);
  TMessageParams(const TQueryParams * AParams);

  const TQueryButtonAlias * Aliases;
  unsigned int AliasesCount;
  unsigned int Params;
  unsigned int Timer;
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  unsigned int TimerAnswers;
  TQueryType TimerQueryType;
  unsigned int Timeout;
  unsigned int TimeoutAnswer;
  unsigned int TimeoutResponse;
  UnicodeString NeverAskAgainTitle;
  unsigned int NeverAskAgainAnswer;
  bool NeverAskAgainCheckedInitially;
  UnicodeString CustomCaption;

private:
  void Reset();
};
//---------------------------------------------------------------------------
// dialogs/MessageDialog.cpp
unsigned int MessageDialog(const UnicodeString & Msg, TQueryType Type,
  unsigned int Answers, const TMessageParams * Params = NULL);
unsigned int MessageDialog(int Ident, TQueryType Type,
  unsigned int Answers, const TMessageParams * Params = NULL);
unsigned int SimpleErrorDialog(const UnicodeString & Msg, const UnicodeString & MoreMessages = L"");
unsigned int MoreMessageDialog(const UnicodeString & Message,
  TStrings * MoreMessages, TQueryType Type, unsigned int Answers,
  const TMessageParams * Params = NULL);
unsigned int ExceptionMessageDialog(Exception * E, TQueryType Type,
  const UnicodeString & MessageFormat = L"", unsigned int Answers = qaOK,
  const TMessageParams * Params = NULL);
unsigned int FatalExceptionMessageDialog(
  Exception * E, TQueryType Type, const UnicodeString & MessageFormat, unsigned int Answers,
  const TMessageParams * Params);

void ShowExtendedExceptionEx(TTerminal * Terminal, Exception * E);

// dialogs/CustomDialog.cpp
bool InputDialog(const UnicodeString & ACaption,
  const UnicodeString & APrompt, UnicodeString & Value,
  TStrings * History = NULL, bool PathInput = false, bool Echo = true);
// Saves the session as a site, asking for its name and whether to save
// its password unless ForceDialog is false and the answers are known;
// returns the new site or NULL when cancelled
TSessionData * DoSaveSession(TSessionData * SessionData,
  TSessionData * OriginalSession, bool ForceDialog, TStrings * AdditionalFolders);
// Checks that a site can be saved under the name (or overwrite a site)
void SessionNameValidate(const UnicodeString & Text, const UnicodeString & OriginalName);
// Asks for the current master password
bool DoMasterPasswordDialog();

// dialogs/CopyDialog.cpp
// Confirms a transfer of the files to the target directory
bool DoCopyDialog(bool ToRemote, TStrings * FileList, UnicodeString & TargetDirectory);
bool DoCreateDirectoryDialog(UnicodeString & Directory);

bool DoTagDialog(bool Add, TStrings * Tags, UnicodeString & Key, UnicodeString & Value);

// dialogs/PropertiesDialog.cpp
// The properties that can be changed
const int cpMode =  0x01;
const int cpOwner = 0x02;
const int cpGroup = 0x04;
const int cpAcl =   0x08;
// Owners and groups are identified by their IDs, the tags can be edited
const int poUserGroupByID = 0x01;
const int poTags =          0x02;
typedef std::function<void(TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats, bool & Close)>
  TCalculateSizeEvent;
typedef std::function<void(const UnicodeString & Alg, TStrings * FileList,
  TCalculatedChecksumEvent OnCalculatedChecksum, bool & Close)> TCalculateChecksumEvent;
// The files of FileList have their TRemoteFile objects (also local ones)
bool DoPropertiesDialog(TStrings * FileList,
  const UnicodeString & Directory, const TRemoteTokenList * GroupList,
  const TRemoteTokenList * UserList, TStrings * ChecksumAlgs,
  TRemoteProperties * Properties,
  int AllowedChanges, int Options, TCalculateSizeEvent OnCalculateSize,
  TCalculateChecksumEvent OnCalculateChecksum);

// dialogs/LoginDialog.cpp
// The site manager: fills DataList with the sessions to open
bool DoLoginDialog(TList * DataList, QWidget * Parent, bool NeedTerminal);
//---------------------------------------------------------------------------
#endif
