#ifndef _LIBRAGEL_H
#define _LIBRAGEL_H

#ifdef __cplusplus 
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

EXTERN_C 
int libragel( char **result, char **out, char **err, int argc, const char **argv, const char *input );

#endif
