/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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


#ifndef DRC_DRILLED_HOLE_TESTER__H
#define DRC_DRILLED_HOLE_TESTER__H

#include <drc/drc_provider.h>


class BOARD;
class BOARD_ITEM;


class DRC_DRILLED_HOLE_TESTER : public DRC_TEST_PROVIDER
{
public:
    DRC_DRILLED_HOLE_TESTER( MARKER_HANDLER aMarkerHandler );

    virtual ~DRC_DRILLED_HOLE_TESTER() {};

    bool RunDRC( EDA_UNITS aUnits, BOARD& aBoard ) override;

private:
    bool checkPad( D_PAD* aPad );
    bool checkVia( VIA* aVia );
    bool checkMicroVia( VIA* aVia );

    void addHole( const wxPoint& aLocation, int aRadius, BOARD_ITEM* aOwner );
    bool checkHoles();

private:
    struct DRILLED_HOLE
    {
        wxPoint     m_location;
        int         m_drillRadius = 0;
        BOARD_ITEM* m_owner = nullptr;
    };

    EDA_UNITS                 m_units;
    BOARD*                    m_board;
    std::vector<DRILLED_HOLE> m_holes;
    int                       m_largestRadius;

    wxString                  m_msg;    // Construct only once for performance
};

#endif // DRC_DRILLED_HOLE_TESTER__H
