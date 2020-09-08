/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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
 * @file cadstar_sch_archive_parser.cpp
 * @brief Reads in a CADSTAR Schematic Archive (*.csa) file
 */

#include <sch_plugins/cadstar/cadstar_sch_archive_parser.h>


void CADSTAR_SCH_ARCHIVE_PARSER::Parse()
{
    XNODE* fileRootNode = LoadArchiveFile( Filename, wxT( "CADSTARSCH" ) );

    XNODE* cNode = fileRootNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "HEADER" ), wxT( "CADSTARSCH" ) );
    /*
    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "HEADER" ) )
        {
            Header.Parse( cNode );

            switch( Header.Resolution )
            {
            case RESOLUTION::HUNDREDTH_MICRON:
                KiCadUnitMultiplier = 10;
                break;

            default:
                wxASSERT_MSG( true, wxT( "Unknown File Resolution" ) );
                break;
            }
        }
        else if( cNode->GetName() == wxT( "ASSIGNMENTS" ) )
            Assignments.Parse( cNode );
        else if( cNode->GetName() == wxT( "LIBRARY" ) )
            Library.Parse( cNode );
        else if( cNode->GetName() == wxT( "DEFAULTS" ) )
        {
            // No design information here (no need to parse)
            // Only contains CADSTAR configuration data such as default shapes, text and units
            // In future some of this could be converted to KiCad but limited value
        }
        else if( cNode->GetName() == wxT( "PARTS" ) )
            Parts.Parse( cNode );
        else if( cNode->GetName() == wxT( "LAYOUT" ) )
            Layout.Parse( cNode );
        else if( cNode->GetName() == wxT( "DISPLAY" ) )
        {
            // No design information here (no need to parse)
            // Contains CADSTAR Display settings such as layer/element colours and visibility.
            // In the future these settings could be converted to KiCad
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "[root]" ) );
        }
    }
    */
    delete fileRootNode;
}

