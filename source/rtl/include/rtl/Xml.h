// A read-only XML DOM on top of expat with the interface of Delphi's
// Xml.XMLIntf/Xml.XMLDoc (IXMLDocument, IXMLNode, IXMLNodeList, TXMLDocument),
// plus the reference-counted interface support it needs (IInterface,
// TInterfacedObject, DelphiInterface, interface_cast).
//---------------------------------------------------------------------------
#ifndef RtlXmlH
#define RtlXmlH
//---------------------------------------------------------------------------
#include <rtl/Classes.h>
#include <rtl/Variants.h>

#include <atomic>
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
class IInterface
{
public:
  virtual unsigned long AddRef() = 0;
  virtual unsigned long Release() = 0;

protected:
  virtual ~IInterface() {}
};
typedef IInterface IUnknown;
//---------------------------------------------------------------------------
class TInterfacedObject : public TObject, public virtual IInterface
{
public:
  virtual unsigned long AddRef() override { return ++FRefCount; }
  virtual unsigned long Release() override
  {
    unsigned long Result = --FRefCount;
    if (Result == 0)
    {
      delete this;
    }
    return Result;
  }
  int GetRefCount() const { return FRefCount; }
  __declspec(property(get = GetRefCount)) int RefCount;

private:
  std::atomic<unsigned long> FRefCount{0};
};
//---------------------------------------------------------------------------
// Smart pointer to an interface (C++Builder's _di_ types)
template <class T>
class DelphiInterface
{
public:
  DelphiInterface() noexcept = default;
  DelphiInterface(T * Intf) : FIntf(Intf) { if (FIntf != nullptr) FIntf->AddRef(); }
  DelphiInterface(const DelphiInterface & Src) : DelphiInterface(Src.FIntf) {}
  DelphiInterface(DelphiInterface && Src) noexcept : FIntf(Src.FIntf) { Src.FIntf = nullptr; }
  ~DelphiInterface() { Reset(); }

  DelphiInterface & operator =(const DelphiInterface & Rhs) { DelphiInterface(Rhs).Swap(*this); return *this; }
  DelphiInterface & operator =(DelphiInterface && Rhs) noexcept { DelphiInterface(std::move(Rhs)).Swap(*this); return *this; }
  DelphiInterface & operator =(T * Rhs) { DelphiInterface(Rhs).Swap(*this); return *this; }

  T * operator ->() const { return FIntf; }
  operator T * () const { return FIntf; }
  T & operator *() const { return *FIntf; }
  T * GetInterface() const { return FIntf; }
  bool operator !() const { return (FIntf == nullptr); }

private:
  void Reset()
  {
    if (FIntf != nullptr)
    {
      T * Intf = FIntf;
      FIntf = nullptr;
      Intf->Release();
    }
  }
  void Swap(DelphiInterface & Other) noexcept { std::swap(FIntf, Other.FIntf); }

  T * FIntf = nullptr;
};
//---------------------------------------------------------------------------
template <class I, class T>
DelphiInterface<I> interface_cast(T * Object)
{
  return DelphiInterface<I>(dynamic_cast<I *>(Object));
}
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
namespace Xml
{
namespace Xmldom
{
RTL_DECLARE_EXCEPTION(EDOMParseError, System::Sysutils::Exception)
} // namespace Xmldom
//---------------------------------------------------------------------------
namespace Xmlintf
{
//---------------------------------------------------------------------------
enum TNodeType
{
  ntReserved, ntElement, ntAttribute, ntText, ntCData, ntEntityRef, ntEntity, ntProcessingInstr,
  ntComment, ntDocument, ntDocType, ntDocFragment, ntNotation
};
//---------------------------------------------------------------------------
class IXMLNode;
class IXMLNodeList;
class IXMLDocument;
typedef System::DelphiInterface<IXMLNode> _di_IXMLNode;
typedef System::DelphiInterface<IXMLNodeList> _di_IXMLNodeList;
typedef System::DelphiInterface<IXMLDocument> _di_IXMLDocument;
//---------------------------------------------------------------------------
class IXMLNodeList : public virtual System::IInterface
{
public:
  virtual int GetCount() = 0;
  virtual _di_IXMLNode Get(int Index) = 0;
  virtual _di_IXMLNode FindNode(const System::UnicodeString NodeName) = 0;
  virtual _di_IXMLNode FindNode(const System::UnicodeString NodeName, const System::UnicodeString NamespaceURI) = 0;
  virtual _di_IXMLNode First() = 0;
  virtual _di_IXMLNode Last() = 0;
  virtual int IndexOf(const System::UnicodeString Name) = 0;

  _di_IXMLNode GetNode(int Index) { return Get(Index); }
  __declspec(property(get = GetCount)) int Count;
  __declspec(property(get = GetNode)) _di_IXMLNode Nodes[];
};
//---------------------------------------------------------------------------
class IXMLNode : public virtual System::IInterface
{
public:
  virtual System::UnicodeString GetNodeName() = 0;
  virtual System::UnicodeString GetLocalName() = 0;
  virtual System::UnicodeString GetNamespaceURI() = 0;
  virtual System::UnicodeString GetPrefix() = 0;
  virtual TNodeType GetNodeType() = 0;
  virtual System::OleVariant GetNodeValue() = 0;
  virtual System::UnicodeString GetText() = 0;
  virtual _di_IXMLNodeList GetChildNodes() = 0;
  virtual _di_IXMLNodeList GetAttributeNodes() = 0;
  virtual _di_IXMLNode GetParentNode() = 0;
  virtual bool GetHasChildNodes() = 0;
  virtual bool GetIsTextElement() = 0;
  virtual System::OleVariant GetAttribute(const System::UnicodeString AttrName) = 0;
  virtual bool HasAttribute(const System::UnicodeString Name) = 0;
  virtual System::UnicodeString GetXML() = 0;

