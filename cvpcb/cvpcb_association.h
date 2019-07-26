/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Ian McInerney <Ian.S.McInerney@ieee.org>
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef CVPCB_ASSOCIATION_H
#define CVPCB_ASSOCIATION_H

#include <lib_id.h>
#include <utf8.h>

/**
 * A class to define a footprint association to be made in cvpcb.
 */
class CVPCB_ASSOCIATION
{

public:

    /**
     * Create an association event that contains all the information needed to modify the footprint
     * association of a component in cvpcb.
     *
     * @param aComponentIndex is the index of the component to change
     * @param aNewFootprint is the new footprint to give to the component
     * @param aOldFootprint is the old footprint from the component
     */
    CVPCB_ASSOCIATION(
            unsigned int aComponentIndex, LIB_ID aNewFootprint, LIB_ID aOldFootprint = LIB_ID() ) :
            m_componentIndex( aComponentIndex ),
            m_newFootprint( aNewFootprint ),
            m_oldFootprint( aOldFootprint )
    {}

    CVPCB_ASSOCIATION(
            unsigned int aComponentIndex, wxString aNewFootprint, wxString aOldFootprint = "" ) :
            m_componentIndex( aComponentIndex )
    {
        m_newFootprint.Parse( aNewFootprint, LIB_ID::ID_PCB );
        m_oldFootprint.Parse( aOldFootprint, LIB_ID::ID_PCB );
    }

    /**
     * Reverse the association.
     *
     * @return the reversed association
     */
    CVPCB_ASSOCIATION Reverse() const
    {
        return CVPCB_ASSOCIATION( m_componentIndex, m_oldFootprint, m_newFootprint );
    }

    /**
     * Get the index of the component to modify the association of.
     *
     * @return the index of the component
     */
    unsigned int GetComponentIndex() const
    {
        return m_componentIndex;
    }

    /**
     * Get the new footprint to associate to the component.
     *
     * @return the LIB_ID of the new footprint
     */
    LIB_ID GetNewFootprint() const
    {
        return m_newFootprint;
    }

    /**
     * Get the old footprint of the component
     *
     * @return the LIB_ID of the old footprint
     */
    LIB_ID GetOldFootprint() const
    {
        return m_oldFootprint;
    }

    /**
     * Set the footprint that should be associated with the component
     *
     * @param aNewFootprint is the LIB_ID of the new footprint
     */
    void SetNewFootprint( const LIB_ID& aNewFootprint )
    {
        m_newFootprint = aNewFootprint;
    }

    /**
     * Set the footprint that was associated with the component before this association event
     *
     * @param aOldFootprint is the LIB_ID of the old footprint
     */
    void SetOldFootprint( const LIB_ID& aOldFootprint )
    {
        m_oldFootprint = aOldFootprint;
    }


private:
    unsigned int m_componentIndex;
    LIB_ID       m_newFootprint;
    LIB_ID       m_oldFootprint;

};


#endif
