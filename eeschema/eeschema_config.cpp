/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <class_library.h>
#include <confirm.h>
#include <dialogs/panel_eeschema_color_settings.h>
#include <dialogs/panel_eeschema_display_options.h>
#include <dialogs/panel_eeschema_settings.h>
#include <dialogs/panel_eeschema_template_fieldnames.h>
#include <dialogs/panel_libedit_color_settings.h>
#include <dialogs/panel_libedit_settings.h>
#include <eeschema_config.h>
#include <eeschema_settings.h>
#include <fctsys.h>
#include <gestfich.h>
#include <gr_text.h>
#include <kiface_i.h>
#include <lib_edit_frame.h>
#include <panel_gal_display_options.h>
#include <panel_hotkeys_editor.h>
#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <sch_junction.h>
#include <sch_painter.h>
#include <schematic.h>
#include <settings/app_settings.h>
#include <settings/settings_manager.h>
#include <symbol_lib_table.h>
#include <widgets/paged_dialog.h>
#include <widgets/symbol_tree_pane.h>
#include <wildcards_and_files_ext.h>
#include <ws_data_model.h>
#include <widgets/ui_common.h>
#include <dialogs/dialog_schematic_setup.h>
#include "erc.h"
#include <default_values.h>    // For some default values


#define FieldNameTemplatesKey         wxT( "FieldNameTemplates" )


PARAM_CFG_FIELDNAMES::PARAM_CFG_FIELDNAMES( TEMPLATES * ptparam, const wxChar* group ) :
        PARAM_CFG( wxEmptyString, PARAM_SEVERITIES, group )
{
    m_Pt_param = ptparam;
}

void PARAM_CFG_FIELDNAMES::ReadParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    wxString templateFieldNames = aConfig->Read( FieldNameTemplatesKey, wxEmptyString );

    if( !templateFieldNames.IsEmpty() )
    {
        TEMPLATE_FIELDNAMES_LEXER  lexer( TO_UTF8( templateFieldNames ) );

        try
        {
            m_Pt_param->Parse( &lexer, false );
        }
        catch( const IO_ERROR& DBG( e ) )
        {
            // @todo show error msg
            DBG( printf( "templatefieldnames parsing error: '%s'\n", TO_UTF8( e.What() ) ); )
        }
    }
}

void PARAM_CFG_FIELDNAMES::SaveParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    STRING_FORMATTER sf;
    m_Pt_param->Format( &sf, 0, false );

    wxString record = FROM_UTF8( sf.GetString().c_str() );
    record.Replace( wxT("\n"), wxT(""), true );   // strip all newlines
    record.Replace( wxT("  "), wxT(" "), true );  // double space to single

    aConfig->Write( FieldNameTemplatesKey, record );
}


class PARAM_CFG_SEVERITIES : public PARAM_CFG
{
protected:
    ERC_SETTINGS* m_Pt_param;   ///< Pointer to the parameter value

public:
    PARAM_CFG_SEVERITIES( ERC_SETTINGS* ptparam, const wxChar* group = nullptr ) :
                          PARAM_CFG( wxEmptyString, PARAM_SEVERITIES, group )
    {
        m_Pt_param = ptparam;
    }

    void ReadParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        wxString oldPath = aConfig->GetPath();

        // Read legacy settings first so that modern settings will overwrite them
        bool flag;

        if( aConfig->Read( wxT( "ERC_TestSimilarLabels" ), &flag, true ) )
        {
            if( flag )
                m_Pt_param->m_Severities[ ERCE_SIMILAR_LABELS ] = RPT_SEVERITY_WARNING;
            else
                m_Pt_param->m_Severities[ ERCE_SIMILAR_LABELS ] = RPT_SEVERITY_IGNORE;
        }

        if( aConfig->Read( wxT( "ERC_CheckUniqueGlobalLabels" ), &flag, true ) )
        {
            if( flag )
                m_Pt_param->m_Severities[ ERCE_GLOBLABEL ] = RPT_SEVERITY_WARNING;
            else
                m_Pt_param->m_Severities[ ERCE_GLOBLABEL ] = RPT_SEVERITY_IGNORE;
        }

