// IniFiles, Masks, IOUtils, EncdDecd, WideStrUtils, JSON, XML
//---------------------------------------------------------------------------
#include <System.IniFiles.hpp>
#include <Masks.hpp>
#include <System.IOUtils.hpp>
#include <Soap.EncdDecd.hpp>
#include <WideStrUtils.hpp>
#include <System.JSON.hpp>
#include <XMLDoc.hpp>
#include <Xml.XMLIntf.hpp>
#include <Consts.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
//---------------------------------------------------------------------------
namespace
{
class DataFormatsTest : public testing::Test
{
protected:
  void SetUp() override
  {
    std::string Pattern = std::string((getenv("TMPDIR") != nullptr) ? getenv("TMPDIR") : "/tmp") + "/linscp-rtl-XXXXXX";
    std::vector<char> Buffer(Pattern.begin(), Pattern.end());
    Buffer.push_back('\0');
    ASSERT_NE(mkdtemp(Buffer.data()), nullptr);
    FRoot = Buffer.data();
    FDir = UnicodeString(FRoot.c_str()) + L"/";
  }

  void TearDown() override
  {
    std::error_code Error;
    std::filesystem::remove_all(FRoot, Error);
  }

  void Write(const UnicodeString & Name, const std::string & Content)
  {
    TFileStream Stream(FDir + Name, fmCreate);
    if (!Content.empty())
    {
      Stream.WriteBuffer(Content.data(), static_cast<int>(Content.size()));
    }
  }

  std::string Read(const UnicodeString & Name)
  {
    TFileStream Stream(FDir + Name, fmOpenRead);
    std::string Result(static_cast<size_t>(Stream.Size), '\0');
    if (!Result.empty())
    {
      Stream.ReadBuffer(Result.data(), static_cast<int>(Result.size()));
    }
    return Result;
  }

