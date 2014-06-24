/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew_config.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <project.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <xnode.h>
#include <macros.h>
#include <pcbcommon.h>
#include <wxPcbStruct.h>
#include <class_board_design_settings.h>
#include <plot_common.h>
#include <worksheet.h>
#include <dialog_hotkeys_editor.h>
#include <fp_lib_table.h>
#include <fp_lib_table_lexer.h>
#include <worksheet_shape_builder.h>

#include <class_board.h>
#include <pcbplot.h>
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <hotkeys.h>
#include <pcbnew_config.h>
#include <module_editor_frame.h>
#include <modview_frame.h>

#include <invoke_pcb_dialog.h>
#include <dialog_mask_clearance.h>
#include <dialog_general_options.h>
#include <wildcards_and_files_ext.h>


void PCB_EDIT_FRAME::Process_Config( wxCommandEvent& event )
{
    int         id = event.GetId();
    wxFileName  fn;

    switch( id )
    {
    case ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG:
        m_show_layer_manager_tools = ! m_show_layer_manager_tools;
        m_auimgr.GetPane( wxT( "m_LayersManagerToolBar" ) ).Show( m_show_layer_manager_tools );
        m_auimgr.Update();

        GetMenuBar()->SetLabel( ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                                m_show_layer_manager_tools ?
                                _("Hide &Layers Manager" ) : _("Show &Layers Manager" ));
        break;

    case ID_MENU_PCB_SHOW_HIDE_MUWAVE_TOOLBAR:
        m_show_microwave_tools  = ! m_show_microwave_tools;
        m_auimgr.GetPane( wxT( "m_microWaveToolBar" ) ).Show( m_show_microwave_tools );
        m_auimgr.Update();

        GetMenuBar()->SetLabel( ID_MENU_PCB_SHOW_HIDE_MUWAVE_TOOLBAR,
                                m_show_microwave_tools ?
                                _( "Hide Microwave Toolbar" ): _( "Show Microwave Toolbar" ));
        break;


    case ID_PCB_LAYERS_SETUP:
        if( InvokeLayerSetup( this, GetBoard() ) )
        {
            LAYER_ID cur_layer = GetActiveLayer();

            // If after showing the dialog the user removed the active layer,
            // then use a sensible alternative layer to set as the active layer.
            if( !GetBoard()->GetEnabledLayers()[ cur_layer ] )
                cur_layer = F_Cu;

            SetActiveLayer( cur_layer, true );

            OnModify();
            ReCreateLayerBox();
            ReFillLayerWidget();
        }
        break;

    case ID_PCB_LIB_TABLE_EDIT:
        {
            bool tableChanged = false;
            int r = InvokePcbLibTableEditor( this, &GFootprintTable, Prj().PcbFootprintLibs() );

            if( r & 1 )
            {
                try
                {
                    FILE_OUTPUTFORMATTER sf( FP_LIB_TABLE::GetGlobalTableFileName() );

                    GFootprintTable.Format( &sf, 0 );
                    tableChanged = true;
                }
                catch( const IO_ERROR& ioe )
                {
                    wxString msg = wxString::Format( _(
                        "Error occurred saving the global footprint library "
                        "table:\n\n%s" ),
                        GetChars( ioe.errorText.GetData() )
                        );
                    wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
                }
            }

            // If no board file is defined, do not save the project specific library table.  It
            // is kept in memory and created in the path when the new board is saved.
            if( (r & 2) && !GetBoard()->GetFileName().IsEmpty() )
            {
                wxString    tblName   = Prj().FootprintLibTblName();

                try
                {
                    Prj().PcbFootprintLibs()->Save( tblName );
                    tableChanged = true;
                }
                catch( const IO_ERROR& ioe )
                {
                    wxString msg = wxString::Format( _(
                        "Error occurred saving project specific footprint library "
                        "table:\n\n%s" ),
                        GetChars( ioe.errorText )
                        );
                    wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
                }
            }

            FOOTPRINT_VIEWER_FRAME* viewer;

            if( tableChanged && (viewer = (FOOTPRINT_VIEWER_FRAME*)Kiway().Player( FRAME_PCB_MODULE_VIEWER, false )) != NULL )
            {
                viewer->ReCreateLibraryList();
            }
        }
        break;

    case ID_PCB_MASK_CLEARANCE:
        {
            DIALOG_PADS_MASK_CLEARANCE dlg( this );
            dlg.ShowModal();
        }
        break;

    case wxID_PREFERENCES:
        {
            DIALOG_GENERALOPTIONS dlg( this );
            dlg.ShowModal();
        }
        break;

    case ID_PCB_PAD_SETUP:
        InstallPadOptionsFrame( NULL );
        break;

    case ID_CONFIG_SAVE:
        SaveProjectSettings( true );
        break;

    case ID_CONFIG_READ:
        {
            fn = GetBoard()->GetFileName();
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
        }
        break;

    // Hotkey IDs
    case ID_PREFERENCES_HOTKEY_EXPORT_CONFIG:
        ExportHotkeyConfigToFile( g_Board_Editor_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_IMPORT_CONFIG:
        ImportHotkeyConfigFromFile( g_Board_Editor_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_EDITOR:
        InstallHotkeyFrame( this, g_Board_Editor_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        // Display current hotkey list for Pcbnew.
        DisplayHotkeyList( this, g_Board_Editor_Hokeys_Descr );
        break;

    // Macros IDs
    case ID_PREFRENCES_MACROS_SAVE:
        SaveMacros();
        break;

    case ID_PREFRENCES_MACROS_READ:
        ReadMacros();
        break;

    default:
        DisplayError( this, wxT( "PCB_EDIT_FRAME::Process_Config error" ) );
    }
}


bool PCB_EDIT_FRAME::LoadProjectSettings( const wxString& aProjectFileName )
{
    wxLogDebug( wxT( "Loading project '%s' settings." ), GetChars( aProjectFileName ) );

    wxFileName  fn = aProjectFileName;

    if( fn.GetExt() != ProjectFileExtension )
        fn.SetExt( ProjectFileExtension );

    // was: wxGetApp().ReadProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters(), false );
    Prj().ConfigLoad( Kiface().KifaceSearch(), fn.GetFullPath(), GROUP_PCB, GetProjectFileParameters(), false );

    // Dick 5-Feb-2012: I don't agree with this, the BOARD contents should dictate
    // what is visible or not, even initially.  And since PCB_EDIT_FRAME projects settings
    // have no control over what is visible (see PCB_EDIT_FRAME::GetProjectFileParameters())
    // this is recklessly turning on things the user may not want to see.
#if 0

    /* Reset the items visibility flag when loading a new configuration because it could
     * create SERIOUS mistakes for the user if board items are not visible after loading
     * a board.  Grid and ratsnest can be left to their previous state.
     */
    bool showGrid = IsElementVisible( GRID_VISIBLE );
    bool showRats = IsElementVisible( RATSNEST_VISIBLE );

    SetVisibleAlls();

    SetElementVisibility( GRID_VISIBLE, showGrid );
    SetElementVisibility( RATSNEST_VISIBLE, showRats );
#endif

    Prj().ElemClear( PROJECT::ELEM_FPTBL );      // Force it to be reloaded on demand.

    // Load the page layout decr file, from the filename stored in
    // BASE_SCREEN::m_PageLayoutDescrFileName, read in config project file
    // If empty, the default descr is loaded
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    pglayout.SetPageLayout( BASE_SCREEN::m_PageLayoutDescrFileName );

    return true;
}


void PCB_EDIT_FRAME::SaveProjectSettings( bool aAskForSave )
{
    wxFileName fn;

    fn = GetBoard()->GetFileName();
    fn.SetExt( ProjectFileExtension );

    if( aAskForSave )
    {
        wxFileDialog dlg( this, _( "Save Project File" ),
                          fn.GetPath(), fn.GetFullName(),
                          ProjectFileWildcard, wxFD_SAVE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();
    }

    Prj().ConfigSave( Kiface().KifaceSearch(), fn.GetFullPath(), GROUP_PCB, GetProjectFileParameters() );
}


PARAM_CFG_ARRAY PCB_EDIT_FRAME::GetProjectFileParameters()
{
    PARAM_CFG_ARRAY         pca;

    pca.push_back( new PARAM_CFG_FILENAME( wxT( "PageLayoutDescrFile" ),
                                          &BASE_SCREEN::m_PageLayoutDescrFileName ) );

    pca.push_back( new PARAM_CFG_FILENAME( wxT( "LastNetListRead" ), &m_lastNetListRead ) );

    pca.push_back( new PARAM_CFG_BOOL( wxT( "UseCmpFile" ), &m_useCmpFileForFpNames, true ) );
    GetBoard()->GetDesignSettings().AppendConfigs( &pca );

    return pca;
}


PARAM_CFG_ARRAY& PCB_EDIT_FRAME::GetConfigurationSettings()
{
    if( m_configSettings.empty() )
    {
        COLORS_DESIGN_SETTINGS cds;         // constructor fills this with sensible colors

        // Units used in dialogs and toolbars
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "Units" ),
                                                       (int*)&g_UserUnit, MILLIMETRES ) );

        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "DisplayPolarCoords" ),
                                                        &DisplayOpt.DisplayPolarCood, false ) );
        // Display options and modes:
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ViaHoleDisplayMode" ),
                                                       (int*) &DisplayOpt.m_DisplayViaMode,
                                                       VIA_SPECIAL_HOLE_SHOW, VIA_HOLE_NOT_SHOW,
                                                       OPT_VIA_HOLE_END - 1 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ShowNetNamesMode" ),
                                                       &DisplayOpt.DisplayNetNamesMode, 3, 0, 3 ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "DisplayTrackFilled" ),
                                                        &DisplayOpt.DisplayPcbTrackFill, true ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "TrackDisplayClearance" ),
                                                       (int*) &DisplayOpt.ShowTrackClearanceMode,
                                                       SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PadFill" ),
                                                        &DisplayOpt.DisplayPadFill, true ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "ViaFill" ),
                                                        &DisplayOpt.DisplayViaFill, true ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PadAffG" ),
                                                        &DisplayOpt.DisplayPadIsol, true ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PadSNum" ),
                                                        &DisplayOpt.DisplayPadNum, true ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ModAffC" ),
                                                       &DisplayOpt.DisplayModEdge, FILLED, 0, 2 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ModAffT" ),
                                                       &DisplayOpt.DisplayModText, FILLED, 0, 2 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "PcbAffT" ),
                                                       &DisplayOpt.DisplayDrawItems, FILLED, 0, 2 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "PcbShowZonesMode" ),
                                                       &DisplayOpt.DisplayZonesMode, 0, 0, 2 ) );

        // layer colors:
        for( int i = 0;  i<LAYER_ID_COUNT;  ++i )
        {
            wxString vn = wxString::Format( wxT( "ColorLayer%dEx" ), i );

            m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, vn, LOC_COLOR( i ), cds.m_LayersColors[i] ) );
        }

        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorTxtFrontEx" ),
                                                            ITEM_COLOR( MOD_TEXT_FR_VISIBLE ),
                                                            LIGHTGRAY ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorTxtBackEx" ),
                                                            ITEM_COLOR( MOD_TEXT_BK_VISIBLE ),
                                                            BLUE ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorTxtInvisEx" ),
                                                            ITEM_COLOR( MOD_TEXT_INVISIBLE ),
                                                            DARKGRAY ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorAnchorEx" ),
                                                            ITEM_COLOR( ANCHOR_VISIBLE ), BLUE ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorPadBackEx" ),
                                                            ITEM_COLOR( PAD_BK_VISIBLE ), GREEN ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorPadFrontEx" ),
                                                            ITEM_COLOR( PAD_FR_VISIBLE ), RED ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorViaThruEx" ),
                                                            ITEM_COLOR( VIA_THROUGH_VISIBLE ),
                                                            LIGHTGRAY ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorViaBBlindEx" ),
                                                            ITEM_COLOR( VIA_BBLIND_VISIBLE ),
                                                            BROWN ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorViaMicroEx" ),
                                                            ITEM_COLOR( VIA_MICROVIA_VISIBLE ),
                                                            CYAN ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorNonPlatedEx" ),
                                                            ITEM_COLOR( NON_PLATED_VISIBLE ),
                                                            YELLOW ) );
        m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true, wxT( "ColorRatsEx" ),
                                                            ITEM_COLOR( RATSNEST_VISIBLE ),
                                                            WHITE ) );

        // Miscellaneous:
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "RotationAngle" ), &m_rotationAngle,
                                                       900, 1, 900 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "MaxLnkS" ), &g_MaxLinksShowed,
                                                       3, 0, 15 ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "ShowMRa" ),
                                                        &g_Show_Module_Ratsnest, true ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "TwoSegT" ),
                                                        &g_TwoSegmentTrackBuild, true ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "SegmPcb45Only" )
                                                        , &g_Segments_45_Only, true ) );
    }

    return m_configSettings;
}


