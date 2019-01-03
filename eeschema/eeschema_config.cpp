/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file eeschema_config.cpp
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <gestfich.h>
#include <sch_edit_frame.h>
#include <invoke_sch_dialog.h>
#include <lib_edit_frame.h>
#include <eeschema_config.h>
#include <hotkeys.h>
#include <worksheet_shape_builder.h>
#include <class_library.h>
#include <symbol_lib_table.h>
#include <dialog_erc.h>
#include <wildcards_and_files_ext.h>
#include <widgets/paged_dialog.h>
#include <dialogs/panel_eeschema_template_fieldnames.h>
#include <dialogs/panel_eeschema_settings.h>
#include <dialogs/panel_eeschema_display_options.h>
#include <dialogs/panel_libedit_display_options.h>
#include <widgets/widget_eeschema_color_config.h>
#include <dialogs/panel_libedit_settings.h>
#include <sch_view.h>
#include <sch_painter.h>
#include "sch_junction.h"

#define FR_HISTORY_LIST_CNT     10   ///< Maximum number of find and replace strings.


static int s_defaultBusThickness = DEFAULTBUSTHICKNESS;

int GetDefaultBusThickness()
{
    return s_defaultBusThickness;
}


void SetDefaultBusThickness( int aThickness)
{
    if( aThickness >= 1 )
        s_defaultBusThickness = aThickness;
    else
        s_defaultBusThickness = 1;
}


/// Default size for text (not only labels)
static int s_defaultTextSize = DEFAULT_SIZE_TEXT;

int GetDefaultTextSize()
{
    return s_defaultTextSize;
}


void SetDefaultTextSize( int aTextSize )
{
    s_defaultTextSize = aTextSize;
}


/*
 * Default line (in Eeschema units) thickness used to draw/plot items having a
 * default thickness line value (i.e. = 0 ).
 */
static int s_drawDefaultLineThickness  = DEFAULTDRAWLINETHICKNESS;


int GetDefaultLineThickness()
{
    return s_drawDefaultLineThickness;
}


void SetDefaultLineThickness( int aThickness )
{
    if( aThickness >=1 )
        s_drawDefaultLineThickness = aThickness;
    else
        s_drawDefaultLineThickness = 1;
}


// Color to draw selected items
COLOR4D GetItemSelectedColor()
{
    return COLOR4D( BROWN );
}


// Color to draw items flagged invisible, in libedit (they are invisible
// in Eeschema
COLOR4D GetInvisibleItemColor()
{
    return COLOR4D( DARKGRAY );
}


void LIB_EDIT_FRAME::Process_Config( wxCommandEvent& event )
{
    int id = event.GetId();

    switch( id )
    {
    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        // Display current hotkey list for LibEdit.
        DisplayHotkeyList( this, g_Libedit_Hokeys_Descr );
        break;

    default:
        DisplayError( this, wxT( "LIB_EDIT_FRAME::Process_Config error" ) );
    }
}


void SCH_EDIT_FRAME::Process_Config( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxFileName fn;

    switch( id )
    {
    case ID_CONFIG_SAVE:
        SaveProjectSettings( true );
        break;

    case ID_CONFIG_READ:
        {
            fn = g_RootSheet->GetScreen()->GetFileName();
            fn.SetExt( ProjectFileExtension );

            wxFileDialog dlg( this, _( "Load Project File" ), fn.GetPath(),
                              fn.GetFullName(), ProjectFileWildcard(),
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST );

            if( dlg.ShowModal() == wxID_CANCEL )
                break;

            wxString chosen = dlg.GetPath();

            if( chosen == Prj().GetProjectFullName() )
                LoadProjectFile();
            else
            {
                // Read library list and library path list
                Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_SCH,
                                  GetProjectFileParametersList() );
                // Read schematic editor setup
                Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_SCH_EDITOR,
                                  GetProjectFileParametersList() );
            }
        }
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        // Display current hotkey list for eeschema.
        DisplayHotkeyList( this, g_Schematic_Hokeys_Descr );
        break;

    default:
        DisplayError( this, wxT( "SCH_EDIT_FRAME::Process_Config error" ) );
    }
}


