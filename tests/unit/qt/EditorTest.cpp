// L3 tests of the internal editor (dialogs/EditorForm.cpp) and the list of
// editors (source/app/EditorPreferences.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <EditorPreferences.h>
#include <QtInterface.h>

#include <QAction>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTemporaryDir>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
QByteArray ReadFile(const QString & FileName)
{
  QFile File(FileName);
  EXPECT_TRUE(File.open(QIODevice::ReadOnly));
  return File.readAll();
}
//---------------------------------------------------------------------------
void WriteFile(const QString & FileName, const QByteArray & Data)
{
  QFile File(FileName);
  ASSERT_TRUE(File.open(QIODevice::WriteOnly));
  File.write(Data);
}
//---------------------------------------------------------------------------
// A local file in a writable editor
QWidget * OpenEditor(const QString & FileName, std::function<void()> OnClosed = nullptr)
{
  return ShowEditorForm(ToUnicodeString(FileName), nullptr, nullptr, nullptr,
    [OnClosed](QWidget *, bool) { if (OnClosed) { OnClosed(); } },
    [](QWidget *) {}, [](QWidget *, bool &) {}, L"", false);
}
//---------------------------------------------------------------------------
void TypeAtEnd(QWidget * Editor, const QString & Text)
{
  QPlainTextEdit * Memo = Child<QPlainTextEdit>(Editor, "EditorMemo");
  QTextCursor Cursor = Memo->textCursor();
  Cursor.movePosition(QTextCursor::End);
  Cursor.insertText(Text);
  Memo->setTextCursor(Cursor);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(EditorTest, KeepsPreambleAndLineEndings)
{
  QTemporaryDir Dir;
  QString FileName = Dir.filePath(QStringLiteral("crlf.txt"));
  WriteFile(FileName, QByteArray("\xEF\xBB\xBF" "one\r\ntwo\r\n"));
  QPointer<QWidget> Editor = OpenEditor(FileName);
  EXPECT_EQ(Child<QPlainTextEdit>(Editor, "EditorMemo")->toPlainText(), QStringLiteral("one\ntwo\n"));
  EXPECT_EQ(Child<QLabel>(Editor, "EncodingStatus")->text(), QStringLiteral("Encoding: UTF-8"));
  EXPECT_FALSE(IsEditorFormModified(Editor));
  TypeAtEnd(Editor, QStringLiteral("t\u0159i\n"));
  EXPECT_TRUE(IsEditorFormModified(Editor));
  EXPECT_EQ(Child<QLabel>(Editor, "ModifiedStatus")->text(), QStringLiteral("Modified"));
  EditorFormFileSave(Editor);
  EXPECT_FALSE(IsEditorFormModified(Editor));
  EXPECT_EQ(ReadFile(FileName), QByteArray("\xEF\xBB\xBF" "one\r\ntwo\r\nt\xC5\x99i\r\n"));
  Editor->close();
  QCoreApplication::processEvents();
}
//---------------------------------------------------------------------------
TEST(EditorTest, FallsBackToTheOtherEncoding)
{
  QTemporaryDir Dir;
  QString FileName = Dir.filePath(QStringLiteral("latin1.txt"));
  WriteFile(FileName, QByteArray("caf\xE9\n"));
  QString Message;
  WhenModal([&](QWidget * Dialog)
  {
    Message = Child<QLabel>(Dialog, "MainMessage")->text();
    Child<QPushButton>(Dialog, "OK")->click();
  });
  QPointer<QWidget> Editor = OpenEditor(FileName);
  EXPECT_TRUE(Message.contains(QStringLiteral("Falling back to 'ISO-8859-1' encoding.")));
  EXPECT_EQ(Child<QPlainTextEdit>(Editor, "EditorMemo")->toPlainText(), QStringLiteral("caf\u00E9\n"));
  EXPECT_EQ(Child<QLabel>(Editor, "EncodingStatus")->text(), QStringLiteral("Encoding: ISO-8859-1"));
  TypeAtEnd(Editor, QStringLiteral("cr\u00E8me"));
  EditorFormFileSave(Editor);
  EXPECT_EQ(ReadFile(FileName), QByteArray("caf\xE9\ncr\xE8me"));
  Editor->close();
  QCoreApplication::processEvents();
}
//---------------------------------------------------------------------------
TEST(EditorTest, ReplaceAll)
{
  QTemporaryDir Dir;
  QString FileName = Dir.filePath(QStringLiteral("replace.txt"));
  WriteFile(FileName, QByteArray("cat dog cat\n"));
  bool Closed = false;
  QPointer<QWidget> Editor = OpenEditor(FileName, [&]() { Closed = true; });
  Child<QAction>(Editor, "ReplaceAction")->trigger();
  QWidget * Replace = Child<QWidget>(Editor, "ReplaceDialog");
  ASSERT_NE(Replace, nullptr);
  Child<QLineEdit>(Replace, "FindTextEdit")->setText(QStringLiteral("cat"));
  Child<QLineEdit>(Replace, "ReplaceTextEdit")->setText(QStringLiteral("cow"));
  QString Message;
  WhenModal([&](QWidget * Dialog)
  {
    Message = Child<QLabel>(Dialog, "MainMessage")->text();
    Child<QPushButton>(Dialog, "OK")->click();
  });
  Child<QPushButton>(Replace, "ReplaceAllButton")->click();
  EXPECT_EQ(Message, QStringLiteral("Total replacements performed: 2"));
  EXPECT_EQ(Child<QPlainTextEdit>(Editor, "EditorMemo")->toPlainText(), QStringLiteral("cow dog cow\n"));

  // Closing asks to save the changes
  WhenModal([&](QWidget * Dialog) { Child<QPushButton>(Dialog, "Yes")->click(); });
  Editor->close();
  QCoreApplication::processEvents();
  EXPECT_TRUE(Closed);
  EXPECT_TRUE(Editor.isNull());
  EXPECT_EQ(ReadFile(FileName), QByteArray("cow dog cow\n"));
}
//---------------------------------------------------------------------------
TEST(EditorTest, EditorList)
{
  TEditorList List;
  TEditorData External;
  External.FileMask = TFileMasks(L"*.c; *.h");
  External.Editor = edExternal;
  External.ExternalEditor = L"/usr/bin/kate !.!";
  List.Add(new TEditorPreferences(External));
  List.Add(new TEditorPreferences());
  TFileMasks::TParams Params;
  const TEditorPreferences * Editor = List.Find(L"main.c", true, Params);
  ASSERT_NE(Editor, nullptr);
  EXPECT_EQ(Editor->Data->Editor, edExternal);
  EXPECT_EQ(Editor->Name, UnicodeString(L"Kate"));
  EXPECT_EQ(List.Find(L"readme.txt", true, Params)->Data->Editor, edInternal);
  EXPECT_EQ(List.Find(L"readme.txt", true, Params)->Name, UnicodeString(L"Internal editor"));

  TEditorList Copy;
  Copy = List;
  EXPECT_TRUE(Copy == List);
  Copy.Delete(0);
  EXPECT_FALSE(Copy == List);
}
//---------------------------------------------------------------------------
