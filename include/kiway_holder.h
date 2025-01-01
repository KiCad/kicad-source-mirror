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

#ifndef KIWAY_HOLDER_H_
#define KIWAY_HOLDER_H_

#include <kicommon.h>

class KIWAY;
class PROJECT;
class wxWindow;

/**
 * A mix in class which holds the location of a wxWindow's KIWAY.
 *
 * It allows calls to Kiway() and SetKiway().
 */
class KICOMMON_API KIWAY_HOLDER
{
public:
    enum HOLDER_TYPE { DIALOG, FRAME, PANEL };

    KIWAY_HOLDER( KIWAY* aKiway, HOLDER_TYPE aType ) :
        m_kiway( aKiway ),
        m_type( aType )
    {}

    HOLDER_TYPE GetType() const { return m_type; }

    /**
     * Return a reference to the #KIWAY that this object has an opportunity to participate in.
     *
     * A KIWAY_HOLDER is not necessarily a KIWAY_PLAYER.
     */
    KIWAY& Kiway() const
    {
        wxASSERT( m_kiway );    // smoke out bugs in Debug build, then Release runs fine.
        return *m_kiway;
    }

    /**
     * Safety check before asking for the Kiway reference
     * @return true if kiway is non-null
     */
    bool HasKiway() const
    {
        return m_kiway != nullptr;
    }

    /**
     * Return a reference to the #PROJECT associated with this #KIWAY.
     */
    PROJECT& Prj() const;

    /**
     * It is only used for debugging, since "this" is not a wxWindow*.  "this" is
     * a KIWAY_HOLDER mix-in.
     *
     * @param aDest is the recipient of \a aKiway pointer.
     * @param aKiway is often from a parent window or from #KIFACE::CreateKiWindow().
     */
    void SetKiway( wxWindow* aDest, KIWAY* aKiway );

private:
    // private, all setting is done through SetKiway().
    KIWAY*          m_kiway;            // no ownership.
    HOLDER_TYPE     m_type;
};


#endif // KIWAY_HOLDER_H_
