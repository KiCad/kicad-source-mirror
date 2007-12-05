/********************/
/* includes systeme */
/********************/
#ifndef FCTSYS_H
#define FCTSYS_H

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <stdio.h>
#ifdef __FreeBSD__
#include <stdlib.h>
#else
#ifndef __DARWIN__
#include <malloc.h>     // MacOSX (DARWIN): malloc() and free() are in stdlib.h
#endif
#endif
#include <time.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#ifdef __WXMAC__
#include <Carbon/Carbon.h>
#endif

#ifndef M_PI
#define M_PI 3.141592653
#endif

#define PCB_INTERNAL_UNIT      10000    //  PCBNEW internal unit = 1/10000 inch
#define EESCHEMA_INTERNAL_UNIT 1000     //  EESCHEMA internal unit = 1/1000 inch

#include "wxstruct.h"
#include "gr_basic.h"

// Old wxWidget compatibility (prior to wxWidget 2.7):
#if ( ( wxMAJOR_VERSION < 2) || ( ( wxMAJOR_VERSION == 2)&& (wxMINOR_VERSION < 7 )  ) )
#define wxFD_OPEN             wxOPEN
#define wxFD_SAVE             wxSAVE
#define wxFD_OVERWRITE_PROMPT wxOVERWRITE_PROMPT
#define WXK_PAGEUP            WXK_PRIOR
#define WXK_PAGEDOWN          WXK_NEXT
#endif

#ifdef __UNIX__
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif

#ifdef __WINDOWS__
#define DIR_SEP        '\\'
#define STRING_DIR_SEP wxT( "\\" )
#else
#define DIR_SEP        '/'
#define STRING_DIR_SEP wxT( "/" )
#endif

#ifdef DEBUG
#define D(x)        x
#else
#define D(x)        // nothing
#endif


#define UNIX_STRING_DIR_SEP wxT( "/" )
#define WIN_STRING_DIR_SEP  wxT( "\\" )

#ifndef TRUE
#define TRUE       ((bool)1)
#define FALSE      ((bool)0)
#endif

#define USE_RESIZE_BORDER
#if defined(__UNIX__) || defined(USE_RESIZE_BORDER)
#define MAYBE_RESIZE_BORDER wxRESIZE_BORDER // linux users like resizeable borders
#else
#define MAYBE_RESIZE_BORDER 0               // no resizeable border
#endif

#endif /* FCTSYS_H */
