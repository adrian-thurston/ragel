#ifndef _LOAD_H
#define _LOAD_H

struct LoadRagel;

LoadRagel *newLoadRagel();
void loadRagel( LoadRagel *lr, const char *inputFileName );
void deleteLoadRagel( LoadRagel * );

#endif
