/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <mutex>

#include <class_library.h>
#include <confirm.h>
#include <dialogs/panel_eeschema_color_settings.h>
#include <dialogs/panel_eeschema_display_options.h>
#include <dialogs/panel_eeschema_editing_options.h>
#include <dialogs/panel_eeschema_template_fieldnames.h>
#include <dialogs/panel_sym_color_settings.h>
#include <dialogs/panel_sym_editing_options.h>
#include <dialogs/dialog_schematic_setup.h>
#include <kiway.h>
#include <symbol_edit_frame.h>
#include <dialogs/panel_gal_display_options.h>
#include <panel_hotkeys_editor.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <schematic.h>
#include <settings/app_settings.h>
#include <settings/settings_manager.h>
#include <symbol_lib_table.h>
#include <widgets/paged_dialog.h>
#include <wildcards_and_files_ext.h>
#include <drawing_sheet/ds_data_model.h>
#include <zoom_defines.h>
#include <wx/treebook.h>


/// Helper for all the old plotting/printing code while it still exists
COLOR4D GetLayerColor( SCH_LAYER_ID aLayer )
{
    return Pgm().GetSettingsManager().GetColorSettings()->GetColor( aLayer );
}


void SCH_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                         PANEL_HOTKEYS_EDITOR* aHotkeysPanel  )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new wxPanel( book ), _( "Schematic Editor" ) );
    book->AddSubPage( new PANEL_EESCHEMA_DISPLAY_OPTIONS( this, book ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_EESCHEMA_EDITING_OPTIONS( this, book ), _( "Editing Options" ) );
    book->AddSubPage( new PANEL_EESCHEMA_COLOR_SETTINGS( this, book ), _( "Colors" ) );
    book->AddSubPage( new PANEL_EESCHEMA_TEMPLATE_FIELDNAMES( this, book, true ),
                      _( "Field Name Templates" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


bool SCH_EDIT_FRAME::LoadProjectSettings()
{
    GetRenderSettings()->SetDefaultPenWidth( m_defaults->m_DefaultLineWidth );
    GetRenderSettings()->m_DefaultWireThickness = m_defaults->m_DefaultWireThickness;
    GetRenderSettings()->m_DefaultBusThickness  = m_defaults->m_DefaultBusThickness;
    GetRenderSettings()->m_TextOffsetRatio      = m_defaults->m_TextOffsetRatio;
    GetRenderSettings()->m_PinSymbolSize        = m_defaults->m_PinSymbolSize;
    GetRenderSettings()->m_JunctionSize         = m_defaults->m_JunctionSize;

    // Verify some values, because the config file can be edited by hand,
    // and have bad values:
    LIB_PART::SetSubpartIdNotation( LIB_PART::GetSubpartIdSeparator(),
                                    LIB_PART::GetSubpartFirstId() );

    // Load the drawing sheet description file, from the filename stored in
    // BASE_SCREEN::m_DrawingSheetFileName, read in config project file
    // If empty, or not existing, the default descr is loaded
    wxString filename = DS_DATA_MODEL::MakeFullFileName( BASE_SCREEN::m_DrawingSheetFileName,
                                                         Prj().GetProjectPath() );

    if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename ) )
    {
        ShowInfoBarError( _( "Error loading drawing sheet" ), true );
    }

    return true;
}


void SCH_EDIT_FRAME::ShowSchematicSetupDialog( const wxString& aInitialPage )
{
    DIALOG_SCHEMATIC_SETUP dlg( this );

    if( !aInitialPage.IsEmpty() )
        dlg.SetInitialPage( aInitialPage, wxEmptyString );

    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        Prj().GetProjectFile().NetSettings().ResolveNetClassAssignments( true );
        SaveProjectSettings();

        Kiway().CommonSettingsChanged( false, true );
        GetCanvas()->Refresh();
    }
}


void SCH_EDIT_FRAME::SaveProjectSettings()
{
    wxFileName fn = Schematic().RootScreen()->GetFileName();  //ConfigFileName

    fn.SetExt( ProjectFileExtension );

    if( !fn.HasName() || !IsWritable( fn ) )
        return;

    RecordERCExclusions();

    GetSettingsManager()->SaveProject( fn.GetFullPath() );
}


