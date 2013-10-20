#ifndef _LOAD_H
#define _LOAD_H

struct LoadRagel;
struct InputData;
struct HostLang;

LoadRagel *newLoadRagel( InputData &id, const HostLang *hostLang );
void loadRagel( LoadRagel *lr, const char *inputFileName );
void deleteLoadRagel( LoadRagel * );

#endif
