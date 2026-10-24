// A read-only XML DOM on top of expat with the interface of Delphi's
// Xml.XMLIntf/Xml.XMLDoc (IXMLDocument, IXMLNode, IXMLNodeList, TXMLDocument).
//---------------------------------------------------------------------------
#ifndef RtlXmlH
#define RtlXmlH
//---------------------------------------------------------------------------
#include <rtl/Classes.h>
#include <rtl/Interface.h>
#include <rtl/Variants.h>
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
  RTL_PROPERTY_GET(Count, GetCount)
  __declspec(property(get = __property_get_Count)) int Count;
  RTL_PROPERTY_GET(Nodes, GetNode)
  __declspec(property(get = __property_get_Nodes)) _di_IXMLNode Nodes[];
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

  RTL_PROPERTY_GET(NodeName, GetNodeName)
  __declspec(property(get = __property_get_NodeName)) System::UnicodeString NodeName;
  RTL_PROPERTY_GET(LocalName, GetLocalName)
  __declspec(property(get = __property_get_LocalName)) System::UnicodeString LocalName;
  RTL_PROPERTY_GET(NamespaceURI, GetNamespaceURI)
  __declspec(property(get = __property_get_NamespaceURI)) System::UnicodeString NamespaceURI;
  RTL_PROPERTY_GET(Prefix, GetPrefix)
  __declspec(property(get = __property_get_Prefix)) System::UnicodeString Prefix;
  RTL_PROPERTY_GET(NodeType, GetNodeType)
  __declspec(property(get = __property_get_NodeType)) TNodeType NodeType;
  RTL_PROPERTY_GET(NodeValue, GetNodeValue)
  __declspec(property(get = __property_get_NodeValue)) System::OleVariant NodeValue;
  RTL_PROPERTY_GET(Text, GetText)
  __declspec(property(get = __property_get_Text)) System::UnicodeString Text;
  RTL_PROPERTY_GET(ChildNodes, GetChildNodes)
  __declspec(property(get = __property_get_ChildNodes)) _di_IXMLNodeList ChildNodes;
  RTL_PROPERTY_GET(AttributeNodes, GetAttributeNodes)
  __declspec(property(get = __property_get_AttributeNodes)) _di_IXMLNodeList AttributeNodes;
  RTL_PROPERTY_GET(ParentNode, GetParentNode)
  __declspec(property(get = __property_get_ParentNode)) _di_IXMLNode ParentNode;
  RTL_PROPERTY_GET(HasChildNodes, GetHasChildNodes)
  __declspec(property(get = __property_get_HasChildNodes)) bool HasChildNodes;
  RTL_PROPERTY_GET(IsTextElement, GetIsTextElement)
  __declspec(property(get = __property_get_IsTextElement)) bool IsTextElement;
  RTL_PROPERTY_GET(Attributes, GetAttribute)
  __declspec(property(get = __property_get_Attributes)) System::OleVariant Attributes[];
  RTL_PROPERTY_GET(XML, GetXML)
  __declspec(property(get = __property_get_XML)) System::UnicodeString XML;
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

  RTL_PROPERTY_GET(ChildNodes, GetChildNodes)
  __declspec(property(get = __property_get_ChildNodes)) _di_IXMLNodeList ChildNodes;
  RTL_PROPERTY_GET(DocumentElement, GetDocumentElement)
  __declspec(property(get = __property_get_DocumentElement)) _di_IXMLNode DocumentElement;
  RTL_PROPERTY_GET(Active, GetActive) RTL_PROPERTY_PUT(Active, SetActive, bool)
  __declspec(property(get = __property_get_Active, put = __property_set_Active)) bool Active;
  RTL_PROPERTY_GET(ParseOptions, GetParseOptions) RTL_PROPERTY_PUT(ParseOptions, SetParseOptions, Xmldoc::TParseOptions)
  __declspec(property(get = __property_get_ParseOptions, put = __property_set_ParseOptions)) Xmldoc::TParseOptions ParseOptions;
  RTL_PROPERTY_GET(Encoding, GetEncoding)
  __declspec(property(get = __property_get_Encoding)) System::UnicodeString Encoding;
  RTL_PROPERTY_GET(Version, GetVersion)
  __declspec(property(get = __property_get_Version)) System::UnicodeString Version;
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

  virtual ULONG AddRef() override { return TInterfacedObject::AddRef(); }
  virtual ULONG Release() override { return TInterfacedObject::Release(); }

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
