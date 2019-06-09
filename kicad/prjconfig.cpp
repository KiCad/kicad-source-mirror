/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>

#include <build_version.h>
#include <config_params.h>
#include <confirm.h>
#include <kiway.h>
#include <project.h>
#include <wildcards_and_files_ext.h>

#include "dialogs/dialog_template_selector.h"

#include "kicad_manager_frame.h"
#include "pgm_kicad.h"
#include "tree_project_frame.h"
#include "kicad_id.h"


#define SEP()   wxFileName::GetPathSeparator()

// Not really useful, provided to save/restore params in project config file,
// (Add them in s_KicadManagerParams if any)
// Used also to create new .pro files from the kicad.pro template file
// for new projects
#define     GeneralGroupName            wxT( "/general" )

PARAM_CFG_ARRAY     s_KicadManagerParams;


void KICAD_MANAGER_FRAME::LoadProject( const wxFileName& aProjectFileName )
{
    // The project file should be valid by the time we get here or something has gone wrong.
    if( !aProjectFileName.Exists() )
        return;

    // Any open KIFACE's must be closed if they are not part of the new project.
    // (We never want a KIWAY_PLAYER open on a KIWAY that isn't in the same project.)
    // User is prompted here to close those KIWAY_PLAYERs:
    if( !Kiway().PlayersClose( false ) )
        return;

    // Save the project file for the currently loaded project.
    if( m_active_project )
        Prj().ConfigLoad( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );

    m_active_project = true;
    ClearMsg();
    SetProjectFileName( aProjectFileName.GetFullPath() );
    Prj().ConfigLoad( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );

    if( aProjectFileName.IsDirWritable() )
        SetMruPath( Prj().GetProjectPath() ); // Only set MRU path if we have write access. Why?

    UpdateFileHistory( aProjectFileName.GetFullPath(), &PgmTop().GetFileHistory() );

    m_LeftWin->ReCreateTreePrj();

    // Rebuild the list of watched paths.
    // however this is possible only when the main loop event handler is running,
    // so we use it to run the rebuild function.
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED, ID_INIT_WATCHED_PATHS );

    wxPostEvent( this, cmd );

    PrintPrjInfo();
}


void KICAD_MANAGER_FRAME::CreateNewProject( const wxFileName& aProjectFileName )
{
    wxCHECK_RET( aProjectFileName.DirExists() && aProjectFileName.IsDirWritable(),
                 "Project folder must exist and be writable to create a new project." );

    // Init project filename.  This clears all elements from the project object.
    SetProjectFileName( aProjectFileName.GetFullPath() );

    // Copy kicad.pro file from template folder.
    if( !aProjectFileName.FileExists() )
    {
        wxString srcFileName = sys_search().FindValidPath( "kicad.pro" );

        // Create a minimal project (.pro) file if the template project file could not be copied.
        if( !wxFileName::FileExists( srcFileName )
          || !wxCopyFile( srcFileName, aProjectFileName.GetFullPath() ) )
        {
            Prj().ConfigSave( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );
        }
    }

    // Ensure a "stub" for a schematic root sheet and a board exist.
    // It will avoid messages from the schematic editor or the board editor to create a new file
    // And forces the user to create main files under the right name for the project manager
    wxFileName fn( aProjectFileName.GetFullPath() );
    fn.SetExt( SchematicFileExtension );

    // If a <project>.sch file does not exist, create a "stub" file ( minimal schematic file )
    if( !fn.FileExists() )
    {
        wxFile file( fn.GetFullPath(), wxFile::write );

        if( file.IsOpened() )
            file.Write( wxT( "EESchema Schematic File Version 2\n"
                             "EELAYER 25 0\nEELAYER END\n$EndSCHEMATC\n" ) );

        // wxFile dtor will close the file
    }

    // If a <project>.kicad_pcb or <project>.brd file does not exist,
    // create a .kicad_pcb "stub" file
    fn.SetExt( KiCadPcbFileExtension );
    wxFileName leg_fn( fn );
    leg_fn.SetExt( LegacyPcbFileExtension );

    if( !fn.FileExists() && !leg_fn.FileExists() )
    {
        wxFile file( fn.GetFullPath(), wxFile::write );

        if( file.IsOpened() )
            file.Write( wxT( "(kicad_pcb (version 4) (host kicad \"dummy file\") )\n" ) );

        // wxFile dtor will close the file
    }
}