void SCH_EDIT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    // For now, axes are forced off in eeschema even if turned on in config
    eeconfig()->m_Window.grid.axes_enabled = false;

    SCH_BASE_FRAME::LoadSettings( eeconfig() );

    GetRenderSettings()->m_ShowPinsElectricalType = false;
    GetRenderSettings()->m_ShowHiddenText = false;
    GetRenderSettings()->m_ShowHiddenPins = false;
    GetRenderSettings()->m_ShowHiddenText = false;
    GetRenderSettings()->SetShowPageLimits( true );
    GetRenderSettings()->m_ShowUmbilicals = true;

    if( eeconfig() )
    {
        GetRenderSettings()->m_ShowHiddenPins = eeconfig()->m_Appearance.show_hidden_pins;
        GetRenderSettings()->m_ShowHiddenText = eeconfig()->m_Appearance.show_hidden_fields;
        GetRenderSettings()->SetShowPageLimits( eeconfig()->m_Appearance.show_page_limits );
    }
}


void SCH_EDIT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    SCH_BASE_FRAME::SaveSettings( eeconfig() );

    // TODO(JE) do we need to keep m_userUnits around?
    if( eeconfig() )
        eeconfig()->m_System.units = static_cast<int>( m_userUnits );
}


void SCH_BASE_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK_RET( aCfg, "Call to SCH_BASE_FRAME::SaveSettings with null settings" );

    EDA_DRAW_FRAME::LoadSettings( aCfg );

    // Currently values read from config file are not used because the user cannot
    // change this config
    // if( aCfg->m_Window.grid.sizes.empty() )  // Will be probably never enabled
    {
        /*
         * Do NOT add others values (mainly grid values in mm), because they can break the
         * schematic: Because wires and pins are considered as connected when the are to the
         * same coordinate we cannot mix coordinates in mils (internal units) and mm (that
         * cannot exactly converted in mils in many cases).  In fact schematic must only use
         * 50 and 25 mils to place labels, wires and components others values are useful only
         * for graphic items (mainly in library editor) so use integer values in mils only.
         * The 100 mil grid is added to help conform to the KiCad Library Convention which
         * states: "Using a 100mil grid, pin ends and origin must lie on grid nodes IEC-60617"
         */
        aCfg->m_Window.grid.sizes = { "100 mil",
                                      "50 mil",
                                      "25 mil",
                                      "10 mil",
                                      "5 mil",
                                      "2 mil",
                                      "1 mil" };
    }

    // Currently values read from config file are not used because the user cannot
    // change this config
    // if( aCfg->m_Window.zoom_factors.empty() )
    {
        aCfg->m_Window.zoom_factors = { ZOOM_LIST_EESCHEMA };
    }
}


void SCH_BASE_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK_RET( aCfg, "Call to SCH_BASE_FRAME::SaveSettings with null settings" );

    EDA_DRAW_FRAME::SaveSettings( aCfg );
}


void SYMBOL_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                            PANEL_HOTKEYS_EDITOR* aHotkeysPanel )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new wxPanel( book ), _( "Symbol Editor" ) );
    book->AddSubPage( new PANEL_GAL_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_SYM_EDITING_OPTIONS( this, book ), _( "Editing Options" ) );
    book->AddSubPage( new PANEL_SYM_COLOR_SETTINGS( this, book ), _( "Colors" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


static std::mutex s_symbolTableMutex;


SYMBOL_LIB_TABLE* PROJECT::SchSymbolLibTable()
{
    std::lock_guard<std::mutex> lock( s_symbolTableMutex );

    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.
    SYMBOL_LIB_TABLE* tbl = (SYMBOL_LIB_TABLE*) GetElem( ELEM_SYMBOL_LIB_TABLE );

    // its gotta be NULL or a SYMBOL_LIB_TABLE, or a bug.
    wxASSERT( !tbl || tbl->Type() == SYMBOL_LIB_TABLE_T );

    if( !tbl )
    {
        // Stack the project specific SYMBOL_LIB_TABLE overlay on top of the global table.
        // ~SYMBOL_LIB_TABLE() will not touch the fallback table, so multiple projects may
        // stack this way, all using the same global fallback table.
        tbl = new SYMBOL_LIB_TABLE( &SYMBOL_LIB_TABLE::GetGlobalLibTable() );

        SetElem( ELEM_SYMBOL_LIB_TABLE, tbl );

        wxString prjPath;

        wxGetEnv( PROJECT_VAR_NAME, &prjPath );

        if( !prjPath.IsEmpty() )
        {
            wxFileName fn( prjPath, SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

            try
            {
                tbl->Load( fn.GetFullPath() );
            }
            catch( const IO_ERROR& ioe )
            {
                wxString msg;
                msg.Printf( _( "An error occurred loading the symbol library table \"%s\"." ),
                            fn.GetFullPath() );
                DisplayErrorMessage( NULL, msg, ioe.What() );
            }
        }
    }

    return tbl;
}
