/**
 * @file eeschema_config.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <appl_wxstruct.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <wxEeschemaStruct.h>

#include <eeschema_id.h>
#include <general.h>
#include <netlist_control.h>
#include <protos.h>
#include <libeditframe.h>
#include <eeschema_config.h>
#include <worksheet.h>
#include <hotkeys.h>
#include <sch_sheet.h>

#include <dialog_hotkeys_editor.h>

#include <dialogs/dialog_color_config.h>
#include <dialogs/dialog_eeschema_config.h>
#include <dialogs/dialog_eeschema_options.h>
#include <dialogs/dialog_schematic_find.h>

#include <wildcards_and_files_ext.h>

#define HOTKEY_FILENAME         wxT( "eeschema" )

#define FR_HISTORY_LIST_CNT     10   ///< Maximum number of find and replace strings.


void LIB_EDIT_FRAME::InstallConfigFrame( wxCommandEvent& event )
{
    DIALOG_EESCHEMA_CONFIG CfgFrame( (SCH_EDIT_FRAME *)GetParent(), this );

    CfgFrame.ShowModal();
}


void LIB_EDIT_FRAME::OnColorConfig( wxCommandEvent& aEvent )
{
    DIALOG_COLOR_CONFIG dlg( this );

    dlg.ShowModal();
}


void LIB_EDIT_FRAME::Process_Config( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxFileName fn;
    SCH_EDIT_FRAME* schFrame = ( SCH_EDIT_FRAME* ) GetParent();

    switch( id )
    {
    case ID_CONFIG_SAVE:
        schFrame->SaveProjectFile();
        break;

    case ID_CONFIG_READ:
    {
        fn = g_RootSheet->GetScreen()->GetFileName();
        fn.SetExt( ProjectFileExtension );

        wxFileDialog dlg( this, _( "Read Project File" ), fn.GetPath(),
                          fn.GetFullName(), ProjectFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            break;

        schFrame->LoadProjectFile( dlg.GetPath(), true );
    }
    break;


    // Hotkey IDs
    case ID_PREFERENCES_HOTKEY_SHOW_EDITOR:
        InstallHotkeyFrame( this, s_Eeschema_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_EXPORT_CONFIG:
        ExportHotkeyConfigToFile( s_Eeschema_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_IMPORT_CONFIG:
        ImportHotkeyConfigFromFile( s_Eeschema_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        // Display current hotkey list for LibEdit.
        DisplayHotkeyList( this, s_Libedit_Hokeys_Descr );
        break;

    default:
        DisplayError( this, wxT( "LIB_EDIT_FRAME::Process_Config error" ) );
    }
}


void SCH_EDIT_FRAME::OnColorConfig( wxCommandEvent& aEvent )
{
    DIALOG_COLOR_CONFIG dlg( this );

    dlg.ShowModal();
}


void SCH_EDIT_FRAME::InstallConfigFrame( wxCommandEvent& event )
{
    DIALOG_EESCHEMA_CONFIG CfgFrame( this, this );

    CfgFrame.ShowModal();
}


void SCH_EDIT_FRAME::Process_Config( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxFileName fn;

    switch( id )
    {
    case ID_CONFIG_SAVE:
        SaveProjectFile();
        break;

    case ID_CONFIG_READ:
    {
        fn = g_RootSheet->GetScreen()->GetFileName();
        fn.SetExt( ProjectFileExtension );

        wxFileDialog dlg( this, _( "Read Project File" ), fn.GetPath(),
                          fn.GetFullName(), ProjectFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            break;

        LoadProjectFile( dlg.GetPath(), true );
    }
    break;


    // Hotkey IDs
    case ID_PREFERENCES_HOTKEY_EXPORT_CONFIG:
        ExportHotkeyConfigToFile( s_Eeschema_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_IMPORT_CONFIG:
        ImportHotkeyConfigFromFile( s_Eeschema_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_EDITOR:
        InstallHotkeyFrame( this, s_Eeschema_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        // Display current hotkey list for eeschema.
        DisplayHotkeyList( this, s_Schematic_Hokeys_Descr );
        break;

    default:
        DisplayError( this, wxT( "SCH_EDIT_FRAME::Process_Config error" ) );
    }
}


void SCH_EDIT_FRAME::OnSetOptions( wxCommandEvent& event )
{
    wxArrayString units;
    GRIDS grid_list;

    GetScreen()->GetGrids( grid_list );

    DIALOG_EESCHEMA_OPTIONS dlg( this );

    units.Add( GetUnitsLabel( INCHES ) );
    units.Add( GetUnitsLabel( MILLIMETRES ) );

    dlg.SetUnits( units, g_UserUnit );
    dlg.SetGridSizes( grid_list, GetScreen()->GetGridId() );
    dlg.SetLineWidth( g_DrawDefaultLineThickness );
    dlg.SetTextSize( g_DefaultTextLabelSize );
    dlg.SetRepeatHorizontal( g_RepeatStep.x );
    dlg.SetRepeatVertical( g_RepeatStep.y );
    dlg.SetRepeatLabel( g_RepeatDeltaLabel );
    dlg.SetAutoSaveInterval( GetAutoSaveInterval() / 60 );
    dlg.SetShowGrid( IsGridVisible() );
    dlg.SetShowHiddenPins( m_showAllPins );
    dlg.SetEnableAutoPan( m_canvas->GetEnableAutoPan() );
    dlg.SetEnableHVBusOrientation( g_HVLines );
    dlg.SetShowPageLimits( g_ShowPageLimits );
    dlg.Layout();
    dlg.Fit();
    dlg.SetMinSize( dlg.GetSize() );

    const TEMPLATE_FIELDNAMES&  tfnames = m_TemplateFieldNames.GetTemplateFieldNames();

    for( unsigned i=0; i<tfnames.size(); ++i )
    {
        D(printf("dlg.SetFieldName(%d, '%s')\n", i, TO_UTF8( tfnames[i].m_Name) );)

        dlg.SetFieldName( i, tfnames[i].m_Name );
    }

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    g_UserUnit = (EDA_UNITS_T)dlg.GetUnitsSelection();

    GetScreen()->SetGrid( grid_list[ (size_t) dlg.GetGridSelection() ].m_Size );

    g_DrawDefaultLineThickness = dlg.GetLineWidth();
    g_DefaultTextLabelSize = dlg.GetTextSize();
    g_RepeatStep.x = dlg.GetRepeatHorizontal();
    g_RepeatStep.y = dlg.GetRepeatVertical();
    g_RepeatDeltaLabel = dlg.GetRepeatLabel();
    SetAutoSaveInterval( dlg.GetAutoSaveInterval() * 60 );
    SetGridVisibility( dlg.GetShowGrid() );
    m_showAllPins = dlg.GetShowHiddenPins();
    m_canvas->SetEnableAutoPan( dlg.GetEnableAutoPan() );
    g_HVLines = dlg.GetEnableHVBusOrientation();
    g_ShowPageLimits = dlg.GetShowPageLimits();

    wxString templateFieldName;

    // @todo this will change when the template field editor is redone to
    // look like the component field property editor, showing visibility and value also

    DeleteAllTemplateFieldNames();

    for( int i=0; i<8; ++i )    // no. fields in this dialog window
    {
        templateFieldName = dlg.GetFieldName( i );

        if( !templateFieldName.IsEmpty() )
        {
            TEMPLATE_FIELDNAME  fld( dlg.GetFieldName( i ) );

            // @todo set visibility and value also from a better editor

            AddTemplateFieldName( fld );
        }
    }

    m_canvas->Refresh( true );
}


PARAM_CFG_ARRAY& SCH_EDIT_FRAME::GetProjectFileParameters()
{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LibDir" ),
                                                           &m_userLibraryPath ) );
    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "LibName" ),
                                                               &m_componentLibFiles,
                                                               GROUPLIB ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "NetFmt" ),
                                                      &m_netListFormat,
                                                      NET_TYPE_PCBNEW,
                                                      NET_TYPE_PCBNEW,
                                                      NET_TYPE_CUSTOM_MAX ) );

    // NOTE: Left as global until supporting code  can be fixed.
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "HPGLSpd" ),
                                                      &g_HPGL_Pen_Descr.m_Pen_Speed,
                                                      20, 2, 45 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "HPGLDm" ),
                                                      &g_HPGL_Pen_Descr.m_Pen_Diam,
                                                      15, 1, 150 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "HPGLNum" ),
                                                      &g_HPGL_Pen_Descr.m_Pen_Num,
                                                      1, 1, 8 ) );

/* these globals don't exist
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_A4" ),
                                                      &g_Sheet_A4.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_A4" ),
                                                      &g_Sheet_A4.m_Offset.y ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_A3" ),
                                                      &g_Sheet_A3.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_A3" ),
                                                      &g_Sheet_A3.m_Offset.y ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_A2" ),
                                                      &g_Sheet_A2.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_A2" ),
                                                      &g_Sheet_A2.m_Offset.y ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_A1" ),
                                                      &g_Sheet_A1.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_A1" ),
                                                      &g_Sheet_A1.m_Offset.y ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_A0" ),
                                                      &g_Sheet_A0.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_A0" ),
                                                      &g_Sheet_A0.m_Offset.y ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_A" ),
                                                      &g_Sheet_A.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_A" ),
                                                      &g_Sheet_A.m_Offset.y ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_B" ),
                                                      &g_Sheet_B.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_B" ),
                                                      &g_Sheet_B.m_Offset.y ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_C" ),
                                                      &g_Sheet_C.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_C" ),
                                                      &g_Sheet_C.m_Offset.y ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_D" ),
                                                      &g_Sheet_D.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_D" ),
                                                      &g_Sheet_D.m_Offset.y ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offX_E" ),
                                                      &g_Sheet_E.m_Offset.x ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "offY_E" ),
                                                      &g_Sheet_E.m_Offset.y ) );
*/

    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "RptD_X" ),
                                                      &g_RepeatStep.x,
                                                      0, -1000, +1000 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "RptD_Y" ),
                                                      &g_RepeatStep.y,
                                                      100, -1000, +1000 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "RptLab" ),
                                                      &g_RepeatDeltaLabel,
                                                      1, -10, +10 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "LabSize" ),
                                                      &g_DefaultTextLabelSize,
                                                      DEFAULT_SIZE_TEXT, 0,
                                                      1000 ) );

    return m_projectFileParams;
}


