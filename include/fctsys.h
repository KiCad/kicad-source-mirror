/********************/
/* System includes. */
/********************/
#ifndef FCTSYS_H_
#define FCTSYS_H_

#include <wx/wx.h>

/**
 * @note This appears to already be included in the OSX build of wxWidgets.
 *       Will someone with OSX please remove this and see if it compiles?
 */
#ifdef __WXMAC__
#include <Carbon/Carbon.h>
#endif

/**
 * @note Do we really need these defined?
 */
#define UNIX_STRING_DIR_SEP wxT( "/" )
#define WIN_STRING_DIR_SEP  wxT( "\\" )

#ifdef DEBUG
#define DBG(x)        x
#else
#define DBG(x)        // nothing
#endif


// wxNullPtr is not defined prior to wxWidgets 2.9.0.
#if !wxCHECK_VERSION( 2, 9, 0 )
#define wxNullPtr ((void *)NULL)
#endif

#include <config.h>

#endif // FCTSYS_H__
