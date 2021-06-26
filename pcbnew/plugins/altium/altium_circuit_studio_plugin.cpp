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

#include <altium_circuit_studio_plugin.h>
#include <altium_pcb.h>

#include <board.h>

#include <compoundfilereader.h>
#include <utf.h>

ALTIUM_CIRCUIT_STUDIO_PLUGIN::ALTIUM_CIRCUIT_STUDIO_PLUGIN()
{
    m_board = nullptr;
    m_props = nullptr;
}


ALTIUM_CIRCUIT_STUDIO_PLUGIN::~ALTIUM_CIRCUIT_STUDIO_PLUGIN()
{
}


const wxString ALTIUM_CIRCUIT_STUDIO_PLUGIN::PluginName() const
{
    return wxT( "Altium Circuit Studio" );
}


const wxString ALTIUM_CIRCUIT_STUDIO_PLUGIN::GetFileExtension() const
{
    return wxT( "CSPcbDoc" );
}


BOARD* ALTIUM_CIRCUIT_STUDIO_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,
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
            { ALTIUM_PCB_DIR::ARCS6, "00C595EB90524FFC8C3BD9670020A2\\" },
            { ALTIUM_PCB_DIR::BOARD6, "88857D7F1DF64F7BBB61848C965636\\" },
            { ALTIUM_PCB_DIR::BOARDREGIONS, "8957CF30F167408D9D263D23FE7C89\\" },
            { ALTIUM_PCB_DIR::CLASSES6, "847EFBF87A5149B1AA326A52AD6357\\" },
            { ALTIUM_PCB_DIR::COMPONENTS6, "465416896A15486999A39C643935D2\\" },
            { ALTIUM_PCB_DIR::COMPONENTBODIES6, "1849D9B5512D452A93EABF4E40B122\\" }, // or B6AD30D75241498BA2536EBF001752 ?
            { ALTIUM_PCB_DIR::DIMENSIONS6, "16C81DBC13C447FF8B42A426677F3C\\" },
            { ALTIUM_PCB_DIR::FILLS6, "4E83BDC3253747F08E9006D7F57020\\" },
            { ALTIUM_PCB_DIR::MODELS, "C0F7599ECC6A4D648DF5BB557679AF\\" },
            { ALTIUM_PCB_DIR::NETS6, "D95A0DA2FE9047779A5194C127F30B\\" },
            { ALTIUM_PCB_DIR::PADS6, "47D69BC5107A4B8DB8DAA23E39C238\\" },
            { ALTIUM_PCB_DIR::POLYGONS6, "D7038392280E4E229B9D9B5426B295\\" },
            { ALTIUM_PCB_DIR::REGIONS6, "FFDDC21382BB42FE8A7D0C328D272C\\" },
            { ALTIUM_PCB_DIR::RULES6, "48B2FA96DB7546818752B34373D6C6\\" },
            { ALTIUM_PCB_DIR::SHAPEBASEDREGIONS6, "D5F54B536E124FB89E2D51B1121508\\" },
            { ALTIUM_PCB_DIR::TEXTS6, "349ABBB211DB4F5B8AE41B1B49555A\\" },
            { ALTIUM_PCB_DIR::TRACKS6, "530C20C225354B858B2578CAB8C08D\\" },
            { ALTIUM_PCB_DIR::VIAS6, "CA5F5989BCDB404DA70A9D1D3D5758\\" }
    };
    // clang-format on

    ParseAltiumPcb( m_board, aFileName, aProgressReporter, mapping );

    return m_board;
}
