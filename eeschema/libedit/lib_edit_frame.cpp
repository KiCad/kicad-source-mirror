/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file lib_edit_frame.cpp
 * @brief LIB_EDIT_FRAME class is the symbol library editor frame.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <kiway_express.h>
#include <sch_draw_panel.h>
#include <base_screen.h>
#include <confirm.h>
#include <eda_doc.h>
#include <gr_basic.h>
#include <sch_edit_frame.h>
#include <msgpanel.h>
#include <confirm.h>
#include <eda_dockart.h>

#include <general.h>
#include <eeschema_id.h>
#include <lib_edit_frame.h>
#include <class_library.h>
#include <lib_polyline.h>
#include <lib_pin.h>

#include <lib_manager.h>
#include <widgets/symbol_tree_pane.h>
#include <widgets/lib_tree.h>
#include <symbol_lib_table.h>
#include <list_operations.h>
#include <kicad_device_context.h>
#include <hotkeys.h>
#include <eeschema_config.h>

#include <dialogs/dialog_lib_edit_text.h>
#include <dialogs/dialog_edit_component_in_lib.h>
#include <dialogs/dialog_lib_edit_pin_table.h>

#include <wildcards_and_files_ext.h>

#include <menus_helpers.h>
#include <wx/progdlg.h>
#include <tool/context_menu.h>
#include <sch_view.h>
#include <sch_painter.h>

int LIB_EDIT_FRAME::           m_unit    = 1;
int LIB_EDIT_FRAME::           m_convert = 1;
LIB_ITEM* LIB_EDIT_FRAME::m_lastDrawItem = NULL;

bool LIB_EDIT_FRAME::          m_showDeMorgan    = false;
wxSize LIB_EDIT_FRAME::        m_clientSize      = wxSize( -1, -1 );
int LIB_EDIT_FRAME::           m_textSize        = -1;
double LIB_EDIT_FRAME::        m_current_text_angle = TEXT_ANGLE_HORIZ;
int LIB_EDIT_FRAME::           m_drawLineWidth   = 0;

// these values are overridden when reading the config
int LIB_EDIT_FRAME::           m_textPinNumDefaultSize = DEFAULTPINNUMSIZE;
int LIB_EDIT_FRAME::           m_textPinNameDefaultSize = DEFAULTPINNAMESIZE;
int LIB_EDIT_FRAME::           m_defaultPinLength = DEFAULTPINLENGTH;

FILL_T LIB_EDIT_FRAME::        m_drawFillStyle   = NO_FILL;


