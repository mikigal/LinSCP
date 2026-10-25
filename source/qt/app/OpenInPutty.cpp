// Opens the session in an SSH client.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <CustomCommands.h>
#include <GUIConfiguration.h>
#include <SessionData.h>

#include "OpenInPutty.h"

#include <QStandardPaths>
//---------------------------------------------------------------------------
// A word of the command of /bin/sh
static UnicodeString ShellQuote(const UnicodeString & Str)
{
  return L"'" + ReplaceStr(Str, L"'", L"'\\''") + L"'";
}
//---------------------------------------------------------------------------
static void AddParam(UnicodeString & Params, const UnicodeString & Param)
{
  Params += L" " + Param;
}
//---------------------------------------------------------------------------
static bool IsSshSession(TSessionData * SessionData)
{
  return (SessionData->FSProtocol == fsSFTP) || (SessionData->FSProtocol == fsSFTPonly) || (SessionData->FSProtocol == fsSCPonly);
}
//---------------------------------------------------------------------------
// OpenSSH's ssh; its keys are its own, it does not read PuTTY's
static UnicodeString SshParams(TSessionData * SessionData)
{
  UnicodeString Result;
  if (IsSshSession(SessionData) && (SessionData->PortNumber != SshPortNumber))
  {
    AddParam(Result, FORMAT(L"-p %d", (SessionData->PortNumber)));
  }
  UnicodeString PublicKeyFile = SessionData->ResolvePublicKeyFile();
  if (!PublicKeyFile.IsEmpty() && !SameText(ExtractFileExt(PublicKeyFile), L".ppk"))
  {
    AddParam(Result, L"-i " + ShellQuote(PublicKeyFile));
  }
  if (SessionData->TryAgent && SessionData->AgentFwd)
  {
    AddParam(Result, L"-A");
  }
  if (SessionData->Compression)
  {
    AddParam(Result, L"-C");
  }
  if (SessionData->AddressFamily == afIPv4)
  {
    AddParam(Result, L"-4");
  }
  else if (SessionData->AddressFamily == afIPv6)
  {
    AddParam(Result, L"-6");
  }
  if (!SessionData->UserName.IsEmpty())
  {
    AddParam(Result, L"-l " + ShellQuote(SessionData->UserName));
  }
  // Not an option
  AddParam(Result, L"-- " + ShellQuote(SessionData->HostName));
  return Result;
}
//---------------------------------------------------------------------------
// PuTTY's command line (upstream's for its UWP version)
static UnicodeString PuttyParams(TSessionData * SessionData, UnicodeString & Password)
{
  UnicodeString Result;
  bool Telnet = (SessionData->FSProtocol == fsFTP) && GUIConfiguration->TelnetForFtpInPutty;
  if (Telnet)
  {
    AddParam(Result, L"-telnet");
    // PuTTY  does not allow -pw for telnet
    Password = L"";
  }
  else
  {
    AddParam(Result, L"-ssh");
  }
  AddParam(Result, ShellQuote(SessionData->HostName));
  if (!SessionData->UserName.IsEmpty())
  {
    AddParam(Result, L"-l " + ShellQuote(SessionData->UserName));
  }
  if ((SessionData->FSProtocol != fsFTP) && (SessionData->PortNumber != SshPortNumber))
  {
    AddParam(Result, FORMAT(L"-P %d", (SessionData->PortNumber)));
  }

  if (!Telnet)
  {
    UnicodeString PublicKeyFile = SessionData->ResolvePublicKeyFile();
    if (!PublicKeyFile.IsEmpty())
    {
      AddParam(Result, L"-i " + ShellQuote(PublicKeyFile));
    }
    AddParam(Result, (SessionData->TryAgent ? L"-agent" : L"-noagent"));
    if (SessionData->TryAgent)
    {
      AddParam(Result, (SessionData->AgentFwd ? L"-A" : L"-a"));
    }
    if (SessionData->Compression)
    {
      AddParam(Result, L"-C");
    }
    if (!SessionData->LogicalHostName.IsEmpty())
    {
      AddParam(Result, L"-loghost " + ShellQuote(SessionData->LogicalHostName));
    }
  }

  if (SessionData->AddressFamily == afIPv4)
  {
    AddParam(Result, L"-4");
  }
  else if (SessionData->AddressFamily == afIPv6)
  {
    AddParam(Result, L"-6");
  }
  return Result;
}
//---------------------------------------------------------------------------
void OpenSessionInPutty(TSessionData * SessionData)
{
  // putty does not support resolving environment variables in session settings
  SessionData->ExpandEnvironmentVariables();
  UnicodeString Program, AParams, Dir;
  SplitCommand(GUIConfiguration->PuttyPath, Program, AParams, Dir);
  Program = ExpandEnvironmentVariables(Program);
  AParams = ExpandEnvironmentVariables(AParams);
  AppLogFmt(L"PuTTY program: %s", (Program));
  AppLogFmt(L"Params: %s", (AParams));
  if (QStandardPaths::findExecutable(ToQString(Program)).isEmpty() &&
      (!StartsStr(L"/", Program) || !FileExists(ApiPath(Program))))
  {
    throw Exception(FMTLOAD(EXECUTE_APP_ERROR, (Program)));
  }

  UnicodeString Password;
  if (GUIConfiguration->PuttyPassword)
  {
    // Passphrase has precendence, as it's more likely entered by user during authentication, hence more likely really needed.
    Password = !SessionData->Passphrase.IsEmpty() ? SessionData->Passphrase : SessionData->Password;
  }
  TCustomCommandData Data(SessionData, SessionData->UserName, Password);
  TLocalCustomCommand LocalCustomCommand(Data, SessionData->RemoteDirectory, SessionData->LocalDirectory);
  TWinInteractiveCustomCommand InteractiveCustomCommand(&LocalCustomCommand, L"PuTTY");
  UnicodeString Params =
    LocalCustomCommand.Complete(InteractiveCustomCommand.Complete(AParams, false), true);
  AppLogFmt(L"Expanded params: %s", (Params));

  if (!LocalCustomCommand.IsSiteCommand(AParams))
  {
    // The client is the last word: "ssh" of "konsole -e ssh", or PuTTY
    UnicodeString Client = Params.Trim();
    int Space = Client.LastDelimiter(L" ");
    Client = Client.IsEmpty() ? Program : Client.SubString(Space + 1, Client.Length() - Space);
    if (SameText(ExtractFileName(Client), L"ssh"))
    {
      Params += SshParams(SessionData);
    }
    else
    {
      Params += PuttyParams(SessionData, Password);
      // As upstream with PuTTY older than 0.77 (no -pwfile); the password
      // shows in the list of the processes
      if (!Password.IsEmpty() && !LocalCustomCommand.IsPasswordCommand(AParams))
      {
        Params += L" -pw " + ShellQuote(Password);
      }
    }
  }

  // The client runs on its own (as upstream's ExecuteShellChecked)
  ExecuteShellChecked(ShellQuote(Program) + L" " + Params);
}
//---------------------------------------------------------------------------
