/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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



%include core/typeinfo.h

%{
#include <core/typeinfo.h>
%}

// Methods like CONNECTIVITY_DATA::GetNetItems take an std::vector<KICAD_T>
// This typemap allows any of the following:
//    conn = board.GetConnectivity()
//    conn.GetNetItems(net.GetNet(), (pcbnew.PCB_PAD_T, pcbnew.PCB_TRACE_T))
//    conn.GetNetItems(net.GetNet(), [pcbnew.PCB_PAD_T, pcbnew.PCB_TRACE_T])
//    conn.GetNetItems(net.GetNet(), pcbnew.PCB_PAD_T)

%typemap(in) std::vector< KICAD_T,std::allocator< KICAD_T > > const & ( std::vector<KICAD_T> vec ) {
    $1 = &vec;

    // Try with a single element
    int value;
    int ecode = SWIG_AsVal_int( $input, &value );

    if ( SWIG_IsOK( ecode ) ) {
        vec.push_back( static_cast<KICAD_T>( value ) );
    } else if ( PySequence_Check( $input ) ) {  // Now try with a sequence
        int elements = PySequence_Size( $input );
        for(int i=0; i<elements; i++) {
            int ecode = SWIG_AsVal_int( PySequence_GetItem( $input, i ), &value );
            if ( !SWIG_IsOK( ecode ) ) {
                SWIG_exception_fail( SWIG_ArgError( ecode ),
                                     "expecting KICAD_T enum values" );
            }
            vec.push_back( static_cast<KICAD_T>( value ) );
        }
    } else {
        SWIG_exception_fail( SWIG_ArgError( ecode ),
                             "expecting KICAD_T enum value" );
    }
 }