BEGIN_EVENT_TABLE( LIB_EDIT_FRAME, EDA_DRAW_FRAME )
    EVT_CLOSE( LIB_EDIT_FRAME::OnCloseWindow )
    EVT_SIZE( LIB_EDIT_FRAME::OnSize )
    EVT_ACTIVATE( LIB_EDIT_FRAME::OnActivate )

    // Actions
    EVT_TOOL( ID_LIBEDIT_NEW_LIBRARY, LIB_EDIT_FRAME::OnCreateNewLibrary )
    EVT_TOOL( ID_LIBEDIT_ADD_LIBRARY, LIB_EDIT_FRAME::OnAddLibrary )
    EVT_TOOL( ID_LIBEDIT_SAVE, LIB_EDIT_FRAME::OnSave )
    EVT_MENU( ID_LIBEDIT_SAVE_AS, LIB_EDIT_FRAME::OnSaveAs )
    EVT_MENU( ID_LIBEDIT_SAVE_ALL, LIB_EDIT_FRAME::OnSaveAll )
    EVT_TOOL( ID_LIBEDIT_REVERT, LIB_EDIT_FRAME::OnRevert )
    EVT_TOOL( ID_LIBEDIT_NEW_PART, LIB_EDIT_FRAME::OnCreateNewPart )
    EVT_TOOL( ID_LIBEDIT_EDIT_PART, LIB_EDIT_FRAME::OnEditPart )
    EVT_TOOL( ID_LIBEDIT_IMPORT_PART, LIB_EDIT_FRAME::OnImportPart )
    EVT_TOOL( ID_LIBEDIT_EXPORT_PART, LIB_EDIT_FRAME::OnExportPart )
    EVT_TOOL( ID_LIBEDIT_REMOVE_PART, LIB_EDIT_FRAME::OnRemovePart )
    EVT_TOOL( ID_LIBEDIT_CUT_PART, LIB_EDIT_FRAME::OnCopyCutPart )
    EVT_TOOL( ID_LIBEDIT_COPY_PART, LIB_EDIT_FRAME::OnCopyCutPart )
    EVT_TOOL( ID_LIBEDIT_PASTE_PART, LIB_EDIT_FRAME::OnPasteDuplicatePart )
    EVT_TOOL( ID_LIBEDIT_DUPLICATE_PART, LIB_EDIT_FRAME::OnPasteDuplicatePart )

    // Main horizontal toolbar.
    EVT_TOOL( ID_TO_LIBVIEW, LIB_EDIT_FRAME::OnOpenLibraryViewer )
    EVT_TOOL( wxID_COPY, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_PASTE, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_CUT, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, LIB_EDIT_FRAME::GetComponentFromUndoList )
    EVT_TOOL( wxID_REDO, LIB_EDIT_FRAME::GetComponentFromRedoList )
    EVT_TOOL( ID_LIBEDIT_GET_FRAME_EDIT_PART, LIB_EDIT_FRAME::OnEditComponentProperties )
    EVT_TOOL( ID_LIBEDIT_CHECK_PART, LIB_EDIT_FRAME::OnCheckComponent )
    EVT_TOOL( ID_DE_MORGAN_NORMAL_BUTT, LIB_EDIT_FRAME::OnSelectBodyStyle )
    EVT_TOOL( ID_DE_MORGAN_CONVERT_BUTT, LIB_EDIT_FRAME::OnSelectBodyStyle )
    EVT_TOOL( ID_LIBEDIT_VIEW_DOC, LIB_EDIT_FRAME::OnViewEntryDoc )
    EVT_TOOL( ID_LIBEDIT_SYNC_PIN_EDIT, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_LIBEDIT_EDIT_PIN_BY_TABLE, LIB_EDIT_FRAME::OnOpenPinTable )

    EVT_COMBOBOX( ID_LIBEDIT_SELECT_PART_NUMBER, LIB_EDIT_FRAME::OnSelectPart )

    // Right vertical toolbar.
    EVT_TOOL( ID_NO_TOOL_SELECTED, LIB_EDIT_FRAME::OnSelectTool )
    EVT_MENU( ID_MENU_ZOOM_SELECTION, LIB_EDIT_FRAME::OnSelectTool )
    EVT_TOOL( ID_ZOOM_SELECTION, LIB_EDIT_FRAME::OnSelectTool )
    EVT_TOOL_RANGE( ID_LIBEDIT_PIN_BUTT, ID_LIBEDIT_DELETE_ITEM_BUTT,
                    LIB_EDIT_FRAME::OnSelectTool )

    // Left vertical toolbar (option toolbar).
    EVT_TOOL( ID_LIBEDIT_SHOW_ELECTRICAL_TYPE, LIB_EDIT_FRAME::OnShowElectricalType )
    EVT_TOOL( ID_LIBEDIT_SHOW_HIDE_SEARCH_TREE, LIB_EDIT_FRAME::OnToggleSearchTree )

    // menubar commands
    EVT_MENU( wxID_EXIT, LIB_EDIT_FRAME::CloseWindow )
    EVT_MENU( ID_LIBEDIT_GEN_PNG_FILE, LIB_EDIT_FRAME::OnPlotCurrentComponent )
    EVT_MENU( ID_LIBEDIT_GEN_SVG_FILE, LIB_EDIT_FRAME::OnPlotCurrentComponent )
    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_INDEX, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( ID_HELP_GET_INVOLVED, EDA_DRAW_FRAME::GetKicadContribute )
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::GetKicadAbout )
    EVT_MENU( ID_GRID_SETTINGS, SCH_BASE_FRAME::OnGridSettings )

    EVT_MENU( wxID_PREFERENCES, LIB_EDIT_FRAME::OnPreferencesOptions )

    // Multiple item selection context menu commands.
    EVT_MENU_RANGE( ID_SELECT_ITEM_START, ID_SELECT_ITEM_END, LIB_EDIT_FRAME::OnSelectItem )

    EVT_MENU( ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, LIB_EDIT_FRAME::Process_Config )

    // Context menu events and commands.
    EVT_MENU( ID_LIBEDIT_EDIT_PIN, LIB_EDIT_FRAME::OnEditPin )
    EVT_MENU( ID_LIBEDIT_ROTATE_ITEM, LIB_EDIT_FRAME::OnRotate )

    EVT_MENU_RANGE( ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_ITEM,
                    ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT,
                    LIB_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    LIB_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_LIBEDIT_MIRROR_X, ID_LIBEDIT_ORIENT_NORMAL,
                    LIB_EDIT_FRAME::OnOrient )

    // Update user interface elements.
    EVT_UPDATE_UI( wxID_PASTE, LIB_EDIT_FRAME::OnUpdatePaste )
    EVT_UPDATE_UI( ID_LIBEDIT_EXPORT_PART, LIB_EDIT_FRAME::OnUpdateHavePart )
    EVT_UPDATE_UI( ID_LIBEDIT_SAVE, LIB_EDIT_FRAME::OnUpdateSave )
    EVT_UPDATE_UI( ID_LIBEDIT_SAVE_AS, LIB_EDIT_FRAME::OnUpdateSaveAs )
    EVT_UPDATE_UI( ID_LIBEDIT_REVERT, LIB_EDIT_FRAME::OnUpdateRevert )
    EVT_UPDATE_UI( ID_LIBEDIT_CHECK_PART, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_GET_FRAME_EDIT_PART, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( wxID_UNDO, LIB_EDIT_FRAME::OnUpdateUndo )
    EVT_UPDATE_UI( wxID_REDO, LIB_EDIT_FRAME::OnUpdateRedo )
    EVT_UPDATE_UI( ID_LIBEDIT_SYNC_PIN_EDIT, LIB_EDIT_FRAME::OnUpdateSyncPinEdit )
    EVT_UPDATE_UI( ID_LIBEDIT_EDIT_PIN_BY_TABLE, LIB_EDIT_FRAME::OnUpdatePinTable )
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_PART_NUMBER, LIB_EDIT_FRAME::OnUpdatePartNumber )
    EVT_UPDATE_UI( ID_DE_MORGAN_NORMAL_BUTT, LIB_EDIT_FRAME::OnUpdateDeMorganNormal )
    EVT_UPDATE_UI( ID_DE_MORGAN_CONVERT_BUTT, LIB_EDIT_FRAME::OnUpdateDeMorganConvert )
    EVT_UPDATE_UI( ID_NO_TOOL_SELECTED, LIB_EDIT_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI( ID_ZOOM_SELECTION, LIB_EDIT_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI_RANGE( ID_LIBEDIT_PIN_BUTT, ID_LIBEDIT_DELETE_ITEM_BUTT,
                         LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_SHOW_ELECTRICAL_TYPE, LIB_EDIT_FRAME::OnUpdateElectricalType )
    EVT_UPDATE_UI( ID_LIBEDIT_SHOW_HIDE_SEARCH_TREE, LIB_EDIT_FRAME::OnUpdateSearchTreeTool )

END_EVENT_TABLE()

LIB_EDIT_FRAME::LIB_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    SCH_BASE_FRAME( aKiway, aParent, FRAME_SCH_LIB_EDITOR, _( "Library Editor" ),
        wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, LIB_EDIT_FRAME_NAME )
{
    m_showAxis   = true;            // true to draw axis
    SetShowDeMorgan( false );
    m_drawSpecificConvert = true;
    m_drawSpecificUnit    = false;
    m_hotkeysDescrList    = g_Libedit_Hokeys_Descr;
    m_syncPinEdit         = false;
    m_repeatPinStep = DEFAULT_REPEAT_OFFSET_PIN;
    SetShowElectricalType( true );
    m_FrameSize = ConvertDialogToPixels( wxSize( 500, 350 ) );    // default in case of no prefs

    m_my_part = NULL;
    m_tempCopyComponent = NULL;
    m_treePane = nullptr;
    m_libMgr = nullptr;

    // Delayed initialization
    if( m_textSize == -1 )
        m_textSize = GetDefaultTextSize();

    // Initialize grid id to the default value 50 mils:
    m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_libedit_xpm ) );
    SetIcon( icon );

    LoadSettings( config() );

    m_dummyScreen = new SCH_SCREEN( aKiway );
    SetScreen( m_dummyScreen );
    GetScreen()->m_Center = true;
    GetScreen()->SetMaxUndoItems( m_UndoRedoCountMax );

    SetCrossHairPosition( wxPoint( 0, 0 ) );

    SetPresetGrid( m_LastGridSizeId );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if( m_canvas )
        m_canvas->SetEnableBlockCommands( true );

    m_libMgr = new LIB_MANAGER( *this );
    SyncLibraries( true );
    m_treePane = new SYMBOL_TREE_PANE( this, m_libMgr );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    updateTitle();
    DisplayCmpDoc();
    UpdatePartSelectList();

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetArtProvider( new EDA_DOCKART( this ) );


    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_treePane, EDA_PANE().Palette().Name( "ComponentTree" ).Left().Layer(1)
                      .Caption( _( "Libraries" ) ).MinSize( 250, -1 ).Resizable() );
    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( "ToolsToolbar" ).Right().Layer(1) );

    m_auimgr.AddPane( m_canvas->GetWindow(), wxAuiPaneInfo().Name( "DrawFrame" ).CentrePane() );

    m_auimgr.Update();

    Raise();
    Show( true );

    Bind( wxEVT_COMMAND_MENU_SELECTED, &LIB_EDIT_FRAME::OnConfigurePaths, this,
          ID_PREFERENCES_CONFIGURE_PATHS );

    Bind( wxEVT_COMMAND_MENU_SELECTED, &LIB_EDIT_FRAME::OnEditSymbolLibTable, this,
          ID_EDIT_SYM_LIB_TABLE );

    wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ID_ZOOM_PAGE );
    wxPostEvent( this, evt );

    SyncView();
    GetGalCanvas()->GetViewControls()->SetSnapping( true );
    GetGalCanvas()->GetView()->UseDrawPriority( true );
    GetGalCanvas()->GetGAL()->SetGridVisibility( IsGridVisible() );
    GetGalCanvas()->GetGAL()->SetAxesEnabled( true );

    // Set the working/draw area size to display a symbol to a reasonable value:
    // A 600mm x 600mm with a origin at the area center looks like a large working area
    double max_size_x = Millimeter2iu( 600 );
    double max_size_y = Millimeter2iu( 600 );
    BOX2D bbox;
    bbox.SetOrigin( -max_size_x /2, -max_size_y/2 );
    bbox.SetSize( max_size_x, max_size_y );
    GetCanvas()->GetView()->SetBoundary( bbox );
}