  __declspec(property(get = GetNodeName)) System::UnicodeString NodeName;
  __declspec(property(get = GetLocalName)) System::UnicodeString LocalName;
  __declspec(property(get = GetNamespaceURI)) System::UnicodeString NamespaceURI;
  __declspec(property(get = GetPrefix)) System::UnicodeString Prefix;
  __declspec(property(get = GetNodeType)) TNodeType NodeType;
  __declspec(property(get = GetNodeValue)) System::OleVariant NodeValue;
  __declspec(property(get = GetText)) System::UnicodeString Text;
  __declspec(property(get = GetChildNodes)) _di_IXMLNodeList ChildNodes;
  __declspec(property(get = GetAttributeNodes)) _di_IXMLNodeList AttributeNodes;
  __declspec(property(get = GetParentNode)) _di_IXMLNode ParentNode;
  __declspec(property(get = GetHasChildNodes)) bool HasChildNodes;
  __declspec(property(get = GetIsTextElement)) bool IsTextElement;
  __declspec(property(get = GetAttribute)) System::OleVariant Attributes[];
  __declspec(property(get = GetXML)) System::UnicodeString XML;
};
//---------------------------------------------------------------------------
} // namespace Xmlintf
//---------------------------------------------------------------------------
namespace Xmldoc
{
//---------------------------------------------------------------------------
enum TParseOption { poResolveExternals, poValidateOnParse, poPreserveWhiteSpace, poAsyncLoad };
typedef System::Set<TParseOption, poResolveExternals, poAsyncLoad> TParseOptions;
//---------------------------------------------------------------------------
} // namespace Xmldoc
//---------------------------------------------------------------------------
namespace Xmlintf
{
//---------------------------------------------------------------------------
class IXMLDocument : public virtual System::IInterface
{
public:
  virtual void LoadFromFile(const System::UnicodeString AFileName) = 0;
  virtual void LoadFromStream(System::Classes::TStream * Stream) = 0;
  virtual void LoadFromXML(const System::UnicodeString XML) = 0;
  virtual _di_IXMLNodeList GetChildNodes() = 0;
  virtual _di_IXMLNode GetDocumentElement() = 0;
  virtual bool GetActive() = 0;
  virtual void SetActive(bool Value) = 0;
  virtual Xmldoc::TParseOptions GetParseOptions() = 0;
  virtual void SetParseOptions(Xmldoc::TParseOptions Value) = 0;
  virtual System::UnicodeString GetEncoding() = 0;
  virtual System::UnicodeString GetVersion() = 0;

  __declspec(property(get = GetChildNodes)) _di_IXMLNodeList ChildNodes;
  __declspec(property(get = GetDocumentElement)) _di_IXMLNode DocumentElement;
  __declspec(property(get = GetActive, put = SetActive)) bool Active;
  __declspec(property(get = GetParseOptions, put = SetParseOptions)) Xmldoc::TParseOptions ParseOptions;
  __declspec(property(get = GetEncoding)) System::UnicodeString Encoding;
  __declspec(property(get = GetVersion)) System::UnicodeString Version;
};
//---------------------------------------------------------------------------
} // namespace Xmlintf
//---------------------------------------------------------------------------
namespace Xmldoc
{
//---------------------------------------------------------------------------
class TXMLNode;
//---------------------------------------------------------------------------
// With a NULL owner, as the core creates it, the document lives as long as
// interface references to it exist.
class TXMLDocument : public System::TInterfacedObject, public Xmlintf::IXMLDocument
{
public:
  explicit TXMLDocument(System::TObject * AOwner);
  explicit TXMLDocument(const System::UnicodeString AFileName);
  virtual ~TXMLDocument();

  virtual unsigned long AddRef() override { return TInterfacedObject::AddRef(); }
  virtual unsigned long Release() override { return TInterfacedObject::Release(); }

  virtual void LoadFromFile(const System::UnicodeString AFileName) override;
  virtual void LoadFromStream(System::Classes::TStream * Stream) override;
  virtual void LoadFromXML(const System::UnicodeString XML) override;
  virtual Xmlintf::_di_IXMLNodeList GetChildNodes() override;
  virtual Xmlintf::_di_IXMLNode GetDocumentElement() override;
  virtual bool GetActive() override { return FActive; }
  virtual void SetActive(bool Value) override;
  virtual TParseOptions GetParseOptions() override { return FParseOptions; }
  virtual void SetParseOptions(TParseOptions Value) override { FParseOptions = Value; }
  virtual System::UnicodeString GetEncoding() override { return FEncoding; }
  virtual System::UnicodeString GetVersion() override { return FVersion; }

private:
  void Parse(const char * Data, size_t Length, const char * ForcedEncoding);

  System::DelphiInterface<TXMLNode> FRoot;
  TParseOptions FParseOptions;
  bool FActive = false;
  System::UnicodeString FFileName;
  System::UnicodeString FEncoding;
  System::UnicodeString FVersion;
};
//---------------------------------------------------------------------------
Xmlintf::_di_IXMLDocument LoadXMLDocument(const System::UnicodeString FileName);
Xmlintf::_di_IXMLDocument LoadXMLData(const System::UnicodeString XMLData);
//---------------------------------------------------------------------------
} // namespace Xmldoc
} // namespace Xml
//---------------------------------------------------------------------------
using namespace Xml;
using namespace Xml::Xmldom;
using namespace Xml::Xmlintf;
using namespace Xml::Xmldoc;
//---------------------------------------------------------------------------
#endif
