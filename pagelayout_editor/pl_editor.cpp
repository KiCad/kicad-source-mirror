/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <wx/file.h>
#include <wx/snglinst.h>

#include <kiface_base.h>
#include <confirm.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <dialogs/panel_pl_editor_display_options.h>
#include <dialogs/panel_pl_editor_color_settings.h>
#include <dialogs/panel_grid_settings.h>

#include <dialogs/panel_toolbar_customization.h>
#include <toolbars_pl_editor.h>

#include "pl_editor_frame.h"
#include "pl_editor_settings.h"


namespace PGE {

static struct IFACE : public KIFACE_BASE, public UNITS_PROVIDER
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aName, aType ),
            UNITS_PROVIDER( drawSheetIUScale, EDA_UNITS::MM )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                              int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_PL_EDITOR:
            return new PL_EDITOR_FRAME( aKiway, aParent );

        case PANEL_DS_DISPLAY_OPTIONS:
            return new PANEL_PL_EDITOR_DISPLAY_OPTIONS( aParent, GetAppSettings<PL_EDITOR_SETTINGS>( "pl_editor" ) );

        case PANEL_DS_GRIDS:
        {
            APP_SETTINGS_BASE* cfg = GetAppSettings<PL_EDITOR_SETTINGS>( "pl_editor" );
            EDA_BASE_FRAME*    frame = aKiway->Player( FRAME_PL_EDITOR, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_GRID_SETTINGS( aParent, this, frame, cfg, FRAME_PL_EDITOR );
        }

        case PANEL_DS_COLORS:
            return new PANEL_PL_EDITOR_COLOR_SETTINGS( aParent );

        case PANEL_DS_TOOLBARS:
        {
            APP_SETTINGS_BASE* cfg = GetAppSettings<PL_EDITOR_SETTINGS>( "pl_editor" );
            TOOLBAR_SETTINGS*  tb  = GetToolbarSettings<PL_EDITOR_TOOLBAR_SETTINGS>( "pl_editor-toolbars" );

            std::vector<TOOL_ACTION*>            actions;
            std::vector<ACTION_TOOLBAR_CONTROL*> controls;

            for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
                actions.push_back( action );

            for( ACTION_TOOLBAR_CONTROL* control : ACTION_TOOLBAR::GetCustomControlList( FRAME_PL_EDITOR ) )
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
     * similar to how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.
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
    void SaveFileAs( const wxString& aProjectBasePath, const wxString& aSrcProjectName,
                     const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                     const wxString& aSrcFilePath, wxString& aErrors ) override;

} kiface( "pl_editor", KIWAY::FACE_PL_EDITOR );

} // namespace


using namespace PGE;


KIFACE_BASE& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
KIFACE_API KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    return &kiface;
}

bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway )
{
    InitSettings( new PL_EDITOR_SETTINGS );
    aProgram->GetSettingsManager().RegisterSettings( KifaceSettings() );
    start_common( aCtlBits );
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
    {
        destPath.Replace( aProjectBasePath, aNewProjectBasePath, false );
        destFile.SetPath( destPath );
    }

    if( ext == "kicad_wks" )
    {
        if( destFile.GetName() == aSrcProjectName )
            destFile.SetName( aNewProjectName );

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else
    {
        wxFAIL_MSG( "Unexpected filetype for Pcbnew::SaveFileAs()" );
    }
}

