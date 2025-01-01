/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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



%include board_connected_item.h

%{
#include <board_connected_item.h>
%}

%typemap(out) std::vector<BOARD_CONNECTED_ITEM*> {
    std::vector<BOARD_CONNECTED_ITEM*> list = $1;
    std::vector<BOARD_CONNECTED_ITEM*>::const_iterator iter;

    PyObject * retval = $result = PyList_New(0);

    for( iter=list.begin(); iter!=list.end(); iter++ ) {
        BOARD_CONNECTED_ITEM* aItem = *iter;
        PyObject* obj = 0x0;

        switch( aItem->Type() ) {
        case PCB_PAD_T:
            obj = SWIG_NewPointerObj( SWIG_as_voidptr(aItem),
                                      SWIGTYPE_p_PAD,
                                      0 | 0 );
            break;

        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
            obj = SWIG_NewPointerObj( SWIG_as_voidptr(aItem),
                                      SWIGTYPE_p_PCB_TRACK,
                                      0 | 0 );
            break;

        case PCB_ZONE_T:
            obj = SWIG_NewPointerObj( SWIG_as_voidptr(aItem),
                                      SWIGTYPE_p_ZONE,
                                      0 | 0 );
            break;

        default:
            obj = SWIG_NewPointerObj( SWIG_as_voidptr(aItem),
                                      SWIGTYPE_p_BOARD_CONNECTED_ITEM,
                                      0 | 0 );
            break;
        }

        assert( obj );
        PyList_Append (retval, obj );
        Py_DECREF( obj );
    }
 }

