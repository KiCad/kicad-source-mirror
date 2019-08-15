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

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <confirm.h>
#include <gestfich.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <lib_edit_frame.h>
#include <eeschema_config.h>
#include <ws_data_model.h>
#include <class_library.h>
#include <symbol_lib_table.h>
#include <wildcards_and_files_ext.h>
#include <widgets/paged_dialog.h>
#include <dialogs/panel_eeschema_template_fieldnames.h>
#include <dialogs/panel_eeschema_settings.h>
#include <dialogs/panel_eeschema_display_options.h>
#include <dialogs/panel_libedit_display_options.h>
#include <panel_hotkeys_editor.h>
#include <widgets/widget_eeschema_color_config.h>
#include <widgets/symbol_tree_pane.h>
#include <dialogs/panel_libedit_settings.h>
#include <sch_painter.h>
#include "sch_junction.h"
#include "eeschema_id.h"

static int s_defaultBusThickness = DEFAULTBUSTHICKNESS;
static int s_defaultWireThickness  = DEFAULTDRAWLINETHICKNESS;
static int s_defaultTextSize = DEFAULT_SIZE_TEXT;
static int s_drawDefaultLineThickness = -1;


int GetDefaultBusThickness()
{
    return s_defaultBusThickness;
}


void SetDefaultBusThickness( int aThickness)
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

    book->AddPage( new PANEL_EESCHEMA_SETTINGS( this, book ), _( "Eeschema" ) );
    book->AddSubPage( new PANEL_EESCHEMA_DISPLAY_OPTIONS( this, book ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_EESCHEMA_COLOR_CONFIG( this, book ), _( "Colors" ) );
    book->AddSubPage( new PANEL_EESCHEMA_TEMPLATE_FIELDNAMES( this, book ),
                      _( "Field Name Templates" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


PARAM_CFG_ARRAY& SCH_EDIT_FRAME::GetProjectFileParameters()

{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "PageLayoutDescrFile" ),
                                        &BASE_SCREEN::m_PageLayoutDescrFileName ) );

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "PlotDirectoryName" ),
                                        &m_plotDirectoryName ) );

    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "SubpartIdSeparator" ),
                                        LIB_PART::SubpartIdSeparatorPtr(), 0, 0, 126 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "SubpartFirstId" ),
                                        LIB_PART::SubpartFirstIdPtr(), 'A', '1', 'z' ) );

    m_projectFileParams.push_back( new PARAM_CFG_WXSTRING( wxT( "NetFmtName" ),
                                            &m_netListFormat) );
    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "SpiceAjustPassiveValues" ),
                                            &m_spiceAjustPassiveValues, false ) );

    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "LabSize" ),
                                            &s_defaultTextSize, DEFAULT_SIZE_TEXT, 5, 1000 ) );

    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "ERC_WriteFile" ),
                                   &m_ercSettings.write_erc_file, false ) );

    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "ERC_TestSimilarLabels" ),
                                   &m_ercSettings.check_similar_labels, true ) );

    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckUniqueGlobalLabels" ),
                                   &m_ercSettings.check_unique_global_labels, true ) );

    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckBusDriverConflicts" ),
                                   &m_ercSettings.check_bus_driver_conflicts, true ) );

    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckBusEntryConflicts" ),
                                   &m_ercSettings.check_bus_entry_conflicts, true ) );

    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckBusToBusConflicts" ),
                                   &m_ercSettings.check_bus_to_bus_conflicts, true ) );

    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "ERC_CheckBusToNetConflicts" ),
                                   &m_ercSettings.check_bus_to_net_conflicts, true ) );

    return m_projectFileParams;
}


bool SCH_EDIT_FRAME::LoadProjectFile()
{
    // Read library list and library path list
    bool ret = Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_SCH, GetProjectFileParameters() );

    // Read schematic editor setup
    ret &= Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_SCH_EDIT, GetProjectFileParameters() );

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

    prj.ConfigSave( Kiface().KifaceSearch(), GROUP_SCH_EDIT, GetProjectFileParameters(), path );
}

///@{
/// \ingroup config

