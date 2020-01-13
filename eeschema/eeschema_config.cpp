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
#include <dialogs/panel_eeschema_display_options.h>
#include <dialogs/panel_eeschema_settings.h>
#include <dialogs/panel_eeschema_template_fieldnames.h>
#include <dialogs/panel_libedit_settings.h>
#include <eeschema_config.h>
#include <eeschema_settings.h>
#include <fctsys.h>
#include <gestfich.h>
#include <gr_text.h>
#include <kiface_i.h>
#include <lib_edit_frame.h>
#include <panel_display_options.h>
#include <panel_hotkeys_editor.h>
#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <sch_junction.h>
#include <sch_painter.h>
#include <sch_sheet.h>
#include <settings/app_settings.h>
#include <symbol_lib_table.h>
#include <widgets/paged_dialog.h>
#include <widgets/symbol_tree_pane.h>
#include <widgets/widget_eeschema_color_config.h>
#include <wildcards_and_files_ext.h>
#include <ws_data_model.h>

static int s_defaultBusThickness = Mils2iu( DEFAULTBUSTHICKNESS );
static int s_defaultWireThickness  = Mils2iu( DEFAULTDRAWLINETHICKNESS );
static int s_defaultTextSize = Mils2iu( DEFAULT_SIZE_TEXT );
static int s_drawDefaultLineThickness = -1;
static bool s_selectTextAsBox = false;
static bool s_selectDrawChildren = true;
static bool s_selectFillShapes = false;
static int  s_selectThickness = Mils2iu( DEFAULTSELECTIONTHICKNESS );

int GetDefaultBusThickness()
{
    return s_defaultBusThickness;
}


void SetDefaultBusThickness( int aThickness )
{
    s_defaultBusThickness = std::max( 1, aThickness );
}


int GetDefaultWireThickness()
{
    return s_defaultWireThickness;
}


void SetDefaultWireThickness( int aThickness )
{
    s_defaultWireThickness = std::max( 1, aThickness );
}


int GetDefaultTextSize()
{
    return s_defaultTextSize;
}


void SetDefaultTextSize( int aTextSize )
{
    s_defaultTextSize = aTextSize;
}


int GetDefaultLineThickness()
{
    return s_drawDefaultLineThickness;
}


void SetDefaultLineThickness( int aThickness )
{
    s_drawDefaultLineThickness = std::max( 1, aThickness );
}


bool GetSelectionTextAsBox()
{
    return s_selectTextAsBox;
}


void SetSelectionTextAsBox( bool aBool )
{
    s_selectTextAsBox = aBool;
}


bool GetSelectionDrawChildItems()
{
    return s_selectDrawChildren;
}


void SetSelectionDrawChildItems( bool aBool )
{
    s_selectDrawChildren = aBool;
}


bool GetSelectionFillShapes()
{
    return s_selectFillShapes;
}


void SetSelectionFillShapes( bool aBool )
{
    s_selectFillShapes = aBool;
}


int GetSelectionThickness()
{
    return s_selectThickness;
}


void SetSelectionThickness( int aThickness )
{
    s_selectThickness = aThickness;
}

// Color to draw selected items
COLOR4D GetItemSelectedColor()
{
    return COLOR4D( BROWN );
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
    book->AddSubPage( new PANEL_EESCHEMA_COLOR_CONFIG( this, book ), _( "Colors" ) );
    book->AddSubPage( new PANEL_EESCHEMA_TEMPLATE_FIELDNAMES( this, book ),
                      _( "Field Name Templates" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


std::vector<PARAM_CFG*>& SCH_EDIT_FRAME::GetProjectFileParameters()

{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    std::vector<PARAM_CFG*>& params = m_projectFileParams;

    params.push_back( new PARAM_CFG_FILENAME( wxT( "PageLayoutDescrFile" ),
                                              &BASE_SCREEN::m_PageLayoutDescrFileName ) );

    params.push_back( new PARAM_CFG_FILENAME( wxT( "PlotDirectoryName" ),
                                              &m_plotDirectoryName ) );

    params.push_back( new PARAM_CFG_INT( wxT( "SubpartIdSeparator" ),
                                         LIB_PART::SubpartIdSeparatorPtr(), 0, 0, 126 ) );
    params.push_back( new PARAM_CFG_INT( wxT( "SubpartFirstId" ),
                                         LIB_PART::SubpartFirstIdPtr(), 'A', '1', 'z' ) );

    params.push_back( new PARAM_CFG_WXSTRING( wxT( "NetFmtName" ),
                                              &m_netListFormat) );
    params.push_back( new PARAM_CFG_BOOL( wxT( "SpiceAjustPassiveValues" ),
                                          &m_spiceAjustPassiveValues, false ) );

    params.push_back( new PARAM_CFG_INT( wxT( "LabSize" ),
                                         &s_defaultTextSize, DEFAULT_SIZE_TEXT, 5, 1000 ) );

    params.push_back( new PARAM_CFG_BOOL( wxT( "ERC_WriteFile" ),
                                          &m_ercSettings.write_erc_file, false ) );

    params.push_back( new PARAM_CFG_BOOL( wxT( "ERC_TestSimilarLabels" ),
                                          &m_ercSettings.check_similar_labels, true ) );

    params.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckUniqueGlobalLabels" ),
                                          &m_ercSettings.check_unique_global_labels, true ) );

    params.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckBusDriverConflicts" ),
                                          &m_ercSettings.check_bus_driver_conflicts, true ) );

    params.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckBusEntryConflicts" ),
                                          &m_ercSettings.check_bus_entry_conflicts, true ) );

    params.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckBusToBusConflicts" ),
                                          &m_ercSettings.check_bus_to_bus_conflicts, true ) );

    params.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckBusToNetConflicts" ),
                                          &m_ercSettings.check_bus_to_net_conflicts, true ) );

    return params;
}