void SCH_EDIT_FRAME::OnPreferencesOptions( wxCommandEvent& event )
{
    if( ShowPreferences( g_Eeschema_Hokeys_Descr, g_Schematic_Hokeys_Descr, wxT( "eeschema" ) ) )
    {
        SaveSettings( config() );  // save values shared by eeschema applications.
        m_canvas->Refresh( true );
    }
}


void SCH_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new PANEL_EESCHEMA_SETTINGS( this, book ), _( "Eeschema" ) );
    book->AddSubPage( new PANEL_EESCHEMA_DISPLAY_OPTIONS( this, book ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_EESCHEMA_COLOR_CONFIG( this, book ), _( "Colors" ) );
    book->AddSubPage( new PANEL_EESCHEMA_TEMPLATE_FIELDNAMES( this, book ), _( "Field Name Templates" ) );
}


PARAM_CFG_ARRAY& SCH_EDIT_FRAME::GetProjectFileParametersList()

{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "PageLayoutDescrFile" ),
                                        &BASE_SCREEN::m_PageLayoutDescrFileName ) );

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "PlotDirectoryName" ),
                                        &m_plotDirectoryName ) );

    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "SubpartIdSeparator" ),
                                        LIB_PART::SubpartIdSeparatorPtr(),
                                        0, 0, 126 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "SubpartFirstId" ),
                                        LIB_PART::SubpartFirstIdPtr(),
                                        'A', '1', 'z' ) );

    /* moved to library load/save specific code, in a specific section in .pro file
    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LibDir" ),
                                                           &m_userLibraryPath ) );
    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "LibName" ),
                                                               &m_componentLibFiles,
                                                               GROUP_SCH_LIBS ) );
    */

    m_projectFileParams.push_back( new PARAM_CFG_WXSTRING( wxT( "NetFmtName" ),
                                            &m_netListFormat) );
    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "SpiceAjustPassiveValues" ),
                                            &m_spiceAjustPassiveValues, false ) );

    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "LabSize" ),
                                            &s_defaultTextSize,
                                            DEFAULT_SIZE_TEXT, 5, 1000 ) );

    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "ERC_TestSimilarLabels" ),
                                            &DIALOG_ERC::m_TestSimilarLabels, true ) );

    return m_projectFileParams;
}


bool SCH_EDIT_FRAME::LoadProjectFile()
{
    // Read library list and library path list
    bool isRead = Prj().ConfigLoad( Kiface().KifaceSearch(),
                    GROUP_SCH, GetProjectFileParametersList() );

    // Read schematic editor setup
    isRead = isRead && Prj().ConfigLoad( Kiface().KifaceSearch(),
                                         GROUP_SCH_EDITOR, GetProjectFileParametersList() );

    // Verify some values, because the config file can be edited by hand,
    // and have bad values:
    LIB_PART::SetSubpartIdNotation(
            LIB_PART::GetSubpartIdSeparator(),
            LIB_PART::GetSubpartFirstId() );

    // Load the page layout decr file, from the filename stored in
    // BASE_SCREEN::m_PageLayoutDescrFileName, read in config project file
    // If empty, or not existing, the default descr is loaded
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    wxString pg_fullfilename = WORKSHEET_LAYOUT::MakeFullFileName(
                                    BASE_SCREEN::m_PageLayoutDescrFileName,
                                    Prj().GetProjectPath() );

    pglayout.SetPageLayout( pg_fullfilename );

    return isRead;
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
        wxFileDialog dlg( this, _( "Save Project File" ),
                          fn.GetPath(), fn.GetFullName(),
                          ProjectFileWildcard(), wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();
    }

    prj.ConfigSave( Kiface().KifaceSearch(), GROUP_SCH_EDITOR, GetProjectFileParametersList() );
}

///@{
/// \ingroup config