void PCB_EDIT_FRAME::SaveMacros()
{
    wxXmlDocument xml;
    wxXmlAttribute *macrosProp, *hkProp, *xProp, *yProp;
    wxString str, hkStr, xStr, yStr;

    wxFileName fn = GetBoard()->GetFileName();
    fn.SetExt( MacrosFileExtension );

    wxFileDialog dlg( this, _( "Save Macros File" ), fn.GetPath(), fn.GetFullName(),
                      MacrosFileWildcard, wxFD_SAVE | wxFD_CHANGE_DIR );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    XNODE *rootNode = new XNODE( wxXML_ELEMENT_NODE, wxT( "macrosrootnode" ), wxEmptyString );
    xml.SetRoot( rootNode );

    for( int number = 9; number >= 0; number-- )
    {
        str.Printf( wxT( "%d" ), number );
        macrosProp = new wxXmlAttribute( wxT( "number" ), str );

            XNODE * macrosNode = new XNODE( rootNode, wxXML_ELEMENT_NODE,
                                            wxT( "macros" ), wxEmptyString,
                                            macrosProp );

        for( std::list<MACROS_RECORD>::reverse_iterator i = m_Macros[number].m_Record.rbegin();
             i != m_Macros[number].m_Record.rend();
             i++ )
        {
            hkStr.Printf( wxT( "%d" ), i->m_HotkeyCode );
            xStr.Printf( wxT( "%d" ), i->m_Position.x );
            yStr.Printf( wxT( "%d" ), i->m_Position.y );

            yProp  = new wxXmlAttribute( wxT( "y" ), yStr );
            xProp  = new wxXmlAttribute( wxT( "x" ), xStr, yProp );
            hkProp = new wxXmlAttribute( wxT( "hkcode" ), hkStr, xProp );

            new XNODE( macrosNode, wxXML_ELEMENT_NODE, wxT( "hotkey" ),
                       wxEmptyString, hkProp );
        }
    }

    xml.SetFileEncoding( wxT( "UTF-8" ) );
    xml.Save( dlg.GetFilename() );
}


