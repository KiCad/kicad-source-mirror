/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @brief Export GenCAD 1.4 format.
 */

#include <board.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <dialogs/dialog_gencad_export_options.h>
#include <pcb_edit_frame.h>
#include <tools/board_editor_control.h>
#include <project/project_file.h> // LAST_PATH_TYPE

#include <export_gencad_writer.h>


/* Driver function: processing starts here */
int BOARD_EDITOR_CONTROL::ExportGenCAD( const TOOL_EVENT& aEvent )
{
    DIALOG_GENCAD_EXPORT_OPTIONS optionsDialog( m_frame, _( "Export to GenCAD" ), nullptr );

    if( optionsDialog.ShowModal() == wxID_CANCEL )
        return 0;

    wxString path = optionsDialog.GetFileName();

    // Get options
    bool flipBottomPads = optionsDialog.GetOption( FLIP_BOTTOM_PADS );
    bool uniquePinName = optionsDialog.GetOption( UNIQUE_PIN_NAMES );
    bool individualShapes = optionsDialog.GetOption( INDIVIDUAL_SHAPES );
    bool storeOriginCoords = optionsDialog.GetOption( STORE_ORIGIN_COORDS );

    // No idea on *why* this should be needed... maybe to fix net names?
    m_frame->Compile_Ratsnest( true );

    GENCAD_EXPORTER exporter( m_frame->GetBoard() );

    // This is the export origin (the auxiliary axis)
    VECTOR2I GencadOffset;
    VECTOR2I auxOrigin = m_frame->GetBoard()->GetDesignSettings().GetAuxOrigin();
    GencadOffset.x = optionsDialog.GetOption( USE_AUX_ORIGIN ) ? auxOrigin.x : 0;
    GencadOffset.y = optionsDialog.GetOption( USE_AUX_ORIGIN ) ? auxOrigin.y : 0;

    exporter.SetPlotOffet( GencadOffset );
    exporter.FlipBottomPads( flipBottomPads );
    exporter.UsePinNamesUnique( uniquePinName );
    exporter.UseIndividualShapes( individualShapes );
    exporter.StoreOriginCoordsInFile( storeOriginCoords );

    bool success = exporter.WriteFile( path );

    if( !success )
        DisplayError( m_frame, wxString::Format( _( "Failed to create file '%s'." ), path ) );

    return 0;
}

