/****************************************/
/** pcbnew_config.cpp : configuration  **/
/****************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "pcbplot.h"
#include "worksheet.h"
#include "pcbnew_id.h"
#include "hotkeys.h"
#include "protos.h"

#include "dialog_mask_clearance.h"
#include "dialog_general_options.h"

#include "pcbnew_config.h"

#define HOTKEY_FILENAME wxT( "pcbnew" )


void WinEDA_PcbFrame::Process_Config( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxPoint    pos;

    wxFileName fn;

    pos    = GetPosition();
    pos.x += 20;
    pos.y += 20;

    switch( id )
    {
    case ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG:
        if( m_OptionsToolBar )
        {   //This command is same as the Options Vertical Toolbar
            // tool Show/hide layers manager
            bool state =
                m_OptionsToolBar->GetToolState( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR );
            m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR,
                                          !state );
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED,
                                  ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR );
            wxPostEvent( this, event );
        }
        break;

    case ID_PCB_LAYERS_SETUP:
        DisplayDialogLayerSetup( this );
        break;

    case ID_CONFIG_REQ:
        InstallConfigFrame( pos );
        break;

    case ID_PCB_MASK_CLEARANCE:
        {
            DIALOG_PADS_MASK_CLEARANCE dlg( this );
            dlg.ShowModal();
        }
        break;

    case ID_OPTIONS_SETUP:
        {
            Dialog_GeneralOptions dlg( this );
            dlg.ShowModal();
        }
        break;

    case ID_PCB_PAD_SETUP:
        InstallPadOptionsFrame( NULL );
        break;

    case ID_CONFIG_SAVE:
        SaveProjectSettings();
        break;

    case ID_CONFIG_READ:
    {
        fn = GetScreen()->m_FileName;
        fn.SetExt( ProjectFileExtension );

        wxFileDialog dlg( this, _( "Read Project File" ), fn.GetPath(),
                          fn.GetFullName(), ProjectFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            break;

        if( !wxFileExists( dlg.GetPath() ) )
        {
            wxString msg;
            msg.Printf( _( "File %s not found" ), GetChars( dlg.GetPath() ) );
            DisplayError( this, msg );
            break;
        }

        LoadProjectSettings( dlg.GetPath() );
        break;
    }
    case ID_PREFERENCES_HOTKEY_CREATE_CONFIG:
        fn.SetPath( ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice ) );
        fn.SetName( HOTKEY_FILENAME );
        fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );
        WriteHotkeyConfigFile( fn.GetFullPath(), s_Pcbnew_Editor_Hokeys_Descr, true );
        break;

    case ID_PREFERENCES_HOTKEY_READ_CONFIG:
        Read_Hotkey_Config( this, true );
        break;

    case ID_PREFERENCES_HOTKEY_EDIT_CONFIG:
    {
        fn.SetPath( ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice ) );
        fn.SetName( HOTKEY_FILENAME );
        fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );

        wxString editorname = wxGetApp().GetEditorName();
        if( !editorname.IsEmpty() )
            ExecuteFile( this, editorname, QuoteFullPath( fn ) );
        break;
    }

    case ID_PREFERENCES_HOTKEY_PATH_IS_HOME:
    case ID_PREFERENCES_HOTKEY_PATH_IS_KICAD:
        HandleHotkeyConfigMenuSelection( this, id );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        DisplayHotkeyList( this, s_Board_Editor_Hokeys_Descr );
        break;

    default:
        DisplayError( this, wxT( "WinEDA_PcbFrame::Process_Config internal error" ) );
    }
}


/*
 * Read the hotkey files config for pcbnew and module_edit
 */
bool Read_Hotkey_Config( WinEDA_DrawFrame* frame, bool verbose )
{
    wxString FullFileName = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );

    FullFileName += HOTKEY_FILENAME;
    FullFileName += wxT( "." );
    FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
    return frame->ReadHotkeyConfigFile( FullFileName, s_Pcbnew_Editor_Hokeys_Descr, verbose );
}


/**
 * Read the project configuration file settings.
 *
 * @param aProjectFileName = The project file name to load.  If aProjectFileName
 *                           is not found load the default project file kicad.pro
 *                           and initialize setting to their default value.
 * @return Always returns true.
 */
bool WinEDA_PcbFrame::LoadProjectSettings( const wxString& aProjectFileName )
{
    wxFileName fn = aProjectFileName;

    if( fn.GetExt() != ProjectFileExtension )
        fn.SetExt( ProjectFileExtension );

    wxGetApp().RemoveLibraryPath( g_UserLibDirBuffer );

    /* Initialize default values. */
    g_LibName_List.Clear();

    wxGetApp().ReadProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters(), FALSE );

    /* User library path takes precedent over default library search paths. */
    wxGetApp().InsertLibraryPath( g_UserLibDirBuffer, 1 );

    /* Reset the items visibility flag when loading a new config
     *  Because it could creates SERIOUS mistakes for the user,
     * if board items are not visible after loading a board...
     * Grid and ratsnest can be left to their previous state
     */
    bool showGrid = IsElementVisible( GRID_VISIBLE );
    bool showRats = IsElementVisible( RATSNEST_VISIBLE );
    SetVisibleAlls();
    SetElementVisibility( GRID_VISIBLE, showGrid );
    SetElementVisibility( RATSNEST_VISIBLE, showRats );
    return TRUE;
}


