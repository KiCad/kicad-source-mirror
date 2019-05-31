/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <confirm.h>
#include <kicad_string.h>
#include <pcb_edit_frame.h>
#include <class_board.h>
#include <class_module.h>
#include <project.h>
#include <wildcards_and_files_ext.h>


bool RecreateCmpFile( BOARD * aBrd, const wxString& aFullCmpFileName )
{
    FILE* cmpFile = wxFopen( aFullCmpFileName, wxT( "wt" ) );

    if( cmpFile == NULL )
        return false;

    fprintf( cmpFile, "Cmp-Mod V01 Created by PcbNew   date = %s\n", TO_UTF8( DateAndTime() ) );

    for( auto module : aBrd->Modules() )
    {
        fprintf( cmpFile, "\nBeginCmp\n" );
        fprintf( cmpFile, "TimeStamp = %8.8lX\n", (unsigned long)module->GetTimeStamp() );
        fprintf( cmpFile, "Path = %s\n", TO_UTF8( module->GetPath() ) );
        fprintf( cmpFile, "Reference = %s;\n",
                 !module->GetReference().IsEmpty() ? TO_UTF8( module->GetReference() ) : "[NoRef]" );
        fprintf( cmpFile, "ValeurCmp = %s;\n",
                 !module->GetValue().IsEmpty() ? TO_UTF8( module->GetValue() ) : "[NoVal]" );
        fprintf( cmpFile, "IdModule  = %s;\n", module->GetFPID().Format().c_str() );
        fprintf( cmpFile, "EndCmp\n" );
    }

    fprintf( cmpFile, "\nEndListe\n" );
    fclose( cmpFile );

    return true;
}


void PCB_EDIT_FRAME::RecreateCmpFileFromBoard( wxCommandEvent& aEvent )
{
    // Build the .cmp file name from the board name
    wxString   projectDir = wxPathOnly( Prj().GetProjectFullName() );
    wxFileName fn = GetBoard()->GetFileName();

    fn.SetExt( ComponentFileExtension );

    wxFileDialog dlg( this, _( "Save Footprint Association File" ),
                      projectDir, fn.GetFullName(), ComponentFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString path = dlg.GetPath();

    if( !RecreateCmpFile( GetBoard(), path ) )
        DisplayError( this, wxString::Format( _( "Could not create file \"%s\"." ), path ) );
}

