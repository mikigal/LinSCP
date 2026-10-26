// Xml.XMLIntf / Xml.XMLDoc on top of expat
//
// All nodes of a parsed document belong to one tree object; references to any
// node keep the whole tree alive (as COM DOM nodes keep their document), so
// there are no reference cycles between parents and children.
//---------------------------------------------------------------------------
#include <rtl/Xml.h>

#include <expat.h>
#include <memory>
#include <vector>
//---------------------------------------------------------------------------
namespace Xml
{
namespace Xmldoc
{
//---------------------------------------------------------------------------
using namespace System;
using namespace Xmlintf;
class TXMLNodeList;
//---------------------------------------------------------------------------
struct TXMLTree
{
  std::atomic<ULONG> RefCount{0};
  std::vector<std::unique_ptr<TXMLNode>> Nodes;
  std::vector<std::unique_ptr<TXMLNodeList>> Lists;
};
//---------------------------------------------------------------------------
class TXMLNode : public TObject, public IXMLNode
{
public:
  TXMLNode(TXMLTree * Tree, TNodeType Type) : FTree(Tree), FType(Type) {}

  virtual HRESULT QueryInterface(REFIID Riid, void ** Object) override { return QueryUnknown(this, Riid, Object); }
  virtual ULONG AddRef() override { return ++FTree->RefCount; }
  virtual ULONG Release() override
  {
    ULONG Result = --FTree->RefCount;
    if (Result == 0)
    {
      delete FTree;
    }
    return Result;
  }

  virtual UnicodeString GetNodeName() override;
  virtual UnicodeString GetLocalName() override { return FLocalName; }
  virtual UnicodeString GetNamespaceURI() override { return FNamespaceURI; }
  virtual UnicodeString GetPrefix() override { return FPrefix; }
  virtual TNodeType GetNodeType() override { return FType; }
  virtual OleVariant GetNodeValue() override;
  virtual UnicodeString GetText() override;
  virtual _di_IXMLNodeList GetChildNodes() override;
  virtual _di_IXMLNodeList GetAttributeNodes() override;
  virtual _di_IXMLNode GetParentNode() override { return FParent; }
  virtual bool GetHasChildNodes() override { return !FChildren.empty(); }
  virtual bool GetIsTextElement() override;
  virtual OleVariant GetAttribute(const UnicodeString AttrName) override;
  virtual bool HasAttribute(const UnicodeString Name) override;
  virtual UnicodeString GetXML() override;

  TXMLTree * FTree;
  TNodeType FType;
  TXMLNode * FParent = nullptr;
  UnicodeString FLocalName;
  UnicodeString FNamespaceURI;
  UnicodeString FPrefix;
  UnicodeString FValue;
  std::vector<TXMLNode *> FChildren;
  std::vector<TXMLNode *> FAttributes;
  TXMLNodeList * FChildList = nullptr;
  TXMLNodeList * FAttributeList = nullptr;
};
//---------------------------------------------------------------------------
class TXMLNodeList : public TObject, public IXMLNodeList
{
public:
  TXMLNodeList(TXMLNode * Owner, std::vector<TXMLNode *> * Nodes) : FOwner(Owner), FNodes(Nodes) {}

  virtual HRESULT QueryInterface(REFIID Riid, void ** Object) override { return QueryUnknown(this, Riid, Object); }
  virtual ULONG AddRef() override { return FOwner->AddRef(); }
  virtual ULONG Release() override { return FOwner->Release(); }

