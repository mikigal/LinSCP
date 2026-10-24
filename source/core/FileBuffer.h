//---------------------------------------------------------------------------
#ifndef FileBufferH
#define FileBufferH

#include <Classes.hpp>
//---------------------------------------------------------------------------
extern const wchar_t * EOLTypeNames;
enum TEOLType : int { eolLF /* \n */, eolCRLF /* \r\n */, eolCR /* \r */ };
const int cpRemoveCtrlZ = 0x01;
const int cpRemoveBOM =   0x02;
//---------------------------------------------------------------------------
typedef TMethodPtr<void(TObject * Sender, const unsigned char * Data, size_t Len)> TTransferOutEvent;
typedef TMethodPtr<size_t(TObject * Sender, unsigned char * Data, size_t Len)> TTransferInEvent;
//---------------------------------------------------------------------------
class TFileBuffer
{
public:
  __fastcall TFileBuffer();
  virtual __fastcall ~TFileBuffer();
  void __fastcall Convert(const char * Source, const char * Dest, int Params, bool & Token);
  void __fastcall Convert(TEOLType Source, TEOLType Dest, int Params, bool & Token);
  void __fastcall Convert(const char * Source, TEOLType Dest, int Params, bool & Token);
  void __fastcall Convert(TEOLType Source, const char * Dest, int Params, bool & Token);
  void __fastcall Insert(int Index, const char * Buf, int Len);
  void __fastcall Delete(int Index, int Len);
  DWORD __fastcall LoadStream(TStream * Stream, const DWORD Len, bool ForceLen);
  DWORD __fastcall ReadStream(TStream * Stream, const DWORD Len, bool ForceLen);
  DWORD __fastcall LoadFromIn(TTransferInEvent OnTransferIn, TObject * Sender, DWORD Len);
  void __fastcall WriteToStream(TStream * Stream, const DWORD Len);
  void __fastcall WriteToOut(TTransferOutEvent OnTransferOut, TObject * Sender, const DWORD Len);
  void Reset();
  TMemoryStream * __property_get_Memory() const { return FMemory; } __declspec(property(get = __property_get_Memory)) TMemoryStream * Memory;
  char * __property_get_Data() const { return ::rtl::Mutable(this)->GetData(); } __declspec(property(get = __property_get_Data)) char * Data;
  int __property_get_Size() const { return FSize; } void __property_set_Size(int Value) { SetSize(Value); } __declspec(property(get = __property_get_Size, put = __property_set_Size)) int Size;

private:
  TMemoryStream * FMemory;
  int FSize;

  char * __fastcall GetData() const { return static_cast<char *>(FMemory->Memory); }
  char * __fastcall GetPointer() const { return GetData() + GetPosition(); }
  void NeedSpace(DWORD Size);
  void __fastcall SetSize(int value);
  int __fastcall GetPosition() const { return static_cast<int>(FMemory->Position); }
  void __fastcall ProcessRead(DWORD Len, DWORD Result);
};
//---------------------------------------------------------------------------
class TSafeHandleStream : public THandleStream
{
public:
  __fastcall TSafeHandleStream(NativeUInt AHandle);
  __fastcall TSafeHandleStream(THandleStream * Source, bool Own);
  static TSafeHandleStream * CreateFromFile(const UnicodeString & FileName, unsigned short Mode);
  virtual __fastcall ~TSafeHandleStream();
  virtual int __fastcall Read(void * Buffer, int Count);
  virtual int __fastcall Write(const void * Buffer, int Count);
  virtual int __fastcall Read(System::DynamicArray<System::Byte> Buffer, int Offset, int Count);
  virtual int __fastcall Write(const System::DynamicArray<System::Byte> Buffer, int Offset, int Count);
private:
  THandleStream * FSource;
};
//---------------------------------------------------------------------------
const char * __fastcall EOLToStr(TEOLType EOLType);
//---------------------------------------------------------------------------
#endif