void WinEDA_PcbFrame::SaveProjectSettings()
{
    wxFileName fn;

    fn = GetScreen()->m_FileName;
    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(), fn.GetFullName(),
                      ProjectFileWildcard, wxFD_SAVE | wxFD_CHANGE_DIR );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxGetApp().WriteProjectConfig( dlg.GetPath(), GROUP, GetProjectFileParameters() );
}


/**
 * Return project file parameter list for PCBNew.
 *
 * Populate the project file parameter array specific to PCBNew if it hasn't
 * already been populated and return a reference to the array to the caller.
 * Creating the parameter list at run time has the advantage of being able
 * to define local variables.  The old method of statically building the array
 * at compile time requiring global variable definitions by design.
 */
PARAM_CFG_ARRAY& WinEDA_PcbFrame::GetProjectFileParameters()
{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_WXSTRING( wxT( "LibDir" ),&g_UserLibDirBuffer,
                                                           GROUPLIB ) );
    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "LibName" ), &g_LibName_List,
                                                               GROUPLIB ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "PadDrlX" ), &g_Pad_Master.m_Drill.x,
                                                      320, 0, 0x7FFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "PadDimH" ), &g_Pad_Master.m_Size.x,
                                                      550, 0, 0x7FFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "PadDimV" ), &g_Pad_Master.m_Size.y,
                                                      550, 0, 0x7FFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "BoardThickness" ),
                                                      &boardDesignSettings.m_BoardThickness,
                                                      630, 0, 0xFFFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_BOOL( wxT( "SgPcb45" ), &Segments_45_Only,
                                                       TRUE ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "TxtPcbV" ),
                                                      &boardDesignSettings.m_PcbTextSize.y,
                                                      600, TEXTS_MIN_SIZE, TEXTS_MAX_SIZE ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "TxtPcbH" ),
                                                      &boardDesignSettings.m_PcbTextSize.x,
                                                      600, TEXTS_MIN_SIZE, TEXTS_MAX_SIZE ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "TxtModV" ), &g_ModuleTextSize.y,
                                                      500, TEXTS_MIN_SIZE, TEXTS_MAX_SIZE ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "TxtModH" ), &g_ModuleTextSize.x,
                                                      500, TEXTS_MIN_SIZE, TEXTS_MAX_SIZE ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "TxtModW" ), &g_ModuleTextWidth,
                                                      100, 1, TEXTS_MAX_WIDTH ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "VEgarde" ),
                                                      &boardDesignSettings.m_SolderMaskMargin,
                                                      100, 0, 10000 ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "DrawLar" ),
                                                      &boardDesignSettings.m_DrawSegmentWidth,
                                                      120, 0, 0xFFFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "EdgeLar" ),
                                                      &boardDesignSettings.m_EdgeSegmentWidth,
                                                      120, 0, 0xFFFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "TxtLar" ),
                                                      &boardDesignSettings.m_PcbTextWidth,
                                                      120, 0, 0xFFFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "MSegLar" ), &g_ModuleSegmentWidth,
                                                      120, 0, 0xFFFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_WXSTRING( wxT( "LastNetListRead" ),
                                                           &m_lastNetListRead ) );
    return m_projectFileParams;
}



/*
 * Return the PCBNew applications settings list.
 *
 * This replaces the old statically define list that had the project
 * file settings and the application settings mixed together.  This
 * was confusing and caused some settings to get saved and loaded
 * incorrectly.  Currently, only the settings that are needed at start
 * up by the main window are defined here.  There are other locally used
 * settings are scattered throughout the PCBNew source code.  If you need
 * to define a configuration setting that need to be loaded at run time,
 * this is the place to define it.
 *
 * @todo: Define the configuration variables as member variables instead of
 *        global variables or move them to the object class where they are
 *        used.
 */