  virtual int GetCount() override { return static_cast<int>(FNodes->size()); }
  virtual _di_IXMLNode Get(int Index) override
  {
    if ((Index < 0) || (Index >= GetCount()))
    {
      throw EListError(L"List index out of bounds (%d)", ARRAYOFCONST((Index)));
    }
    return (*FNodes)[Index];
  }
  virtual _di_IXMLNode FindNode(const UnicodeString NodeName) override
  {
    for (TXMLNode * Node : *FNodes)
    {
      if (Node->GetNodeName() == NodeName)
      {
        return Node;
      }
    }
    return nullptr;
  }
  virtual _di_IXMLNode FindNode(const UnicodeString NodeName, const UnicodeString NamespaceURI) override
  {
    for (TXMLNode * Node : *FNodes)
    {
      if (((Node->GetNodeName() == NodeName) || (Node->FLocalName == NodeName)) &&
          (NamespaceURI.IsEmpty() || (Node->FNamespaceURI == NamespaceURI)))
      {
        return Node;
      }
    }
    return nullptr;
  }
  virtual _di_IXMLNode First() override { return FNodes->empty() ? nullptr : FNodes->front(); }
  virtual _di_IXMLNode Last() override { return FNodes->empty() ? nullptr : FNodes->back(); }
  virtual int IndexOf(const UnicodeString Name) override
  {
    for (size_t Index = 0; Index < FNodes->size(); Index++)
    {
      if ((*FNodes)[Index]->GetNodeName() == Name)
      {
        return static_cast<int>(Index);
      }
    }
    return -1;
  }

private:
  TXMLNode * FOwner;
  std::vector<TXMLNode *> * FNodes;
};
//---------------------------------------------------------------------------
UnicodeString TXMLNode::GetNodeName()
{
  switch (FType)
  {
    case ntText: return L"#text";
    case ntCData: return L"#cdata-section";
    case ntComment: return L"#comment";
    case ntDocument: return L"#document";
    default: return FPrefix.IsEmpty() ? FLocalName : (FPrefix + L":" + FLocalName);
  }
}
//---------------------------------------------------------------------------
bool TXMLNode::GetIsTextElement()
{
  return
    (FType == ntElement) && (FChildren.size() == 1) &&
    ((FChildren[0]->FType == ntText) || (FChildren[0]->FType == ntCData));
}
//---------------------------------------------------------------------------
OleVariant TXMLNode::GetNodeValue()
{
  switch (FType)
  {
    case ntElement:
      return GetIsTextElement() ? OleVariant(FChildren[0]->FValue) : Null();
    case ntDocument:
      return Null();
    default:
      return OleVariant(FValue);
  }
}
//---------------------------------------------------------------------------
UnicodeString TXMLNode::GetText()
{
  if ((FType != ntElement) && (FType != ntDocument))
  {
    return FValue;
  }
  UnicodeString Result;
  for (TXMLNode * Child : FChildren)
  {
    if ((Child->FType == ntText) || (Child->FType == ntCData))
    {
      Result += Child->FValue;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
_di_IXMLNodeList TXMLNode::GetChildNodes()
{
  if (FChildList == nullptr)
  {
    FTree->Lists.emplace_back(new TXMLNodeList(this, &FChildren));
    FChildList = FTree->Lists.back().get();
  }
  return FChildList;
}
//---------------------------------------------------------------------------
_di_IXMLNodeList TXMLNode::GetAttributeNodes()
{
  if (FAttributeList == nullptr)
  {
    FTree->Lists.emplace_back(new TXMLNodeList(this, &FAttributes));
    FAttributeList = FTree->Lists.back().get();
  }
  return FAttributeList;
}
//---------------------------------------------------------------------------
OleVariant TXMLNode::GetAttribute(const UnicodeString AttrName)
{
  for (TXMLNode * Attribute : FAttributes)
  {
    if (Attribute->GetNodeName() == AttrName)
    {
      return OleVariant(Attribute->FValue);
    }
  }
  return Null();
}
//---------------------------------------------------------------------------
bool TXMLNode::HasAttribute(const UnicodeString Name)
{
  for (TXMLNode * Attribute : FAttributes)
  {
    if (Attribute->GetNodeName() == Name)
    {
      return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
namespace
{
UnicodeString Escape(const UnicodeString & S, bool Attribute)
{
  std::wstring Result;
  for (wchar_t Ch : S)
  {
    switch (Ch)
    {
      case L'<': Result += L"&lt;"; break;
      case L'>': Result += L"&gt;"; break;
      case L'&': Result += L"&amp;"; break;
      case L'"':
        if (Attribute)
        {
          Result += L"&quot;";
          break;
        }
        Result.push_back(Ch);
        break;
      default: Result.push_back(Ch); break;
    }
  }
  return UnicodeString(std::move(Result));
}
} // namespace
//---------------------------------------------------------------------------
UnicodeString TXMLNode::GetXML()
{
  switch (FType)
  {
    case ntText: return Escape(FValue, false);
    case ntCData: return L"<![CDATA[" + FValue + L"]]>";
    case ntComment: return L"<!--" + FValue + L"-->";
    case ntProcessingInstr: return L"<?" + FLocalName + L" " + FValue + L"?>";
    case ntDocument:
      {
        UnicodeString Result;
        for (TXMLNode * Child : FChildren)
        {
          Result += Child->GetXML();
        }
        return Result;
      }
    default:
      {
        UnicodeString Result = L"<" + GetNodeName();
        for (TXMLNode * Attribute : FAttributes)
        {
          Result += L" " + Attribute->GetNodeName() + L"=\"" + Escape(Attribute->FValue, true) + L"\"";
        }
        if (FChildren.empty())
        {
          return Result + L"/>";
        }
        Result += L">";
        for (TXMLNode * Child : FChildren)
        {
          Result += Child->GetXML();
        }
        return Result + L"</" + GetNodeName() + L">";
      }
  }
}
//---------------------------------------------------------------------------
// Parsing
//---------------------------------------------------------------------------
namespace
{
const char NamespaceSeparator = '\x01';

struct TParseContext
{
  TXMLTree * Tree;
  TXMLNode * Current;
  bool InCData = false;
  UnicodeString Version;
  UnicodeString Encoding;
};

UnicodeString FromUtf8(const XML_Char * S, int Len = -1)
{
  return UnicodeString(rtl::DecodeUtf8(S, (Len < 0) ? strlen(S) : static_cast<size_t>(Len)));
}

TXMLNode * NewNode(TParseContext * Context, TNodeType Type)
{
  Context->Tree->Nodes.emplace_back(new TXMLNode(Context->Tree, Type));
  return Context->Tree->Nodes.back().get();
}

// "uri\x01local\x01prefix", "uri\x01local" or "local"
void SplitName(const XML_Char * Name, TXMLNode * Node)
{
  std::string S(Name);
  size_t First = S.find(NamespaceSeparator);
  if (First == std::string::npos)
  {
    Node->FLocalName = FromUtf8(Name);
    return;
  }
  Node->FNamespaceURI = FromUtf8(S.substr(0, First).c_str());
  size_t Second = S.find(NamespaceSeparator, First + 1);
  if (Second == std::string::npos)
  {
    Node->FLocalName = FromUtf8(S.substr(First + 1).c_str());
  }
  else
  {
    Node->FLocalName = FromUtf8(S.substr(First + 1, Second - First - 1).c_str());
    Node->FPrefix = FromUtf8(S.substr(Second + 1).c_str());
  }
}

void AppendChild(TXMLNode * Parent, TXMLNode * Child)
{
  Child->FParent = Parent;
  Parent->FChildren.push_back(Child);
}

void XMLCALL StartElement(void * UserData, const XML_Char * Name, const XML_Char ** Attributes)
{
  TParseContext * Context = static_cast<TParseContext *>(UserData);
  TXMLNode * Node = NewNode(Context, ntElement);
  SplitName(Name, Node);
  for (const XML_Char ** Attribute = Attributes; *Attribute != nullptr; Attribute += 2)
  {
    TXMLNode * AttributeNode = NewNode(Context, ntAttribute);
    SplitName(Attribute[0], AttributeNode);
    AttributeNode->FValue = FromUtf8(Attribute[1]);
    AttributeNode->FParent = Node;
    Node->FAttributes.push_back(AttributeNode);
  }
  AppendChild(Context->Current, Node);
  Context->Current = Node;
}

void XMLCALL EndElement(void * UserData, const XML_Char *)
{
  TParseContext * Context = static_cast<TParseContext *>(UserData);
  Context->Current = Context->Current->FParent;
}

void XMLCALL CharacterData(void * UserData, const XML_Char * Data, int Len)
{
  TParseContext * Context = static_cast<TParseContext *>(UserData);
  TNodeType Type = Context->InCData ? ntCData : ntText;
  std::vector<TXMLNode *> & Children = Context->Current->FChildren;
  if (!Children.empty() && (Children.back()->FType == Type))
  {
    Children.back()->FValue += FromUtf8(Data, Len);
  }
  else
  {
    TXMLNode * Node = NewNode(Context, Type);
    Node->FValue = FromUtf8(Data, Len);
    AppendChild(Context->Current, Node);
  }
}

void XMLCALL StartCData(void * UserData)
{
  TParseContext * Context = static_cast<TParseContext *>(UserData);
  Context->InCData = true;
  // Adjacent CDATA sections are separate nodes
  TXMLNode * Node = NewNode(Context, ntCData);
  AppendChild(Context->Current, Node);
}

void XMLCALL EndCData(void * UserData)
{
  static_cast<TParseContext *>(UserData)->InCData = false;
}

void XMLCALL Comment(void * UserData, const XML_Char * Data)
{
  TParseContext * Context = static_cast<TParseContext *>(UserData);
  TXMLNode * Node = NewNode(Context, ntComment);
  Node->FValue = FromUtf8(Data);
  AppendChild(Context->Current, Node);
}

void XMLCALL ProcessingInstruction(void * UserData, const XML_Char * Target, const XML_Char * Data)
{
  TParseContext * Context = static_cast<TParseContext *>(UserData);
  TXMLNode * Node = NewNode(Context, ntProcessingInstr);
  Node->FLocalName = FromUtf8(Target);
  Node->FValue = FromUtf8(Data);
  AppendChild(Context->Current, Node);
}

void XMLCALL XmlDecl(void * UserData, const XML_Char * Version, const XML_Char * Encoding, int)
{
  TParseContext * Context = static_cast<TParseContext *>(UserData);
  if (Version != nullptr)
  {
    Context->Version = FromUtf8(Version);
  }
  if (Encoding != nullptr)
  {
    Context->Encoding = FromUtf8(Encoding);
  }
}

bool IsBlank(const UnicodeString & S)
{
  for (wchar_t Ch : S)
  {
    if ((Ch != L' ') && (Ch != L'\t') && (Ch != L'\n') && (Ch != L'\r'))
    {
      return false;
    }
  }
  return true;
}

void RemoveBlankText(TXMLNode * Node)
{
  std::vector<TXMLNode *> Children;
  for (TXMLNode * Child : Node->FChildren)
  {
    if ((Child->FType == ntText) && IsBlank(Child->FValue))
    {
      continue;
    }
    RemoveBlankText(Child);
    Children.push_back(Child);
  }
  Node->FChildren.swap(Children);
}

#if (XML_MAJOR_VERSION > 2) || ((XML_MAJOR_VERSION == 2) && (XML_MINOR_VERSION >= 9))
inline long long LineNumber(XML_Parser Parser) { return XML_GetCurrentLineNumber64(Parser); }
inline long long ColumnNumber(XML_Parser Parser) { return XML_GetCurrentColumnNumber64(Parser); }
#else
inline long long LineNumber(XML_Parser Parser) { return XML_GetCurrentLineNumber(Parser); }
inline long long ColumnNumber(XML_Parser Parser) { return XML_GetCurrentColumnNumber(Parser); }
#endif

class TParser
{
public:
  explicit TParser(const char * Encoding) : FParser(XML_ParserCreateNS(Encoding, NamespaceSeparator)) {}
  ~TParser() { XML_ParserFree(FParser); }
  XML_Parser Handle() const { return FParser; }

private:
  XML_Parser FParser;
};
} // namespace
//---------------------------------------------------------------------------
// TXMLDocument
//---------------------------------------------------------------------------
TXMLDocument::TXMLDocument(TObject *)
{
}
//---------------------------------------------------------------------------
TXMLDocument::TXMLDocument(const UnicodeString AFileName) :
  FFileName(AFileName)
{
}
//---------------------------------------------------------------------------
TXMLDocument::~TXMLDocument()
{
}
//---------------------------------------------------------------------------
void TXMLDocument::Parse(const char * Data, size_t Length, const char * ForcedEncoding)
{
  TXMLTree * Tree = new TXMLTree();
  Tree->Nodes.emplace_back(new TXMLNode(Tree, ntDocument));
  // Holds the tree until the end of the function
  DelphiInterface<TXMLNode> Root(Tree->Nodes.back().get());
  TParseContext Context;
  Context.Tree = Tree;
  Context.Current = Root;

  TParser Parser(ForcedEncoding);
  XML_Parser Handle = Parser.Handle();
  XML_SetReturnNSTriplet(Handle, 1);
  XML_SetUserData(Handle, &Context);
  XML_SetElementHandler(Handle, StartElement, EndElement);
  XML_SetCharacterDataHandler(Handle, CharacterData);
  XML_SetCdataSectionHandler(Handle, StartCData, EndCData);
  XML_SetCommentHandler(Handle, Comment);
  XML_SetProcessingInstructionHandler(Handle, ProcessingInstruction);
  XML_SetXmlDeclHandler(Handle, XmlDecl);
  if (XML_Parse(Handle, Data, static_cast<int>(Length), XML_TRUE) == XML_STATUS_ERROR)
  {
    throw EDOMParseError(L"%s (line %d, column %d)", ARRAYOFCONST((
      FromUtf8(XML_ErrorString(XML_GetErrorCode(Handle))),
      static_cast<int>(LineNumber(Handle)), static_cast<int>(ColumnNumber(Handle)))));
  }
  if (!FParseOptions.Contains(poPreserveWhiteSpace))
  {
    RemoveBlankText(Root);
  }
  FRoot = Root;
  FVersion = Context.Version;
  FEncoding = Context.Encoding;
  FActive = true;
}
//---------------------------------------------------------------------------
void TXMLDocument::LoadFromFile(const UnicodeString AFileName)
{
  TFileStream Stream(AFileName, static_cast<Word>(fmOpenRead | fmShareDenyWrite));
  LoadFromStream(&Stream);
  FFileName = AFileName;
}
//---------------------------------------------------------------------------
void TXMLDocument::LoadFromStream(TStream * Stream)
{
  std::string Data(static_cast<size_t>(Stream->Size - Stream->Position), '\0');
  if (!Data.empty())
  {
    Stream->ReadBuffer(Data.data(), static_cast<int>(Data.size()));
  }
  Parse(Data.data(), Data.size(), nullptr);
}
//---------------------------------------------------------------------------
void TXMLDocument::LoadFromXML(const UnicodeString XML)
{
  // The string is not in the encoding its declaration may state
  std::string Data = rtl::EncodeUtf8(XML.c_str(), XML.Length());
  Parse(Data.data(), Data.size(), "UTF-8");
}
//---------------------------------------------------------------------------
_di_IXMLNodeList TXMLDocument::GetChildNodes()
{
  if (!FRoot)
  {
    throw EDOMParseError(L"XML document is not active");
  }
  return FRoot->GetChildNodes();
}
//---------------------------------------------------------------------------
_di_IXMLNode TXMLDocument::GetDocumentElement()
{
  if (FRoot)
  {
    for (TXMLNode * Child : FRoot->FChildren)
    {
      if (Child->FType == ntElement)
      {
        return Child;
      }
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
void TXMLDocument::SetActive(bool Value)
{
  if (Value && !FActive && !FFileName.IsEmpty())
  {
    LoadFromFile(FFileName);
  }
  else if (!Value)
  {
    FRoot = nullptr;
    FActive = false;
  }
}
//---------------------------------------------------------------------------
_di_IXMLDocument LoadXMLDocument(const UnicodeString FileName)
{
  _di_IXMLDocument Result = interface_cast<IXMLDocument>(new TXMLDocument(static_cast<TObject *>(nullptr)));
  Result->LoadFromFile(FileName);
  return Result;
}
//---------------------------------------------------------------------------
_di_IXMLDocument LoadXMLData(const UnicodeString XMLData)
{
  _di_IXMLDocument Result = interface_cast<IXMLDocument>(new TXMLDocument(static_cast<TObject *>(nullptr)));
  Result->LoadFromXML(XMLData);
  return Result;
}
//---------------------------------------------------------------------------
} // namespace Xmldoc
} // namespace Xml
