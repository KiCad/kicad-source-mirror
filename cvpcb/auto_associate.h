/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef AUTO_ASSOCIATE_H
#define AUTO_ASSOCIATE_H

// A helper class to handle info read in .equ files, which gives a footprint LIB_ID
// corresponding to a component value.
// Each line is something like:
// 'FT232BL'		'QFP:LQFP-32_7x7mm_Pitch0.8mm'
//

#include <boost/ptr_container/ptr_vector.hpp>

class FOOTPRINT_EQUIVALENCE
{
public:
    wxString    m_ComponentValue;   // The value of a component
    wxString    m_FootprintFPID;    // the footprint LIB_ID corresponding to this value

    FOOTPRINT_EQUIVALENCE() {}
};

typedef boost::ptr_vector< FOOTPRINT_EQUIVALENCE > FOOTPRINT_EQUIVALENCE_LIST;

#endif      // ifndef AUTO_ASSOCIATE_H
