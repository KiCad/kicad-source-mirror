/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file kicad.i
 * @brief General wrappers for kicad / wx structures and classes
 */

%include <std_vector.i>
%include <std_string.i>
%include <std_map.i>

/* ignore some constructors of EDA_ITEM that will make the build fail */

%nodefaultctor EDA_ITEM;
%ignore EDA_ITEM::EDA_ITEM( EDA_ITEM* parent, KICAD_T idType );
%ignore EDA_ITEM::EDA_ITEM( KICAD_T idType );
%ignore EDA_ITEM::EDA_ITEM( const EDA_ITEM& base );

/* swig tries to wrap SetBack/SetNext on derived classes, but this method is
   private for most childs, so if we don't ignore it won't compile */

%ignore EDA_ITEM::SetBack;
%ignore EDA_ITEM::SetNext;

/* ignore other functions that cause trouble */

%ignore InitKiCadAbout;
%ignore GetCommandOptions;

%rename(getWxRect) operator wxRect;
%ignore operator <<;
%ignore operator=;

/* headers/imports that must be included in the _wrapper.cpp at top */

%{
    #include <cstddef>
    #include <dlist.h>
    #include <base_struct.h>
    #include <common.h>
    #include <wx_python_helpers.h>
    #include <cstddef>
      #include <vector>

    #include <class_title_block.h>
    #include <class_colors_design_settings.h>
    #include <class_marker_base.h>
    #include <eda_text.h>
    #include <convert_from_iu.h>
    #include <convert_to_biu.h>

%}

/* all the wx wrappers for wxString, wxPoint, wxRect, wxChar .. */
%include <wx.i>

/* exception handling */

/* the IO_ERROR exception handler, not working yet... */
/*
%exception
{
  try {
  $function
  }
  catch (IO_ERROR e) {
    PyErr_SetString(PyExc_IOError,"IO error");
    return NULL;
  }
}
*/

/* header files that must be wrapped */

%include <dlist.h>
%include <base_struct.h>
%include <common.h>
%include <class_title_block.h>
%include <class_colors_design_settings.h>
%include <class_marker_base.h>
%include <eda_text.h>
%include <convert_from_iu.h>
%include <convert_to_biu.h>
%include <fpid.h>

/* special iteration wrapper for DLIST objects */
%include "dlist.i"

/* std template mappings */
%template(intVector) std::vector<int>;

/* KiCad plugin handling */
%include "kicadplugins.i"