  std::string FRoot;
  UnicodeString FDir;
};
} // namespace
//---------------------------------------------------------------------------
TEST_F(DataFormatsTest, MemIniFileReadsWinScpIni)
{
  Write(L"WinSCP.ini",
    "[Configuration\\Interface]\r\n"
    "RandomSeedFile=%APPDATA%\\winscp.rnd\r\n"
    "  Speed = 1024  \r\n"
    "; comment\r\n"
    "\r\n"
    "[Sessions\\My%20Server]\r\n"
    "HostName=example.com\r\n"
    "PortNumber=0x16\r\n"
    "Compression=1\r\n"
    "Weird line without separator\r\n");
  TMemIniFile Ini(FDir + L"WinSCP.ini");
  EXPECT_EQ(Ini.ReadString(L"Configuration\\Interface", L"RandomSeedFile", L""), L"%APPDATA%\\winscp.rnd");
  // Sections and keys are case-insensitive, spaces around '=' are removed
  EXPECT_EQ(Ini.ReadString(L"configuration\\interface", L"SPEED", L""), L"1024");
  EXPECT_EQ(Ini.ReadInteger(L"Sessions\\My%20Server", L"PortNumber", 0), 22);
  EXPECT_TRUE(Ini.ReadBool(L"Sessions\\My%20Server", L"Compression", false));
  EXPECT_EQ(Ini.ReadString(L"Sessions\\My%20Server", L"Missing", L"default"), L"default");
  EXPECT_EQ(Ini.ReadInteger(L"Missing", L"Missing", 7), 7);
  EXPECT_TRUE(Ini.SectionExists(L"Sessions\\My%20Server"));
  EXPECT_FALSE(Ini.SectionExists(L"Nothing"));
  EXPECT_TRUE(Ini.ValueExists(L"Sessions\\My%20Server", L"hostname"));
  EXPECT_FALSE(Ini.ValueExists(L"Sessions\\My%20Server", L"Port"));

  TStringList Sections;
  Ini.ReadSections(&Sections);
  EXPECT_EQ(Sections.CommaText, L"Configuration\\Interface,Sessions\\My%20Server");
  TStringList Keys;
  Ini.ReadSection(L"Sessions\\My%20Server", &Keys);
  // The line without a separator has an empty name
  EXPECT_EQ(Keys.CommaText, L"HostName,PortNumber,Compression,");
  TStringList Values;
  Ini.ReadSectionValues(L"Configuration\\Interface", &Values);
  EXPECT_EQ(Values.Values[L"Speed"], L"1024");
}
//---------------------------------------------------------------------------
TEST_F(DataFormatsTest, MemIniFileWrites)
{
  {
    TMemIniFile Ini(FDir + L"new.ini");
    Ini.WriteString(L"S1", L"A", L"1");
    Ini.WriteInteger(L"S1", L"B", 2);
    Ini.WriteBool(L"S2", L"C", true);
    Ini.WriteString(L"s1", L"a", L"one");
    Ini.WriteString(L"S3", L"X", L"x");
    Ini.EraseSection(L"S3");
    Ini.WriteString(L"S2", L"D", L"d");
    Ini.DeleteKey(L"S2", L"D");
    Ini.UpdateFile();
  }
  EXPECT_EQ(Read(L"new.ini"), "[S1]\na=one\nB=2\n\n[S2]\nC=1\n\n");
  TMemIniFile Ini(FDir + L"new.ini");
  EXPECT_EQ(Ini.ReadString(L"S1", L"A", L""), L"one");
  TStringList Strings;
  Ini.GetStrings(&Strings);
  EXPECT_EQ(Strings.Count, 7);
  // A BOM is kept when saving
  Write(L"bom.ini", "\xEF\xBB\xBF[S]\nK=\xC5\xBC\n");
  TMemIniFile Bom(FDir + L"bom.ini");
  EXPECT_EQ(Bom.ReadString(L"S", L"K", L""), L"\u017C");
  Bom.WriteString(L"S", L"L", L"x");
  Bom.UpdateFile();
  EXPECT_EQ(Read(L"bom.ini"), "\xEF\xBB\xBF[S]\nK=\xC5\xBC\nL=x\n\n");
  // An INI file without a name only lives in memory
  TMemIniFile Memory(L"");
  Memory.WriteFloat(L"S", L"F", 1.5);
  EXPECT_DOUBLE_EQ(Memory.ReadFloat(L"S", L"F", 0), 1.5);
}
//---------------------------------------------------------------------------
TEST(Masks, Matches)
{
  EXPECT_TRUE(MatchesMask(L"file.txt", L"*.txt"));
  EXPECT_TRUE(MatchesMask(L"FILE.TXT", L"*.txt"));
  EXPECT_FALSE(MatchesMask(L"file.txt.bak", L"*.txt"));
  EXPECT_TRUE(MatchesMask(L"a.b.c", L"*.*"));
  EXPECT_FALSE(MatchesMask(L"noext", L"*.*"));
  EXPECT_TRUE(MatchesMask(L"noext", L"*"));
  EXPECT_TRUE(MatchesMask(L"", L"*"));
  EXPECT_TRUE(MatchesMask(L"abc", L"a?c"));
  EXPECT_FALSE(MatchesMask(L"ac", L"a?c"));
  EXPECT_TRUE(MatchesMask(L"file1.log", L"file[0-9].log"));
  EXPECT_FALSE(MatchesMask(L"filex.log", L"file[0-9].log"));
  EXPECT_TRUE(MatchesMask(L"filex.log", L"file[!0-9].log"));
  EXPECT_TRUE(MatchesMask(L"b", L"[abc]"));
  EXPECT_TRUE(MatchesMask(L"\u017C\u00F3\u0142w", L"\u017B*W"));
  EXPECT_TRUE(MatchesMask(L"aXbXc", L"a*b*c"));
  EXPECT_FALSE(MatchesMask(L"aXbXd", L"a*b*c"));
  EXPECT_THROW(MatchesMask(L"x", L"[abc"), EMaskException);
  EXPECT_THROW(MatchesMask(L"x", L"[]"), EMaskException);
  TMask Mask(L"*.TXT");
  EXPECT_TRUE(Mask.Matches(L"readme.txt"));
}
//---------------------------------------------------------------------------
TEST_F(DataFormatsTest, IOUtils)
{
  EXPECT_EQ(TPath::Combine(L"/a", L"b"), L"/a/b");
  EXPECT_EQ(TPath::Combine(L"/a/", L"b"), L"/a/b");
  EXPECT_EQ(TPath::Combine(L"/a", L"/b"), L"/b");
  EXPECT_EQ(TPath::Combine(L"", L"b"), L"b");
  EXPECT_FALSE(TPath::IsDriveRooted(L"C:\\x"));
  EXPECT_TRUE(TPath::IsPathRooted(L"/x"));
  EXPECT_EQ(TPath::GetExtension(L"/a/b.txt"), L".txt");
  EXPECT_EQ(TPath::GetFileNameWithoutExtension(L"/a/b.txt"), L"b");
  EXPECT_EQ(TPath::GetDirectoryName(L"/a/b.txt"), L"/a");
  EXPECT_EQ(TPath::ChangeExtension(L"b.txt", L"bak"), L"b.bak");
  EXPECT_EQ(TPath::DirectorySeparatorChar, L'/');
  EXPECT_TRUE(IsPathDelimiter(TPath::GetTempPath(), TPath::GetTempPath().Length()));
  EXPECT_EQ(TPath::GetRandomFileName().Length(), 12);
  EXPECT_THROW(TPath::Combine(UnicodeString(L"a\0b", 3), L"c"), EArgumentException);

  UnicodeString File = FDir + L"text.txt";
  TFile::WriteAllText(File, L"za\u017C\nline");
  EXPECT_EQ(Read(L"text.txt"), "za\xC5\xBC\nline");
  EXPECT_EQ(TFile::ReadAllText(File), L"za\u017C\nline");
  TFile::WriteAllText(File, L"bom", TEncoding::UTF8);
  EXPECT_EQ(Read(L"text.txt"), "\xEF\xBB\xBF" "bom");
  EXPECT_EQ(TFile::ReadAllText(File), L"bom");
  TFile::AppendAllText(File, L"!");
  EXPECT_EQ(TFile::ReadAllText(File), L"bom!");
  EXPECT_EQ(TFile::ReadAllBytes(File).Length, 7);
  EXPECT_TRUE(TFile::Exists(File));
  EXPECT_EQ(TFile::GetSize(File), 7);
  TFile::Copy(File, FDir + L"copy.txt");
  EXPECT_THROW(TFile::Copy(File, FDir + L"copy.txt"), EInOutError);
  TFile::Move(FDir + L"copy.txt", FDir + L"moved.txt");
  EXPECT_TRUE(TFile::Exists(FDir + L"moved.txt"));
  TFile::Delete(FDir + L"moved.txt");
  EXPECT_FALSE(TFile::Exists(FDir + L"moved.txt"));
  EXPECT_THROW(TFile::ReadAllText(FDir + L"missing"), EFOpenError);

  TDirectory::CreateDirectory(FDir + L"d/e");
  EXPECT_TRUE(TDirectory::Exists(FDir + L"d/e"));
  EXPECT_TRUE(TDirectory::IsEmpty(FDir + L"d/e"));
  TFile::WriteAllText(FDir + L"d/f.txt", L"");
  EXPECT_EQ(TDirectory::GetFiles(FDir + L"d").Length, 1);
  EXPECT_EQ(TDirectory::GetDirectories(FDir + L"d")[0], FDir + L"d/e");
  EXPECT_THROW(TDirectory::Delete(FDir + L"d"), EInOutError);
  TDirectory::Delete(FDir + L"d", true);
  EXPECT_FALSE(TDirectory::Exists(FDir + L"d"));
}
//---------------------------------------------------------------------------
TEST(EncdDecd, Base64)
{
  EXPECT_EQ(EncodeBase64("", 0), L"");
  EXPECT_EQ(EncodeBase64("f", 1), L"Zg==");
  EXPECT_EQ(EncodeBase64("fo", 2), L"Zm8=");
  EXPECT_EQ(EncodeBase64("foo", 3), L"Zm9v");
  EXPECT_EQ(EncodeBase64("foobar", 6), L"Zm9vYmFy");
  std::string Long(100, 'x');
  UnicodeString Encoded = EncodeBase64(Long.data(), static_cast<int>(Long.size()));
  EXPECT_EQ(Encoded.Pos(sLineBreak), 77);
  TBytes Decoded = DecodeBase64(Encoded);
  ASSERT_EQ(Decoded.Length, 100);
  EXPECT_EQ(Decoded[99], 'x');
  EXPECT_EQ(DecodeBase64(L"Zm9vYmFy").Length, 6);
  EXPECT_EQ(DecodeBase64(L"Zg==").Length, 1);
  EXPECT_EQ(DecodeString(EncodeString(L"za\u017C")), L"za\u017C");

  EXPECT_EQ(DetectUTF8Encoding("plain"), etUSASCII);
  EXPECT_EQ(DetectUTF8Encoding("za\xC5\xBC"), etUTF8);
  EXPECT_EQ(DetectUTF8Encoding("za\xBF"), etANSI);
  EXPECT_EQ(Vcl_Consts_SMsgDlgCancel, L"Cancel");
}
//---------------------------------------------------------------------------
TEST(Json, Parse)
{
  std::unique_ptr<TJSONValue> Value(TJSONObject::ParseJSONValue(
    L"{ \"Code\" : \"Success\", \"Expiration\": \"2026-10-24T13:05:07Z\", \"N\": -1.5e3,"
    L" \"T\": true, \"Z\": null, \"A\": [1, \"two\", {}], \"E\": \"a\\\"b\\\\c\\u017C\\ud83d\\ude00\\n\" }"));
  TJSONObject * Object = dynamic_cast<TJSONObject *>(Value.get());
  ASSERT_NE(Object, nullptr);
  EXPECT_EQ(Object->Count, 7);
  EXPECT_EQ(Object->Values[L"Code"]->Value(), L"Success");
  EXPECT_EQ(Object->Values[L"Missing"], nullptr);
  EXPECT_EQ(Object->Values[L"N"]->Value(), L"-1.5e3");
  EXPECT_DOUBLE_EQ(dynamic_cast<TJSONNumber *>(Object->Values[L"N"])->AsDouble, -1500);
  EXPECT_EQ(Object->Values[L"T"]->Value(), L"true");
  EXPECT_TRUE(Object->Values[L"Z"]->Null());
  EXPECT_EQ(Object->Values[L"E"]->Value(), L"a\"b\\c\u017C\U0001F600\n");
  TJSONArray * Array = dynamic_cast<TJSONArray *>(Object->Values[L"A"]);
  ASSERT_NE(Array, nullptr);
  EXPECT_EQ(Array->Count, 3);
  EXPECT_EQ(Array->Items[1]->Value(), L"two");

  std::unique_ptr<TJSONObject::TEnumerator> Enumerator(Object->GetEnumerator());
  UnicodeString Names;
  while (Enumerator->MoveNext())
  {
    Names += Enumerator->Current->JsonString->Value() + L";";
  }
  EXPECT_EQ(Names, L"Code;Expiration;N;T;Z;A;E;");

  EXPECT_EQ(TJSONObject::ParseJSONValue(L"{\"a\":}"), nullptr);
  EXPECT_EQ(TJSONObject::ParseJSONValue(L"[1,2] x"), nullptr);
  EXPECT_EQ(TJSONObject::ParseJSONValue(L""), nullptr);
  std::unique_ptr<TJSONValue> Round(TJSONObject::ParseJSONValue(L"{\"a\":[1,\"x\\ty\"],\"b\":false}"));
  EXPECT_EQ(Round->ToJSON(), L"{\"a\":[1,\"x\\ty\"],\"b\":false}");
}
//---------------------------------------------------------------------------
TEST_F(DataFormatsTest, Xml)
{
  const _di_IXMLDocument Document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
  EXPECT_TRUE(Document->ParseOptions == TParseOptions());
  Document->LoadFromXML(
    L"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
    L"<Tagging xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n"
    L"  <TagSet>\n"
    L"    <Tag><Key>k1</Key><Value> v 1 </Value></Tag>\n"
    L"    <Tag><Key>k\u017C</Key><Value><![CDATA[<x>]]></Value></Tag>\n"
    L"  </TagSet>\n"
    L"  <!-- comment -->\n"
    L"</Tagging>\n");
  _di_IXMLNode Tagging = Document->ChildNodes->FindNode(L"Tagging");
  ASSERT_TRUE(Tagging != NULL);
  EXPECT_EQ(Tagging->NamespaceURI, L"http://s3.amazonaws.com/doc/2006-03-01/");
  EXPECT_TRUE(Document->ChildNodes->FindNode(L"Tagging", L"http://s3.amazonaws.com/doc/2006-03-01/") != NULL);
  EXPECT_TRUE(Document->ChildNodes->FindNode(L"Tagging", L"urn:other") == NULL);
  EXPECT_TRUE(Document->DocumentElement == Tagging);
  _di_IXMLNodeList Tags = Tagging->ChildNodes->FindNode(L"TagSet")->GetChildNodes();
  // Whitespace-only text is dropped without poPreserveWhiteSpace
  ASSERT_EQ(Tags->Count, 2);
  _di_IXMLNode Tag = Tags->Get(0);
  EXPECT_EQ(Tag->NodeName, L"Tag");
  EXPECT_EQ(Tag->NodeType, ntElement);
  EXPECT_EQ(Tag->ChildNodes->FindNode(L"Key")->Text, L"k1");
  EXPECT_EQ(Tag->ChildNodes->FindNode(L"Value")->Text, L" v 1 ");
  EXPECT_EQ(Tags->Get(1)->ChildNodes->FindNode(L"Key")->Text, L"k\u017C");
  EXPECT_EQ(Tags->Get(1)->ChildNodes->FindNode(L"Value")->Text, L"<x>");
  EXPECT_TRUE(Tag->ChildNodes->FindNode(L"Missing") == NULL);
  EXPECT_TRUE(Tagging->ChildNodes->Get(1)->NodeType == ntComment);
  EXPECT_THROW(Tags->Get(5), EListError);

  // A node keeps the document alive
  _di_IXMLNode Kept = Tags->Get(1);
  Tags = NULL;
  Tag = NULL;
  Tagging = NULL;
  EXPECT_EQ(Kept->ParentNode->NodeName, L"TagSet");
}
//---------------------------------------------------------------------------
TEST_F(DataFormatsTest, XmlFileAndAttributes)
{
  Write(L"sitemanager.xml",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<FileZilla3>\n"
    "  <Servers>\n"
    "    <Folder expanded=\"1\">My \xC5\xBC sites\n"
    "      <Server>\n"
    "        <Host>example.com</Host>\n"
    "        <Pass encoding=\"base64\">c2VjcmV0</Pass>\n"
    "      </Server>\n"
    "    </Folder>\n"
    "  </Servers>\n"
    "  <Settings><Setting name=\"Timeout\">20</Setting></Settings>\n"
    "</FileZilla3>\n");
  _di_IXMLDocument Document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
  Document->LoadFromFile(FDir + L"sitemanager.xml");
  EXPECT_EQ(Document->Encoding, L"UTF-8");
  _di_IXMLNode Root = Document->ChildNodes->FindNode(L"FileZilla3");
  ASSERT_TRUE(Root != NULL);
  _di_IXMLNode Folder = Root->ChildNodes->FindNode(L"Servers")->ChildNodes->FindNode(L"Folder");
  UnicodeString Name;
  for (int Index = 0; Index < Folder->ChildNodes->Count; Index++)
  {
    _di_IXMLNode Child = Folder->ChildNodes->Get(Index);
    if (Child->NodeType == ntText)
    {
      UnicodeString Value = Child->NodeValue;
      Name += Value.Trim();
    }
  }
  EXPECT_EQ(Name, L"My \u017C sites");
  EXPECT_EQ(Folder->GetAttribute(L"expanded"), OleVariant(UnicodeString(L"1")));
  _di_IXMLNode Pass = Folder->ChildNodes->FindNode(L"Server")->ChildNodes->FindNode(L"Pass");
  OleVariant Encoding = Pass->GetAttribute(L"encoding");
  ASSERT_FALSE(Encoding.IsNull());
  UnicodeString EncodingStr = Encoding;
  EXPECT_TRUE(SameText(EncodingStr, L"base64"));
  EXPECT_TRUE(Pass->GetAttribute(L"missing").IsNull());
  EXPECT_TRUE(Pass->HasAttribute(L"encoding"));
  EXPECT_EQ(TEncoding::UTF8->GetString(DecodeBase64(Pass->Text.Trim())), L"secret");
  _di_IXMLNode Setting = Root->ChildNodes->FindNode(L"Settings")->ChildNodes->Get(0);
  OleVariant SettingName = Setting->GetAttribute(L"name");
  EXPECT_TRUE(SettingName == UnicodeString(L"Timeout"));
  EXPECT_EQ(StrToIntDef(Setting->Text.Trim(), 0), 20);
  EXPECT_EQ(UnicodeString(Setting->NodeValue), L"20");
  EXPECT_EQ(Setting->XML, L"<Setting name=\"Timeout\">20</Setting>");

  Write(L"broken.xml", "<a><b></a>");
  EXPECT_THROW(Document->LoadFromFile(FDir + L"broken.xml"), EDOMParseError);
  _di_IXMLDocument Preserving = LoadXMLData(L"<a> <b/> </a>");
  EXPECT_EQ(Preserving->DocumentElement->ChildNodes->Count, 1);
  _di_IXMLDocument WithSpace = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
  WithSpace->ParseOptions = TParseOptions() << poPreserveWhiteSpace;
  WithSpace->LoadFromXML(L"<a> <b/> </a>");
  EXPECT_EQ(WithSpace->DocumentElement->ChildNodes->Count, 3);
}