const wxChar RescueNeverShowEntry[] =               wxT( "RescueNeverShow" );
const wxChar AutoplaceFieldsEntry[] =               wxT( "AutoplaceFields" );
const wxChar AutoplaceJustifyEntry[] =              wxT( "AutoplaceJustify" );
const wxChar AutoplaceAlignEntry[] =                wxT( "AutoplaceAlign" );
static const wxChar FootprintPreviewEntry[] =       wxT( "FootprintPreview" );
static const wxChar DefaultBusWidthEntry[] =        wxT( "DefaultBusWidth" );
static const wxChar DefaultDrawLineWidthEntry[] =   wxT( "DefaultDrawLineWidth" );
static const wxChar DefaultJctSizeEntry[] =         wxT( "DefaultJunctionSize" );
static const wxChar ShowHiddenPinsEntry[] =         wxT( "ShowHiddenPins" );
static const wxChar HorzVertLinesOnlyEntry[] =      wxT( "HorizVertLinesOnly" );
static const wxChar FindReplaceFlagsEntry[] =       wxT( "LastFindReplaceFlags" );
static const wxChar FindStringEntry[] =             wxT( "LastFindString" );
static const wxChar ReplaceStringEntry[] =          wxT( "LastReplaceString" );
static const wxChar FindStringHistoryEntry[] =      wxT( "FindStringHistoryList%d" );
static const wxChar ReplaceStringHistoryEntry[] =   wxT( "ReplaceStringHistoryList%d" );
static const wxChar FieldNamesEntry[] =             wxT( "FieldNames" );
static const wxChar SimulatorCommandEntry[] =       wxT( "SimCmdLine" );
static const wxString ShowPageLimitsEntry =         "ShowPageLimits";
static const wxString UnitsEntry =                  "Units";
static const wxString PrintMonochromeEntry =        "PrintMonochrome";
static const wxString PrintSheetRefEntry =          "PrintSheetReferenceAndTitleBlock";
static const wxString RepeatStepXEntry =            "RepeatStepX";
static const wxString RepeatStepYEntry =            "RepeatStepY";
static const wxString RepeatLabelIncrementEntry =   "RepeatLabelIncrement";

// Library editor wxConfig entry names.
static const wxChar defaultPinNumSizeEntry[] =      wxT( "LibeditPinNumSize" );
static const wxChar defaultPinNameSizeEntry[] =     wxT( "LibeditPinNameSize" );
static const wxChar DefaultPinLengthEntry[] =       wxT( "DefaultPinLength" );
static const wxChar repeatLibLabelIncEntry[] =      wxT( "LibeditRepeatLabelInc" );
static const wxChar pinRepeatStepEntry[] =          wxT( "LibeditPinRepeatStep" );
static const wxChar repeatLibStepXEntry[] =         wxT( "LibeditRepeatStepX" );
static const wxChar repeatLibStepYEntry[] =         wxT( "LibeditRepeatStepY" );
static const wxChar showPinElectricalType[] =       wxT( "LibeditShowPinElectricalType" );

///@}

PARAM_CFG_ARRAY& SCH_EDIT_FRAME::GetConfigurationSettings()
{
    if( !m_configSettings.empty() )
        return m_configSettings;

    m_configSettings.push_back( new PARAM_CFG_BOOL( true, ShowPageLimitsEntry,
                                                    &m_showPageLimits, true ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, UnitsEntry,
                                                   (int*)&m_UserUnits, MILLIMETRES ) );

    m_configSettings.push_back( new PARAM_CFG_BOOL( true, PrintMonochromeEntry,
                                                    &m_printMonochrome, true ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, PrintSheetRefEntry,
                                                    &m_printSheetReference, true ) );

    m_configSettings.push_back( new PARAM_CFG_INT( true, RepeatStepXEntry,
                                                      &m_repeatStep.x,
                                                      DEFAULT_REPEAT_OFFSET_X,
                                                      -REPEAT_OFFSET_MAX,
                                                      REPEAT_OFFSET_MAX ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, RepeatStepYEntry,
                                                      &m_repeatStep.y,
                                                      DEFAULT_REPEAT_OFFSET_Y,
                                                      -REPEAT_OFFSET_MAX,
                                                      REPEAT_OFFSET_MAX ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, RepeatLabelIncrementEntry,
                                                      &m_repeatDeltaLabel,
                                                      DEFAULT_REPEAT_LABEL_INC, -10, +10 ) );
    return m_configSettings;
}


