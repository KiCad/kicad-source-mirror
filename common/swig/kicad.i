/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

%include <std_deque.i>
%include <std_vector.i>
%include <std_list.i>
%include <std_basic_string.i>
%include <std_string.i>
%include <std_map.i>
%include <std_shared_ptr.i>
%include <std_set.i>
%include <stdint.i>

%include "ki_exception.i"   // affects all that follow it

/*
http://www.swig.org/Doc3.0/CPlusPlus11.html
7.3.3 Hash tables

The new hash tables in the STL are unordered_set, unordered_multiset,
unordered_map, unordered_multimap. These are not available in SWIG, but in
principle should be easily implemented by adapting the current STL containers.

%include <std_unordered_map.i>
*/

// ignore some constructors of EDA_ITEM that will make the build fail

%nodefaultctor      EDA_ITEM;
%ignore EDA_ITEM::EDA_ITEM( EDA_ITEM* parent, KICAD_T idType );
%ignore EDA_ITEM::EDA_ITEM( KICAD_T idType );
%ignore EDA_ITEM::EDA_ITEM( const EDA_ITEM& base );


%warnfilter(401)    EDA_ITEM;
%warnfilter(509)    UTF8;

/* swig tries to wrap SetBack/SetNext on derived classes, but this method is
   private for most childs, so if we don't ignore it won't compile */

%ignore EDA_ITEM::SetBack;
%ignore EDA_ITEM::SetNext;

// ignore other functions that cause trouble

%ignore InitKiCadAbout;
%ignore GetCommandOptions;

%rename(getWxRect) operator wxRect;
%rename(getBOX2I) operator BOX2I;
%ignore operator <<;
%ignore operator=;

// headers/imports that must be included in the _wrapper.cpp at top

%{
    #include <macros.h>
    #include <cstddef>
    #include <base_struct.h>
    #include <eda_rect.h>
    #include <common.h>
    #include <wx_python_helpers.h>
    #include <cstddef>
    #include <vector>
    #include <bitset>

    #include <title_block.h>
    #include <colors_design_settings.h>
    #include <marker_base.h>
    #include <eda_text.h>
    #include <convert_to_biu.h>
    #include <id.h>
%}

// all the wx wrappers for wxString, wxPoint, wxRect, wxChar ..
%include wx.i

// SWIG is incompatible with std::unique_ptr
%ignore GetNewConfig;

// header files that must be wrapped
%include macros.h
%include core/typeinfo.h
%include base_struct.h
%include eda_rect.h
%include common.h
%include title_block.h
%include gal/color4d.h
%include core/settings.h
%include colors_design_settings.h
%include marker_base.h
%include eda_text.h

// Cast time_t to known type for Python
typedef long time_t;

// std template mappings
%template(intVector) std::vector<int>;
%template(str_utf8_Map) std::map< std::string,UTF8 >;

// wrapper of BASE_SEQ (see typedef std::vector<PCB_LAYER_ID> BASE_SEQ;)
%template(base_seqVect) std::vector<enum PCB_LAYER_ID>;

// TODO: wrapper of BASE_SET (see std::bitset<PCB_LAYER_ID_COUNT> BASE_SET;)


// KiCad plugin handling
%include "kicadplugins.i"

#include <geometry/shape.h>
%include <geometry/shape.h>

// Contains VECTOR2I
%include math.i

// ignore warning from nested classes
#pragma SWIG nowarn=325
%ignore SHAPE_LINE_CHAIN::convertFromClipper;
#include <geometry/shape_line_chain.h>
%include <geometry/shape_line_chain.h>

#include <geometry/shape_poly_set.h>
%include <geometry/shape_poly_set.h>

// ignore warning relative to operator = and operator ++:
#pragma SWIG nowarn=362,383

// Rename operators defined in utf8.h
%rename(utf8_to_charptr) operator char* () const;
%rename(utf8_to_wxstring) operator wxString () const;
%rename(utf8_to_string) operator const std::string& () const;

#include <utf8.h>
%include <utf8.h>

%extend UTF8
{
    const char*   Cast_to_CChar()    { return (self->c_str()); }

    %pythoncode
    %{

    # Get the char buffer of the UTF8 string
    def GetChars(self):
        return self.Cast_to_CChar()

    # Convert the UTF8 string to a python string
    # Same as GetChars(), but more easy to use in print command
    def __str__(self):
        return self.GetChars()

    %}
}
