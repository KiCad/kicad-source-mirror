/*
 * Copyright (C) 2018 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ENABLER_H
#define ENABLER_H

#include <wx/window.h>

/**
 * Simple class to automatically enable/disable widgets.
 *
 * As long as an ENABLER object exists, the handled widget will be kept in the requested state.
 */
class ENABLER
{
public:
    /**
     * Constructor.
     *
     * @param aObject is the object to be temporarily enabled or disabled.
     * @param aState is the requested temporary state (true for enabled, false for disabled).
     */
    ENABLER( wxWindow& aObject, bool aState )
        : m_object( aObject ), m_state( aState )
    {
        m_object.Enable( m_state );
    }

    ~ENABLER()
    {
        m_object.Enable( !m_state );
    }

private:
    wxWindow& m_object;
    bool m_state;
};

#endif /* ENABLER_H */
