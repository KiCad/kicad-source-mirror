/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file vrml2_helpers.h
 * helper functions for VRML2 processing
 */

#ifndef VRML2_HELPERS_H
#define VRML2_HELPERS_H


// Function to find a node object given a (non-unique) node name
#define FIND_NODE( aName, aNodeList, aCallingNode ) do { \
    std::vector< aType* >::iterator sLA = aNodeList.begin(); \
    std::vector< aType* >::iterator eLA = aNodeList.end(); \
    WRL2NODE* psg = NULL; \
    while( sLA != eLA ) { \
        if( (WRL2NODE*)*sLA != aCallingNode ) { \
            psg = (WRL2NODE*) (*sLA)->FindNode( aName, this ); \
            if( NULL != psg) \
                return psg; \
        } \
        ++sLA; \
    } } while ( 0 )

#endif  // VRML2_HELPERS_H