        if( aConfig->Read( wxT( "ERC_CheckBusDriverConflicts" ), &flag, true ) )
        {
            if( flag )
                m_Pt_param->m_Severities[ ERCE_DRIVER_CONFLICT ] = RPT_SEVERITY_WARNING;
            else
                m_Pt_param->m_Severities[ ERCE_DRIVER_CONFLICT ] = RPT_SEVERITY_IGNORE;
        }

        if( aConfig->Read( wxT( "ERC_CheckBusEntryConflicts" ), &flag, true ) )
        {
            if( flag )
                m_Pt_param->m_Severities[ ERCE_BUS_ENTRY_CONFLICT ] = RPT_SEVERITY_WARNING;
            else
                m_Pt_param->m_Severities[ ERCE_BUS_ENTRY_CONFLICT ] = RPT_SEVERITY_IGNORE;
        }

        if( aConfig->Read( wxT( "ERC_CheckBusToBusConflicts" ), &flag, true ) )
        {
            if( flag )
                m_Pt_param->m_Severities[ ERCE_BUS_TO_BUS_CONFLICT ] = RPT_SEVERITY_ERROR;
            else
                m_Pt_param->m_Severities[ ERCE_BUS_TO_BUS_CONFLICT ] = RPT_SEVERITY_IGNORE;
        }

        if( aConfig->Read( wxT( "ERC_CheckBusToNetConflicts" ), &flag, true ) )
        {
            if( flag )
                m_Pt_param->m_Severities[ ERCE_BUS_TO_NET_CONFLICT ] = RPT_SEVERITY_ERROR;
            else
                m_Pt_param->m_Severities[ ERCE_BUS_TO_NET_CONFLICT ] = RPT_SEVERITY_IGNORE;
        }

        // TO DO: figure out what we're going to use as keys here so we can read/write these....

        aConfig->SetPath( oldPath );
    }

    void SaveParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        wxString oldPath = aConfig->GetPath();

        // TO DO: figure out what we're going to use as keys here so we can read/write these....

        // TO DO: for now just write out the legacy ones so we don't lose them
        // TO DO: remove this once the new scheme is in place
        aConfig->Write( wxT( "ERC_TestSimilarLabels" ),
                        m_Pt_param->IsTestEnabled( ERCE_SIMILAR_LABELS ) );
        aConfig->Write( wxT( "ERC_CheckUniqueGlobalLabels" ),
                        m_Pt_param->IsTestEnabled( ERCE_GLOBLABEL ) );
        aConfig->Write( wxT( "ERC_CheckBusDriverConflicts" ),
                        m_Pt_param->IsTestEnabled( ERCE_DRIVER_CONFLICT ) );
        aConfig->Write( wxT( "ERC_CheckBusEntryConflicts" ),
                        m_Pt_param->IsTestEnabled( ERCE_BUS_ENTRY_CONFLICT ) );
        aConfig->Write( wxT( "ERC_CheckBusToBusConflicts" ),
                        m_Pt_param->IsTestEnabled( ERCE_BUS_TO_BUS_CONFLICT ) );
        aConfig->Write( wxT( "ERC_CheckBusToNetConflicts" ),
                        m_Pt_param->IsTestEnabled( ERCE_BUS_TO_NET_CONFLICT ) );

        aConfig->SetPath( oldPath );
    }
};


/// Helper for all the old plotting/printing code while it still exists
COLOR4D GetLayerColor( SCH_LAYER_ID aLayer )
{
    return Pgm().GetSettingsManager().GetColorSettings()->GetColor( aLayer );
}


// Color to draw items flagged invisible, in libedit (they are invisible in Eeschema)
COLOR4D GetInvisibleItemColor()
{
    return COLOR4D( DARKGRAY );
}


