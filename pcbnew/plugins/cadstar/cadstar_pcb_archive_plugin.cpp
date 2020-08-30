/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <@Qbort>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file cadstar_pcb_archive_plugin.cpp
 * @brief Pcbnew PLUGIN for CADSTAR PCB Archive (*.cpa) format: an ASCII format
 *        based on S-expressions.
 */

#include <cadstar_pcb_archive_loader.h>
#include <cadstar_pcb_archive_plugin.h>
#include <class_board.h>


CADSTAR_PCB_ARCHIVE_PLUGIN::CADSTAR_PCB_ARCHIVE_PLUGIN()
{
    m_board = nullptr;
    m_props = nullptr;
}


CADSTAR_PCB_ARCHIVE_PLUGIN::~CADSTAR_PCB_ARCHIVE_PLUGIN()
{
}


const wxString CADSTAR_PCB_ARCHIVE_PLUGIN::PluginName() const
{
    return wxT( "CADSTAR PCB Archive" );
}


const wxString CADSTAR_PCB_ARCHIVE_PLUGIN::GetFileExtension() const
{
    return wxT( "cpa" );
}


BOARD* CADSTAR_PCB_ARCHIVE_PLUGIN::Load(
        const wxString& aFileName, BOARD* aAppendToMe, const PROPERTIES* aProperties )
{
    m_props = aProperties;
    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    CADSTAR_PCB_ARCHIVE_LOADER tempPCB( aFileName );
    tempPCB.Load( m_board );

    return m_board;
}
