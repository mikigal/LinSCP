// Operations on the local files of the GUI that upstream leaves to the
// Windows shell: the properties of local files (upstream shows the shell's
// properties dialog), their size and their permissions, the links (upstream
// creates Windows shortcuts).
//---------------------------------------------------------------------------
#ifndef LocalFilesH
#define LocalFilesH
//---------------------------------------------------------------------------
#include <QtInterface.h>
//---------------------------------------------------------------------------
class TRemoteProperties;
struct TCalculateSizeStats;
//---------------------------------------------------------------------------
// The local files (full paths) with TRemoteFile objects describing them
// (owned by the list), for the properties dialog
TStrings * CreateLocalFileList(TStrings * FileNames);
// The size of the files, recursively (symbolic links to directories are
// not followed, as by the core)
void CalculateLocalFilesSize(TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats);
// Changes the permissions of the files (the owner and the group are not
// changed); recursively as chmod -R does, symbolic links found in the
// directories are skipped
void ChangeLocalFilesProperties(TStrings * FileList, const TRemoteProperties & Properties);
// Creates a symbolic link to PointTo (as given, relative to the link) or a
// hard link to it (relative to the directory of the link); an existing
// link of the name is replaced when Replace
void CreateLocalLink(const UnicodeString & FileName, const UnicodeString & PointTo, bool Symbolic, bool Replace);
//---------------------------------------------------------------------------
#endif
