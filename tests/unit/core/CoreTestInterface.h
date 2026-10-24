// The core's front end interface (core/Interface.h) for the core's L1 tests:
// no user interface, a configuration without persistent storage.
//---------------------------------------------------------------------------
#ifndef CoreTestInterfaceH
#define CoreTestInterfaceH
//---------------------------------------------------------------------------
#include <Configuration.h>
//---------------------------------------------------------------------------
class TTestConfiguration : public TConfiguration
{
public:
  TTestConfiguration();

  virtual UnicodeString TemporaryDir(bool Mask = false);
};
//---------------------------------------------------------------------------
// The messages of the exceptions passed to ShowExtendedException
TStrings * ShownExceptions();
//---------------------------------------------------------------------------
#endif