LIB_EDIT_FRAME::~LIB_EDIT_FRAME()
{
    Unbind( wxEVT_COMMAND_MENU_SELECTED, &LIB_EDIT_FRAME::OnEditSymbolLibTable, this,
            ID_EDIT_SYM_LIB_TABLE );

    // current screen is destroyed in EDA_DRAW_FRAME
    SetScreen( m_dummyScreen );

    m_lastDrawItem = NULL;
    SetDrawItem( m_lastDrawItem );

    delete m_tempCopyComponent;
    delete m_libMgr;
    delete m_my_part;
}


void LIB_EDIT_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( saveAllLibraries( true ) )
        Destroy();
    else
        Event.Veto();
}


double LIB_EDIT_FRAME::BestZoom()
{
    LIB_PART*  part = GetCurPart();
    double     defaultLibraryZoom = 7.33;

    if( !part )
    {
        SetScrollCenterPosition( wxPoint( 0, 0 ) );
        return defaultLibraryZoom;
    }

    EDA_RECT boundingBox = part->GetUnitBoundingBox( m_unit, m_convert );

    double  sizeX  = (double) boundingBox.GetWidth();
    double  sizeY  = (double) boundingBox.GetHeight();
    wxPoint centre = boundingBox.Centre();

    // Reserve a 20% margin around component bounding box.
    double margin_scale_factor = 1.2;

    return bestZoom( sizeX, sizeY, margin_scale_factor, centre);
}


void LIB_EDIT_FRAME::UpdatePartSelectList()
{
    if( m_partSelectBox == NULL )
        return;

    if( m_partSelectBox->GetCount() != 0 )
        m_partSelectBox->Clear();

    LIB_PART*      part = GetCurPart();

    if( !part || part->GetUnitCount() <= 1 )
    {
        m_unit = 1;
        m_partSelectBox->Append( wxEmptyString );
    }
    else
    {
        for( int i = 0; i < part->GetUnitCount(); i++ )
        {
            wxString sub  = LIB_PART::SubReference( i+1, false );
            wxString unit = wxString::Format( _( "Unit %s" ), GetChars( sub ) );
            m_partSelectBox->Append( unit );
        }
    }

    // Ensure the current selected unit is compatible with
    // the number of units of the current part:
    if( part && part->GetUnitCount() < m_unit )
        m_unit = 1;

    m_partSelectBox->SetSelection( ( m_unit > 0 ) ? m_unit - 1 : 0 );
}


void LIB_EDIT_FRAME::OnShowElectricalType( wxCommandEvent& event )
{
    m_showPinElectricalTypeName = !m_showPinElectricalTypeName;

    // Update canvas
    GetRenderSettings()->m_ShowPinsElectricalType = m_showPinElectricalTypeName;
    GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    GetCanvas()->Refresh();
}


void LIB_EDIT_FRAME::OnToggleSearchTree( wxCommandEvent& event )
{
    auto& treePane = m_auimgr.GetPane( m_treePane );
    treePane.Show( !IsSearchTreeShown() );
    m_auimgr.Update();
}


void LIB_EDIT_FRAME::OnEditSymbolLibTable( wxCommandEvent& aEvent )
{
    m_libMgr->GetAdapter()->Freeze();

    SCH_BASE_FRAME::OnEditSymbolLibTable( aEvent );
    SyncLibraries( true );

    m_libMgr->GetAdapter()->Thaw();
}


bool LIB_EDIT_FRAME::IsSearchTreeShown()
{
    return m_auimgr.GetPane( m_treePane ).IsShown();
}


void LIB_EDIT_FRAME::ClearSearchTreeSelection()
{
    m_treePane->GetLibTree()->Unselect();
}