bool SCH_EDIT_FRAME::LoadProjectFile( const wxString& aFileName, bool aForceReread )
{
    wxFileName              fn;
    bool                    IsRead = true;
    wxArrayString           liblist_tmp = m_componentLibFiles;

    if( aFileName.IsEmpty() )
        fn = g_RootSheet->GetScreen()->GetFileName();
    else
        fn = aFileName;

    m_componentLibFiles.Clear();

    /* Change the schematic file extension (.sch) to the project file
     * extension (.pro). */
    fn.SetExt( ProjectFileExtension );

    wxGetApp().RemoveLibraryPath( m_userLibraryPath );

    if( !wxGetApp().ReadProjectConfig( fn.GetFullPath(), GROUP,
                                       GetProjectFileParameters(),
                                       !aForceReread ) )
    {
        m_componentLibFiles = liblist_tmp;
        IsRead = false;
    }

    // User library path takes precedent over default library search paths.
    wxGetApp().InsertLibraryPath( m_userLibraryPath, 1 );

    /* If the list is void, force loading the library "power.lib" that is
     * the "standard" library for power symbols.
     */
    if( m_componentLibFiles.GetCount() == 0 )
        m_componentLibFiles.Add( wxT( "power" ) );

    LoadLibraries();
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    return IsRead;
}


