/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef DRC_INTERACTIVE_COURTYARD_CLEARANCE_H
#define DRC_INTERACTIVE_COURTYARD_CLEARANCE_H

#include <drc/drc_test_provider_clearance_base.h>


class DRC_INTERACTIVE_COURTYARD_CLEARANCE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_INTERACTIVE_COURTYARD_CLEARANCE( const std::shared_ptr<DRC_ENGINE>& aDRCEngine ) :
            DRC_TEST_PROVIDER_CLEARANCE_BASE(),
            m_largestCourtyardClearance( 0 )
    {
        m_isRuleDriven = false;
        SetDRCEngine( aDRCEngine.get() );
    }

    virtual ~DRC_INTERACTIVE_COURTYARD_CLEARANCE ()
    {
    }

    void Init( BOARD* aBoard );

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "interactive_courtyard_clearance" );
    }

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests footprints' courtyard collisions" );
    }

    void UpdateConflicts( KIGFX::VIEW* aView, bool aHighlightMoved );
    void ClearConflicts( KIGFX::VIEW* aView );

public:
    std::vector<FOOTPRINT*>   m_FpInMove;             // The list of moved footprints

private:
    void testCourtyardClearances();

private:
    int m_largestCourtyardClearance;

    std::set<BOARD_ITEM*>     m_itemsInConflict;      // The list of items in conflict
    std::vector<BOARD_ITEM*>  m_lastItemsInConflict;  // The list of items last highlighted
};

#endif // DRC_INTERACTIVE_COURTYARD_CLEARANCE_H