void SCH_EDIT_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    long tmp;

    ReadHotkeyConfig( SCH_EDIT_FRAME_NAME, g_Schematic_Hokeys_Descr );
    wxConfigLoadSetups( aCfg, GetConfigurationSettings() );

    SetDefaultBusThickness( (int) aCfg->Read( DefaultBusWidthEntry, DEFAULTBUSTHICKNESS ) );
    SetDefaultLineThickness( (int) aCfg->Read( DefaultDrawLineWidthEntry, DEFAULTDRAWLINETHICKNESS ) );
    SCH_JUNCTION::SetSymbolSize( (int) aCfg->Read( DefaultJctSizeEntry, SCH_JUNCTION::GetSymbolSize() ) );
    aCfg->Read( ShowHiddenPinsEntry, &m_showAllPins, false );
    aCfg->Read( HorzVertLinesOnlyEntry, &m_forceHVLines, true );
    aCfg->Read( AutoplaceFieldsEntry, &m_autoplaceFields, true );
    aCfg->Read( AutoplaceJustifyEntry, &m_autoplaceJustify, true );
    aCfg->Read( AutoplaceAlignEntry, &m_autoplaceAlign, false );
    aCfg->Read( FootprintPreviewEntry, &m_footprintPreview, false );

    // Load netlists options:
    aCfg->Read( SimulatorCommandEntry, &m_simulatorCommand );

    wxASSERT_MSG( m_findReplaceData,
                  wxT( "Find dialog data settings object not created. Bad programmer!" ) );

    aCfg->Read( FindReplaceFlagsEntry, &tmp, (long) wxFR_DOWN );
    m_findReplaceData->SetFlags( (wxUint32) tmp & ~FR_REPLACE_ITEM_FOUND );
    m_findReplaceData->SetFindString( aCfg->Read( FindStringEntry, wxEmptyString ) );
    m_findReplaceData->SetReplaceString( aCfg->Read( ReplaceStringEntry, wxEmptyString ) );

    // Load the find and replace string history list.
    for( int i = 0; i < FR_HISTORY_LIST_CNT; ++i )
    {
        wxString tmpHistory;
        wxString entry;
        entry.Printf( FindStringHistoryEntry, i );
        tmpHistory = aCfg->Read( entry, wxEmptyString );

        if( !tmpHistory.IsEmpty() )
            m_findStringHistoryList.Add( tmpHistory );

        entry.Printf( ReplaceStringHistoryEntry, i );
        tmpHistory = aCfg->Read( entry, wxEmptyString );

        if( !tmpHistory.IsEmpty() )
            m_replaceStringHistoryList.Add( tmpHistory );
    }

    wxString templateFieldNames = aCfg->Read( FieldNamesEntry, wxEmptyString );

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

    auto painter = static_cast<KIGFX::SCH_PAINTER*>( GetCanvas()->GetView()->GetPainter() );
    KIGFX::SCH_RENDER_SETTINGS* settings = painter->GetSettings();
    settings->m_ShowPinsElectricalType = false;
    settings->m_ShowHiddenText = false;
    settings->m_ShowHiddenPins = m_showAllPins;
    settings->SetShowPageLimits( m_showPageLimits );
    settings->m_ShowUmbilicals = true;
}


void SCH_EDIT_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    wxConfigSaveSetups( aCfg, GetConfigurationSettings() );

    aCfg->Write( DefaultBusWidthEntry, (long) GetDefaultBusThickness() );
    aCfg->Write( DefaultDrawLineWidthEntry, (long) GetDefaultLineThickness() );
    aCfg->Write( DefaultJctSizeEntry, (long) SCH_JUNCTION::GetSymbolSize() );
    aCfg->Write( ShowHiddenPinsEntry, m_showAllPins );
    aCfg->Write( HorzVertLinesOnlyEntry, GetForceHVLines() );
    aCfg->Write( AutoplaceFieldsEntry, m_autoplaceFields );
    aCfg->Write( AutoplaceJustifyEntry, m_autoplaceJustify );
    aCfg->Write( AutoplaceAlignEntry, m_autoplaceAlign );
    aCfg->Write( FootprintPreviewEntry, m_footprintPreview );

    // Save netlists options:
    aCfg->Write( SimulatorCommandEntry, m_simulatorCommand );

    // Save find dialog session setting.
    wxASSERT_MSG( m_findReplaceData,
                  wxT( "Find dialog data settings object not created. Bad programmer!" ) );
    aCfg->Write( FindReplaceFlagsEntry,
                (long) m_findReplaceData->GetFlags() & ~FR_REPLACE_ITEM_FOUND );
    aCfg->Write( FindStringEntry, m_findReplaceData->GetFindString() );
    aCfg->Write( ReplaceStringEntry, m_findReplaceData->GetReplaceString() );

    // Save the find and replace string history list.
    unsigned i;
    wxString tmpHistory;
    wxString entry;     // invoke constructor outside of any loops

    for( i = 0; i < m_findStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        entry.Printf( FindStringHistoryEntry, i );
        aCfg->Write( entry, m_findStringHistoryList[ i ] );
    }

    for( i = 0; i < m_replaceStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        entry.Printf( ReplaceStringHistoryEntry, i );
        aCfg->Write( entry, m_replaceStringHistoryList[ i ] );
    }

    // Save template fieldnames
    STRING_FORMATTER sf;
    m_templateFieldNames.Format( &sf, 0 );

    wxString record = FROM_UTF8( sf.GetString().c_str() );
    record.Replace( wxT("\n"), wxT(""), true );   // strip all newlines
    record.Replace( wxT("  "), wxT(" "), true );  // double space to single

    aCfg->Write( FieldNamesEntry, record );
}


