/******************************************************************************\
*
*                                 Indent.H
*
\******************************************************************************/

#include <windows.h>
#include "plugin.h"

#define DEBUG

#ifdef DEBUG
#include <stdio.h>
#include <stdlib.h>
FILE *flog=NULL;
#define DBG(i) i
#else
#define DBG(i) 
#endif

int IndentBlock();
int UnIndentBlock();
int GetTabSize(char *text,int tabsize);
int Autoindent();
int GetFirstChar(char *text,int length);
bool BracketOpened(char*s);
char*GetExtension(char*fname);
bool CompatibleExtension(char*ext);
int ProcessCloseBracket();
bool EmptyString(char *text,int length);
int MyStrCpy(char*dst,char*src,int srclen,EditorInfo*ei);
void ExpandTabs(EditorInfo*ei,int line);
