//---------------------------------------------------------------------------
#ifndef BookmarksH
#define BookmarksH
//---------------------------------------------------------------------------
#include <CopyParam.h>
//---------------------------------------------------------------------------
class THierarchicalStorage;
class TBookmarkList;
class TShortCuts;
//---------------------------------------------------------------------------
class TBookmarks : public TObject
{
public:
  __fastcall TBookmarks();
  virtual __fastcall ~TBookmarks();

  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage, bool All);
  void __fastcall ModifyAll(bool Modify);
  void __fastcall Clear();

  TBookmarkList * __property_get_Bookmarks(UnicodeString Index) const { return ::rtl::Mutable(this)->GetBookmarks(Index); } void __property_set_Bookmarks(UnicodeString Index, TBookmarkList * Value) { SetBookmarks(Index, Value); } __declspec(property(get = __property_get_Bookmarks, put = __property_set_Bookmarks)) TBookmarkList * Bookmarks[];
  TBookmarkList * __property_get_SharedBookmarks() const { return ::rtl::Mutable(this)->GetSharedBookmarks(); } void __property_set_SharedBookmarks(TBookmarkList * Value) { SetSharedBookmarks(Value); } __declspec(property(get = __property_get_SharedBookmarks, put = __property_set_SharedBookmarks)) TBookmarkList * SharedBookmarks;

private:
  TStringList * FBookmarkLists;
  UnicodeString FSharedKey;
  static UnicodeString Keys[];

  TBookmarkList * __fastcall GetBookmarks(UnicodeString Index);
  void __fastcall SetBookmarks(UnicodeString Index, TBookmarkList * value);
  TBookmarkList * __fastcall GetSharedBookmarks();
  void __fastcall SetSharedBookmarks(TBookmarkList * value);
  void __fastcall LoadLevel(THierarchicalStorage * Storage, const UnicodeString Key,
    int Index, TBookmarkList * BookmarkList);
};
//---------------------------------------------------------------------------
class TBookmark;
//---------------------------------------------------------------------------
class TBookmarkList : public TPersistent
{
friend class TBookmarks;
friend class TBookmark;
public:
  __fastcall TBookmarkList();
  virtual __fastcall ~TBookmarkList();

  void __fastcall Clear();
  void __fastcall Add(TBookmark * Bookmark);
  void __fastcall Insert(int Index, TBookmark * Bookmark);
  void __fastcall InsertBefore(TBookmark * BeforeBookmark, TBookmark * Bookmark);
  void __fastcall MoveTo(TBookmark * ToBookmark, TBookmark * Bookmark, bool Before);
  void __fastcall Delete(TBookmark * Bookmark);
  TBookmark * __fastcall FindByName(const UnicodeString Node, const UnicodeString Name);
  TBookmark * __fastcall FindByShortCut(TShortCut ShortCut);
  virtual void __fastcall Assign(TPersistent * Source);
  void __fastcall LoadOptions(THierarchicalStorage * Storage);
  void __fastcall SaveOptions(THierarchicalStorage * Storage);
  void __fastcall ShortCuts(TShortCuts & ShortCuts);

  int __property_get_Count() const { return ::rtl::Mutable(this)->GetCount(); } __declspec(property(get = __property_get_Count)) int Count;
  TBookmark * __property_get_Bookmarks(int Index) const { return ::rtl::Mutable(this)->GetBookmarks(Index); } __declspec(property(get = __property_get_Bookmarks)) TBookmark * Bookmarks[];
  bool __property_get_NodeOpened(UnicodeString Index) const { return ::rtl::Mutable(this)->GetNodeOpened(Index); } void __property_set_NodeOpened(UnicodeString Index, bool Value) { SetNodeOpened(Index, Value); } __declspec(property(get = __property_get_NodeOpened, put = __property_set_NodeOpened)) bool NodeOpened[];

protected:
  int __fastcall IndexOf(TBookmark * Bookmark);
  void __fastcall KeyChanged(int Index);

  bool & __property_get_Modified() const { return ::rtl::Mutable(this)->FModified; } void __property_set_Modified(bool Value) { FModified = Value; } __declspec(property(get = __property_get_Modified, put = __property_set_Modified)) bool Modified;

private:
  TStringList * FBookmarks;
  TStringList * FOpenedNodes;
  bool FModified;

  int __fastcall GetCount();
  TBookmark * __fastcall GetBookmarks(int Index);
  bool __fastcall GetNodeOpened(UnicodeString Index);
  void __fastcall SetNodeOpened(UnicodeString Index, bool value);
};
//---------------------------------------------------------------------------
class TBookmark : public TPersistent
{
friend class TBookmarkList;
public:
  __fastcall TBookmark();

  virtual void __fastcall Assign(TPersistent * Source);

  UnicodeString __fastcall GetSideDirectory(TOperationSide Side);

  UnicodeString & __property_get_Name() const { return ::rtl::Mutable(this)->FName; } void __property_set_Name(UnicodeString Value) { SetName(Value); } __declspec(property(get = __property_get_Name, put = __property_set_Name)) UnicodeString Name;
  UnicodeString & __property_get_Local() const { return ::rtl::Mutable(this)->FLocal; } void __property_set_Local(UnicodeString Value) { SetLocal(Value); } __declspec(property(get = __property_get_Local, put = __property_set_Local)) UnicodeString Local;
  UnicodeString & __property_get_Remote() const { return ::rtl::Mutable(this)->FRemote; } void __property_set_Remote(UnicodeString Value) { SetRemote(Value); } __declspec(property(get = __property_get_Remote, put = __property_set_Remote)) UnicodeString Remote;
  UnicodeString & __property_get_Node() const { return ::rtl::Mutable(this)->FNode; } void __property_set_Node(UnicodeString Value) { SetNode(Value); } __declspec(property(get = __property_get_Node, put = __property_set_Node)) UnicodeString Node;
  TShortCut & __property_get_ShortCut() const { return ::rtl::Mutable(this)->FShortCut; } void __property_set_ShortCut(TShortCut Value) { SetShortCut(Value); } __declspec(property(get = __property_get_ShortCut, put = __property_set_ShortCut)) TShortCut ShortCut;

protected:
  TBookmarkList * FOwner;

  static UnicodeString __fastcall BookmarkKey(const UnicodeString Node, const UnicodeString Name);
  UnicodeString __property_get_Key() const { return ::rtl::Mutable(this)->GetKey(); } __declspec(property(get = __property_get_Key)) UnicodeString Key;

private:
  UnicodeString FName;
  UnicodeString FLocal;
  UnicodeString FRemote;
  UnicodeString FNode;
  TShortCut FShortCut;

  void __fastcall SetName(const UnicodeString value);
  void __fastcall SetLocal(const UnicodeString value);
  void __fastcall SetRemote(const UnicodeString value);
  void __fastcall SetNode(const UnicodeString value);
  void __fastcall SetShortCut(TShortCut value);
  UnicodeString __fastcall GetKey();
  void __fastcall Modify(int OldIndex);
};
//---------------------------------------------------------------------------
#endif