void SCH_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                         PANEL_HOTKEYS_EDITOR* aHotkeysPanel  )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new wxPanel( book ), _( "Eeschema" ) );
    book->AddSubPage( new PANEL_EESCHEMA_DISPLAY_OPTIONS( this, book ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_EESCHEMA_SETTINGS( this, book ), _( "Editing Options" ) );
    book->AddSubPage( new PANEL_EESCHEMA_COLOR_SETTINGS( this, book ), _( "Colors" ) );
    book->AddSubPage( new PANEL_EESCHEMA_TEMPLATE_FIELDNAMES( this, book, true ),
                      _( "Field Name Templates" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


void SCH_EDIT_FRAME::AddFormattingParameters( std::vector<PARAM_CFG*>& params )
{
    EESCHEMA_SETTINGS* appSettings = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    wxCHECK( appSettings, /*void*/ );

    SCHEMATIC_SETTINGS& settings = Schematic().Settings();

    params.push_back( new PARAM_CFG_INT( wxT( "SubpartIdSeparator" ),
                                     LIB_PART::SubpartIdSeparatorPtr(), 0, 0, 126 ) );
    params.push_back( new PARAM_CFG_INT( wxT( "SubpartFirstId" ),
                                     LIB_PART::SubpartFirstIdPtr(), 'A', '1', 'z' ) );

    params.push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "LabSize" ),
                                     &settings.m_DefaultTextSize,
                                     Mils2iu( DEFAULT_SIZE_TEXT ),
                                     Mils2iu( 5 ), Mils2iu( 1000 ), nullptr, 1 / IU_PER_MILS ) );
    params.push_back( new PARAM_CFG_DOUBLE( wxT( "TextOffsetRatio" ),
                                     &settings.m_TextOffsetRatio,
                                     (double) TXT_MARGIN / DEFAULT_SIZE_TEXT,
                                     -200.0, 200.0 ) );
    params.push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "LineThickness" ),
                                     &settings.m_DefaultLineWidth,
                                     Mils2iu( appSettings->m_Drawing.default_line_thickness ),
                                     Mils2iu( 5 ), Mils2iu( 1000 ), nullptr, 1 / IU_PER_MILS ) );

    params.push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "BusThickness" ),
                                     &settings.m_DefaultBusThickness,
                                     Mils2iu( appSettings->m_Drawing.default_bus_thickness ),
                                     Mils2iu( 5 ), Mils2iu( 1000 ), nullptr, 1 / IU_PER_MILS ) );
    params.push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "WireThickness" ),
                                     &settings.m_DefaultWireThickness,
                                     Mils2iu( appSettings->m_Drawing.default_wire_thickness ),
                                     Mils2iu( 5 ), Mils2iu( 1000 ), nullptr, 1 / IU_PER_MILS ) );
    params.push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "PinSymbolSize" ),
                                     &settings.m_PinSymbolSize,
                                     Mils2iu( appSettings->m_Drawing.pin_symbol_size ),
                                     Mils2iu( 5 ), Mils2iu( 1000 ), nullptr, 1 / IU_PER_MILS ) );
    params.push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "JunctionSize" ),
                                     &settings.m_JunctionSize,
                                     Mils2iu( appSettings->m_Drawing.default_junction_size ),
                                     Mils2iu( 5 ), Mils2iu( 1000 ), nullptr, 1 / IU_PER_MILS ) );
}


std::vector<PARAM_CFG*>& SCH_EDIT_FRAME::GetProjectFileParameters()
{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    std::vector<PARAM_CFG*>& params = m_projectFileParams;

    params.push_back( new PARAM_CFG_FILENAME( wxT( "PageLayoutDescrFile" ),
                                              &BASE_SCREEN::m_PageLayoutDescrFileName ) );
    params.push_back( new PARAM_CFG_FILENAME( wxT( "PlotDirectoryName" ), &m_plotDirectoryName ) );
    params.push_back( new PARAM_CFG_WXSTRING( wxT( "NetFmtName" ), &m_netListFormat) );
    params.push_back( new PARAM_CFG_BOOL( wxT( "SpiceAjustPassiveValues" ),
                                          &m_spiceAjustPassiveValues, false ) );

    AddFormattingParameters( params );

    params.push_back( new PARAM_CFG_FIELDNAMES( &m_templateFieldNames ) );

    params.push_back( new PARAM_CFG_SEVERITIES( Schematic().ErcSettings() ) );

    return params;
}


std::vector<PARAM_CFG*> ERC_SETTINGS::GetProjectFileParameters()
{
    std::vector<PARAM_CFG*> params;

    params.push_back( new PARAM_CFG_SEVERITIES( this ) );

    return params;
}


