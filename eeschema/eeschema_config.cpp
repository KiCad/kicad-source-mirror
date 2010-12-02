/******************/
/** eeconfig.cpp **/
/******************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "wxEeschemaStruct.h"

#include "eeschema_id.h"
#include "general.h"
#include "netlist.h"
#include "protos.h"
#include "libeditframe.h"
#include "eeschema_config.h"
#include "worksheet.h"
#include "hotkeys.h"
#include "sch_sheet.h"

#include "dialog_hotkeys_editor.h"

#include "dialogs/dialog_color_config.h"
#include "dialogs/dialog_eeschema_config.h"
#include "dialogs/dialog_eeschema_options.h"

#include <wx/fdrepdlg.h>


#define HOTKEY_FILENAME         wxT( "eeschema" )

#define FR_HISTORY_LIST_CNT     10   ///< Maximum number of find and replace strings.


void LIB_EDIT_FRAME::InstallConfigFrame( wxCommandEvent& event )
{
    DIALOG_EESCHEMA_CONFIG CfgFrame( (WinEDA_SchematicFrame *)GetParent(), this );

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
    WinEDA_SchematicFrame * schFrame = ( WinEDA_SchematicFrame * ) GetParent();

    switch( id )
    {
    case ID_CONFIG_SAVE:
        schFrame->SaveProjectFile( this, false );
        break;

    case ID_CONFIG_READ:
    {
        fn = g_RootSheet->m_AssociatedScreen->m_FileName;
        fn.SetExt( ProjectFileExtension );

        wxFileDialog dlg( this, _( "Read Project File" ), fn.GetPath(),
                          fn.GetFullName(), ProjectFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            break;

        schFrame->LoadProjectFile( dlg.GetPath(), TRUE );
    }
    break;


    /* Hotkey IDs */
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


void WinEDA_SchematicFrame::OnColorConfig( wxCommandEvent& aEvent )
{
    DIALOG_COLOR_CONFIG dlg( this );

    dlg.ShowModal();
}


void WinEDA_SchematicFrame::InstallConfigFrame( wxCommandEvent& event )
{
    DIALOG_EESCHEMA_CONFIG CfgFrame( this, this );

    CfgFrame.ShowModal();
}