void LIB_EDIT_FRAME::OnUpdateSelectTool( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_drawToolBar || aEvent.GetEventObject() == m_mainToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void LIB_EDIT_FRAME::OnUpdateElectricalType( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( GetShowElectricalType() );
}


void LIB_EDIT_FRAME::OnUpdateSearchTreeTool( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( IsSearchTreeShown() );
}


void LIB_EDIT_FRAME::OnUpdateSave( wxUpdateUIEvent& aEvent )
{
    LIB_ID libId = getTargetLibId();
    const wxString& libName = libId.GetLibNickname();
    const wxString& partName = libId.GetLibItemName();
    bool readOnly = libName.IsEmpty() || m_libMgr->IsLibraryReadOnly( libName );

    if( partName.IsEmpty() )
        aEvent.Enable( !readOnly && m_libMgr->IsLibraryModified( libName ) );
    else
        aEvent.Enable( !readOnly && m_libMgr->IsPartModified( partName, libName ) );
}


void LIB_EDIT_FRAME::OnUpdateSaveAs( wxUpdateUIEvent& aEvent )
{
    LIB_ID libId = getTargetLibId();
    const wxString& libName = libId.GetLibNickname();
    const wxString& partName = libId.GetLibItemName();

    aEvent.Enable( !libName.IsEmpty() || !partName.IsEmpty() );
}


void LIB_EDIT_FRAME::OnUpdateRevert( wxUpdateUIEvent& aEvent )
{
    LIB_ID libId = getTargetLibId();
    const wxString& libName = libId.GetLibNickname();
    const wxString& partName = libId.GetLibItemName();

    if( partName.IsEmpty() )
        aEvent.Enable( !libName.IsEmpty() && m_libMgr->IsLibraryModified( libName ) );
    else
        aEvent.Enable( !libName.IsEmpty() && m_libMgr->IsPartModified( partName, libName ) );
}


void LIB_EDIT_FRAME::OnUpdateHavePart( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( getTargetLibId().IsValid() );
}


void LIB_EDIT_FRAME::OnUpdateEditingPart( wxUpdateUIEvent& aEvent )
{
    LIB_PART* part = GetCurPart();

    aEvent.Enable( part != NULL );

    if( part && aEvent.GetEventObject() == m_drawToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void LIB_EDIT_FRAME::OnUpdatePaste( wxUpdateUIEvent& event )
{
    event.Enable( m_clipboard.GetCount() > 0 );
}


void LIB_EDIT_FRAME::OnUpdateUndo( wxUpdateUIEvent& event )
{
    event.Enable( GetCurPart() && GetScreen() &&
        GetScreen()->GetUndoCommandCount() != 0 && !IsEditingDrawItem() );
}


void LIB_EDIT_FRAME::OnUpdateRedo( wxUpdateUIEvent& event )
{
    event.Enable( GetCurPart() && GetScreen() &&
        GetScreen()->GetRedoCommandCount() != 0 && !IsEditingDrawItem() );
}


void LIB_EDIT_FRAME::OnUpdateSyncPinEdit( wxUpdateUIEvent& event )
{
    LIB_PART* part = GetCurPart();
    event.Enable( part && part->IsMulti() && !part->UnitsLocked() );
    event.Check( m_syncPinEdit );
}


void LIB_EDIT_FRAME::OnUpdatePinTable( wxUpdateUIEvent& event )
{
    LIB_PART* part = GetCurPart();
    event.Enable( part != NULL );
}


void LIB_EDIT_FRAME::OnUpdatePartNumber( wxUpdateUIEvent& event )
{
    if( m_partSelectBox == NULL )
        return;

    LIB_PART*      part = GetCurPart();

    // Using the typical event.Enable() call doesn't seem to work with wxGTK
    // so use the pointer to alias combobox to directly enable or disable.
    m_partSelectBox->Enable( part && part->GetUnitCount() > 1 );
}


void LIB_EDIT_FRAME::OnUpdateDeMorganNormal( wxUpdateUIEvent& event )
{
    if( m_mainToolBar == NULL )
        return;

    LIB_PART*      part = GetCurPart();

    event.Enable( GetShowDeMorgan() || ( part && part->HasConversion() ) );
    event.Check( m_convert <= 1 );
}


void LIB_EDIT_FRAME::OnUpdateDeMorganConvert( wxUpdateUIEvent& event )
{
    if( m_mainToolBar == NULL )
        return;

    LIB_PART*      part = GetCurPart();

    event.Enable( GetShowDeMorgan() || ( part && part->HasConversion() ) );
    event.Check( m_convert > 1 );
}


void LIB_EDIT_FRAME::OnSelectPart( wxCommandEvent& event )
{
    int i = event.GetSelection();

    if( ( i == wxNOT_FOUND ) || ( ( i + 1 ) == m_unit ) )
        return;

    m_lastDrawItem = NULL;
    m_unit = i + 1;

    RebuildView();
    DisplayCmpDoc();
}


void LIB_EDIT_FRAME::OnViewEntryDoc( wxCommandEvent& event )
{
    LIB_PART* part = GetCurPart();

    if( !part )
        return;

    wxString filename;

    if( part->GetAliasCount() > 1 )
    {
        CONTEXT_MENU popup;
        wxString     msg;
        int          id = 0;

        for( LIB_ALIAS* alias : part->GetAliases() )
        {
            msg.Printf( wxT( "%s (%s)" ), alias->GetName(), alias->GetDocFileName() );
            popup.Append( id++, msg );
        }

        PopupMenu( &popup );

        if( popup.GetSelected() >= 0 )
            filename = part->GetAlias( (unsigned) popup.GetSelected() )->GetDocFileName();
    }
    else
        filename = part->GetAlias( 0 )->GetDocFileName();

    if( !filename.IsEmpty() && filename != wxT( "~" ) )
    {
        SEARCH_STACK* lib_search = Prj().SchSearchS();

        GetAssociatedDocument( this, filename, lib_search );
    }
}


void LIB_EDIT_FRAME::OnSelectBodyStyle( wxCommandEvent& event )
{
    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, GetGalCanvas()->GetDefaultCursor() );

    if( event.GetId() == ID_DE_MORGAN_NORMAL_BUTT )
        m_convert = 1;
    else
        m_convert = 2;

    m_lastDrawItem = NULL;

    RebuildView();
}


void LIB_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int     id = event.GetId();
    wxPoint pos;
    SCH_SCREEN* screen = GetScreen();
    BLOCK_SELECTOR& block = screen->m_BlockLocate;
    LIB_ITEM* item = screen->GetCurLibItem();

    m_canvas->SetIgnoreMouseEvents( true );

    wxGetMousePosition( &pos.x, &pos.y );
    pos.y += 20;

    switch( id )   // Stop placement commands before handling new command.
    {
    case wxID_COPY:
    case ID_POPUP_COPY_BLOCK:
    case wxID_CUT:
    case ID_POPUP_CUT_BLOCK:
    case ID_POPUP_LIBEDIT_END_CREATE_ITEM:
    case ID_LIBEDIT_EDIT_PIN:
    case ID_POPUP_LIBEDIT_BODY_EDIT_ITEM:
    case ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_DUPLICATE_BLOCK:
    case ID_POPUP_SELECT_ITEMS_BLOCK:
    case ID_POPUP_MIRROR_X_BLOCK:
    case ID_POPUP_MIRROR_Y_BLOCK:
    case ID_POPUP_ROTATE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT:
        break;

    case ID_POPUP_LIBEDIT_CANCEL_EDITING:
        if( m_canvas->IsMouseCaptured() )
            m_canvas->EndMouseCapture();
        else
            m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, GetGalCanvas()->GetDefaultCursor() );
        break;

    case ID_POPUP_LIBEDIT_DELETE_ITEM:
        m_canvas->EndMouseCapture();
        break;

    default:
        m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, GetGalCanvas()->GetDefaultCursor(),
                                   wxEmptyString );
        break;
    }

    switch( id )
    {
    case ID_POPUP_LIBEDIT_CANCEL_EDITING:
        break;

    case ID_LIBEDIT_SYNC_PIN_EDIT:
        m_syncPinEdit = m_mainToolBar->GetToolToggled( ID_LIBEDIT_SYNC_PIN_EDIT );
        break;

    case ID_POPUP_LIBEDIT_END_CREATE_ITEM:
        m_canvas->MoveCursorToCrossHair();
        if( item )
        {
            EndDrawGraphicItem( nullptr );
        }
        break;

    case ID_POPUP_LIBEDIT_BODY_EDIT_ITEM:
        if( item )
        {
            m_canvas->CrossHairOff( );

            switch( item->Type() )
            {
            case LIB_ARC_T:
            case LIB_CIRCLE_T:
            case LIB_RECTANGLE_T:
            case LIB_POLYLINE_T:
                EditGraphicSymbol( nullptr, item );
                break;

            case LIB_TEXT_T:
                EditSymbolText( nullptr, item );
                break;

            default:
                ;
            }

            m_canvas->CrossHairOn( );
        }
        break;

    case ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT:
        {
            // Delete the last created segment, while creating a polyline draw item
            if( item == NULL )
                break;

            m_canvas->MoveCursorToCrossHair();
            STATUS_FLAGS oldFlags = item->GetFlags();
            item->ClearFlags();
    /*        item->Draw( m_canvas, &dc, wxPoint( 0, 0 ), COLOR4D::UNSPECIFIED, g_XorMode, NULL,
                              DefaultTransform );
            ( (LIB_POLYLINE*) item )->DeleteSegment( GetCrossHairPosition( true ) );
            item->Draw( m_canvas, &dc, wxPoint( 0, 0 ), COLOR4D::UNSPECIFIED, g_XorMode, NULL,
                              DefaultTransform );*/
            item->SetFlags( oldFlags );
            m_lastDrawItem = NULL;
        }
        break;

    case ID_POPUP_LIBEDIT_DELETE_ITEM:
        if( item )
            deleteItem( nullptr, item );

        break;

    case ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST:
        if( item == NULL )
            break;

        if( item->Type() == LIB_PIN_T )
        {
            StartMovePin( item );
        }
        else
        {
            StartMoveDrawSymbol( nullptr, item );
        }
        break;

    case ID_POPUP_LIBEDIT_MODIFY_ITEM:

        if( item == NULL )
            break;

        m_canvas->MoveCursorToCrossHair();
        if( item->Type() == LIB_RECTANGLE_T
            || item->Type() == LIB_CIRCLE_T
            || item->Type() == LIB_POLYLINE_T
            || item->Type() == LIB_ARC_T
            )
        {
            StartModifyDrawSymbol( nullptr, item );
        }

        break;

    case ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM:
        if( item == NULL )
            break;

        m_canvas->CrossHairOff( nullptr );

        if( item->Type() == LIB_FIELD_T )
        {
            EditField( (LIB_FIELD*) item );
        }

        m_canvas->MoveCursorToCrossHair();
        m_canvas->CrossHairOn( );
        break;

    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
        {
            if( !item || item->Type() != LIB_PIN_T )
                break;

            LIB_PART*      part = GetCurPart();

            SaveCopyInUndoList( part );

            GlobalSetPins( (LIB_PIN*) item, id );
            m_canvas->MoveCursorToCrossHair();
        }
        break;

    case ID_POPUP_ZOOM_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        block.SetCommand( BLOCK_ZOOM );
        HandleBlockEnd( nullptr );
        break;

    case ID_POPUP_DELETE_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        block.SetCommand( BLOCK_DELETE );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockEnd( nullptr );
        break;

    case ID_POPUP_DUPLICATE_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        block.SetCommand( BLOCK_DUPLICATE );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockEnd( nullptr );
        break;

    case ID_POPUP_SELECT_ITEMS_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        block.SetCommand( BLOCK_SELECT_ITEMS_ONLY );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockEnd( nullptr );
        break;

    case ID_POPUP_MIRROR_X_BLOCK:
    case ID_POPUP_MIRROR_Y_BLOCK:
        OnOrient( event );
        break;

    case ID_POPUP_ROTATE_BLOCK:
        OnRotate( event );
        break;

    case ID_POPUP_PLACE_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockPlace( nullptr );
        break;

    case wxID_COPY:
    case ID_POPUP_COPY_BLOCK:
        block.SetCommand( BLOCK_COPY );
        block.SetMessageBlock( this );
        HandleBlockEnd( nullptr );
        break;

    case wxID_PASTE:
    case ID_POPUP_PASTE_BLOCK:
        HandleBlockBegin( nullptr, BLOCK_PASTE, GetCrossHairPosition() );
        break;

    case wxID_CUT:
    case ID_POPUP_CUT_BLOCK:
        if( block.GetCommand() != BLOCK_MOVE )
            break;

        block.SetCommand( BLOCK_CUT );
        block.SetMessageBlock( this );
        HandleBlockEnd( nullptr );
        break;

    default:
        wxFAIL_MSG( "LIB_EDIT_FRAME::Process_Special_Functions error" );
        break;
    }

    m_canvas->SetIgnoreMouseEvents( false );

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        m_lastDrawItem = NULL;
}