void SCH_EDIT_FRAME::SaveProjectFile()
{
    wxFileName fn;

    fn = g_RootSheet->GetScreen()->GetFileName();  //ConfigFileName
    fn.SetExt( ProjectFileExtension );

    if( !IsWritable( fn ) )
        return;

    wxGetApp().WriteProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters() );
}


static const wxString DefaultDrawLineWidthEntry( wxT( "DefaultDrawLineWidth" ) );
static const wxString ShowHiddenPinsEntry( wxT( "ShowHiddenPins" ) );
static const wxString HorzVertLinesOnlyEntry( wxT( "HorizVertLinesOnly" ) );
static const wxString PreviewFramePositionXEntry( wxT( "PreviewFramePositionX" ) );
static const wxString PreviewFramePositionYEntry( wxT( "PreviewFramePositionY" ) );
static const wxString PreviewFrameWidthEntry( wxT( "PreviewFrameWidth" ) );
static const wxString PreviewFrameHeightEntry( wxT( "PreviewFrameHeight" ) );
static const wxString PrintDialogPositionXEntry( wxT( "PrintDialogPositionX" ) );
static const wxString PrintDialogPositionYEntry( wxT( "PrintDialogPositionY" ) );
static const wxString PrintDialogWidthEntry( wxT( "PrintDialogWidth" ) );
static const wxString PrintDialogHeightEntry( wxT( "PrintDialogHeight" ) );
static const wxString FindDialogPositionXEntry( wxT( "FindDialogPositionX" ) );
static const wxString FindDialogPositionYEntry( wxT( "FindDialogPositionY" ) );
static const wxString FindDialogWidthEntry( wxT( "FindDialogWidth" ) );
static const wxString FindDialogHeightEntry( wxT( "FindDialogHeight" ) );
static const wxString FindReplaceFlagsEntry( wxT( "LastFindReplaceFlags" ) );
static const wxString FindStringEntry( wxT( "LastFindString" ) );
static const wxString ReplaceStringEntry( wxT( "LastReplaceString" ) );
static const wxString FindStringHistoryEntry( wxT( "FindStringHistoryList%d" ) );
static const wxString ReplaceStringHistoryEntry( wxT( "ReplaceStringHistoryList%d" ) );
static const wxString FieldNamesEntry( wxT( "FieldNames" ) );
static const wxString SpiceNetNamesEntry( wxT( "SpiceUseNetNames" ) );
static const wxString SimulatorCommandEntry( wxT( "SimCmdLine" ) );