void WinEDA_SchematicFrame::Process_Config( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxFileName fn;

    switch( id )
    {
    case ID_CONFIG_SAVE:
        SaveProjectFile( this, false );
        break;

    case ID_CONFIG_READ:
    {
        fn = g_RootSheet->m_AssociatedScreen->m_FileName;
        fn.SetExt( ProjectFileExtension );

        wxFileDialog dlg( this, _( "Read Project File" ), fn.GetPath(),
                          fn.GetFullName(), ProjectFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            break;

        LoadProjectFile( dlg.GetPath(), TRUE );
    }
    break;


    /* Hotkey IDs */
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
        DisplayError( this, wxT( "WinEDA_SchematicFrame::Process_Config error" ) );
    }
}


void WinEDA_SchematicFrame::OnSetOptions( wxCommandEvent& event )
{
    wxArrayString units;
    GridArray& grid_list = GetBaseScreen()->m_GridList;

    DIALOG_EESCHEMA_OPTIONS dlg( this );

    wxLogDebug( wxT( "Current grid array index %d." ),
                grid_list.Index( GetBaseScreen()->GetGrid() ) );

    units.Add( GetUnitsLabel( INCHES ) );
    units.Add( GetUnitsLabel( MILLIMETRES ) );

    dlg.SetUnits( units, g_UserUnit );
    dlg.SetGridSizes( grid_list, GetBaseScreen()->GetGridId() );
    dlg.SetLineWidth( g_DrawDefaultLineThickness );
    dlg.SetTextSize( g_DefaultTextLabelSize );
    dlg.SetRepeatHorizontal( g_RepeatStep.x );
    dlg.SetRepeatVertical( g_RepeatStep.y );
    dlg.SetRepeatLabel( g_RepeatDeltaLabel );
    dlg.SetShowGrid( IsGridVisible() );
    dlg.SetShowHiddenPins( m_ShowAllPins );
    dlg.SetEnableAutoPan( DrawPanel->m_AutoPAN_Enable );
    dlg.SetEnableHVBusOrientation( g_HVLines );
    dlg.SetShowPageLimits( g_ShowPageLimits );
    dlg.Layout();
    dlg.Fit();
    dlg.SetMinSize( dlg.GetSize() );

    const TEMPLATE_FIELDNAMES&  tfnames = m_TemplateFieldNames.GetTemplateFieldNames();

    for( unsigned i=0; i<tfnames.size(); ++i )
    {
        D(printf("dlg.SetFieldName(%d, '%s')\n",
            i, CONV_TO_UTF8( tfnames[i].m_Name) );)

        dlg.SetFieldName( i, tfnames[i].m_Name );
    }

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    g_UserUnit = (UserUnitType)dlg.GetUnitsSelection();

    GetBaseScreen()->SetGrid(
        grid_list[ (size_t) dlg.GetGridSelection() ].m_Size );

    g_DrawDefaultLineThickness = dlg.GetLineWidth();
    g_DefaultTextLabelSize = dlg.GetTextSize();
    g_RepeatStep.x = dlg.GetRepeatHorizontal();
    g_RepeatStep.y = dlg.GetRepeatVertical();
    g_RepeatDeltaLabel = dlg.GetRepeatLabel();
    SetGridVisibility( dlg.GetShowGrid() );
    m_ShowAllPins = dlg.GetShowHiddenPins();
    DrawPanel->m_AutoPAN_Enable = dlg.GetEnableAutoPan();
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

    DrawPanel->Refresh( true );
}


/**
 * Return project file parameter list for EESchema.
 *
 * Populate the project file parameter array specific to EESchema if it hasn't
 * already been populated and return a reference to the array to the caller.
 * Creating the parameter list at run time has the advantage of being able
 * to define local variables.  The old method of statically building the array
 * at compile time requiring global variable definitions.
 */
PARAM_CFG_ARRAY& WinEDA_SchematicFrame::GetProjectFileParameters( void )
{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_WXSTRING( wxT( "LibDir" ),
                                                           &m_UserLibraryPath ) );
    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "LibName" ),
                                                               &m_ComponentLibFiles,
                                                               GROUPLIB ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "NetFmt" ),
                                                      &m_NetlistFormat,
                                                      NET_TYPE_PCBNEW,
                                                      NET_TYPE_PCBNEW,
                                                      NET_TYPE_CUSTOM_MAX ) );

    /* NOTE: Left as global until supporting code  can be fixed. */
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "HPGLSpd" ),
                                                      &g_HPGL_Pen_Descr.m_Pen_Speed,
                                                      20, 2, 45 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "HPGLDm" ),
                                                      &g_HPGL_Pen_Descr.m_Pen_Diam,
                                                      15, 1, 150 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "HPGLNum" ),
                                                      &g_HPGL_Pen_Descr.m_Pen_Num,
                                                      1, 1, 8 ) );
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


/*
 * Load the Kicad project file (*.pro) settings specific to EESchema.
 */