void LIB_EDIT_FRAME::OnActivate( wxActivateEvent& event )
{
    EDA_DRAW_FRAME::OnActivate( event );
}


wxString LIB_EDIT_FRAME::GetCurLib() const
{
    wxString libNickname = Prj().GetRString( PROJECT::SCH_LIBEDIT_CUR_LIB );

    if( !libNickname.empty() )
    {
        if( !Prj().SchSymbolLibTable()->HasLibrary( libNickname ) )
        {
            Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, wxEmptyString );
            libNickname = wxEmptyString;
        }
    }

    return libNickname;
}


wxString LIB_EDIT_FRAME::SetCurLib( const wxString& aLibNickname )
{
    wxString old = GetCurLib();

    if( aLibNickname.empty() || !Prj().SchSymbolLibTable()->HasLibrary( aLibNickname ) )
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, wxEmptyString );
    else
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, aLibNickname );

    m_libMgr->SetCurrentLib( aLibNickname );

    return old;
}


void LIB_EDIT_FRAME::SetCurPart( LIB_PART* aPart )
{
    if( !aPart && !m_my_part )
        return;

    GetScreen()->SetCurItem( nullptr );

    if( m_my_part != aPart )
    {
        if( m_my_part )
            delete m_my_part;

        m_my_part = aPart;
    }

    // select the current component in the tree widget
    if( aPart )
        m_treePane->GetLibTree()->SelectLibId( aPart->GetLibId() );

    wxString partName = aPart ? aPart->GetName() : wxString();
    m_libMgr->SetCurrentPart( partName );

    // retain in case this wxFrame is re-opened later on the same PROJECT
    Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_PART, partName );

    // Ensure synchronized pin edit can be enabled only symbols with interchangeable units
    m_syncPinEdit = aPart && aPart->IsMulti() && !aPart->UnitsLocked();

    RebuildView();
}


void LIB_EDIT_FRAME::TempCopyComponent()
{
    delete m_tempCopyComponent;

    if( LIB_PART* part = GetCurPart() )
        // clone it and own the clone.
        m_tempCopyComponent = new LIB_PART( *part );
    else
        // clear it, there was no CurPart
        m_tempCopyComponent = NULL;
}


void LIB_EDIT_FRAME::RestoreComponent()
{
    if( m_tempCopyComponent )
    {
        // transfer ownership to CurPart
        SetCurPart( m_tempCopyComponent );
        m_tempCopyComponent = NULL;
    }
}


void LIB_EDIT_FRAME::ClearTempCopyComponent()
{
    delete m_tempCopyComponent;
    m_tempCopyComponent = NULL;
}


void LIB_EDIT_FRAME::EditSymbolText( wxDC* DC, LIB_ITEM* DrawItem )
{
    if ( ( DrawItem == NULL ) || ( DrawItem->Type() != LIB_TEXT_T ) )
        return;

    // Deleting old text
    if( DC && !DrawItem->InEditMode() )
        DrawItem->Draw( m_canvas, DC, wxPoint( 0, 0 ), COLOR4D::UNSPECIFIED, g_XorMode, NULL,
                        DefaultTransform );

    DIALOG_LIB_EDIT_TEXT dlg( this, (LIB_TEXT*) DrawItem );

    if( dlg.ShowModal() != wxID_OK )
        return;

    GetCanvas()->GetView()->Update( DrawItem );
    GetCanvas()->Refresh();
    OnModify();

    // Display new text
    if( DC && !DrawItem->InEditMode() )
        DrawItem->Draw( m_canvas, DC, wxPoint( 0, 0 ), COLOR4D::UNSPECIFIED, GR_DEFAULT_DRAWMODE,
                        NULL, DefaultTransform );
}


void LIB_EDIT_FRAME::OnEditComponentProperties( wxCommandEvent& event )
{
    bool partLocked = GetCurPart()->UnitsLocked();
    wxString oldName = GetCurPart()->GetName();

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, GetGalCanvas()->GetDefaultCursor() );

    if( GetDrawItem() && GetDrawItem()->Type() == LIB_FIELD_T )
        SetDrawItem( nullptr );     // selected LIB_FIELD might be deleted

    DIALOG_EDIT_COMPONENT_IN_LIBRARY dlg( this, GetCurPart() );

    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    // if m_UnitSelectionLocked has changed, set some edit options or defaults
    // to the best value
    if( partLocked != GetCurPart()->UnitsLocked() )
    {
        // Enable synchronized pin edit mode for symbols with interchangeable units
        m_syncPinEdit = !GetCurPart()->UnitsLocked();
        // also set default edit options to the better value
        // Usually if units are locked, graphic items are specific to each unit
        // and if units are interchangeable, graphic items are common to units
        m_drawSpecificUnit = GetCurPart()->UnitsLocked();
    }

    if( oldName != GetCurPart()->GetName() )
        m_libMgr->UpdatePartAfterRename( GetCurPart(), oldName, GetCurLib() );
    else
        m_libMgr->UpdatePart( GetCurPart(), GetCurLib() );

    UpdatePartSelectList();
    updateTitle();
    DisplayCmpDoc();

    RebuildView();
    GetCanvas()->Refresh();
    OnModify();
}