PARAM_CFG_ARRAY& SCH_EDIT_FRAME::GetConfigurationSettings( void )
{
    if( !m_configSettings.empty() )
        return m_configSettings;

    m_configSettings.push_back( new PARAM_CFG_INT( wxT( "Unite" ),
                                                   (int*)&g_UserUnit, 0 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColWire" ),
                                                        &g_LayerDescr.LayerColor[LAYER_WIRE],
                                                        GREEN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorBus" ),
                                                        &g_LayerDescr.LayerColor[LAYER_BUS],
                                                        BLUE ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorConn" ),
                                                        &g_LayerDescr.LayerColor[LAYER_JUNCTION],
                                                        GREEN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorLlab" ),
                                                        &g_LayerDescr.LayerColor[LAYER_LOCLABEL],
                                                        BLACK ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorHlab" ),
                                                        &g_LayerDescr.LayerColor[LAYER_HIERLABEL],
                                                        BROWN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorGbllab" ),
                                                        &g_LayerDescr.LayerColor[LAYER_GLOBLABEL],
                                                        RED ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorPinF" ),
                                                        &g_LayerDescr.LayerColor[LAYER_PINFUN],
                                                        MAGENTA ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColPinN" ),
                                                        &g_LayerDescr.LayerColor[LAYER_PINNUM],
                                                        RED ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorPNam" ),
                                                        &g_LayerDescr.LayerColor[LAYER_PINNAM],
                                                        CYAN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorField" ),
                                                        &g_LayerDescr.LayerColor[LAYER_FIELDS],
                                                        MAGENTA ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorRef" ),
                                                        &g_LayerDescr.LayerColor[LAYER_REFERENCEPART],
                                                        CYAN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorValue" ),
                                                        &g_LayerDescr.LayerColor[LAYER_VALUEPART],
                                                        CYAN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorNote" ),
                                                        &g_LayerDescr.LayerColor[LAYER_NOTES],
                                                        LIGHTBLUE ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorBody" ),
                                                        &g_LayerDescr.LayerColor[LAYER_DEVICE],
                                                        RED ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorBodyBg" ),
                                                        &g_LayerDescr.LayerColor[LAYER_DEVICE_BACKGROUND],
                                                        LIGHTYELLOW ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorNetN" ),
                                                        &g_LayerDescr.LayerColor[LAYER_NETNAM],
                                                        DARKGRAY ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorPin" ),
                                                        &g_LayerDescr.LayerColor[LAYER_PIN],
                                                        RED ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorSheet" ),
                                                        &g_LayerDescr.LayerColor[LAYER_SHEET],
                                                        MAGENTA ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true,
                                                        wxT( "ColorSheetFileName" ),
                                                        &g_LayerDescr.LayerColor[LAYER_SHEETFILENAME],
                                                        BROWN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorSheetName" ),
                                                        &g_LayerDescr.LayerColor[LAYER_SHEETNAME],
                                                        CYAN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorSheetLab" ),
                                                        &g_LayerDescr.LayerColor[LAYER_SHEETLABEL],
                                                        BROWN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorNoCo" ),
                                                        &g_LayerDescr.LayerColor[LAYER_NOCONNECT],
                                                        BLUE ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorErcW" ),
                                                        &g_LayerDescr.LayerColor[LAYER_ERC_WARN],
                                                        GREEN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorErcE" ),
                                                        &g_LayerDescr.LayerColor[LAYER_ERC_ERR],
                                                        RED ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorGrid" ),
                                                        &g_LayerDescr.LayerColor[LAYER_GRID],
                                                        DARKGRAY ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PrintMonochrome" ),
                                                    &m_printMonochrome, true ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PrintSheetReferenceAndTitleBlock" ),
                                                    &m_printSheetReference, true ) );

    return m_configSettings;
}


