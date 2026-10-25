// L3 tests of the custom commands (source/app/CustomCommands.cpp and the
// local programs of app/LocalProcesses.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <CustomCommands.h>
#include <QtInterface.h>
#include <ShortCuts.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(CustomCommandsTest, LocalPatterns)
{
  TCustomCommandData Data;
  TLocalCustomCommand Command(Data, L"/remote", L"/home/user/local", L"/tmp/x/a.txt", L"/home/user/b.txt", L"");
  EXPECT_EQ(Command.Complete(L"diff \"!\" \"!^!\" > !\\/out", true),
    UnicodeString(L"diff \"/tmp/x/a.txt\" \"/home/user/b.txt\" > /home/user/local/out"));
  EXPECT_TRUE(Command.HasLocalFileName(L"diff \"!\" \"!^!\""));
  EXPECT_TRUE(Command.IsFileCommand(L"cat \"!^!\""));
  EXPECT_FALSE(Command.IsFileCommand(L"ls !\\"));
  // Local file names are never escaped
  TLocalCustomCommand Quoted(Data, L"/remote", L"/l", L"a\"b", L"", L"");
  EXPECT_EQ(Quoted.Complete(L"cat \"!\"", true), UnicodeString(L"cat \"a\"b\""));
}
//---------------------------------------------------------------------------
TEST(CustomCommandsTest, List)
{
  TCustomCommandList List;
  List.Add(L"First", L"ls", 0);
  List.Add(L"Second", L"wc -l \"!\"", ccShowResults);
  TCustomCommandType * Third = new TCustomCommandType();
  Third->Name = L"Third";
  Third->Command = L"touch \"!\"";
  Third->ShortCut = ShortCut(L'3', TShiftState() << ssCtrl);
  List.Insert(0, Third);
  EXPECT_TRUE(List.Modified);
  List.Reset();
  ASSERT_EQ(List.Count, 3);
  EXPECT_EQ(List.Commands[0]->Name, UnicodeString(L"Third"));
  List.Move(0, 2);
  EXPECT_EQ(List.Commands[2]->Name, UnicodeString(L"Third"));
  EXPECT_TRUE(List.Modified);
  EXPECT_EQ(List.Find(L"Second")->Params, ccShowResults);
  EXPECT_EQ(List.Find(ShortCut(L'3', TShiftState() << ssCtrl)), List.Commands[2]);
  EXPECT_EQ(List.Find(L"None"), nullptr);

  TShortCuts ShortCuts;
  List.ShortCuts(ShortCuts);
  EXPECT_TRUE(ShortCuts.Has(ShortCut(L'3', TShiftState() << ssCtrl)));

  // A change to the same command is no change
  List.Reset();
  List.Change(1, new TCustomCommandType(*List.Commands[1]));
  EXPECT_FALSE(List.Modified);

  TCustomCommandList Copy;
  Copy.Assign(&List);
  EXPECT_TRUE(Copy.Equals(&List));
  Copy.Delete(0);
  EXPECT_FALSE(Copy.Equals(&List));
  EXPECT_EQ(Copy.Count, 2);
}
//---------------------------------------------------------------------------
TEST(CustomCommandsTest, LocalProcesses)
{
  UnicodeString Output;
  int ExitCode = -1;
  ExecuteProcessAndReadOutput(L"echo one; echo two >&2; exit 3", Output, ExitCode, false);
  EXPECT_EQ(Output, UnicodeString(L"one\n"));
  EXPECT_EQ(ExitCode, 3);

  Output = L"";
  ExecuteProcessChecked(L"printf '%s' \"$((1 + 2))\"", &Output);
  EXPECT_EQ(Output, UnicodeString(L"3"));

  // A failing command, with its output
  Output = L"";
  try
  {
    ExecuteProcessCheckedAndWait(L"echo broken >&2; exit 2", &Output);
    FAIL() << "No exception";
  }
  catch (ExtException & E)
  {
    EXPECT_NE(E.Message.Pos(L"2"), 0);
    ASSERT_NE(E.MoreMessages, nullptr);
    EXPECT_EQ(E.MoreMessages->Text.Trim(), UnicodeString(L"broken"));
  }
}
//---------------------------------------------------------------------------