bool SCH_EDIT_FRAME::LoadProjectFile()
{
    bool ret = Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_SCH_EDIT,
                                 GetProjectFileParameters() );

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

    // Load the page layout decr file, from the filename stored in
    // BASE_SCREEN::m_PageLayoutDescrFileName, read in config project file
    // If empty, or not existing, the default descr is loaded
    WS_DATA_MODEL& pglayout = WS_DATA_MODEL::GetTheInstance();
    wxString filename = WS_DATA_MODEL::MakeFullFileName( BASE_SCREEN::m_PageLayoutDescrFileName,
                                                         Prj().GetProjectPath() );

    pglayout.SetPageLayout( filename );

    return ret;
}


void SCH_EDIT_FRAME::ShowSchematicSetupDialog( const wxString& aInitialPage )
{
    DIALOG_SCHEMATIC_SETUP dlg( this );

    if( !aInitialPage.IsEmpty() )
        dlg.SetInitialPage( aInitialPage, wxEmptyString );

    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        SaveProjectSettings();

        GetCanvas()->Refresh();
        OnModify();
    }
}


void SCH_EDIT_FRAME::SaveProjectSettings()
{
    PROJECT&        prj = Prj();
    wxFileName      fn = Schematic().RootScreen()->GetFileName();  //ConfigFileName

    fn.SetExt( ProjectFileExtension );

    if( !fn.HasName() || !IsWritable( fn ) )
        return;

    wxString path = fn.GetFullPath();

    prj.ConfigSave( Kiface().KifaceSearch(), GROUP_SCH_EDIT, GetProjectFileParameters(), path );
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

    if( aCfg->m_Window.grid.sizes.empty() )
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

    if( aCfg->m_Window.zoom_factors.empty() )
    {
        aCfg->m_Window.zoom_factors = { 0.5,
                                        0.7,
                                        1.0,
                                        1.5,
                                        2.0,
                                        3.0,
                                        4.5,
                                        6.5,
                                        10.0,
                                        15.0,
                                        20.0,
                                        30.0,
                                        45.0,
                                        65.0,
                                        100.0,
                                        150.0 };
    }

    for( double& factor : aCfg->m_Window.zoom_factors )
        factor = std::min( factor, MAX_ZOOM_FACTOR );

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg );

    if( cfg )
    {
        wxString templateFieldNames = cfg->m_Drawing.field_names;

        if( !templateFieldNames.IsEmpty() )
        {
            TEMPLATE_FIELDNAMES_LEXER  lexer( TO_UTF8( templateFieldNames ) );

            try
            {
                m_templateFieldNames.Parse( &lexer, true );
            }
            catch( const IO_ERROR& DBG( e ) )
            {
                // @todo show error msg
                DBG( printf( "templatefieldnames parsing error: '%s'\n", TO_UTF8( e.What() ) ); )
            }
        }
    }
}


void SCH_BASE_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK_RET( aCfg, "Call to SCH_BASE_FRAME::SaveSettings with null settings" );

    EDA_DRAW_FRAME::SaveSettings( aCfg );

    if( eeconfig() )
    {
        // Save template fieldnames
        STRING_FORMATTER sf;
        m_templateFieldNames.Format( &sf, 0, true );

        wxString record = FROM_UTF8( sf.GetString().c_str() );
        record.Replace( wxT("\n"), wxT(""), true );   // strip all newlines
        record.Replace( wxT("  "), wxT(" "), true );  // double space to single

        eeconfig()->m_Drawing.field_names = record.ToStdString();
    }
}


void LIB_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                         PANEL_HOTKEYS_EDITOR* aHotkeysPanel )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new wxPanel( book ), _( "Symbol Editor" ) );
    book->AddSubPage( new PANEL_GAL_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_LIBEDIT_SETTINGS( this, book ), _( "Editing Options" ) );
    book->AddSubPage( new PANEL_LIBEDIT_COLOR_SETTINGS( this, book ), _( "Colors" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


SYMBOL_LIB_TABLE* PROJECT::SchSymbolLibTable()
{
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