void SCH_EDIT_FRAME::LoadSettings()
{
    wxASSERT( wxGetApp().GetSettings() != NULL );

    long tmp;

    wxConfig* cfg = wxGetApp().GetSettings();

    EDA_DRAW_FRAME::LoadSettings();

    wxGetApp().ReadCurrentSetupValues( GetConfigurationSettings() );

    // This is required until someone gets rid of the global variable g_LayerDescription().
    m_GridColor = g_LayerDescr.LayerColor[LAYER_GRID];

    g_DrawDefaultLineThickness = cfg->Read( DefaultDrawLineWidthEntry,(long) 6 );
    cfg->Read( ShowHiddenPinsEntry, &m_showAllPins, false );
    cfg->Read( HorzVertLinesOnlyEntry, &g_HVLines, true );

    // Load print preview window session settings.
    cfg->Read( PreviewFramePositionXEntry, &tmp, -1 );
    m_previewPosition.x = (int) tmp;
    cfg->Read( PreviewFramePositionYEntry, &tmp, -1 );
    m_previewPosition.y = (int) tmp;
    cfg->Read( PreviewFrameWidthEntry, &tmp, -1 );
    m_previewSize.SetWidth( (int) tmp );
    cfg->Read( PreviewFrameHeightEntry, &tmp, -1 );
    m_previewSize.SetHeight( (int) tmp );

    // Load print dialog session settings.
    cfg->Read( PrintDialogPositionXEntry, &tmp, -1 );
    m_printDialogPosition.x = (int) tmp;
    cfg->Read( PrintDialogPositionYEntry, &tmp, -1 );
    m_printDialogPosition.y = (int) tmp;
    cfg->Read( PrintDialogWidthEntry, &tmp, -1 );
    m_printDialogSize.SetWidth( (int) tmp );
    cfg->Read( PrintDialogHeightEntry, &tmp, -1 );
    m_printDialogSize.SetHeight( (int) tmp );

    // Load netlists options:
    cfg->Read( SpiceNetNamesEntry,  &g_OptNetListUseNames, false );
    cfg->Read( SimulatorCommandEntry, &m_simulatorCommand );

    // Load find dialog session setting.
    cfg->Read( FindDialogPositionXEntry, &tmp, -1 );
    m_findDialogPosition.x = (int) tmp;
    cfg->Read( FindDialogPositionYEntry, &tmp, -1 );
    m_findDialogPosition.y = (int) tmp;
    cfg->Read( FindDialogWidthEntry, &tmp, -1 );
    m_findDialogSize.SetWidth( (int) tmp );
    cfg->Read( FindDialogHeightEntry, &tmp, -1 );
    m_findDialogSize.SetHeight( (int) tmp );
    wxASSERT_MSG( m_findReplaceData,
                  wxT( "Find dialog data settings object not created. Bad programmer!" ) );
    cfg->Read( FindReplaceFlagsEntry, &tmp, (long) wxFR_DOWN );
    m_findReplaceData->SetFlags( (wxUint32) tmp & ~FR_REPLACE_ITEM_FOUND );
    m_findReplaceData->SetFindString( cfg->Read( FindStringEntry, wxEmptyString ) );
    m_findReplaceData->SetReplaceString( cfg->Read( ReplaceStringEntry, wxEmptyString ) );

    // Load the find and replace string history list.
    for( int i = 0; i < FR_HISTORY_LIST_CNT; ++i )
    {
        wxString tmpHistory;
        wxString entry;
        entry.Printf( FindStringHistoryEntry, i );
        tmpHistory = cfg->Read( entry, wxEmptyString );

        if( !tmpHistory.IsEmpty() )
            m_findStringHistoryList.Add( tmpHistory );

        entry.Printf( ReplaceStringHistoryEntry, i );
        tmpHistory = cfg->Read( entry, wxEmptyString );

        if( !tmpHistory.IsEmpty() )
            m_replaceStringHistoryList.Add( tmpHistory );
    }

    wxString templateFieldNames = cfg->Read( FieldNamesEntry, wxEmptyString );

    if( !templateFieldNames.IsEmpty() )
    {
        TEMPLATE_FIELDNAMES_LEXER  lexer( TO_UTF8( templateFieldNames ) );
        try
        {
            m_TemplateFieldNames.Parse( &lexer );
        }
        catch( IO_ERROR& e )
        {
            // @todo show error msg
            D( printf( "templatefieldnames parsing error: '%s'\n",
                       TO_UTF8( e.errorText ) ); )
        }
    }
}


