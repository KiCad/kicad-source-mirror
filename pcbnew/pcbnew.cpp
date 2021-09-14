/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew.cpp
 * @brief Pcbnew main program.
 */

#include <pcbnew_scripting_helpers.h>
#include <pgm_base.h>
#include <kiface_base.h>
#include <kiface_ids.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <eda_dde.h>
#include <macros.h>
#include <wx/snglinst.h>
#include <gestfich.h>
#include <paths.h>
#include <pcbnew_settings.h>
#include <settings/settings_manager.h>
#include <class_draw_panel_gal.h>
#include <fp_lib_table.h>
#include <footprint_edit_frame.h>
#include <footprint_viewer_frame.h>
#include <footprint_wizard_frame.h>
#include <footprint_preview_panel.h>
#include <footprint_info_impl.h>
#include <dialogs/dialog_configure_paths.h>
#include <python_scripting.h>

#include "invoke_pcb_dialog.h"
#include "dialog_global_fp_lib_table_config.h"
#include <wildcards_and_files_ext.h>


/* init functions defined by swig */

extern "C" PyObject* PyInit__pcbnew( void );

namespace PCB {

static struct IFACE : public KIFACE_BASE
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                            int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_PCB_EDITOR:
        {
            auto frame = new PCB_EDIT_FRAME( aKiway, aParent );

            // give the scripting helpers access to our frame
            ScriptingSetPcbEditFrame( frame );

            if( Kiface().IsSingle() )
            {
                // only run this under single_top, not under a project manager.
                frame->CreateServer( KICAD_PCB_PORT_SERVICE_NUMBER );
            }

            return frame;
        }

        case FRAME_FOOTPRINT_EDITOR:
            return new FOOTPRINT_EDIT_FRAME( aKiway, aParent,
                                             EDA_DRAW_PANEL_GAL::GAL_TYPE_UNKNOWN );

        case FRAME_FOOTPRINT_VIEWER:
        case FRAME_FOOTPRINT_VIEWER_MODAL:
            return new FOOTPRINT_VIEWER_FRAME( aKiway, aParent, FRAME_T( aClassId ) );

        case FRAME_FOOTPRINT_WIZARD:
            return new FOOTPRINT_WIZARD_FRAME( aKiway, aParent, FRAME_T( aClassId ) );

        case FRAME_FOOTPRINT_PREVIEW:
            return dynamic_cast< wxWindow* >( FOOTPRINT_PREVIEW_PANEL::New( aKiway, aParent ) );

        case DIALOG_CONFIGUREPATHS:
        {
            DIALOG_CONFIGURE_PATHS dlg( aParent, aKiway->Prj().Get3DFilenameResolver() );

            // The dialog's constructor probably failed to set its Kiway because the
            // dynamic_cast fails when aParent was allocated by a separate compilation
            // module.  So set it directly.
            dlg.SetKiway( &dlg, aKiway );

            // Use QuasiModal so that HTML help window will work
            if( dlg.ShowQuasiModal() == wxID_OK )
                aKiway->CommonSettingsChanged( true, false );

            // Dialog has completed; nothing to return.
            return nullptr;
        }

        case DIALOG_PCB_LIBRARY_TABLE:
            InvokePcbLibTableEditor( aKiway, aParent );
            // Dialog has completed; nothing to return.
            return nullptr;

        default:
            return nullptr;
        }
    }

    /**
     * Return a pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an interface,
     * similar to how the KIFACE interface is exported.  But if you know what you are doing use
     * it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     * @return the object which must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        switch( aDataId )
        {
        // Return a pointer to the global instance of the footprint list.
        case KIFACE_FOOTPRINT_LIST:
            return (void*) &GFootprintList;

        // Return a new FP_LIB_TABLE with the global table installed as a fallback.
        case KIFACE_NEW_FOOTPRINT_TABLE:
            return (void*) new FP_LIB_TABLE( &GFootprintTable );

        // Return a pointer to the global instance of the global footprint table.
        case KIFACE_GLOBAL_FOOTPRINT_TABLE:
            return (void*) &GFootprintTable;

        case KIFACE_SCRIPTING_LEGACY:
            return reinterpret_cast<void*>( PyInit__pcbnew );

        default:
            return nullptr;
        }
    }

    /**
     * Saving a file under a different name is delegated to the various KIFACEs because
     * the project doesn't know the internal format of the various files (which may have
     * paths in them that need updating).
     */
    void SaveFileAs( const wxString& aProjectBasePath, const wxString& aSrcProjectName,
                     const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                     const wxString& aSrcFilePath, wxString& aErrors ) override;

} kiface( "pcbnew", KIWAY::FACE_PCB );

} // namespace


using namespace PCB;


static PGM_BASE* process;


KIFACE_BASE& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
MY_API( KIFACE* ) KIFACE_GETTER( int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    process = aProgram;
    return &kiface;
}


