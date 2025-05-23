/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <confirm.h>
#include <string_utils.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <footprint.h>
#include <project.h>
#include <wildcards_and_files_ext.h>
#include <tools/board_editor_control.h>
#include <wx/filedlg.h>



bool RecreateCmpFile( BOARD * aBrd, const wxString& aFullCmpFileName )
{
    FILE* cmpFile = wxFopen( aFullCmpFileName, wxT( "wt" ) );

    if( cmpFile == nullptr )
        return false;

    fprintf( cmpFile, "Cmp-Mod V01 Created by PcbNew   date = %s\n", TO_UTF8( GetISO8601CurrentDateTime() ) );

    for( FOOTPRINT* fp : aBrd->Footprints() )
    {
        fprintf( cmpFile, "\nBeginCmp\n" );
        fprintf( cmpFile, "TimeStamp = %s\n", TO_UTF8( fp->m_Uuid.AsString() ) );
        fprintf( cmpFile, "Path = %s\n", TO_UTF8( fp->GetPath().AsString() ) );
        fprintf( cmpFile, "Reference = %s;\n",
                 !fp->GetReference().IsEmpty() ? TO_UTF8( fp->GetReference() ) : "[NoRef]" );
        fprintf( cmpFile, "ValeurCmp = %s;\n",
                 !fp->GetValue().IsEmpty() ? TO_UTF8( fp->GetValue() ) : "[NoVal]" );
        fprintf( cmpFile, "IdModule  = %s;\n", fp->GetFPID().Format().c_str() );
        fprintf( cmpFile, "EndCmp\n" );
    }

    fprintf( cmpFile, "\nEndListe\n" );
    fclose( cmpFile );

    return true;
}


int BOARD_EDITOR_CONTROL::ExportCmpFile( const TOOL_EVENT& aEvent )
{
    // Build the .cmp file name from the board name
    BOARD*     board = m_frame->GetBoard();
    wxString   projectDir = wxPathOnly( m_frame->Prj().GetProjectFullName() );
    wxFileName fn = board->GetFileName();

    fn.SetExt( FILEEXT::FootprintAssignmentFileExtension );

    wxFileDialog dlg( m_frame, _( "Save Footprint Association File" ), projectDir, fn.GetFullName(),
                      FILEEXT::FootprintAssignmentFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    wxString path = dlg.GetPath();

    if( !RecreateCmpFile( board, path ) )
        DisplayError( m_frame, wxString::Format( _( "Failed to create file '%s'." ), path ) );

    return 0;
}

