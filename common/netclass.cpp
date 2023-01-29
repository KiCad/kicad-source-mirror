/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@inpg.fr
 * Copyright (C) 2009-2022 KiCad Developers, see change_log.txt for contributors.
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

#include <netclass.h>
#include <macros.h>
#include <base_units.h>

// This will get mapped to "kicad_default" in the specctra_export.
const char NETCLASS::Default[] = "Default";

// Initial values for netclass initialization
const int DEFAULT_CLEARANCE        = pcbIUScale.mmToIU( 0.2 ); // track to track and track to pads clearance
const int DEFAULT_VIA_DIAMETER     = pcbIUScale.mmToIU( 0.6 );
const int DEFAULT_VIA_DRILL        = pcbIUScale.mmToIU( 0.3 );
const int DEFAULT_UVIA_DIAMETER    = pcbIUScale.mmToIU( 0.3 );
const int DEFAULT_UVIA_DRILL       = pcbIUScale.mmToIU( 0.1 );
const int DEFAULT_TRACK_WIDTH      = pcbIUScale.mmToIU( 0.2 );
const int DEFAULT_DIFF_PAIR_WIDTH  = pcbIUScale.mmToIU( 0.2 );
const int DEFAULT_DIFF_PAIR_GAP    = pcbIUScale.mmToIU( 0.25 );
const int DEFAULT_DIFF_PAIR_VIAGAP = pcbIUScale.mmToIU( 0.25 );

const int DEFAULT_WIRE_WIDTH       = schIUScale.MilsToIU( 6 );
const int DEFAULT_BUS_WIDTH        = schIUScale.MilsToIU( 12 );

const int DEFAULT_LINE_STYLE = 0; // solid


NETCLASS::NETCLASS( const wxString& aName ) :
    m_Name( aName ),
    m_PcbColor( KIGFX::COLOR4D::UNSPECIFIED )
{
    // Default settings
    SetClearance( DEFAULT_CLEARANCE );
    SetViaDrill( DEFAULT_VIA_DRILL );
    SetuViaDrill( DEFAULT_UVIA_DRILL );
    // These defaults will be overwritten by SetParams,
    // from the board design parameters, later
    SetTrackWidth( DEFAULT_TRACK_WIDTH );
    SetViaDiameter( DEFAULT_VIA_DIAMETER );
    SetuViaDiameter( DEFAULT_UVIA_DIAMETER );
    SetDiffPairWidth( DEFAULT_DIFF_PAIR_WIDTH );
    SetDiffPairGap( DEFAULT_DIFF_PAIR_GAP );
    SetDiffPairViaGap( DEFAULT_DIFF_PAIR_VIAGAP );

    SetWireWidth( DEFAULT_WIRE_WIDTH );
    SetBusWidth( DEFAULT_BUS_WIDTH );
    SetSchematicColor( COLOR4D::UNSPECIFIED );
    SetLineStyle( DEFAULT_LINE_STYLE );
}


NETCLASS::~NETCLASS()
{
}