bool SCH_EDIT_FRAME::LoadProjectFile()
{
    // Read library list and library path list
    bool ret = Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_SCH, GetProjectFileParameters() );

    // Read schematic editor setup
    ret &= Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_SCH_EDIT, GetProjectFileParameters() );

    // Convert default text size to internal units.
    SetDefaultTextSize( Mils2iu( GetDefaultTextSize() ) );

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


void SCH_EDIT_FRAME::SaveProjectSettings( bool aAskForSave )
{
    PROJECT&        prj = Prj();
    wxFileName      fn = g_RootSheet->GetScreen()->GetFileName();  //ConfigFileName

    fn.SetExt( ProjectFileExtension );

    if( !IsWritable( fn ) )
        return;

    if( aAskForSave )
    {
        wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(), fn.GetFullName(),
                          ProjectFileWildcard(), wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();
    }

    wxString path = fn.GetFullPath();

    // Convert default text size from internal units temporarily.
    SetDefaultTextSize( Iu2Mils( GetDefaultTextSize() ) );

    prj.ConfigSave( Kiface().KifaceSearch(), GROUP_SCH_EDIT, GetProjectFileParameters(), path );

    SetDefaultTextSize( Mils2iu( GetDefaultTextSize() ) );
}


void SCH_EDIT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg );

    m_repeatStep.x = Mils2iu( cfg->m_Drawing.default_repeat_offset_x );
    m_repeatStep.y = Mils2iu( cfg->m_Drawing.default_repeat_offset_y );

    // LibEdit owns this one, but we must read it in if LibEdit hasn't set it yet
    if( GetDefaultLineThickness() < 0 )
        SetDefaultLineThickness( Mils2iu( cfg->m_Drawing.default_line_thickness ) );

    SetDefaultBusThickness( Mils2iu( cfg->m_Drawing.default_bus_thickness ) );

    // Property introduced in 6.0; use DefaultLineWidth for earlier projects
    SetDefaultWireThickness( Mils2iu( cfg->m_Drawing.default_wire_thickness) );

    SetSelectionTextAsBox( cfg->m_Selection.text_as_box );
    SetSelectionDrawChildItems( cfg->m_Selection.draw_selected_children );
    SetSelectionFillShapes( cfg->m_Selection.fill_shapes );
    SetSelectionThickness( Mils2iu( cfg->m_Selection.thickness ) );

    SetTextMarkupFlags( cfg->m_Drawing.text_markup_flags );

    SCH_JUNCTION::SetSymbolSize( Mils2iu( cfg->m_Drawing.default_junction_size ) );

    m_footprintPreview      = cfg->m_Appearance.footprint_preview;
    m_navigatorStaysOpen    = cfg->m_Appearance.navigator_stays_open;
    m_showAllPins           = cfg->m_Appearance.show_hidden_pins;
    m_autoplaceFields       = cfg->m_AutoplaceFields.enable;
    m_autoplaceAlign        = cfg->m_AutoplaceFields.align_to_grid;
    m_autoplaceJustify      = cfg->m_AutoplaceFields.allow_rejustify;
    m_forceHVLines          = cfg->m_Drawing.hv_lines_only;
    m_dragActionIsMove      = cfg->m_Input.drag_is_move;
    m_selectPinSelectSymbol = cfg->m_Selection.select_pin_selects_symbol;
    m_repeatDeltaLabel      = cfg->m_Drawing.repeat_label_increment;

    wxString templateFieldNames = cfg->m_Drawing.field_names;

    if( !templateFieldNames.IsEmpty() )
    {
        TEMPLATE_FIELDNAMES_LEXER  lexer( TO_UTF8( templateFieldNames ) );

        try
        {
            m_templateFieldNames.Parse( &lexer );
        }
        catch( const IO_ERROR& DBG( e ) )
        {
            // @todo show error msg
            DBG( printf( "templatefieldnames parsing error: '%s'\n", TO_UTF8( e.What() ) ); )
        }
    }

    KIGFX::SCH_RENDER_SETTINGS* settings = GetRenderSettings();
    settings->m_ShowPinsElectricalType = false;
    settings->m_ShowHiddenText = false;
    settings->m_ShowHiddenPins = m_showAllPins;
    settings->SetShowPageLimits( m_showPageLimits );
    settings->m_ShowUmbilicals = true;
}


