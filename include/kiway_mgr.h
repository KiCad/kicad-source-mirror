
#ifndef KIWAY_MGR_H_
#define KIWAY_MGR_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#include <kiway.h>
#include <boost/ptr_container/ptr_vector.hpp>


/**
 * Class KIWAY_MGR
 * is a container for all KIWAYS [and PROJECTS].  This class needs to work both
 * for a C++ project manager and an a wxPython one (after being moved into a
 * header later).
 */
class KIWAY_MGR
{
public:
    //KIWAY_MGR();
    // ~KIWAY_MGR();

    bool OnStart( wxApp* aProcess );

    void OnEnd();

    KIWAY& operator[]( int aIndex )
    {
        wxASSERT( m_kiways.size() );    // stuffed in OnStart()
        return m_kiways[aIndex];
    }

private:

    // KIWAYs may not be moved once doled out, since window DNA depends on the
    // pointer being good forever.
    // boost_ptr::vector however never moves the object pointed to.
    typedef boost::ptr_vector<KIWAY>    KIWAYS;

    KIWAYS  m_kiways;
};

extern KIWAY_MGR Kiways;

#endif  // KIWAY_MGR_H_