PARAM_CFG_ARRAY& WinEDA_PcbFrame::GetConfigurationSettings()
{
    if( !m_configSettings.empty() )
        return m_configSettings;

    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ViaSHole" ),
                                                   &DisplayOpt.m_DisplayViaMode,
                                                   VIA_SPECIAL_HOLE_SHOW, VIA_HOLE_NOT_SHOW,
                                                   OPT_VIA_HOLE_END - 1 ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ShowNetNamesMode" ),
                                                   &DisplayOpt.DisplayNetNamesMode, 3, 0, 3 ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "Unite" ), (int*)&g_UserUnit, MILLIMETRES ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "SegFill" ),
                                                    &DisplayOpt.DisplayPcbTrackFill, TRUE ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "TrackDisplayClearance" ),
                                                   &DisplayOpt.ShowTrackClearanceMode,
                                                   SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PadFill" ),
                                                    &DisplayOpt.DisplayPadFill, TRUE ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "ViaFill" ),
                                                    &DisplayOpt.DisplayViaFill, TRUE ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PadAffG" ),
                                                    &DisplayOpt.DisplayPadIsol, TRUE ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PadSNum" ),
                                                    &DisplayOpt.DisplayPadNum, TRUE ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ModAffC" ),
                                                   &DisplayOpt.DisplayModEdge, FILLED, 0, 2 ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ModAffT" ),
                                                   &DisplayOpt.DisplayModText, FILLED, 0, 2 ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "PcbAffT" ),
                                                   &DisplayOpt.DisplayDrawItems, FILLED, 0, 2 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay0" ), LOC_COLOR( 0 ),
                                                        GREEN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay1" ), LOC_COLOR( 1 ),
                                                        BLUE ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay2" ), LOC_COLOR( 2 ),
                                                        LIGHTGRAY ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay3" ), LOC_COLOR( 3 ),
                                                        5 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay4" ), LOC_COLOR( 4 ),
                                                        4 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay5" ), LOC_COLOR( 5 ),
                                                        5 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay6" ), LOC_COLOR( 6 ),
                                                        6 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay7" ), LOC_COLOR( 7 ),
                                                        5 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay8" ), LOC_COLOR( 8 ),
                                                        7 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLay9" ), LOC_COLOR( 9 ),
                                                        1 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayA" ), LOC_COLOR( 10 ),
                                                        2 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayB" ), LOC_COLOR( 11 ),
                                                        3 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayC" ), LOC_COLOR( 12 ),
                                                        12 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayD" ), LOC_COLOR( 13 ),
                                                        13 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayE" ), LOC_COLOR( 14 ),
                                                        14 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayF" ), LOC_COLOR( 15 ),
                                                        RED ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayG" ), LOC_COLOR( 16 ),
                                                        1 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayH" ), LOC_COLOR( 17 ),
                                                        5 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayI" ), LOC_COLOR( 18 ),
                                                        11 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayJ" ), LOC_COLOR( 19 ),
                                                        4 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayK" ), LOC_COLOR( 20 ),
                                                        5 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayL" ), LOC_COLOR( 21 ),
                                                        3 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayM" ), LOC_COLOR( 22 ),
                                                        6 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayN" ), LOC_COLOR( 23 ),
                                                        5 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayO" ), LOC_COLOR( 24 ),
                                                        LIGHTGRAY ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayP" ), LOC_COLOR( 25 ),
                                                        1 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayQ" ), LOC_COLOR( 26 ),
                                                        2 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayR" ), LOC_COLOR( 27 ),
                                                        14 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayS" ), LOC_COLOR( 28 ),
                                                        YELLOW ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayT" ), LOC_COLOR( 29 ),
                                                        13 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayU" ), LOC_COLOR( 30 ),
                                                        14 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColLayV" ), LOC_COLOR( 31 ),
                                                        7 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CTxtMoC" ),
                                                        ITEM_COLOR( MOD_TEXT_FR_VISIBLE ),
                                                        LIGHTGRAY ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CTxtMoS" ),
                                                        ITEM_COLOR( MOD_TEXT_BK_VISIBLE ),
                                                        BLUE ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CTxtVis" ),
                                                        ITEM_COLOR( MOD_TEXT_INVISIBLE ),
                                                        DARKGRAY ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CAncreM" ),
                                                        ITEM_COLOR( ANCHOR_VISIBLE ), BLUE ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CoPadCu" ),
                                                        ITEM_COLOR( PAD_BK_VISIBLE ), GREEN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CoPadCm" ),
                                                        ITEM_COLOR( PAD_FR_VISIBLE ), RED ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CoViaTh" ),
                                                        ITEM_COLOR( VIA_THROUGH_VISIBLE ),
                                                        LIGHTGRAY ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CoViaBu" ),
                                                        ITEM_COLOR( VIA_BBLIND_VISIBLE ),
                                                        BROWN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CoViaMi" ),
                                                        ITEM_COLOR( VIA_MICROVIA_VISIBLE ),
                                                        CYAN ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "CoRatsN" ),
                                                        ITEM_COLOR( RATSNEST_VISIBLE ),
                                                        WHITE ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "HPGLnum" ),
                                                   &g_pcb_plot_options.HPGL_Pen_Num,
                                                   1, 1, 16 ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "HPGdiam" ),
                                                   &g_pcb_plot_options.HPGL_Pen_Diam,
                                                   15, 0, 100 ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "HPGLSpd" ),
                                                   &g_pcb_plot_options.HPGL_Pen_Speed,
                                                   20, 0, 1000 ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "HPGLrec" ),
                                                   &g_pcb_plot_options.HPGL_Pen_Recouvrement,
                                                   2, 0, 0x100 ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "TimeOut" ), &g_TimeOut,
                                                   600, 0, 60000 ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "DPolair" ),
                                                    &DisplayOpt.DisplayPolarCood, FALSE ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "MaxLnkS" ), &g_MaxLinksShowed,
                                                   3, 0, 15 ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "ShowMRa" ),
                                                    &g_Show_Module_Ratsnest, TRUE ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "TwoSegT" ),
                                                    &g_TwoSegmentTrackBuild, TRUE ) );

    return m_configSettings;
}