bool WinEDA_SchematicFrame::LoadProjectFile( const wxString& CfgFileName,
                                             bool ForceRereadConfig )
{
    wxFileName              fn;
    bool                    IsRead = TRUE;
    wxArrayString           liblist_tmp = m_ComponentLibFiles;

    if( CfgFileName.IsEmpty() )
        fn = g_RootSheet->m_AssociatedScreen->m_FileName;
    else
        fn = CfgFileName;

    m_ComponentLibFiles.Clear();

    /* Change the schematic file extension (.sch) to the project file
     * extension (.pro). */
    fn.SetExt( ProjectFileExtension );

    wxGetApp().RemoveLibraryPath( m_UserLibraryPath );

    if( !wxGetApp().ReadProjectConfig( fn.GetFullPath(), GROUP,
                                       GetProjectFileParameters(),
                                       ForceRereadConfig ? FALSE : TRUE ) )
    {
        m_ComponentLibFiles = liblist_tmp;
        IsRead = FALSE;
    }

    /* User library path takes precedent over default library search paths. */
    wxGetApp().InsertLibraryPath( m_UserLibraryPath, 1 );

    /* If the list is void, force loadind the library "power.lib" that is
     * the "standard" library for power symbols.
     */
    if( m_ComponentLibFiles.GetCount() == 0 )
        m_ComponentLibFiles.Add( wxT( "power" ) );

    LoadLibraries();
    GetBaseScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    return IsRead;
}


/*
 * Save the Kicad project file (*.pro) settings specific to EESchema.
 */
void WinEDA_SchematicFrame::SaveProjectFile( wxWindow* displayframe, bool askoverwrite )
{
    wxFileName fn;

    fn = g_RootSheet->m_AssociatedScreen->m_FileName  /*ConfigFileName*/;
    fn.SetExt( ProjectFileExtension );

    int options = wxFD_SAVE;
    if( askoverwrite )
        options |= wxFD_OVERWRITE_PROMPT;
    wxFileDialog dlg( displayframe, _( "Save Project Settings" ), wxGetCwd(),
                      fn.GetFullName(), ProjectFileWildcard, options );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxGetApp().WriteProjectConfig( dlg.GetPath(), GROUP,
                                   GetProjectFileParameters() );
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

/*
 * Return the EESchema applications settings list.
 *
 * This replaces the old statically define list that had the project
 * file settings and the application settings mixed together.  This
 * was confusing and caused some settings to get saved and loaded
 * incorrectly.  Currently, only the settings that are needed at start
 * up by the main window are defined here.  There are other locally used
 * settings scattered thoughout the EESchema source code.  If you need
 * to define a configuration setting that need to be loaded at run time,
 * this is the place to define it.
 *
 * TODO: Define the configuration variables as member variables instead of
 *       global variables or move them to the object class where they are
 *       used.
 */
PARAM_CFG_ARRAY& WinEDA_SchematicFrame::GetConfigurationSettings( void )
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
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PrintMonochrome" ),
                                                        &m_printMonochrome, true ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PrintSheetReferenceAndTitleBlock" ),
                                                        &m_printSheetReference, true ) );

    return m_configSettings;
}


/*
 * Load the EESchema configuration parameters.
 */
void WinEDA_SchematicFrame::LoadSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    long tmp;

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    WinEDA_DrawFrame::LoadSettings();

    wxGetApp().ReadCurrentSetupValues( GetConfigurationSettings() );

    g_DrawDefaultLineThickness = cfg->Read( DefaultDrawLineWidthEntry,
                                            (long) 6 );
    cfg->Read( ShowHiddenPinsEntry, &m_ShowAllPins, false );
    cfg->Read( HorzVertLinesOnlyEntry, &g_HVLines, true );

    /* Load print preview window session settings. */
    cfg->Read( PreviewFramePositionXEntry, &tmp, -1 );
    m_previewPosition.x = (int) tmp;
    cfg->Read( PreviewFramePositionYEntry, &tmp, -1 );
    m_previewPosition.y = (int) tmp;
    cfg->Read( PreviewFrameWidthEntry, &tmp, -1 );
    m_previewSize.SetWidth( (int) tmp );
    cfg->Read( PreviewFrameHeightEntry, &tmp, -1 );
    m_previewSize.SetHeight( (int) tmp );

    /* Load print dialog session settings. */
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
    cfg->Read( SimulatorCommandEntry, &g_SimulatorCommandLine );

    /* Load find dialog session setting. */
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
    m_findReplaceData->SetFlags( (wxUint32) tmp );
    m_findReplaceData->SetFindString( cfg->Read( FindStringEntry, wxEmptyString ) );
    m_findReplaceData->SetReplaceString( cfg->Read( ReplaceStringEntry, wxEmptyString ) );

    /* Load the find and replace string history list. */
    for ( size_t i = 0; i < FR_HISTORY_LIST_CNT; i++ )
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
        TEMPLATE_FIELDNAMES_LEXER  lexer( CONV_TO_UTF8( templateFieldNames ) );
        try
        {
            m_TemplateFieldNames.Parse( &lexer );
        }
        catch( IO_ERROR e )
        {
            // @todo show error msg
            D(printf("templatefieldnames parsing error: '%s'\n",
                CONV_TO_UTF8(e.errorText) );)
        }
    }

}


