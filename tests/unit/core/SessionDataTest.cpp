// Session data: parsing and generating session URLs.
//---------------------------------------------------------------------------
#include <CorePCH.h>

#include <SessionData.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
std::unique_ptr<TSessionData> ParseUrl(
  const UnicodeString & Url, int * ParsedInfo = NULL, UnicodeString * FileName = NULL)
{
  int Info = 0;
  std::unique_ptr<TSessionData> Result(StoredSessions->ParseUrl(Url, NULL, Info, FileName));
  if (ParsedInfo != NULL)
  {
    *ParsedInfo = Info;
  }
  return Result;
}
}
//---------------------------------------------------------------------------
TEST(SessionDataTest, SftpUrl)
{
  int ParsedInfo = 0;
  std::unique_ptr<TSessionData> Data =
    ParseUrl(L"sftp://user:secret@example.com:2222/home/user/", &ParsedInfo);
  EXPECT_EQ(Data->FSProtocol, fsSFTPonly);
  EXPECT_EQ(Data->HostName, L"example.com");
  EXPECT_EQ(Data->PortNumber, 2222);
  EXPECT_EQ(Data->UserName, L"user");
  EXPECT_EQ(Data->Password, L"secret");
  EXPECT_EQ(Data->RemoteDirectory, L"/home/user/");
  EXPECT_TRUE(FLAGSET(ParsedInfo, piProtocolDefined));
  EXPECT_FALSE(FLAGSET(ParsedInfo, piDefaultsOnly));
}
//---------------------------------------------------------------------------
TEST(SessionDataTest, Protocols)
{
  std::unique_ptr<TSessionData> Scp = ParseUrl(L"scp://example.com");
  EXPECT_EQ(Scp->FSProtocol, fsSCPonly);
  EXPECT_EQ(Scp->PortNumber, 22);

  std::unique_ptr<TSessionData> Ftp = ParseUrl(L"ftp://example.com/");
  EXPECT_EQ(Ftp->FSProtocol, fsFTP);
  EXPECT_EQ(Ftp->Ftps, ftpsNone);
  EXPECT_EQ(Ftp->PortNumber, 21);

  std::unique_ptr<TSessionData> Ftps = ParseUrl(L"ftps://example.com/");
  EXPECT_EQ(Ftps->FSProtocol, fsFTP);
  EXPECT_EQ(Ftps->Ftps, ftpsImplicit);
  EXPECT_EQ(Ftps->PortNumber, 990);

  std::unique_ptr<TSessionData> Dav = ParseUrl(L"davs://example.com/dav/");
  EXPECT_EQ(Dav->FSProtocol, fsWebDAV);
  EXPECT_EQ(Dav->Ftps, ftpsImplicit);
  EXPECT_EQ(Dav->PortNumber, 443);
  EXPECT_EQ(Dav->RemoteDirectory, L"/dav/");

  std::unique_ptr<TSessionData> S3 = ParseUrl(L"s3://s3.amazonaws.com/bucket/");
  EXPECT_EQ(S3->FSProtocol, fsS3);
  EXPECT_EQ(S3->HostName, L"s3.amazonaws.com");
}
//---------------------------------------------------------------------------
TEST(SessionDataTest, EncodedAndIPv6)
{
  std::unique_ptr<TSessionData> Data = ParseUrl(L"sftp://us%40er:p%3Ass%20w@[::1]:2222/");
  EXPECT_EQ(Data->UserName, L"us@er");
  EXPECT_EQ(Data->Password, L"p:ss w");
  EXPECT_EQ(Data->HostName, L"::1");
  EXPECT_EQ(Data->PortNumber, 2222);
}
//---------------------------------------------------------------------------
TEST(SessionDataTest, FileName)
{
  UnicodeString FileName;
  std::unique_ptr<TSessionData> Data = ParseUrl(L"sftp://example.com/dir/file.txt", NULL, &FileName);
  EXPECT_EQ(Data->RemoteDirectory, L"/dir/");
  EXPECT_EQ(FileName, L"file.txt");
}
//---------------------------------------------------------------------------
TEST(SessionDataTest, HostKey)
{
  std::unique_ptr<TSessionData> Data =
    ParseUrl(L"sftp://user;fingerprint=ssh-ed25519-AAAA@example.com/");
  EXPECT_EQ(Data->UserName, L"user");
  // As given; TSecureShell verifies the host key against it
  EXPECT_EQ(Data->HostKey, L"ssh-ed25519-AAAA");
}
//---------------------------------------------------------------------------
TEST(SessionDataTest, GenerateUrl)
{
  std::unique_ptr<TSessionData> Data(new TSessionData(L""));
  Data->FSProtocol = fsSFTPonly;
  Data->HostName = L"example.com";
  Data->PortNumber = 2222;
  Data->UserName = L"us@er";
  Data->Password = L"secret";
  EXPECT_EQ(Data->GenerateSessionUrl(sufOpen), L"sftp://us%40er:secret@example.com:2222/");
  EXPECT_EQ(Data->GenerateSessionUrl(sufUserName), L"sftp://us%40er@example.com:2222/");
  Data->PortNumber = 22;
  EXPECT_EQ(Data->GenerateSessionUrl(0), L"sftp://example.com/");

  // And back
  std::unique_ptr<TSessionData> Parsed = ParseUrl(L"sftp://us%40er:secret@example.com:2222/");
  EXPECT_EQ(Parsed->UserName, L"us@er");
  EXPECT_EQ(Parsed->Password, L"secret");
}
//---------------------------------------------------------------------------
TEST(SessionDataTest, Defaults)
{
  std::unique_ptr<TSessionData> Data(new TSessionData(L""));
  EXPECT_EQ(Data->FSProtocol, fsSFTP);
  EXPECT_EQ(Data->PortNumber, 22);
  Data->HostName = L"example.com";
  Data->UserName = L"user";
  EXPECT_EQ(Data->SessionName, L"user@example.com");
}
//---------------------------------------------------------------------------
