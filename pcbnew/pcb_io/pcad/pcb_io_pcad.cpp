/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcb_io_pcad.cpp
 * @brief Pcbnew PLUGIN for P-Cad 200x ASCII *.pcb format.
 */

#include <pcad/pcb_io_pcad.h>
#include <pcad/pcad_pcb.h>
#include <pcad/s_expr_loader.h>
#include <io/io_utils.h>

#include <board.h>

#include <cerrno>
#include <wx/string.h>
#include <wx/filename.h>
#include <wx/xml/xml.h>

using namespace PCAD2KICAD;


PCB_IO_PCAD::PCB_IO_PCAD() : PCB_IO( wxS( "P-Cad" ) )
{
}


PCB_IO_PCAD::~PCB_IO_PCAD()
{
}


bool PCB_IO_PCAD::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    return IO_UTILS::fileStartsWithPrefix( aFileName, wxT( "ACCEL_ASCII" ), false );
}


BOARD* PCB_IO_PCAD::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                               const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    wxXmlDocument   xmlDoc;

    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    PCAD_PCB pcb( m_board );

    LoadInputFile( aFileName, &xmlDoc );
    pcb.ParseBoard( nullptr, &xmlDoc, wxT( "PCB" ) );
    pcb.AddToBoard();

    return m_board;
}
