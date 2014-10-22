/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

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
