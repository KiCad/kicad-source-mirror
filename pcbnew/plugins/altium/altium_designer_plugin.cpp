/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file altium_plugin.cpp
 * @brief Pcbnew PLUGIN for Altium *.PcbDoc format.
 */

#include <iomanip>

#include <wx/string.h>

#include <altium_designer_plugin.h>
#include <altium_pcb.h>

#include <board.h>

#include <compoundfilereader.h>
#include <utf.h>

ALTIUM_DESIGNER_PLUGIN::ALTIUM_DESIGNER_PLUGIN()
{
    m_board = nullptr;
    m_props = nullptr;
}


ALTIUM_DESIGNER_PLUGIN::~ALTIUM_DESIGNER_PLUGIN()
{
}


const wxString ALTIUM_DESIGNER_PLUGIN::PluginName() const
{
    return wxT( "Altium Designer" );
}


const wxString ALTIUM_DESIGNER_PLUGIN::GetFileExtension() const
{
    return wxT( "PcbDoc" );
}


BOARD* ALTIUM_DESIGNER_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,
                                     const PROPERTIES* aProperties, PROJECT* aProject,
                                     PROGRESS_REPORTER* aProgressReporter )
{
    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // clang-format off
    const std::map<ALTIUM_PCB_DIR, std::string> mapping = {
            { ALTIUM_PCB_DIR::FILE_HEADER, "FileHeader" },
            { ALTIUM_PCB_DIR::ARCS6, "Arcs6\\Data" },
            { ALTIUM_PCB_DIR::BOARD6, "Board6\\Data" },
            { ALTIUM_PCB_DIR::BOARDREGIONS, "BoardRegions\\Data" },
            { ALTIUM_PCB_DIR::CLASSES6, "Classes6\\Data" },
            { ALTIUM_PCB_DIR::COMPONENTS6, "Components6\\Data" },
            { ALTIUM_PCB_DIR::COMPONENTBODIES6, "ComponentBodies6\\Data" },
            { ALTIUM_PCB_DIR::DIMENSIONS6, "Dimensions6\\Data" },
            { ALTIUM_PCB_DIR::FILLS6, "Fills6\\Data" },
            { ALTIUM_PCB_DIR::MODELS, "Models\\Data" },
            { ALTIUM_PCB_DIR::NETS6, "Nets6\\Data" },
            { ALTIUM_PCB_DIR::PADS6, "Pads6\\Data" },
            { ALTIUM_PCB_DIR::POLYGONS6, "Polygons6\\Data" },
            { ALTIUM_PCB_DIR::REGIONS6, "Regions6\\Data" },
            { ALTIUM_PCB_DIR::RULES6, "Rules6\\Data" },
            { ALTIUM_PCB_DIR::SHAPEBASEDREGIONS6, "ShapeBasedRegions6\\Data" },
            { ALTIUM_PCB_DIR::TEXTS6, "Texts6\\Data" },
            { ALTIUM_PCB_DIR::TRACKS6, "Tracks6\\Data" },
            { ALTIUM_PCB_DIR::VIAS6, "Vias6\\Data" }
    };
    // clang-format on

    ParseAltiumPcb( m_board, aFileName, mapping );

    return m_board;
}