#if defined( BUILD_KIWAY_DLL )
PGM_BASE& Pgm()
{
    wxASSERT( process );    // KIFACE_GETTER has already been called.
    return *process;
}


// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face is run from a python script.
PGM_BASE* PgmOrNull()
{
    return process;
}
#endif


/// The global footprint library table.  This is not dynamically allocated because
/// in a multiple project environment we must keep its address constant (since it is
/// the fallback table for multiple projects).
FP_LIB_TABLE          GFootprintTable;

/// The global footprint info table.  This is performance-intensive to build so we
/// keep a hash-stamped global version.  Any deviation from the request vs. stored
/// hash will result in it being rebuilt.
FOOTPRINT_LIST_IMPL   GFootprintList;


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits )
{
    // This is process-level-initialization, not project-level-initialization of the DSO.
    // Do nothing in here pertinent to a project!
    InitSettings( new PCBNEW_SETTINGS );
    aProgram->GetSettingsManager().RegisterSettings( KifaceSettings() );

    start_common( aCtlBits );

    wxFileName fn = FP_LIB_TABLE::GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        DIALOG_GLOBAL_FP_LIB_TABLE_CONFIG fpDialog( nullptr );

        fpDialog.ShowModal();
    }
    else
    {
        try
        {
            // The global table is not related to a specific project.  All projects
            // will use the same global table.  So the KIFACE::OnKifaceStart() contract
            // of avoiding anything project specific is not violated here.
            if( !FP_LIB_TABLE::LoadGlobalTable( GFootprintTable ) )
                return false;
        }
        catch( const IO_ERROR& ioe )
        {
            // if we are here, a incorrect global footprint library table was found.
            // Incorrect global symbol library table is not a fatal error:
            // the user just has to edit the (partially) loaded table.
            wxString msg = _(
                "An error occurred attempting to load the global footprint library table.\n"
                "Please edit this global footprint library table in Preferences menu."
                );

            DisplayErrorMessage( nullptr, msg, ioe.What() );
        }
    }

    return true;
}


void IFACE::OnKifaceEnd()
{
    end_common();
}


void IFACE::SaveFileAs( const wxString& aProjectBasePath, const wxString& aSrcProjectName,
                        const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                        const wxString& aSrcFilePath, wxString& aErrors )
{
    wxFileName destFile( aSrcFilePath );
    wxString   destPath = destFile.GetPathWithSep();
    wxUniChar  pathSep = wxFileName::GetPathSeparator();
    wxString   ext = destFile.GetExt();

    if( destPath.StartsWith( aProjectBasePath + pathSep ) )
        destPath.Replace( aProjectBasePath, aNewProjectBasePath, false );

    wxString srcProjectFootprintLib = pathSep + aSrcProjectName + ".pretty" + pathSep;
    wxString newProjectFootprintLib = pathSep + aNewProjectName + ".pretty" + pathSep;

    destPath.Replace( srcProjectFootprintLib, newProjectFootprintLib, true );

    destFile.SetPath( destPath );

    if( ext == KiCadPcbFileExtension || ext == KiCadPcbFileExtension + BackupFileSuffix )
    {
        if( destFile.GetName() == aSrcProjectName )
            destFile.SetName( aNewProjectName );

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == LegacyPcbFileExtension )
    {
        if( destFile.GetName() == aSrcProjectName )
            destFile.SetName( aNewProjectName );

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == LegacyFootprintLibPathExtension || ext == KiCadFootprintFileExtension )
    {
        // Footprints are not project-specific.  Keep their source names.
        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == FootprintAssignmentFileExtension )
    {
        // JEY TODO
    }
    else if( ext == "rpt" )
    {
        // DRC must be the "gold standard".  Since we can't guarantee that there aren't
        // any non-deterministic cases in the save-as algorithm, we don't want to certify
        // the result with the source's DRC report.  Therefore copy it under the old
        // name.
        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( destFile.GetName() == "fp-lib-table" )
    {
        try
        {
            FP_LIB_TABLE fpLibTable;
            fpLibTable.Load( aSrcFilePath );

            for( unsigned i = 0; i < fpLibTable.GetCount(); i++ )
            {
                LIB_TABLE_ROW& row = fpLibTable.At( i );
                wxString       uri = row.GetFullURI();

                uri.Replace( "/" + aSrcProjectName + ".pretty", "/" + aNewProjectName + ".pretty" );

                row.SetFullURI( uri );
            }

            fpLibTable.Save( destFile.GetFullPath() );
        }
        catch( ... )
        {
            wxString msg;

            if( !aErrors.empty() )
                aErrors += "\n";

            msg.Printf( _( "Cannot copy file '%s'." ), destFile.GetFullPath() );
            aErrors += msg;
        }
    }
    else
    {
        wxFAIL_MSG( "Unexpected filetype for Pcbnew::SaveFileAs()" );
    }
}