void SCH_EDIT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg );

    // TODO(JE) do most of these need to live as class members here, or can the sites that need
    // the setting just grab a pointer to the EESCHEMA_SETTINGS and look them up directly?

    cfg->m_Appearance.footprint_preview         = m_footprintPreview;
    cfg->m_Appearance.navigator_stays_open      = m_navigatorStaysOpen;
    cfg->m_Appearance.print_sheet_reference     = m_printSheetReference;
    cfg->m_Appearance.show_hidden_pins          = m_showAllPins;
    cfg->m_Appearance.show_illegal_symbol_lib_dialog = m_showIllegalSymbolLibDialog;
    cfg->m_Appearance.show_page_limits          = m_showPageLimits;
    cfg->m_Appearance.show_sheet_filename_case_sensitivity_dialog =
            m_showSheetFileNameCaseSensitivityDlg;

    cfg->m_AutoplaceFields.enable               = m_autoplaceFields;
    cfg->m_AutoplaceFields.allow_rejustify      = m_autoplaceJustify;
    cfg->m_AutoplaceFields.align_to_grid        = m_autoplaceAlign;

    cfg->m_Drawing.default_bus_thickness        = Iu2Mils( GetDefaultBusThickness() );
    cfg->m_Drawing.default_line_thickness       = Iu2Mils( GetDefaultLineThickness() );
    cfg->m_Drawing.default_junction_size        = Iu2Mils( SCH_JUNCTION::GetSymbolSize() );
    cfg->m_Drawing.default_repeat_offset_x      = Iu2Mils( m_repeatStep.x );
    cfg->m_Drawing.default_repeat_offset_y      = Iu2Mils( m_repeatStep.y );
    cfg->m_Drawing.default_wire_thickness       = Iu2Mils( GetDefaultWireThickness() );
    cfg->m_Drawing.hv_lines_only                = GetForceHVLines();
    cfg->m_Drawing.repeat_label_increment       = m_repeatDeltaLabel;
    cfg->m_Drawing.text_markup_flags            = GetTextMarkupFlags();

    cfg->m_Input.drag_is_move                   = m_dragActionIsMove;

    cfg->m_Printing.monochrome                  = m_printMonochrome;

    cfg->m_Selection.thickness                  = Iu2Mils( GetSelectionThickness() );
    cfg->m_Selection.draw_selected_children     = GetSelectionDrawChildItems();
    cfg->m_Selection.fill_shapes                = GetSelectionFillShapes();
    cfg->m_Selection.select_pin_selects_symbol  = GetSelectPinSelectSymbol();
    cfg->m_Selection.text_as_box                = GetSelectionTextAsBox();

    cfg->m_System.units                         = static_cast<int>( m_userUnits );

    // Save template fieldnames
    STRING_FORMATTER sf;
    m_templateFieldNames.Format( &sf, 0 );

    wxString record = FROM_UTF8( sf.GetString().c_str() );
    record.Replace( wxT("\n"), wxT(""), true );   // strip all newlines
    record.Replace( wxT("  "), wxT(" "), true );  // double space to single

    cfg->m_Drawing.field_names = record.ToStdString();
}


void LIB_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                         PANEL_HOTKEYS_EDITOR* aHotkeysPanel )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new wxPanel( book ), _( "Symbol Editor" ) );
    book->AddSubPage( new PANEL_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_LIBEDIT_SETTINGS( this, book ), _( "Defaults" ) );

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
