#ifndef HASHTABLES_H_
#define HASHTABLES_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <base_struct.h>
#include <wx/string.h>

// Three strategies for providing a portable hashtable are given.
// C++, boost, and wx, in that order.  C++ solution is no good for mingw.
// So will soon try boost for all platforms.


#if 0   // C++

#include <unordered_map>

/// Map a C string to a wxString, used in PLUGINs.
typedef unordered_map< const char*, wxString >  PROPERTIES;

/// Map a C string to an integer.  Used in DSNLEXER.
typedef unordered_map< const char*, int >       KEYWORD_MAP;

/// Map a C string to an EDA_RECT.
/// The key is the classname of the derived wxformbuilder dialog.
typedef unordered_map< const char*, EDA_RECT >  RECT_MAP;


#elif 0     // boost::unordered_map

// fix a compile bug at line 97 of boost/detail/container_fwd.hpp
#define BOOST_DETAIL_TEST_FORCE_CONTAINER_FWD

#include <boost/unordered_map.hpp>

// see http://www.boost.org/doc/libs/1_49_0/doc/html/boost/unordered_map.html

/// Map a C string to a wxString, used in PLUGINs.
typedef boost::unordered_map< const char*, wxString >  PROPERTIES;

/// Map a C string to an integer.  Used in DSNLEXER.
typedef boost::unordered_map< const char*, int >       KEYWORD_MAP;

/// Map a C string to an EDA_RECT.
/// The key is the classname of the derived wxformbuilder dialog.
typedef boost::unordered_map< const char*, EDA_RECT >  RECT_MAP;


#elif 1     // wx is inconsistent accross platforms, will soon switch to boost

// http://docs.wxwidgets.org/trunk/classwx_hash_map.html
#include <wx/hashmap.h>

/// Map a C string to a wxString, used in PLUGINs.
WX_DECLARE_HASH_MAP( char*, wxString, wxStringHash, wxStringEqual, PROPERTIES );

/// Map a C string to an integer.  Used in DSNLEXER.
WX_DECLARE_HASH_MAP( char*, int, wxStringHash, wxStringEqual, KEYWORD_MAP );

/// Map a C string to an EDA_RECT.
/// The key is the classname of the derived wxformbuilder dialog.
WX_DECLARE_HASH_MAP( char*, EDA_RECT, wxStringHash, wxStringEqual, RECT_MAP );

#endif

#endif // HASHTABLES_H_
