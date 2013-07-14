#ifndef _LOAD_H
#define _LOAD_H

struct LoadRagel;
struct InputData;

LoadRagel *newLoadRagel( InputData &id );
void loadRagel( LoadRagel *lr, const char *inputFileName );
void deleteLoadRagel( LoadRagel * );

#endif