const wxChar RescueNeverShowEntry[] =               wxT( "RescueNeverShow" );
const wxChar AutoplaceFieldsEntry[] =               wxT( "AutoplaceFields" );
const wxChar AutoplaceJustifyEntry[] =              wxT( "AutoplaceJustify" );
const wxChar AutoplaceAlignEntry[] =                wxT( "AutoplaceAlign" );
static const wxChar DragActionIsMoveEntry[] =       wxT( "DragActionIsMove" );
static const wxChar FootprintPreviewEntry[] =       wxT( "FootprintPreview" );
static const wxChar DefaultBusWidthEntry[] =        wxT( "DefaultBusWidth" );
static const wxChar DefaultWireWidthEntry[] =       wxT( "DefaultWireWidth" );
static const wxChar DefaultDrawLineWidthEntry[] =   wxT( "DefaultDrawLineWidth" );
static const wxChar DefaultJctSizeEntry[] =         wxT( "DefaultJunctionSize" );
static const wxChar ShowHiddenPinsEntry[] =         wxT( "ShowHiddenPins" );
static const wxChar HorzVertLinesOnlyEntry[] =      wxT( "HorizVertLinesOnly" );
static const wxChar FieldNamesEntry[] =             wxT( "FieldNames" );
static const wxString ShowPageLimitsEntry =         "ShowPageLimits";
static const wxString UnitsEntry =                  "Units";
static const wxString PrintMonochromeEntry =        "PrintMonochrome";
static const wxString PrintSheetRefEntry =          "PrintSheetReferenceAndTitleBlock";
static const wxString RepeatStepXEntry =            "RepeatStepX";
static const wxString RepeatStepYEntry =            "RepeatStepY";
static const wxString RepeatLabelIncrementEntry =   "RepeatLabelIncrement";
static const wxString ShowIllegalSymboLibDialog =   "ShowIllegalSymbolLibDialog";

// Library editor wxConfig entry names.
static const wxChar defaultLibWidthEntry[] =        wxT( "LibeditLibWidth" );
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
                                                   (int*)&m_userUnits, MILLIMETRES ) );

    m_configSettings.push_back( new PARAM_CFG_BOOL( true, PrintMonochromeEntry,
                                                    &m_printMonochrome, true ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, PrintSheetRefEntry,
                                                    &m_printSheetReference, true ) );

    m_configSettings.push_back( new PARAM_CFG_INT( true, RepeatStepXEntry,
                                                   &m_repeatStep.x, DEFAULT_REPEAT_OFFSET_X,
                                                   -REPEAT_OFFSET_MAX, REPEAT_OFFSET_MAX ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, RepeatStepYEntry,
                                                   &m_repeatStep.y, DEFAULT_REPEAT_OFFSET_Y,
                                                   -REPEAT_OFFSET_MAX, REPEAT_OFFSET_MAX ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, RepeatLabelIncrementEntry,
                                                   &m_repeatDeltaLabel, DEFAULT_REPEAT_LABEL_INC,
                                                   -10, +10 ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, ShowIllegalSymboLibDialog,
                                                    &m_showIllegalSymbolLibDialog, true ) );

    return m_configSettings;
}


