/*-*- mode: c;-*-*/
#if !defined(_syntax_h_)
#define _syntax_h_
/***************************
 **
 ** Project: *current project*
 **
 ** Datatype List:
 **    <routine-list-end>
 **
 **
 **
 ***/
#include "copper.h"

#ifdef SKIP_VERSION
#define COPPER_VERSION "bootstrap"
#else
#include "copper_ver.h"
#endif

extern bool file_ParserInit(Copper input);
extern bool file_WriteTree(Copper input, FILE* output, const char* name);

#endif
