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

#include <altium_circuit_maker_plugin.h>
#include <altium_pcb.h>

#include <board.h>

#include <compoundfilereader.h>
#include <utf.h>

ALTIUM_CIRCUIT_MAKER_PLUGIN::ALTIUM_CIRCUIT_MAKER_PLUGIN()
{
    m_board = nullptr;
    m_props = nullptr;
}


ALTIUM_CIRCUIT_MAKER_PLUGIN::~ALTIUM_CIRCUIT_MAKER_PLUGIN()
{
}


const wxString ALTIUM_CIRCUIT_MAKER_PLUGIN::PluginName() const
{
    return wxT( "Altium Circuit Maker" );
}


const wxString ALTIUM_CIRCUIT_MAKER_PLUGIN::GetFileExtension() const
{
    return wxT( "CMPcbDoc" );
}


BOARD* ALTIUM_CIRCUIT_MAKER_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,
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
            { ALTIUM_PCB_DIR::ARCS6, "1CEEB63FB33847F8AFC4485F64735E\\" },
            { ALTIUM_PCB_DIR::BOARD6, "96B09F5C6CEE434FBCE0DEB3E88E70\\" },
            { ALTIUM_PCB_DIR::BOARDREGIONS, "E3A544335C30403A991912052C936F\\" },
            { ALTIUM_PCB_DIR::CLASSES6, "4F71DD45B09143988210841EA1C28D\\" },
            { ALTIUM_PCB_DIR::COMPONENTS6, "F9D060ACC7DD4A85BC73CB785BAC81\\" },
            { ALTIUM_PCB_DIR::COMPONENTBODIES6, "44D9487C98CE4F0EB46AB6E9CDAF40\\" }, // or: A0DB41FBCB0D49CE8C32A271AA7EF5 ?
            { ALTIUM_PCB_DIR::DIMENSIONS6, "068B9422DBB241258BA2DE9A6BA1A6\\" },
            { ALTIUM_PCB_DIR::FILLS6, "6FFE038462A940E9B422EFC8F5D85E\\" },
            { ALTIUM_PCB_DIR::MODELS, "0DB009C021D946C88F1B3A32DAE94B\\" },
            { ALTIUM_PCB_DIR::NETS6, "35D7CF51BB9B4875B3A138B32D80DC\\" },
            { ALTIUM_PCB_DIR::PADS6, "4F501041A9BC4A06BDBDAB67D3820E\\" },
            { ALTIUM_PCB_DIR::POLYGONS6, "A1931C8B0B084A61AA45146575FDD3\\" },
            { ALTIUM_PCB_DIR::REGIONS6, "F513A5885418472886D3EF18A09E46\\" },
            { ALTIUM_PCB_DIR::RULES6, "C27718A40C94421388FAE5BD7785D7\\" },
            { ALTIUM_PCB_DIR::SHAPEBASEDREGIONS6,"BDAA2C70289849078C8EBEEC7F0848\\" },
            { ALTIUM_PCB_DIR::TEXTS6, "A34BC67C2A5F408D8F377378C5C5E2\\" },
            { ALTIUM_PCB_DIR::TRACKS6, "412A754DBB864645BF01CD6A80C358\\" },
            { ALTIUM_PCB_DIR::VIAS6, "C87A685A0EFA4A90BEEFD666198B56\\" },
            { ALTIUM_PCB_DIR::WIDESTRINGS6, "C1C6540EA23C48D3BF8F9A4ABB9D6D\\" }
    };
    // clang-format on

    ParseAltiumPcb( m_board, aFileName, aProgressReporter, mapping );

    return m_board;
}
