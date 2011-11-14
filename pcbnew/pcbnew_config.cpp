/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "xnode.h"
#include "macros.h"
#include "pcbcommon.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "plot_common.h"
#include "worksheet.h"
#include "dialog_hotkeys_editor.h"

#include "class_pad.h"

#include "pcbplot.h"
#include "pcbnew.h"
#include "pcbnew_id.h"
#include "hotkeys.h"
#include "protos.h"
#include "pcbnew_config.h"

#include "dialog_mask_clearance.h"
#include "dialog_general_options.h"


#define HOTKEY_FILENAME wxT( "pcbnew" )

void PCB_EDIT_FRAME::Process_Config( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxFileName fn;

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

    case ID_PCB_LAYERS_SETUP:
        InstallDialogLayerSetup();
        break;

    case ID_CONFIG_REQ:
        InstallConfigFrame();
        break;

    case ID_PCB_MASK_CLEARANCE:
        {
            DIALOG_PADS_MASK_CLEARANCE dlg( this );
            dlg.ShowModal();
        }
        break;

    case wxID_PREFERENCES:
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
        fn = GetScreen()->GetFileName();
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

   /* Hotkey IDs */
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

   /* Macros IDs*/
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
    wxFileName fn = aProjectFileName;

    if( fn.GetExt() != ProjectFileExtension )
        fn.SetExt( ProjectFileExtension );

    wxGetApp().RemoveLibraryPath( g_UserLibDirBuffer );

    /* Initialize default values. */
    g_LibraryNames.Clear();

    wxGetApp().ReadProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters(), false );

    /* User library path takes precedent over default library search paths. */
    wxGetApp().InsertLibraryPath( g_UserLibDirBuffer, 1 );

    /* Reset the items visibility flag when loading a new configuration because it could
     * create SERIOUS mistakes for the user f board items are not visible after loading
     * a board.  Grid and ratsnest can be left to their previous state.
     */
    bool showGrid = IsElementVisible( GRID_VISIBLE );
    bool showRats = IsElementVisible( RATSNEST_VISIBLE );
    SetVisibleAlls();
    SetElementVisibility( GRID_VISIBLE, showGrid );
    SetElementVisibility( RATSNEST_VISIBLE, showRats );
    return true;
}


void PCB_EDIT_FRAME::SaveProjectSettings()
{
    wxFileName fn;

    fn = GetScreen()->GetFileName();
    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(), fn.GetFullName(),
                      ProjectFileWildcard, wxFD_SAVE | wxFD_CHANGE_DIR );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxGetApp().WriteProjectConfig( dlg.GetPath(), GROUP, GetProjectFileParameters() );
}


PARAM_CFG_ARRAY& PCB_EDIT_FRAME::GetProjectFileParameters()
{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LibDir" ),&g_UserLibDirBuffer,
                                                           GROUPLIB ) );
    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "LibName" ),
                                                               &g_LibraryNames,
                                                               GROUPLIB ) );
#ifdef KICAD_NANOMETRE
/* TODO: something should be done here!!! */
#else
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "PadDrlX" ), &g_Pad_Master.m_Drill.x,
                                                      320, 0, 0x7FFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "PadDimH" ), &g_Pad_Master.m_Size.x,
                                                      550, 0, 0x7FFF ) );
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "PadDimV" ), &g_Pad_Master.m_Size.y,
                                                      550, 0, 0x7FFF ) );
#endif
    m_projectFileParams.push_back( new PARAM_CFG_INT( wxT( "BoardThickness" ),
                                                      &boardDesignSettings.m_BoardThickness,
                                                      630, 0, 0xFFFF ) );
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
    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LastNetListRead" ),
                                                           &m_lastNetListRead ) );
    return m_projectFileParams;
}



PARAM_CFG_ARRAY& PCB_EDIT_FRAME::GetConfigurationSettings()
{
    if( !m_configSettings.empty() )
        return m_configSettings;

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

    // Colors:
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

    // Miscellaneous:
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "RotationAngle" ), &g_RotationAngle,
                                                   900, 450, 900 ) );
    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "MaxLnkS" ), &g_MaxLinksShowed,
                                                   3, 0, 15 ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "ShowMRa" ),
                                                    &g_Show_Module_Ratsnest, true ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "TwoSegT" ),
                                                    &g_TwoSegmentTrackBuild, true ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "SegmPcb45Only" ), &Segments_45_Only,
                                                    true ) );
    return m_configSettings;
}


void PCB_EDIT_FRAME::SaveMacros()
{
    wxFileName fn;
    wxXmlDocument xml;
    XNODE *rootNode = new XNODE( wxXML_ELEMENT_NODE, wxT( "macrosrootnode" ), wxEmptyString );
    XNODE *macrosNode, *hkNode;
    wxXmlProperty *macrosProp, *hkProp, *xProp, *yProp;
    wxString str, hkStr, xStr, yStr;

    fn = GetScreen()->GetFileName();
    fn.SetExt( MacrosFileExtension );

    wxFileDialog dlg( this, _( "Save Macros File" ), fn.GetPath(), fn.GetFullName(),
                      MacrosFileWildcard, wxFD_SAVE | wxFD_CHANGE_DIR );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    xml.SetRoot( rootNode );

    for( int number = 9; number >= 0; number-- )
    {
        str.Printf( wxT( "%d" ), number );
        macrosProp = new wxXmlProperty( wxT( "number" ), str );

        macrosNode = new XNODE( rootNode, wxXML_ELEMENT_NODE, wxT( "macros" ), wxEmptyString,
                                macrosProp );

        for( std::list<MACROS_RECORD>::reverse_iterator i = m_Macros[number].m_Record.rbegin();
             i != m_Macros[number].m_Record.rend();
             i++ )
        {
            hkStr.Printf( wxT( "%d" ), i->m_HotkeyCode );
            xStr.Printf( wxT( "%d" ), i->m_Position.x );
            yStr.Printf( wxT( "%d" ), i->m_Position.y );

            yProp = new wxXmlProperty( wxT( "y" ), yStr );
            xProp = new wxXmlProperty( wxT( "x" ), xStr, yProp );
            hkProp = new wxXmlProperty( wxT( "hkcode" ), hkStr, xProp );

            hkNode = new XNODE( macrosNode, wxXML_ELEMENT_NODE, wxT( "hotkey" ),
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

    fn = GetScreen()->GetFileName();
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
