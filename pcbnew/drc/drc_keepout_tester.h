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


#ifndef DRC_KEEPOUT_TESTER__H
#define DRC_KEEPOUT_TESTER__H

#include <drc/drc_provider.h>


class BOARD;


class DRC_KEEPOUT_TESTER : public DRC_TEST_PROVIDER
{
public:
    DRC_KEEPOUT_TESTER( MARKER_HANDLER aMarkerHandler );

    virtual ~DRC_KEEPOUT_TESTER() {};

    bool RunDRC( EDA_UNITS aUnits, BOARD& aBoard ) override;

private:
    bool checkTracksAndVias();
    bool checkFootprints();
    bool checkPads( MODULE* aModule );
    bool checkDrawings();

private:
    EDA_UNITS                 m_units;
    BOARD*                    m_board;

    // Temp variables for use while testing:
    ZONE_CONTAINER*           m_zone;
    EDA_RECT                  m_zoneBBox;
    int                       m_keepoutFlags;   // bitset of DISALLOW_* flags
    std::map<int, wxString>   m_sources;        // map of DISALLOW_* flag to source
    wxString                  m_msg;            // avoid lots of calls to wxString's c'tor.
};

#endif // DRC_KEEPOUT_TESTER__H