void LIB_EDIT_FRAME::OnSelectTool( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();
    int lastToolID = GetToolId();

    if( GetToolId() == ID_NO_TOOL_SELECTED || GetToolId() == ID_ZOOM_SELECTION )
        m_lastDrawItem = NULL;

    // Stop the current command and deselect the current tool.
    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, GetGalCanvas()->GetDefaultCursor() );

    LIB_PART*      part = GetCurPart();

    switch( id )
    {
    case ID_NO_TOOL_SELECTED:
        SetToolID( id, GetGalCanvas()->GetDefaultCursor(), wxEmptyString );
        break;

    case ID_MENU_ZOOM_SELECTION:
    case ID_ZOOM_SELECTION:
        // This tool is located on the main toolbar: switch it on or off on click on it
        if( lastToolID != ID_ZOOM_SELECTION )
            SetToolID( ID_ZOOM_SELECTION, wxCURSOR_MAGNIFIER, _( "Zoom to selection" ) );
        else
            SetNoToolSelected();
        break;

    case ID_LIBEDIT_PIN_BUTT:
        if( part )
        {
            SetToolID( id, wxCURSOR_PENCIL, _( "Add pin" ) );
        }
        else
        {
            SetToolID( id, wxCURSOR_ARROW, _( "Set pin options" ) );

            wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );

            cmd.SetId( ID_LIBEDIT_EDIT_PIN );
            GetEventHandler()->ProcessEvent( cmd );
            SetNoToolSelected();
        }
        break;

    case ID_LIBEDIT_BODY_TEXT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add text" ) );
        break;

    case ID_LIBEDIT_BODY_RECT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add rectangle" ) );
        break;

    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add circle" ) );
        break;

    case ID_LIBEDIT_BODY_ARC_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add arc" ) );
        break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add line" ) );
        break;

    case ID_LIBEDIT_ANCHOR_ITEM_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Set anchor position" ) );
        break;

    case ID_LIBEDIT_IMPORT_BODY_BUTT:
        SetToolID( id, GetGalCanvas()->GetDefaultCursor(), _( "Import" ) );
        LoadOneSymbol();
        SetNoToolSelected();
        break;

    case ID_LIBEDIT_EXPORT_BODY_BUTT:
        SetToolID( id, GetGalCanvas()->GetDefaultCursor(), _( "Export" ) );
        SaveOneSymbol();
        SetNoToolSelected();
        break;

    case ID_LIBEDIT_DELETE_ITEM_BUTT:
        if( !part )
        {
            wxBell();
            break;
        }

        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;

    default:
        break;
    }

    m_canvas->SetIgnoreMouseEvents( false );
}


void LIB_EDIT_FRAME::OnRotate( wxCommandEvent& aEvent )
{
    LIB_PART*       part = GetCurPart();
    BLOCK_SELECTOR& block = GetScreen()->m_BlockLocate;
    LIB_ITEM*       item = GetDrawItem();

    // Allows block rotate operation on hot key.
    if( block.GetState() != STATE_NO_BLOCK )
    {
        // Compute the rotation center and put it on grid:
        wxPoint rotationPoint = block.Centre();
        rotationPoint = GetNearestGridPosition( rotationPoint );

        // The Y axis orientation is bottom to top for symbol items.
        // so change the Y coord value of the rotation center
        rotationPoint.y = -rotationPoint.y;

        if( block.AppendUndo() )
            ; // UR_LIBEDIT saves entire state, so no need to append anything more
        else
        {
            SaveCopyInUndoList( part, UR_LIBEDIT );
            block.SetAppendUndo();
        }

        for( unsigned ii = 0; ii < block.GetCount(); ii++ )
        {
            item = static_cast<LIB_ITEM*>( block.GetItem( ii ) );
            item->Rotate( rotationPoint );
        }

        GetCanvas()->CallMouseCapture( nullptr, wxDefaultPosition, false );
        GetCanvas()->Refresh();
    }
    else if( item )
    {
        wxPoint rotationPoint = item->GetPosition();

        if( !item->InEditMode() )
            SaveCopyInUndoList( part, UR_LIBEDIT );

        item->Rotate( rotationPoint );

        if( item->InEditMode() )
            GetCanvas()->CallMouseCapture( nullptr, wxDefaultPosition, false );
        else
            GetCanvas()->GetView()->Update( item );
        GetCanvas()->Refresh();
        OnModify();

        if( !item->InEditMode() )
            item->ClearFlags();

        if( GetToolId() == ID_NO_TOOL_SELECTED )
            m_lastDrawItem = NULL;
    }
}


void LIB_EDIT_FRAME::OnOrient( wxCommandEvent& aEvent )
{
    LIB_PART*       part = GetCurPart();
    BLOCK_SELECTOR& block = GetScreen()->m_BlockLocate;
    LIB_ITEM*       item = GetDrawItem();

    // Allows block rotate operation on hot key.
    if( block.GetState() != STATE_NO_BLOCK )
    {
        // Compute the mirror center and put it on grid.
        wxPoint mirrorPoint = block.Centre();
        mirrorPoint = GetNearestGridPosition( mirrorPoint );

        // The Y axis orientation is bottom to top for symbol items.
        // so change the Y coord value of the rotation center
        mirrorPoint.y = -mirrorPoint.y;

        if( block.AppendUndo() )
            ; // UR_LIBEDIT saves entire state, so no need to append anything more
        else
        {
            SaveCopyInUndoList( part, UR_LIBEDIT );
            block.SetAppendUndo();
        }

        for( unsigned ii = 0; ii < block.GetCount(); ii++ )
        {
            item = static_cast<LIB_ITEM*>( block.GetItem( ii ) );

            if( aEvent.GetId() == ID_LIBEDIT_MIRROR_Y || aEvent.GetId() == ID_POPUP_MIRROR_Y_BLOCK )
                item->MirrorHorizontal( mirrorPoint );
            else
                item->MirrorVertical( mirrorPoint );
        }

        m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );
        GetCanvas()->Refresh();
    }
    else if( item )
    {
        wxPoint mirrorPoint = item->GetPosition();
        mirrorPoint.y = -mirrorPoint.y;

        if( !item->InEditMode() )
            SaveCopyInUndoList( part, UR_LIBEDIT );

        if( aEvent.GetId() == ID_LIBEDIT_MIRROR_Y || aEvent.GetId() == ID_POPUP_MIRROR_Y_BLOCK )
            item->MirrorHorizontal( mirrorPoint );
        else
            item->MirrorVertical( mirrorPoint );

        if( item->InEditMode() )
            m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );
        else
            GetCanvas()->GetView()->Update( item );
        GetCanvas()->Refresh();
        OnModify();

        if( !item->InEditMode() )
            item->ClearFlags();

        if( GetToolId() == ID_NO_TOOL_SELECTED )
            m_lastDrawItem = NULL;
    }
}