void SCH_EDIT_FRAME::SaveSettings()
{
    wxASSERT( wxGetApp().GetSettings() != NULL );

    wxConfig* cfg = wxGetApp().GetSettings();

    EDA_DRAW_FRAME::SaveSettings();

    wxGetApp().SaveCurrentSetupValues( GetConfigurationSettings() );

    cfg->Write( DefaultDrawLineWidthEntry, (long) g_DrawDefaultLineThickness );
    cfg->Write( ShowHiddenPinsEntry, m_showAllPins );
    cfg->Write( HorzVertLinesOnlyEntry, g_HVLines );

    // Save print preview window session settings.
    cfg->Write( PreviewFramePositionXEntry, m_previewPosition.x );
    cfg->Write( PreviewFramePositionYEntry, m_previewPosition.y );
    cfg->Write( PreviewFrameWidthEntry, m_previewSize.GetWidth() );
    cfg->Write( PreviewFrameHeightEntry, m_previewSize.GetHeight() );

    // Save print dialog session settings.
    cfg->Write( PrintDialogPositionXEntry, m_printDialogPosition.x );
    cfg->Write( PrintDialogPositionYEntry, m_printDialogPosition.y );
    cfg->Write( PrintDialogWidthEntry, m_printDialogSize.GetWidth() );
    cfg->Write( PrintDialogHeightEntry, m_printDialogSize.GetHeight() );

    // Save netlists options:
    cfg->Write( SpiceNetNamesEntry,  g_OptNetListUseNames );
    cfg->Write( SimulatorCommandEntry, m_simulatorCommand );

    // Save find dialog session setting.
    cfg->Write( FindDialogPositionXEntry, m_findDialogPosition.x );
    cfg->Write( FindDialogPositionYEntry, m_findDialogPosition.y );
    cfg->Write( FindDialogWidthEntry, m_findDialogSize.GetWidth() );
    cfg->Write( FindDialogHeightEntry, m_findDialogSize.GetHeight() );
    wxASSERT_MSG( m_findReplaceData,
                  wxT( "Find dialog data settings object not created. Bad programmer!" ) );
    cfg->Write( FindReplaceFlagsEntry,
                (long) m_findReplaceData->GetFlags() & ~FR_REPLACE_ITEM_FOUND );
    cfg->Write( FindStringEntry, m_findReplaceData->GetFindString() );
    cfg->Write( ReplaceStringEntry, m_findReplaceData->GetReplaceString() );

    // Save the find and replace string history list.
    unsigned i;
    wxString tmpHistory;
    wxString entry;     // invoke constructor outside of any loops

    for( i = 0; i < m_findStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        entry.Printf( FindStringHistoryEntry, i );
        cfg->Write( entry, m_findStringHistoryList[ i ] );
    }

    for( i = 0; i < m_replaceStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        entry.Printf( ReplaceStringHistoryEntry, i );
        cfg->Write( entry, m_replaceStringHistoryList[ i ] );
    }

    // Save template fieldnames
    STRING_FORMATTER sf;

    m_TemplateFieldNames.Format( &sf, 0 );

    D(printf("saving formatted template fieldnames:'%s'\n", sf.GetString().c_str() );)

    wxString record = FROM_UTF8( sf.GetString().c_str() );
    record.Replace( wxT("\n"), wxT(""), true );   // strip all newlines
    record.Replace( wxT("  "), wxT(" "), true );  // double space to single

    cfg->Write( FieldNamesEntry, record );
}
