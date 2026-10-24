// System.Classes: streams
//---------------------------------------------------------------------------
#include <rtl/Classes.h>

#include <cerrno>
#include <cstdlib>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Classes
{
//---------------------------------------------------------------------------
// TStream
//---------------------------------------------------------------------------
int TStream::Read(void *, int)
{
  return 0;
}
//---------------------------------------------------------------------------
int TStream::Write(const void *, int)
{
  return 0;
}
//---------------------------------------------------------------------------
int TStream::Read(DynamicArray<Byte> Buffer, int Offset, int Count)
{
  if (Count <= 0)
  {
    return 0;
  }
  if ((Offset < 0) || (Offset + Count > Buffer.Length))
  {
    rtl::ThrowRangeError(Offset + Count, Buffer.Length);
  }
  return Read(Buffer.data() + Offset, Count);
}
//---------------------------------------------------------------------------
int TStream::Write(const DynamicArray<Byte> Buffer, int Offset, int Count)
{
  if (Count <= 0)
  {
    return 0;
  }
  if ((Offset < 0) || (Offset + Count > Buffer.Length))
  {
    rtl::ThrowRangeError(Offset + Count, Buffer.Length);
  }
  return Write(Buffer.data() + Offset, Count);
}
//---------------------------------------------------------------------------
int TStream::Read(DynamicArray<Byte> & Buffer, int Count)
{
  return Read(Buffer, 0, Count);
}
//---------------------------------------------------------------------------
int TStream::Write(const DynamicArray<Byte> Buffer, int Count)
{
  return Write(Buffer, 0, Count);
}
//---------------------------------------------------------------------------
int TStream::Seek(int Offset, Word Origin)
{
  return static_cast<int>(Seek(static_cast<__int64>(Offset), static_cast<TSeekOrigin>(Origin)));
}
//---------------------------------------------------------------------------
__int64 TStream::Seek(const __int64, TSeekOrigin)
{
  throw EStreamError(L"%s.Seek not implemented", ARRAYOFCONST((ClassName())));
}
//---------------------------------------------------------------------------
__int64 TStream::Seek(const __int64 Offset, Word Origin)
{
  return Seek(Offset, static_cast<TSeekOrigin>(Origin));
}
//---------------------------------------------------------------------------
void TStream::ReadBuffer(void * Buffer, int Count)
{
  if ((Count != 0) && (Read(Buffer, Count) != Count))
  {
    throw EReadError(L"Stream read error");
  }
}
//---------------------------------------------------------------------------
void TStream::ReadBuffer(DynamicArray<Byte> & Buffer, int Count)
{
  ReadBuffer(Buffer, 0, Count);
}
//---------------------------------------------------------------------------
void TStream::ReadBuffer(DynamicArray<Byte> & Buffer, int Offset, int Count)
{
  if ((Count != 0) && (Read(Buffer, Offset, Count) != Count))
  {
    throw EReadError(L"Stream read error");
  }
}
//---------------------------------------------------------------------------
void TStream::WriteBuffer(const void * Buffer, int Count)
{
  if ((Count != 0) && (Write(Buffer, Count) != Count))
  {
    throw EWriteError(L"Stream write error");
  }
}
//---------------------------------------------------------------------------
void TStream::WriteBuffer(const DynamicArray<Byte> Buffer, int Count)
{
  WriteBuffer(Buffer, 0, Count);
}
//---------------------------------------------------------------------------
void TStream::WriteBuffer(const DynamicArray<Byte> Buffer, int Offset, int Count)
{
  if ((Count != 0) && (Write(Buffer, Offset, Count) != Count))
  {
    throw EWriteError(L"Stream write error");
  }
}
//---------------------------------------------------------------------------
__int64 TStream::CopyFrom(TStream * Source, __int64 Count, int BufferSize)
{
  const int MaxBufSize = 0xF000;
  if (Count == 0)
  {
    Source->Position = 0;
    Count = Source->Size;
  }
  __int64 Result = Count;
  int BufSize = (BufferSize > 0) ? BufferSize : ((Count > MaxBufSize) ? MaxBufSize : static_cast<int>(Count));
  // The TBytes overloads are used, as in Delphi; descendants may override them
  DynamicArray<Byte> Buffer;
  Buffer.Length = BufSize;
  while (Count != 0)
  {
    int N = (Count > BufSize) ? BufSize : static_cast<int>(Count);
    Source->ReadBuffer(Buffer, N);
    WriteBuffer(Buffer, N);
    Count -= N;
  }
  return Result;
}
//---------------------------------------------------------------------------
__int64 TStream::GetSize()
{
  __int64 Pos = Seek(static_cast<__int64>(0), soCurrent);
  __int64 Result = Seek(static_cast<__int64>(0), soEnd);
  Seek(Pos, soBeginning);
  return Result;
}
//---------------------------------------------------------------------------
void TStream::SetSize(int NewSize)
{
  SetSize(static_cast<__int64>(NewSize));
}
//---------------------------------------------------------------------------
void TStream::SetSize(const __int64)
{
}
//---------------------------------------------------------------------------
// THandleStream
//---------------------------------------------------------------------------
int THandleStream::Read(void * Buffer, int Count)
{
  int Result = FileRead(FHandle, Buffer, static_cast<unsigned int>(Count));
  return (Result == -1) ? 0 : Result;
}
//---------------------------------------------------------------------------
int THandleStream::Write(const void * Buffer, int Count)
{
  int Result = FileWrite(FHandle, Buffer, static_cast<unsigned int>(Count));
  return (Result == -1) ? 0 : Result;
}
//---------------------------------------------------------------------------
__int64 THandleStream::Seek(const __int64 Offset, TSeekOrigin Origin)
{
  return FileSeek(FHandle, Offset, static_cast<int>(Origin));
}
//---------------------------------------------------------------------------
void THandleStream::SetSize(const __int64 NewSize)
{
  Seek(NewSize, soBeginning);
  if (ftruncate(static_cast<int>(static_cast<intptr_t>(FHandle)), static_cast<off_t>(NewSize)) != 0)
  {
    rtl::SetLastErrorFromErrno();
    RaiseLastOSError();
  }
}
//---------------------------------------------------------------------------
// TFileStream
//---------------------------------------------------------------------------
TFileStream::TFileStream(const UnicodeString AFileName, Word Mode) :
  TFileStream(AFileName, Mode, 0)
{
}
//---------------------------------------------------------------------------
TFileStream::TFileStream(const UnicodeString AFileName, Word Mode, unsigned int Rights) :
  THandleStream(static_cast<THandle>(-1))
{
  int FileRights = (Rights != 0) ? static_cast<int>(Rights) : 0666;
  if ((Mode & fmCreate) == fmCreate)
  {
    FHandle = FileCreate(AFileName, Mode & 0xFF, FileRights);
    if (FHandle == static_cast<THandle>(-1))
    {
      throw EFCreateError(L"Cannot create file \"%s\". %s",
        ARRAYOFCONST((ExpandFileName(AFileName), SysErrorMessage(GetLastError()))));
    }
  }
  else
  {
    FHandle = FileOpen(AFileName, Mode);
    if (FHandle == static_cast<THandle>(-1))
    {
      throw EFOpenError(L"Cannot open file \"%s\". %s",
        ARRAYOFCONST((ExpandFileName(AFileName), SysErrorMessage(GetLastError()))));
    }
  }
  FFileName = AFileName;
}
//---------------------------------------------------------------------------
TFileStream::~TFileStream()
{
  if (FHandle != static_cast<THandle>(-1))
  {
    FileClose(FHandle);
  }
}
//---------------------------------------------------------------------------
// TCustomMemoryStream
//---------------------------------------------------------------------------
void TCustomMemoryStream::SetPointer(void * Ptr, NativeInt Size)
{
  FMemory = Ptr;
  FSize = Size;
}
//---------------------------------------------------------------------------
int TCustomMemoryStream::Read(void * Buffer, int Count)
{
  if ((FPosition >= 0) && (Count >= 0))
  {
    NativeInt Result = FSize - FPosition;
    if (Result > 0)
    {
      if (Result > Count)
      {
        Result = Count;
      }
      memmove(Buffer, static_cast<char *>(FMemory) + FPosition, Result);
      FPosition += Result;
      return static_cast<int>(Result);
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
__int64 TCustomMemoryStream::Seek(const __int64 Offset, TSeekOrigin Origin)
{
  switch (Origin)
  {
    case soBeginning: FPosition = static_cast<NativeInt>(Offset); break;
    case soCurrent: FPosition += static_cast<NativeInt>(Offset); break;
    case soEnd: FPosition = FSize + static_cast<NativeInt>(Offset); break;
  }
  return FPosition;
}
//---------------------------------------------------------------------------
void TCustomMemoryStream::SaveToStream(TStream * Stream)
{
  if (FSize != 0)
  {
    Stream->WriteBuffer(FMemory, static_cast<int>(FSize));
  }
}
//---------------------------------------------------------------------------
void TCustomMemoryStream::SaveToFile(const UnicodeString FileName)
{
  TFileStream Stream(FileName, fmCreate);
  SaveToStream(&Stream);
}
//---------------------------------------------------------------------------
// TMemoryStream
//---------------------------------------------------------------------------
TMemoryStream::~TMemoryStream()
{
  Clear();
}
//---------------------------------------------------------------------------
void TMemoryStream::Clear()
{
  SetCapacity(0);
  FSize = 0;
  FPosition = 0;
}
//---------------------------------------------------------------------------
void TMemoryStream::LoadFromStream(TStream * Stream)
{
  Stream->Position = 0;
  __int64 Count = Stream->Size;
  SetSize(Count);
  if (Count != 0)
  {
    Stream->ReadBuffer(FMemory, static_cast<int>(Count));
  }
}
//---------------------------------------------------------------------------
void TMemoryStream::LoadFromFile(const UnicodeString FileName)
{
  TFileStream Stream(FileName, static_cast<Word>(fmOpenRead | fmShareDenyWrite));
  LoadFromStream(&Stream);
}
//---------------------------------------------------------------------------
void TMemoryStream::SetCapacity(NativeInt NewCapacity)
{
  SetPointer(Realloc(NewCapacity), FSize);
  FCapacity = NewCapacity;
}
//---------------------------------------------------------------------------
void TMemoryStream::SetSize(const __int64 NewSize)
{
  NativeInt OldPosition = FPosition;
  NativeInt OldSize = FSize;
  SetCapacity(static_cast<NativeInt>(NewSize));
  FSize = static_cast<NativeInt>(NewSize);
  if (FSize > OldSize)
  {
    memset(static_cast<char *>(FMemory) + OldSize, 0, FSize - OldSize);
  }
  if (OldPosition > NewSize)
  {
    Seek(static_cast<__int64>(0), soEnd);
  }
}
//---------------------------------------------------------------------------
void * TMemoryStream::Realloc(NativeInt & NewCapacity)
{
  const NativeInt MemoryDelta = 0x2000;
  if ((NewCapacity > 0) && (NewCapacity != FSize))
  {
    NewCapacity = (NewCapacity + (MemoryDelta - 1)) & ~(MemoryDelta - 1);
  }
  void * Result = FMemory;
  if (NewCapacity != FCapacity)
  {
    if (NewCapacity == 0)
    {
      free(FMemory);
      Result = nullptr;
    }
    else
    {
      Result = realloc(FMemory, NewCapacity);
      if (Result == nullptr)
      {
        throw EStreamError(L"Out of memory while expanding memory stream");
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int TMemoryStream::Write(const void * Buffer, int Count)
{
  if ((FPosition >= 0) && (Count >= 0))
  {
    NativeInt Pos = FPosition + Count;
    if (Pos > 0)
    {
      if (Pos > FSize)
      {
        if (Pos > FCapacity)
        {
          SetCapacity(Pos);
        }
        if (FPosition > FSize)
        {
          memset(static_cast<char *>(FMemory) + FSize, 0, FPosition - FSize);
        }
        FSize = Pos;
      }
      memmove(static_cast<char *>(FMemory) + FPosition, Buffer, Count);
      FPosition = Pos;
      return Count;
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
// TBytesStream
//---------------------------------------------------------------------------
TBytesStream::TBytesStream(const TBytes & ABytes)
{
  if (ABytes.Length > 0)
  {
    WriteBuffer(ABytes.data(), ABytes.Length);
    Position = 0;
  }
}
//---------------------------------------------------------------------------
TBytes TBytesStream::GetBytes()
{
  return BytesOf(Memory, static_cast<int>(Size));
}
//---------------------------------------------------------------------------
} // namespace Classes
} // namespace System