void SCH_EDIT_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    long tmp;

    wxConfigLoadSetups( aCfg, GetConfigurationSettings() );

    // LibEdit owns this one, but we must read it in if LibEdit hasn't set it yet
    if( GetDefaultLineThickness() < 0 )
    {
        SetDefaultLineThickness( (int) aCfg->Read( DefaultDrawLineWidthEntry,
                                                   DEFAULTDRAWLINETHICKNESS ) );
    }

    SetDefaultBusThickness( (int) aCfg->Read( DefaultBusWidthEntry, DEFAULTBUSTHICKNESS ) );

    // Property introduced in 6.0; use DefaultLineWidth for earlier projects
    if( !aCfg->Read( DefaultWireWidthEntry, &tmp ) )
        aCfg->Read( DefaultDrawLineWidthEntry, &tmp, DEFAULTDRAWLINETHICKNESS );

    SetDefaultWireThickness( (int) tmp );

    if( aCfg->Read( DefaultJctSizeEntry, &tmp ) )
        SCH_JUNCTION::SetSymbolSize( (int) tmp );

    aCfg->Read( DragActionIsMoveEntry, &m_dragActionIsMove, true );
    aCfg->Read( ShowHiddenPinsEntry, &m_showAllPins, false );
    aCfg->Read( HorzVertLinesOnlyEntry, &m_forceHVLines, true );
    aCfg->Read( AutoplaceFieldsEntry, &m_autoplaceFields, true );
    aCfg->Read( AutoplaceJustifyEntry, &m_autoplaceJustify, true );
    aCfg->Read( AutoplaceAlignEntry, &m_autoplaceAlign, false );
    aCfg->Read( FootprintPreviewEntry, &m_footprintPreview, false );

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

    aCfg->Write( DragActionIsMoveEntry, m_dragActionIsMove );
    aCfg->Write( DefaultBusWidthEntry, (long) GetDefaultBusThickness() );
    aCfg->Write( DefaultWireWidthEntry, (long) GetDefaultWireThickness() );
    aCfg->Write( DefaultJctSizeEntry, (long) SCH_JUNCTION::GetSymbolSize() );
    aCfg->Write( ShowHiddenPinsEntry, m_showAllPins );
    aCfg->Write( HorzVertLinesOnlyEntry, GetForceHVLines() );
    aCfg->Write( AutoplaceFieldsEntry, m_autoplaceFields );
    aCfg->Write( AutoplaceJustifyEntry, m_autoplaceJustify );
    aCfg->Write( AutoplaceAlignEntry, m_autoplaceAlign );
    aCfg->Write( FootprintPreviewEntry, m_footprintPreview );

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

    SetDefaultLineThickness( (int) aCfg->Read( DefaultDrawLineWidthEntry,
                                               DEFAULTDRAWLINETHICKNESS ) );
    SetDefaultPinLength( (int) aCfg->Read( DefaultPinLengthEntry, DEFAULTPINLENGTH ) );
    m_textPinNumDefaultSize = (int) aCfg->Read( defaultPinNumSizeEntry, DEFAULTPINNUMSIZE );
    m_textPinNameDefaultSize = (int) aCfg->Read( defaultPinNameSizeEntry, DEFAULTPINNAMESIZE );
    SetRepeatDeltaLabel( (int) aCfg->Read( repeatLibLabelIncEntry, DEFAULT_REPEAT_LABEL_INC ) );
    SetRepeatPinStep( (int) aCfg->Read( pinRepeatStepEntry, DEFAULT_REPEAT_OFFSET_PIN ) );

    wxPoint step;
    aCfg->Read( repeatLibStepXEntry, &step.x, DEFAULT_REPEAT_OFFSET_X );
    aCfg->Read( repeatLibStepYEntry, &step.y, DEFAULT_REPEAT_OFFSET_Y );
    SetRepeatStep( step );
    m_showPinElectricalTypeName = aCfg->ReadBool( showPinElectricalType, true );
    aCfg->Read( defaultLibWidthEntry, &m_defaultLibWidth, DEFAULTLIBWIDTH );

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

    aCfg->Write( DefaultDrawLineWidthEntry, GetDefaultLineThickness() );
    aCfg->Write( DefaultPinLengthEntry, GetDefaultPinLength() );
    aCfg->Write( defaultPinNumSizeEntry, GetPinNumDefaultSize() );
    aCfg->Write( defaultPinNameSizeEntry, GetPinNameDefaultSize() );
    aCfg->Write( repeatLibLabelIncEntry, GetRepeatDeltaLabel() );
    aCfg->Write( pinRepeatStepEntry, GetRepeatPinStep() );
    aCfg->Write( repeatLibStepXEntry, GetRepeatStep().x );
    aCfg->Write( repeatLibStepYEntry, GetRepeatStep().y );
    aCfg->Write( showPinElectricalType, GetShowElectricalType() );
    aCfg->Write( defaultLibWidthEntry, m_treePane->GetSize().x );
}


void LIB_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                         PANEL_HOTKEYS_EDITOR* aHotkeysPanel )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new PANEL_LIBEDIT_SETTINGS( this, book ), _( "Symbol Editor" ) );
    book->AddSubPage( new PANEL_LIBEDIT_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );

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
