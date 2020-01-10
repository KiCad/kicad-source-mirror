/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include <altium_plugin.h>
#include <altium_pcb.h>

#include <class_board.h>

#include <compoundfilereader.h>
#include <utf.h>

ALTIUM_PLUGIN::ALTIUM_PLUGIN()
{
    m_board = nullptr;
    m_props = nullptr;
}


ALTIUM_PLUGIN::~ALTIUM_PLUGIN()
{
}


const wxString ALTIUM_PLUGIN::PluginName() const
{
    return wxT( "Altium" );
}


const wxString ALTIUM_PLUGIN::GetFileExtension() const
{
    return wxT( "PcbDoc" );
}


BOARD* ALTIUM_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe, const PROPERTIES* aProperties )
{
    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // Open file
    FILE* fp = fopen(aFileName, "rb");
    if (fp == nullptr)
    {
        std::cerr << "read file error" << std::endl;
        return m_board;
    }

    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    std::unique_ptr<unsigned char> buffer(new unsigned char[len]);
    fseek(fp, 0, SEEK_SET);

    len = fread(buffer.get(), 1, len, fp);
    CFB::CompoundFileReader reader(buffer.get(), len);

    // Parse File
    ALTIUM_PCB pcb(m_board);
    pcb.Parse(reader);

    // Close File
    fclose(fp);

    return m_board;
}