void LIB_EDIT_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    ReadHotkeyConfig( LIB_EDIT_FRAME_NAME, g_Libedit_Hokeys_Descr );

    SetDefaultLineThickness( (int) aCfg->Read( DefaultDrawLineWidthEntry, DEFAULTDRAWLINETHICKNESS ) );
    SetDefaultPinLength( (int) aCfg->Read( DefaultPinLengthEntry, DEFAULTPINLENGTH ) );
    m_textPinNumDefaultSize = (int) aCfg->Read( defaultPinNumSizeEntry, DEFAULTPINNUMSIZE );
    m_textPinNameDefaultSize = (int) aCfg->Read( defaultPinNameSizeEntry, DEFAULTPINNAMESIZE );
    SetRepeatDeltaLabel( (int) aCfg->Read( repeatLibLabelIncEntry, DEFAULT_REPEAT_LABEL_INC ) );
    SetRepeatPinStep( (int) aCfg->Read( pinRepeatStepEntry, DEFAULT_REPEAT_OFFSET_PIN ) );
    wxPoint step;
    step.x = (int) aCfg->Read( repeatLibStepXEntry, (long) DEFAULT_REPEAT_OFFSET_X );
    step.y = (int) aCfg->Read( repeatLibStepYEntry, (long) DEFAULT_REPEAT_OFFSET_Y );
    SetRepeatStep( step );
    m_showPinElectricalTypeName = aCfg->ReadBool( showPinElectricalType, true );

    wxString templateFieldNames = aCfg->Read( FieldNamesEntry, wxEmptyString );

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

    auto painter = static_cast<KIGFX::SCH_PAINTER*>( GetCanvas()->GetView()->GetPainter() );
    KIGFX::SCH_RENDER_SETTINGS* settings = painter->GetSettings();
    settings->m_ShowPinsElectricalType = m_showPinElectricalTypeName;

    // Hidden elements must be editable
    settings->m_ShowHiddenText = true;
    settings->m_ShowHiddenPins = true;
    settings->m_ShowUmbilicals = false;
}


void LIB_EDIT_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    aCfg->Write( DefaultPinLengthEntry, (long) GetDefaultPinLength() );
    aCfg->Write( defaultPinNumSizeEntry, (long) GetPinNumDefaultSize() );
    aCfg->Write( defaultPinNameSizeEntry, (long) GetPinNameDefaultSize() );
    aCfg->Write( repeatLibLabelIncEntry, (long) GetRepeatDeltaLabel() );
    aCfg->Write( pinRepeatStepEntry, (long) GetRepeatPinStep() );
    aCfg->Write( repeatLibStepXEntry, (long) GetRepeatStep().x );
    aCfg->Write( repeatLibStepYEntry, (long) GetRepeatStep().y );
    aCfg->Write( showPinElectricalType, GetShowElectricalType() );
}


void LIB_EDIT_FRAME::OnPreferencesOptions( wxCommandEvent& event )
{
    if( ShowPreferences( g_Eeschema_Hokeys_Descr, g_Libedit_Hokeys_Descr, wxT( "eeschema" ) ) )
    {
        SaveSettings( config() );  // save values shared by eeschema applications.
        m_canvas->Refresh( true );
    }
}


void LIB_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new PANEL_LIBEDIT_SETTINGS( this, book ), _( "Symbol Editor" ) );
    book->AddSubPage( new PANEL_LIBEDIT_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
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
