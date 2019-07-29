https://github.com/Pink2Dev/Pink2/pull/34/conflict?name=src%252Fclientversion.h&ancestor_oid=d8d5927e63c1ea6c57ee9439ba44dd0a3ed9c8db&base_oid=19556a05aed392ee14903f2be82579df35338628&head_oid=cb42bb6e7ec06a70350000dfd955703d7d3da5ee#ifndef CLIENTVERSION_H
#define CLIENTVERSION_H

//
// client versioning
//

// These need to be macros, as version.cpp's and bitcoin-qt.rc's voodoo requires it
#define CLIENT_VERSION_MAJOR       2
#define CLIENT_VERSION_MINOR       3
#define CLIENT_VERSION_REVISION    1
#define CLIENT_VERSION_BUILD       0

// Converts the parameter X to a string after macro replacement on X has been performed.
// Don't merge these into one macro!
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

#endif // CLIENTVERSION_H