LIB_ITEM* LIB_EDIT_FRAME::LocateItemUsingCursor( const wxPoint& aPosition,
                                                 const KICAD_T aFilterList[] )
{
    wxPoint        pos;
    LIB_PART*      part = GetCurPart();

    if( !part )
        return NULL;

    LIB_ITEM* item = locateItem( aPosition, aFilterList );

    // If the user aborted the clarification context menu, don't show it again at the
    // grid position.
    if( !item && m_canvas->GetAbortRequest() )
        return NULL;

    pos = GetNearestGridPosition( aPosition );

    if( item == NULL && aPosition != pos )
        item = locateItem( pos, aFilterList );

    return item;
}


LIB_ITEM* LIB_EDIT_FRAME::locateItem( const wxPoint& aPosition, const KICAD_T aFilterList[] )
{
    LIB_PART*      part = GetCurPart();

    if( !part )
        return NULL;

    LIB_ITEM* item = NULL;

    m_collectedItems.Collect( part->GetDrawItems(), aFilterList, aPosition,
                              m_unit, m_convert );

    if( m_collectedItems.GetCount() == 0 )
    {
        ClearMsgPanel();
    }
    else if( m_collectedItems.GetCount() == 1 )
    {
        item = m_collectedItems[0];
    }
    else
    {
        if( item == NULL )
        {
            wxASSERT_MSG( m_collectedItems.GetCount() <= MAX_SELECT_ITEM_IDS,
                          "Select item clarification context menu size limit exceeded." );

            wxMenu selectMenu;
            AddMenuItem( &selectMenu, wxID_NONE, _( "Clarify Selection" ),
                         KiBitmap( info_xpm ) );

            selectMenu.AppendSeparator();

            for( int i = 0;  i < m_collectedItems.GetCount() && i < MAX_SELECT_ITEM_IDS;  i++ )
            {
                wxString    text = m_collectedItems[i]->GetSelectMenuText( m_UserUnits );
                BITMAP_DEF  xpm = m_collectedItems[i]->GetMenuImage();

                AddMenuItem( &selectMenu, ID_SELECT_ITEM_START + i, text, KiBitmap( xpm ) );
            }

            // Set to NULL in case user aborts the clarification context menu.
            SetDrawItem( NULL );
            m_canvas->SetAbortRequest( true );   // Changed to false if an item is selected
            PopupMenu( &selectMenu );
            m_canvas->MoveCursorToCrossHair();
            item = GetDrawItem();
        }
    }

    if( item )
    {
        MSG_PANEL_ITEMS items;
        item->GetMsgPanelInfo( m_UserUnits, items );
        SetMsgPanel( items );
    }
    else
    {
        ClearMsgPanel();
    }

    return item;
}


void LIB_EDIT_FRAME::deleteItem( wxDC* aDC, LIB_ITEM* aItem )
{
    if( !aItem )
        return;

    m_canvas->CrossHairOff( aDC );

    LIB_PART*      part = GetCurPart();

    SaveCopyInUndoList( part );

    if( aItem->Type() == LIB_PIN_T )
    {
        LIB_PIN*    pin = static_cast<LIB_PIN*>( aItem );
        wxPoint     pos = pin->GetPosition();

        part->RemoveDrawItem( (LIB_ITEM*) pin, m_canvas, aDC );

        // when pin editing is synchronized, all pins of the same body style are removed:
        if( SynchronizePins() )
        {
            int curr_convert = pin->GetConvert();
            LIB_PIN* next_pin = part->GetNextPin();

            while( next_pin != NULL )
            {
                pin = next_pin;
                next_pin = part->GetNextPin( pin );

                if( pin->GetPosition() != pos )
                    continue;

                if( pin->GetConvert() != curr_convert )
                    continue;

                part->RemoveDrawItem( pin );
            }
        }
    }
    else
    {
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallEndMouseCapture( aDC );
        else
            part->RemoveDrawItem( aItem, m_canvas, aDC );
    }

    SetDrawItem( NULL );
    m_lastDrawItem = NULL;
    OnModify();
    m_canvas->CrossHairOn( aDC );

    RebuildView();
    GetCanvas()->Refresh();
}


void LIB_EDIT_FRAME::OnModify()
{
    GetScreen()->SetModify();
    storeCurrentPart();

    m_treePane->GetLibTree()->Refresh();
}


void LIB_EDIT_FRAME::OnSelectItem( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();
    int index = id - ID_SELECT_ITEM_START;

    if( (id >= ID_SELECT_ITEM_START && id <= ID_SELECT_ITEM_END)
        && (index >= 0 && index < m_collectedItems.GetCount()) )
    {
        LIB_ITEM* item = m_collectedItems[index];
        m_canvas->SetAbortRequest( false );
        SetDrawItem( item );
    }
}


void LIB_EDIT_FRAME::OnOpenPinTable( wxCommandEvent& aEvent )
{
    LIB_PART* part = GetCurPart();
    SaveCopyInUndoList( part );
    SetDrawItem( nullptr );

    DIALOG_LIB_EDIT_PIN_TABLE dlg( this, part );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    RebuildView();
    GetCanvas()->Refresh();
    OnModify();
}


bool LIB_EDIT_FRAME::SynchronizePins()
{
    LIB_PART* part = GetCurPart();

    return m_syncPinEdit && part && part->IsMulti() && !part->UnitsLocked();
}


void LIB_EDIT_FRAME::refreshSchematic()
{
    // There may be no parent window so use KIWAY message to refresh the schematic editor
    // in case any symbols have changed.
    Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_REFRESH, std::string( "" ), this );
}


bool LIB_EDIT_FRAME::addLibraryFile( bool aCreateNew )
{
    wxFileName fn = m_libMgr->GetUniqueLibraryName();

    if( !LibraryFileBrowser( !aCreateNew, fn,
                             SchematicLibraryFileWildcard(), SchematicLibraryFileExtension,
                             false ) )
    {
        return false;
    }

    wxString libName = fn.GetName();

    if( libName.IsEmpty() )
        return false;

    if( m_libMgr->LibraryExists( libName ) )
    {
        DisplayError( this, wxString::Format( _( "Library \"%s\" already exists" ), libName ) );
        return false;
    }

    // Select the target library table (global/project)
    SYMBOL_LIB_TABLE* libTable = selectSymLibTable();

    if( !libTable )
        return false;

    if( aCreateNew )
    {
        if( !m_libMgr->CreateLibrary( fn.GetFullPath(), libTable ) )
        {
            DisplayError( this, _( "Could not create the library file. Check write permission." ) );
            return false;
        }
    }
    else
    {
        if( !m_libMgr->AddLibrary( fn.GetFullPath(), libTable ) )
        {
            DisplayError( this, _( "Could not open the library file." ) );
            return false;
        }
    }

    bool globalTable = ( libTable == &SYMBOL_LIB_TABLE::GetGlobalLibTable() );
    saveSymbolLibTables( globalTable, !globalTable );

    return true;
}


LIB_PART* LIB_EDIT_FRAME::getTargetPart() const
{
    LIB_ALIAS* alias = nullptr;

    if( m_treePane->GetLibTree()->IsMenuActive() )
    {
        LIB_ID libId = m_treePane->GetLibTree()->GetSelectedLibId();
        alias = m_libMgr->GetAlias( libId.GetLibItemName(), libId.GetLibNickname() );
    }
    else if( LIB_PART* part = GetCurPart() )
    {
        alias = part->GetAlias( 0 );
    }

    return alias ? alias->GetPart() : nullptr;
}


