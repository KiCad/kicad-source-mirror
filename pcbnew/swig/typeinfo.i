/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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



%include core/typeinfo.h

%{
#include <core/typeinfo.h>
%}

// methods like CONNECTIVITY_DATA::GetNetItems take an array of KICAD_T values,
// terminated by EOT. for example, class_board.cpp calls the method with this argument:
//    const KICAD_T types[] = { PCB_PAD_T, PCB_ZONE_AREA_T, EOT };
// this typemap allows any of the following:
//    conn = board.GetConnectivity()
//    conn.GetNetItems(net.GetNet(), (pcbnew.PCB_PAD_T, pcbnew.PCB_TRACE_T))
//    conn.GetNetItems(net.GetNet(), [pcbnew.PCB_PAD_T, pcbnew.PCB_TRACE_T])
//    conn.GetNetItems(net.GetNet(), pcbnew.PCB_PAD_T)


%typemap(in) KICAD_T [] ( KICAD_T retval[5] ){
    retval[0] = EOT;
    $1 = retval;
  
    int type;
    int ecode = SWIG_AsVal_int( $input, &type );

    if ( SWIG_IsOK( ecode ) ) {
        retval[0] = static_cast<KICAD_T>( type );
        retval[1] = EOT;
    } else if ( PySequence_Check( $input ) ) {
        // compare less than because we still need space for the EOT terminator
        assert( PySequence_Size( $input ) <=
                static_cast<int>( sizeof( retval )/sizeof( KICAD_T ) ) );
        for(int i=0; i<PySequence_Size( $input ); i++) {
            int ecode = SWIG_AsVal_int( PySequence_GetItem( $input, i ), &type );
            if ( !SWIG_IsOK( ecode ) ) {
                SWIG_exception_fail( SWIG_ArgError( ecode ),
                                     "expecting KICAD_T enum values" );        
            }
            retval[i] = static_cast<KICAD_T>( type );
            retval[i+1] = EOT;
        }
    } else {
        SWIG_exception_fail( SWIG_ArgError( ecode ),
                             "expecting KICAD_T enum value" );
    }
 }
