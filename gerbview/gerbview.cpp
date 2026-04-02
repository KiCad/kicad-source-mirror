/**
 * @file gerbview.cpp
 * @brief GERBVIEW main file.
 */

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

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_settings.h>
#include <gestfich.h>
#include <kiface_base.h>
#include <macros.h>
#include <json_common.h>
#include <pgm_base.h>
#include <richio.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <dialogs/panel_gerbview_display_options.h>
#include <dialogs/panel_gerbview_excellon_settings.h>
#include <dialogs/panel_grid_settings.h>
#include <dialogs/panel_gerbview_color_settings.h>
#include <wildcards_and_files_ext.h>
#include <wx/ffile.h>

#include <dialogs/panel_toolbar_customization.h>
#include <toolbars_gerber.h>

using json = nlohmann::json;


namespace GERBV {

static struct IFACE : public KIFACE_BASE, public UNITS_PROVIDER
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aName, aType ),
            UNITS_PROVIDER( gerbIUScale, EDA_UNITS::MM )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                              int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_GERBER:
            return new GERBVIEW_FRAME( aKiway, aParent );

        case PANEL_GBR_DISPLAY_OPTIONS:
            return new PANEL_GERBVIEW_DISPLAY_OPTIONS( aParent );

        case PANEL_GBR_EXCELLON_OPTIONS:
            return new PANEL_GERBVIEW_EXCELLON_SETTINGS( aParent );

        case PANEL_GBR_GRIDS:
        {
            GERBVIEW_SETTINGS* cfg = GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" );
            EDA_BASE_FRAME*    frame = aKiway->Player( FRAME_GERBER, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_GRID_SETTINGS( aParent, this, frame, cfg, FRAME_GERBER );
        }

        case PANEL_GBR_COLORS:
            return new PANEL_GERBVIEW_COLOR_SETTINGS( aParent );

        case PANEL_GBR_TOOLBARS:
        {
            GERBVIEW_SETTINGS* cfg = GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" );
            TOOLBAR_SETTINGS*  tb  = GetToolbarSettings<GERBVIEW_TOOLBAR_SETTINGS>( "gerbview-toolbars" );

            std::vector<TOOL_ACTION*>            actions;
            std::vector<ACTION_TOOLBAR_CONTROL*> controls;

            for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
                actions.push_back( action );

            for( ACTION_TOOLBAR_CONTROL* control : ACTION_TOOLBAR::GetCustomControlList( FRAME_GERBER ) )
                controls.push_back( control );

            return new PANEL_TOOLBAR_CUSTOMIZATION( aParent, cfg, tb, actions, controls );
        }

        default:
            ;
        }

        return nullptr;
    }

    /**
     * Return a pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an interface,
     * similar to how the KIFACE interface is exported.  But if you know what you are doing use
     * it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     * @return the object requested and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        return nullptr;
    }

    /**
     * Saving a file under a different name is delegated to the various KIFACEs because
     * the project doesn't know the internal format of the various files (which may have
     * paths in them that need updating).
     */
    void SaveFileAs( const wxString& aProjectBasePath, const wxString& aProjectName,
                     const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                     const wxString& aSrcFilePath, wxString& aErrors ) override;

} kiface( "gerbview", KIWAY::FACE_GERBVIEW );

} // namespace


using namespace GERBV;


KIFACE_BASE& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
KIFACE_API KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    return &kiface;
}


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway )
{
    InitSettings( new GERBVIEW_SETTINGS );
    aProgram->GetSettingsManager().RegisterSettings( KifaceSettings() );
    start_common( aCtlBits );
    return true;
}


void IFACE::OnKifaceEnd()
{
    end_common();
}


void IFACE::SaveFileAs( const wxString& aProjectBasePath, const wxString& aProjectName,
                        const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                        const wxString& aSrcFilePath, wxString& aErrors )
{
    wxFileName destFile( aSrcFilePath );
    wxString   destPath = destFile.GetPathWithSep();
    wxUniChar  pathSep = wxFileName::GetPathSeparator();
    wxString   ext = destFile.GetExt();

    if( destPath.StartsWith( aProjectBasePath + pathSep ) )
    {
        destPath.Replace( aProjectBasePath, aNewProjectBasePath, false );
        destFile.SetPath( destPath );
    }

    if( FILEEXT::IsGerberFileExtension( ext ) )
    {
        wxString destFileName = destFile.GetName();

        if( destFileName.StartsWith( aProjectName + "-" ) )
        {
            destFileName.Replace( aProjectName, aNewProjectName, false );
            destFile.SetName( destFileName );
        }

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == FILEEXT::GerberJobFileExtension )
    {
        if( destFile.GetName() == aProjectName + wxT( "-job" ) )
            destFile.SetName( aNewProjectName + wxT( "-job" )  );

         FILE_LINE_READER jobfileReader( aSrcFilePath );

         char*    line;
         wxString data;

         while( ( line = jobfileReader.ReadLine() ) != nullptr )
            data << line << '\n';

        // detect the file format: old (deprecated) gerber format or official JSON format
        if( !data.Contains( wxT( "{" ) ) )
        {
            KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
            return;
        }

        bool success = false;

        try
        {
            // Will throw on parse error
            json js = json::parse( TO_UTF8( data ) );

            for( auto& entry : js["FilesAttributes"] )
            {
                wxString path = wxString( entry["Path"].get<std::string>() );

                if( path.StartsWith( aProjectName + wxT( "-" ) ) )
                {
                    path.Replace( aProjectName, aNewProjectName, false );
                    entry["Path"] = path.ToStdString();
                }
            }

            wxFFile destJobFile( destFile.GetFullPath(), wxT( "wb" ) );

            if( destJobFile.IsOpened() )
                success = destJobFile.Write( js.dump( 0 ) );

            // wxFFile dtor will close the file
        }
        catch( ... )
        {
            success = false;
        }

        if( !success )
        {
            wxString msg;

            if( !aErrors.empty() )
                aErrors += wxT( "\n" );

            msg.Printf( _( "Cannot copy file '%s'." ), destFile.GetFullPath() );
            aErrors += msg;
        }
    }
    else if( ext == FILEEXT::DrillFileExtension )
    {
        wxString destFileName = destFile.GetName();

        if( destFileName == aProjectName )
            destFileName = aNewProjectName;
        else if( destFileName.StartsWith( aProjectName + wxT( "-" ) ) )
            destFileName.Replace( aProjectName, aNewProjectName, false );

        destFile.SetName( destFileName );

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else
    {
        wxFAIL_MSG( wxT( "Unexpected filetype for GerbView::SaveFileAs()" ) );
    }
}