LIB_ID LIB_EDIT_FRAME::getTargetLibId() const
{
    LIB_ID   id = m_treePane->GetLibTree()->GetSelectedLibId();
    wxString nickname = id.GetLibNickname();

    if( nickname.IsEmpty() && GetCurPart() )
        id = GetCurPart()->GetLibId();

    return id;
}


wxString LIB_EDIT_FRAME::getTargetLib() const
{
    return getTargetLibId().GetLibNickname();
}


void LIB_EDIT_FRAME::SyncLibraries( bool aShowProgress )
{
    LIB_ID selected;

    if( m_treePane )
        selected = m_treePane->GetLibTree()->GetSelectedLibId();

    if( aShowProgress )
    {
        wxProgressDialog progressDlg( _( "Loading Symbol Libraries" ), wxEmptyString,
                                      m_libMgr->GetAdapter()->GetLibrariesCount(), this );

        m_libMgr->Sync( true, [&]( int progress, int max, const wxString& libName )
        {
            progressDlg.Update( progress, wxString::Format( _( "Loading library \"%s\"" ), libName ) );
        } );
    }
    else
    {
        m_libMgr->Sync( true );
    }

    if( m_treePane )
    {
        wxDataViewItem found;

        if( selected.IsValid() )
        {
            // Check if the previously selected item is still valid,
            // if not - it has to be unselected to prevent crash
            found = m_libMgr->GetAdapter()->FindItem( selected );

            if( !found )
                m_treePane->GetLibTree()->Unselect();
        }

        m_treePane->Regenerate();

        // Try to select the parent library, in case the part is not found
        if( !found && selected.IsValid() )
        {
            selected.SetLibItemName( "" );
            found = m_libMgr->GetAdapter()->FindItem( selected );

            if( found )
                m_treePane->GetLibTree()->SelectLibId( selected );
        }
    }
}


SYMBOL_LIB_TABLE* LIB_EDIT_FRAME::selectSymLibTable( bool aOptional )
{
    wxArrayString libTableNames;
    libTableNames.Add( _( "Global" ) );
    libTableNames.Add( _( "Project" ) );

    wxSingleChoiceDialog dlg( this, _( "Choose the Library Table to add the library to:" ),
                              _( "Add To Library Table" ), libTableNames );

    if( aOptional )
    {
        dlg.FindWindow( wxID_CANCEL )->SetLabel( _( "Skip" ) );
        dlg.FindWindow( wxID_OK )->SetLabel( _( "Add" ) );
    }

    if( dlg.ShowModal() != wxID_OK )
        return nullptr;

    switch( dlg.GetSelection() )
    {
    case 0:  return &SYMBOL_LIB_TABLE::GetGlobalLibTable();
    case 1:  return Prj().SchSymbolLibTable();
    default: return nullptr;
    }
}


bool LIB_EDIT_FRAME::backupFile( const wxFileName& aOriginalFile, const wxString& aBackupExt )
{
    if( aOriginalFile.FileExists() )
    {
        wxFileName backupFileName( aOriginalFile );
        backupFileName.SetExt( "bck" );

        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxCopyFile( aOriginalFile.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            DisplayError( this, _( "Failed to save backup document to file " ) +
                  backupFileName.GetFullPath() );
            return false;
        }
    }

    return true;
}


void LIB_EDIT_FRAME::storeCurrentPart()
{
    if( m_my_part && !GetCurLib().IsEmpty() && GetScreen()->IsModify() )
        m_libMgr->UpdatePart( m_my_part, GetCurLib() ); // UpdatePart() makes a copy
}


bool LIB_EDIT_FRAME::isCurrentPart( const LIB_ID& aLibId ) const
{
    // This will return the root part of any alias
    LIB_PART* part = m_libMgr->GetBufferedPart( aLibId.GetLibItemName(), aLibId.GetLibNickname() );
    // Now we can compare the libId of the current part and the root part
    return ( part && GetCurPart() && part->GetLibId() == GetCurPart()->GetLibId() );
}


void LIB_EDIT_FRAME::emptyScreen()
{
    SetCurLib( wxEmptyString );
    SetCurPart( nullptr );
    m_lastDrawItem = nullptr;
    SetDrawItem( NULL );
    SetScreen( m_dummyScreen );
    m_dummyScreen->ClearUndoRedoList();
    Zoom_Automatique( false );
    Refresh();
}


void LIB_EDIT_FRAME::CommonSettingsChanged()
{
    SCH_BASE_FRAME::CommonSettingsChanged();

    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();
    Layout();
    SendSizeEvent();
}


void LIB_EDIT_FRAME::ShowChangedLanguage()
{
    // call my base class
    SCH_BASE_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    // status bar
    UpdateMsgPanel();
}


void LIB_EDIT_FRAME::SetScreen( BASE_SCREEN* aScreen )
{
    EDA_DRAW_FRAME::SetScreen( aScreen );
}


void LIB_EDIT_FRAME::RebuildView()
{
    GetRenderSettings()->m_ShowUnit = m_unit;
    GetRenderSettings()->m_ShowConvert = m_convert;
    GetCanvas()->DisplayComponent( m_my_part );

    GetCanvas()->GetView()->HideWorksheet();
    GetCanvas()->GetView()->ClearHiddenFlags();

    GetCanvas()->Refresh();
}


void LIB_EDIT_FRAME::HardRedraw()
{
    RebuildView();
}


const BOX2I LIB_EDIT_FRAME::GetDocumentExtents() const
{
    LIB_PART*  part = GetCurPart();

    if( !part )
    {
        return BOX2I( VECTOR2I(-100, -100), VECTOR2I( 200, 200 ) );
    }
    else
    {
        EDA_RECT boundingBox = part->GetUnitBoundingBox( m_unit, m_convert );
        return BOX2I( boundingBox.GetOrigin(), VECTOR2I( boundingBox.GetWidth(), boundingBox.GetHeight() ) );
    }
}


void LIB_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_LIB_EDIT:
        if( !payload.empty() )
        {
            wxString libFileName( payload );
            wxString libNickname;
            wxString msg;

            SYMBOL_LIB_TABLE*    libTable = Prj().SchSymbolLibTable();
            const LIB_TABLE_ROW* libTableRow = libTable->FindRowByURI( libFileName );

            if( !libTableRow )
            {
                msg.Printf( _( "The current configuration does not include the symbol library\n"
                               "\"%s\".\nUse Manage Symbol Libraries to edit the configuration." ),
                            libFileName );
                DisplayErrorMessage( this, _( "Library not found in symbol library table." ), msg );
                break;
            }

            libNickname = libTableRow->GetNickName();

            if( !libTable->HasLibrary( libNickname, true ) )
            {
                msg.Printf( _( "The library with the nickname \"%s\" is not enabled\n"
                               "in the current configuration.  Use Manage Symbol Libraries to\n"
                               "edit the configuration." ), libNickname );
                DisplayErrorMessage( this, _( "Symbol library not enabled." ), msg );
                break;
            }

            SetCurLib( libNickname );

            if( m_treePane )
            {
                LIB_ID id( libNickname, wxEmptyString );
                m_treePane->GetLibTree()->ExpandLibId( id );
                m_treePane->GetLibTree()->CenterLibId( id );
            }
        }

        break;

    default:
        ;
    }
}