/*
 * Save the EESchema configuration parameters.
 */
void WinEDA_SchematicFrame::SaveSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    WinEDA_DrawFrame::SaveSettings();

    wxGetApp().SaveCurrentSetupValues( GetConfigurationSettings() );

    cfg->Write( DefaultDrawLineWidthEntry, (long) g_DrawDefaultLineThickness );
    cfg->Write( ShowHiddenPinsEntry, m_ShowAllPins );
    cfg->Write( HorzVertLinesOnlyEntry, g_HVLines );

    /* Save print preview window session settings. */
    cfg->Write( PreviewFramePositionXEntry, m_previewPosition.x );
    cfg->Write( PreviewFramePositionYEntry, m_previewPosition.y );
    cfg->Write( PreviewFrameWidthEntry, m_previewSize.GetWidth() );
    cfg->Write( PreviewFrameHeightEntry, m_previewSize.GetHeight() );

    /* Save print dialog session settings. */
    cfg->Write( PrintDialogPositionXEntry, m_printDialogPosition.x );
    cfg->Write( PrintDialogPositionYEntry, m_printDialogPosition.y );
    cfg->Write( PrintDialogWidthEntry, m_printDialogSize.GetWidth() );
    cfg->Write( PrintDialogHeightEntry, m_printDialogSize.GetHeight() );

    // Save netlists options:
    cfg->Write( SpiceNetNamesEntry,  g_OptNetListUseNames );
    cfg->Write( SimulatorCommandEntry, g_SimulatorCommandLine );

    /* Save find dialog session setting. */
    cfg->Write( FindDialogPositionXEntry, m_findDialogPosition.x );
    cfg->Write( FindDialogPositionYEntry, m_findDialogPosition.y );
    cfg->Write( FindDialogWidthEntry, m_findDialogSize.GetWidth() );
    cfg->Write( FindDialogHeightEntry, m_findDialogSize.GetHeight() );
    wxASSERT_MSG( m_findReplaceData,
                  wxT( "Find dialog data settings object not created. Bad programmer!" ) );
    cfg->Write( FindReplaceFlagsEntry, (long) m_findReplaceData->GetFlags() );
    cfg->Write( FindStringEntry, m_findReplaceData->GetFindString() );
    cfg->Write( ReplaceStringEntry, m_findReplaceData->GetReplaceString() );

    /* Save the find and replace string history list. */
    size_t i;
    wxString tmpHistory;
    wxString entry;     // invoke constructor outside of any loops

    for ( i = 0; i < m_findStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        entry.Printf( FindStringHistoryEntry, i );
        cfg->Write( entry, m_findStringHistoryList[ i ] );
    }

    for ( i = 0; i < m_replaceStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        entry.Printf( ReplaceStringHistoryEntry, i );
        cfg->Write( entry, m_replaceStringHistoryList[ i ] );
    }

    // Save template fieldnames
    STRING_FORMATTER sf;

    m_TemplateFieldNames.Format( &sf, 0 );

    wxString record = CONV_FROM_UTF8( sf.GetString().c_str() );
    record.Replace( wxT("\n"), wxT(""), true );   // strip all newlines
    record.Replace( wxT("  "), wxT(" "), true );  // double space to single


    cfg->Write( FieldNamesEntry, record );
}

