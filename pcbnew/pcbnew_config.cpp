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
#include <pcb_draw_panel_gal.h>
#include <confirm.h>
#include <gestfich.h>
#include <xnode.h>
#include <common.h>
#include <macros.h>
#include <wxPcbStruct.h>
#include <class_board_design_settings.h>
#include <plot_common.h>
#include <worksheet.h>
#include <dialog_hotkeys_editor.h>
#include <fp_lib_table.h>
#include <fp_lib_table_lexer.h>
#include <worksheet_shape_builder.h>

#include <class_board.h>
#include <class_module.h>
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

            SetActiveLayer( cur_layer );

            OnModify();
            ReCreateLayerBox();
            ReFillLayerWidget();

            if( IsGalCanvasActive() )
                static_cast<PCB_DRAW_PANEL_GAL*>( GetGalCanvas() )->SyncLayersVisibility( GetBoard() );
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

            if( dlg.ShowModal() == 1 && IsGalCanvasActive() )
            {
                for( MODULE* module = GetBoard()->m_Modules; module; module = module->Next() )
                    module->ViewUpdate();

                GetGalCanvas()->Refresh();
            }
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
                wxString msg = wxString::Format( _(
                        "File %s not found" ),
                        GetChars( dlg.GetPath() )
                        );
                DisplayError( this, msg );
                break;
            }

            wxString pro_file = dlg.GetPath();

            Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_PCB, GetProjectFileParameters(), pro_file );
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


bool PCB_EDIT_FRAME::LoadProjectSettings()
{
    wxLogDebug( wxT( "Loading project '%s' settings." ),
            GetChars( Prj().GetProjectFullName() ) );

    bool rc = Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_PCB, GetProjectFileParameters() );

    // Load the page layout decr file, from the filename stored in
    // BASE_SCREEN::m_PageLayoutDescrFileName, read in config project file
    // If empty, or not existing, the default descr is loaded
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    wxString pg_fullfilename = WORKSHEET_LAYOUT::MakeFullFileName(
                                    BASE_SCREEN::m_PageLayoutDescrFileName,
                                    Prj().GetProjectPath() );

    pglayout.SetPageLayout( pg_fullfilename );

    return rc;
}


void PCB_EDIT_FRAME::SaveProjectSettings( bool aAskForSave )
{
    wxFileName fn = Prj().GetProjectFullName();

    if( aAskForSave )
    {
        wxFileDialog dlg( this, _( "Save Project File" ),
                          fn.GetPath(), fn.GetFullName(),
                          ProjectFileWildcard, wxFD_SAVE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();
    }

    wxString pro_name = fn.GetFullPath();

    Prj().ConfigSave( Kiface().KifaceSearch(), GROUP_PCB, GetProjectFileParameters(), pro_name );
}


PARAM_CFG_ARRAY PCB_EDIT_FRAME::GetProjectFileParameters()
{
    PARAM_CFG_ARRAY pca;

    // This one cannot be cached because some settings are going to/from the BOARD,
    // so pointers into that cannot be saved for long.

    pca.push_back( new PARAM_CFG_FILENAME( wxT( "PageLayoutDescrFile" ),
                                          &BASE_SCREEN::m_PageLayoutDescrFileName ) );

    pca.push_back( new PARAM_CFG_FILENAME( wxT( "LastNetListRead" ), &m_lastNetListRead ) );

    pca.push_back( new PARAM_CFG_BOOL( wxT( "UseCmpFile" ), &m_useCmpFileForFpNames, true ) );
    GetBoard()->GetDesignSettings().AppendConfigs( &pca );

    return pca;
}


PARAM_CFG_ARRAY& PCB_EDIT_FRAME::GetConfigurationSettings()
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();

    if( m_configSettings.empty() )
    {
        COLORS_DESIGN_SETTINGS cds;         // constructor fills this with sensible colors

        // Units used in dialogs and toolbars
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "Units" ),
                                                       (int*)&g_UserUnit, MILLIMETRES ) );

        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "DisplayPolarCoords" ),
                                                        &displ_opts->m_DisplayPolarCood, false ) );
        // Display options and modes:
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ViaHoleDisplayMode" ),
                                                       (int*) &displ_opts->m_DisplayViaMode,
                                                       VIA_SPECIAL_HOLE_SHOW, VIA_HOLE_NOT_SHOW,
                                                       OPT_VIA_HOLE_END - 1 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ShowNetNamesMode" ),
                                                       &displ_opts->m_DisplayNetNamesMode, 3, 0, 3 ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "DisplayTrackFilled" ),
                                                        &displ_opts->m_DisplayPcbTrackFill, true ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "TrackDisplayClearance" ),
                                                       (int*) &displ_opts->m_ShowTrackClearanceMode,
                                                       SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PadFill" ),
                                                        &displ_opts->m_DisplayPadFill, true ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "ViaFill" ),
                                                        &displ_opts->m_DisplayViaFill, true ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PadAffG" ),
                                                        &displ_opts->m_DisplayPadIsol, true ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "PadSNum" ),
                                                        &displ_opts->m_DisplayPadNum, true ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ModAffC" ),
                                                       &displ_opts->m_DisplayModEdge, FILLED, 0, 2 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "ModAffT" ),
                                                       &displ_opts->m_DisplayModText, FILLED, 0, 2 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "PcbAffT" ),
                                                       &displ_opts->m_DisplayDrawItems, FILLED, 0, 2 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "PcbShowZonesMode" ),
                                                       &displ_opts->m_DisplayZonesMode, 0, 0, 2 ) );

        // layer colors:
        wxASSERT( DIM( cds.m_LayersColors ) == LAYER_ID_COUNT );
        for( int i = 0;  i<LAYER_ID_COUNT;  ++i )
        {
            wxString vn = wxString::Format(
                            wxT( "ColorPCBLayer_%s" ),
                            LSET::Name( LAYER_ID( i ) ) );

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
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "MaxLnkS" ),
                                                       &displ_opts->m_MaxLinksShowed,
                                                       3, 0, 15 ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "ShowMRa" ),
                                                        &displ_opts->m_Show_Module_Ratsnest, true ) );
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
