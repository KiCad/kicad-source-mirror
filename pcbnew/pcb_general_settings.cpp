/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012-2017 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_general_settings.h>

PCB_GENERAL_SETTINGS::PCB_GENERAL_SETTINGS( FRAME_T aFrameType )
    : m_colorsSettings( aFrameType )
{
    m_frameType = aFrameType;

    if( m_frameType == FRAME_PCB )
    {
        Add( "LegacyAutoDeleteOldTrack", &m_legacyAutoDeleteOldTrack, true );
        Add( "LegacyUse45DegreeTracks",&m_legacyUse45DegreeTracks, true);
        Add( "LegacyUseTwoSegmentTracks", &m_legacyUseTwoSegmentTracks, true);
        Add( "Use45DegreeGraphicSegments", &m_use45DegreeGraphicSegments, false);
        Add( "MagneticPads", reinterpret_cast<int*>( &m_magneticPads ), CAPTURE_CURSOR_IN_TRACK_TOOL );
        Add( "MagneticTracks", reinterpret_cast<int*>( &m_magneticTracks ), CAPTURE_CURSOR_IN_TRACK_TOOL );
        Add( "EditActionChangesTrackWidth", &m_editActionChangesTrackWidth, false );
    }
}

void PCB_GENERAL_SETTINGS::Load( wxConfigBase* aCfg )
{
    m_colorsSettings.Load( aCfg );
    SETTINGS::Load( aCfg );
}


void PCB_GENERAL_SETTINGS::Save( wxConfigBase* aCfg )
{
    m_colorsSettings.Save( aCfg );
    SETTINGS::Save( aCfg );
}