void PCB_EDIT_FRAME::ReadMacros()
{
    wxString str;
    wxFileName fn;

    fn = GetBoard()->GetFileName();
    fn.SetExt( MacrosFileExtension );

    wxFileDialog dlg( this, _( "Read Macros File" ), fn.GetPath(),
                      fn.GetFullName(), MacrosFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( !wxFileExists( dlg.GetPath() ) )
    {
        wxString msg;
        msg.Printf( _( "File %s not found" ), GetChars( dlg.GetPath() ) );
        DisplayError( this, msg );
        return;
    }

    wxXmlDocument xml;

    xml.SetFileEncoding( wxT( "UTF-8" ) );

    if( !xml.Load( dlg.GetFilename() ) )
            return;

    XNODE *macrosNode = (XNODE*) xml.GetRoot()->GetChildren();

    while( macrosNode )
    {
        int number = -1;

        if( macrosNode->GetName() == wxT( "macros" ) )
        {
            number = wxAtoi( macrosNode->GetAttribute( wxT( "number" ), wxT( "-1" ) ) );

            if( number >= 0  && number < 10 )
            {
                m_Macros[number].m_Record.clear();

                XNODE *hotkeyNode = (XNODE*) macrosNode->GetChildren();

                while( hotkeyNode )
                {
                    if( hotkeyNode->GetName() == wxT( "hotkey" ) )
                    {
                        int x = wxAtoi( hotkeyNode->GetAttribute( wxT( "x" ), wxT( "0" ) ) );
                        int y = wxAtoi( hotkeyNode->GetAttribute( wxT( "y" ), wxT( "0" ) ) );
                        int hk = wxAtoi( hotkeyNode->GetAttribute( wxT( "hkcode" ), wxT( "0" ) ) );

                        MACROS_RECORD macros_record;
                        macros_record.m_HotkeyCode = hk;
                        macros_record.m_Position.x = x;
                        macros_record.m_Position.y = y;
                        m_Macros[number].m_Record.push_back( macros_record );
                    }

                    hotkeyNode = (XNODE*) hotkeyNode->GetNext();
                }
            }
        }

        macrosNode = (XNODE*) macrosNode->GetNext();
    }
}
