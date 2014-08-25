/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2013 KiCad Developers, see change_log.txt for contributors.
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
 * @file libeditframe.cpp
 * @brief LIB_EDIT_FRAME class is the component library editor frame.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <eda_doc.h>
#include <gr_basic.h>
#include <wxEeschemaStruct.h>
#include <msgpanel.h>

#include <general.h>
#include <protos.h>
#include <eeschema_id.h>
#include <libeditframe.h>
#include <class_library.h>
#include <lib_polyline.h>
#include <lib_pin.h>

#include <kicad_device_context.h>
#include <hotkeys.h>

#include <dialogs/dialog_lib_edit_text.h>
#include <dialogs/dialog_edit_component_in_lib.h>
#include <dialogs/dialog_libedit_dimensions.h>

#include <menus_helpers.h>

#include <boost/foreach.hpp>


/* This method guarantees unique IDs for the library this run of Eeschema
 * which prevents ID conflicts and eliminates the need to recompile every
 * source file in the project when adding IDs to include/id.h. */
int ExportPartId = ::wxNewId();
int ImportPartId = ::wxNewId();
int CreateNewLibAndSavePartId = ::wxNewId();


wxString LIB_EDIT_FRAME::      m_aliasName;
int LIB_EDIT_FRAME::           m_unit    = 1;
int LIB_EDIT_FRAME::           m_convert = 1;
LIB_ITEM* LIB_EDIT_FRAME::m_lastDrawItem = NULL;
LIB_ITEM* LIB_EDIT_FRAME::m_drawItem = NULL;
bool LIB_EDIT_FRAME::          m_showDeMorgan    = false;
wxSize LIB_EDIT_FRAME::        m_clientSize      = wxSize( -1, -1 );
int LIB_EDIT_FRAME::           m_textSize        = -1;
int LIB_EDIT_FRAME::           m_textOrientation = TEXT_ORIENT_HORIZ;
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

    // Main horizontal toolbar.
    EVT_TOOL( ID_LIBEDIT_SAVE_CURRENT_LIB, LIB_EDIT_FRAME::OnSaveActiveLibrary )
    EVT_TOOL( ID_LIBEDIT_SELECT_CURRENT_LIB, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_LIBEDIT_DELETE_PART, LIB_EDIT_FRAME::DeleteOnePart )
    EVT_TOOL( ID_TO_LIBVIEW, LIB_EDIT_FRAME::OnOpenLibraryViewer )
    EVT_TOOL( ID_LIBEDIT_NEW_PART, LIB_EDIT_FRAME::CreateNewLibraryPart )
    EVT_TOOL( ID_LIBEDIT_NEW_PART_FROM_EXISTING, LIB_EDIT_FRAME::OnCreateNewPartFromExisting )

    EVT_TOOL( ID_LIBEDIT_SELECT_PART, LIB_EDIT_FRAME::LoadOneLibraryPart )
    EVT_TOOL( ID_LIBEDIT_SAVE_CURRENT_PART, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, LIB_EDIT_FRAME::GetComponentFromUndoList )
    EVT_TOOL( wxID_REDO, LIB_EDIT_FRAME::GetComponentFromRedoList )
    EVT_TOOL( ID_LIBEDIT_GET_FRAME_EDIT_PART, LIB_EDIT_FRAME::OnEditComponentProperties )
    EVT_TOOL( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, LIB_EDIT_FRAME::InstallFieldsEditorDialog )
    EVT_TOOL( ID_LIBEDIT_CHECK_PART, LIB_EDIT_FRAME::OnCheckComponent )
    EVT_TOOL( ID_DE_MORGAN_NORMAL_BUTT, LIB_EDIT_FRAME::OnSelectBodyStyle )
    EVT_TOOL( ID_DE_MORGAN_CONVERT_BUTT, LIB_EDIT_FRAME::OnSelectBodyStyle )
    EVT_TOOL( ID_LIBEDIT_VIEW_DOC, LIB_EDIT_FRAME::OnViewEntryDoc )
    EVT_TOOL( ID_LIBEDIT_EDIT_PIN_BY_PIN, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ExportPartId, LIB_EDIT_FRAME::OnExportPart )
    EVT_TOOL( CreateNewLibAndSavePartId, LIB_EDIT_FRAME::OnExportPart )
    EVT_TOOL( ImportPartId, LIB_EDIT_FRAME::OnImportPart )

    EVT_COMBOBOX( ID_LIBEDIT_SELECT_PART_NUMBER, LIB_EDIT_FRAME::OnSelectPart )
    EVT_COMBOBOX( ID_LIBEDIT_SELECT_ALIAS, LIB_EDIT_FRAME::OnSelectAlias )

    // Right vertical toolbar.
    EVT_TOOL( ID_NO_TOOL_SELECTED, LIB_EDIT_FRAME::OnSelectTool )
    EVT_TOOL_RANGE( ID_LIBEDIT_PIN_BUTT, ID_LIBEDIT_DELETE_ITEM_BUTT,
                    LIB_EDIT_FRAME::OnSelectTool )

    // menubar commands
    EVT_MENU( wxID_EXIT, LIB_EDIT_FRAME::CloseWindow )
    EVT_MENU( ID_LIBEDIT_SAVE_CURRENT_LIB_AS, LIB_EDIT_FRAME::OnSaveActiveLibrary )
    EVT_MENU( ID_LIBEDIT_GEN_PNG_FILE, LIB_EDIT_FRAME::OnPlotCurrentComponent )
    EVT_MENU( ID_LIBEDIT_GEN_SVG_FILE, LIB_EDIT_FRAME::OnPlotCurrentComponent )
    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_INDEX, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::GetKicadAbout )

    EVT_MENU( ID_COLORS_SETUP, LIB_EDIT_FRAME::OnColorConfig )
    EVT_MENU( wxID_PREFERENCES, LIB_EDIT_FRAME::OnPreferencesOptions )
    EVT_MENU( ID_CONFIG_REQ, LIB_EDIT_FRAME::InstallConfigFrame )
    EVT_MENU( ID_COLORS_SETUP, LIB_EDIT_FRAME::Process_Config )
    EVT_MENU( ID_LIBEDIT_DIMENSIONS, LIB_EDIT_FRAME::InstallDimensionsDialog )

    // Multiple item selection context menu commands.
    EVT_MENU_RANGE( ID_SELECT_ITEM_START, ID_SELECT_ITEM_END, LIB_EDIT_FRAME::OnSelectItem )

    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START, ID_PREFERENCES_HOTKEY_END,
                    LIB_EDIT_FRAME::Process_Config )

    // Context menu events and commands.
    EVT_MENU( ID_LIBEDIT_EDIT_PIN, LIB_EDIT_FRAME::OnEditPin )
    EVT_MENU( ID_LIBEDIT_ROTATE_ITEM, LIB_EDIT_FRAME::OnRotateItem )

    EVT_MENU_RANGE( ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_ITEM,
                    ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT,
                    LIB_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    LIB_EDIT_FRAME::Process_Special_Functions )

    // Update user interface elements.
    EVT_UPDATE_UI( ExportPartId, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( CreateNewLibAndSavePartId, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_SAVE_CURRENT_PART, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_CHECK_PART, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_GET_FRAME_EDIT_PART, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_NEW_PART_FROM_EXISTING, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( wxID_UNDO, LIB_EDIT_FRAME::OnUpdateUndo )
    EVT_UPDATE_UI( wxID_REDO, LIB_EDIT_FRAME::OnUpdateRedo )
    EVT_UPDATE_UI( ID_LIBEDIT_SAVE_CURRENT_LIB, LIB_EDIT_FRAME::OnUpdateSaveCurrentLib )
    EVT_UPDATE_UI( ID_LIBEDIT_VIEW_DOC, LIB_EDIT_FRAME::OnUpdateViewDoc )
    EVT_UPDATE_UI( ID_LIBEDIT_EDIT_PIN_BY_PIN, LIB_EDIT_FRAME::OnUpdatePinByPin )
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_PART_NUMBER, LIB_EDIT_FRAME::OnUpdatePartNumber )
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_ALIAS, LIB_EDIT_FRAME::OnUpdateSelectAlias )
    EVT_UPDATE_UI( ID_DE_MORGAN_NORMAL_BUTT, LIB_EDIT_FRAME::OnUpdateDeMorganNormal )
    EVT_UPDATE_UI( ID_DE_MORGAN_CONVERT_BUTT, LIB_EDIT_FRAME::OnUpdateDeMorganConvert )
    EVT_UPDATE_UI( ID_NO_TOOL_SELECTED, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI_RANGE( ID_LIBEDIT_PIN_BUTT, ID_LIBEDIT_DELETE_ITEM_BUTT,
                         LIB_EDIT_FRAME::OnUpdateEditingPart )
END_EVENT_TABLE()

#define LIB_EDIT_FRAME_NAME wxT( "LibeditFrame" )

LIB_EDIT_FRAME::LIB_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    SCH_BASE_FRAME( aKiway, aParent, FRAME_SCH_LIB_EDITOR, _( "Library Editor" ),
        wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, GetLibEditFrameName() ),
    m_my_part( 0 ),
    m_tempCopyComponent( 0 )
{
    wxASSERT( aParent );

    m_FrameName  = GetLibEditFrameName();
    m_showAxis   = true;            // true to draw axis
    m_configPath = wxT( "LibraryEditor" );
    SetShowDeMorgan( false );
    m_drawSpecificConvert = true;
    m_drawSpecificUnit    = false;
    m_HotkeysZoomAndGridList = s_Libedit_Hokeys_Descr;
    m_editPinsPerPartOrConvert = false;

    // Delayed initialization
    if( m_textSize == -1 )
        m_textSize = GetDefaultTextSize();

    // Initialize grid id to the default value 50 mils:
    m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( libedit_icon_xpm ) );
    SetIcon( icon );

    SetScreen( new SCH_SCREEN( aKiway ) );
    GetScreen()->m_Center = true;

    SetCrossHairPosition( wxPoint( 0, 0 ) );

    LoadSettings( config() );

    // Ensure m_LastGridSizeId is an offset inside the allowed schematic range
    if( m_LastGridSizeId < ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000 )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    if( m_LastGridSizeId > ID_POPUP_GRID_LEVEL_1 - ID_POPUP_GRID_LEVEL_1000 )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_1 - ID_POPUP_GRID_LEVEL_1000;

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    if( m_canvas )
        m_canvas->SetEnableBlockCommands( true );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    CreateOptionToolbar();
    DisplayLibInfos();
    DisplayCmpDoc();
    UpdateAliasSelectList();
    UpdatePartSelectList();

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO mesg;
    mesg.MessageToolbarPane();

    m_auimgr.AddPane( m_mainToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_mainToolBar" ) ).Top().Row( 0 ) );

    m_auimgr.AddPane( m_drawToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right() );

    m_auimgr.AddPane( m_optionsToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_optionsToolBar" ) ).Left() );

    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().Layer(10) );

    m_auimgr.Update();

    Show( true );

    wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ID_ZOOM_PAGE );
    wxPostEvent( this, evt );
}


LIB_EDIT_FRAME::~LIB_EDIT_FRAME()
{
    m_drawItem = m_lastDrawItem = NULL;

    delete m_tempCopyComponent;
    delete m_my_part;
}

const wxChar* LIB_EDIT_FRAME::GetLibEditFrameName()
{
    return LIB_EDIT_FRAME_NAME;
}


void LIB_EDIT_FRAME::SetDrawItem( LIB_ITEM* drawItem )
{
    m_drawItem = drawItem;
}


void LIB_EDIT_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( GetScreen()->IsModify() )
    {
        int ii = DisplayExitDialog( this, _( "Save the changes in the library before closing?" ) );

        switch( ii )
        {
        case wxID_NO:
            break;

        case wxID_YES:
            if ( this->SaveActiveLibrary( false ) )
                break;

            // fall through: cancel the close because of an error

        case wxID_CANCEL:
            Event.Veto();
            return;
        }
        GetScreen()->ClrModify();
    }

    PART_LIBS* libs = Prj().SchLibs();

    BOOST_FOREACH( const PART_LIB& lib, *libs )
    {
        if( lib.IsModified() )
        {
            wxString msg = wxString::Format( _(
                "Library '%s' was modified!\nDiscard changes?" ),
                GetChars( lib.GetName() )
                );

            if( !IsOK( this, msg ) )
            {
                Event.Veto();
                return;
            }
        }
    }

    Destroy();
}


double LIB_EDIT_FRAME::BestZoom()
{
    /* Please, note: wxMSW before version 2.9 seems have
     * problems with zoom values < 1 ( i.e. userscale > 1) and needs to be patched:
     * edit file <wxWidgets>/src/msw/dc.cpp
     * search for line static const int VIEWPORT_EXTENT = 1000;
     * and replace by static const int VIEWPORT_EXTENT = 10000;
     */
    int      dx, dy;

    LIB_PART*      part = GetCurPart();

    if( part )
    {
        EDA_RECT boundingBox = part->GetBoundingBox( m_unit, m_convert );

        dx = boundingBox.GetWidth();
        dy = boundingBox.GetHeight();
        SetScrollCenterPosition( wxPoint( 0, 0 ) );
    }
    else
    {
        const PAGE_INFO& pageInfo = GetScreen()->GetPageSettings();

        dx = pageInfo.GetSizeIU().x;
        dy = pageInfo.GetSizeIU().y;

        SetScrollCenterPosition( wxPoint( 0, 0 ) );
    }

    wxSize size = m_canvas->GetClientSize();

    // Reserve a 10% margin around component bounding box.
    double margin_scale_factor = 0.8;
    double zx =(double) dx / ( margin_scale_factor * (double)size.x );
    double zy = (double) dy / ( margin_scale_factor * (double)size.y );

    double bestzoom = std::max( zx, zy );

    // keep it >= minimal existing zoom (can happen for very small components
    // for instance when starting a new component
    if( bestzoom  < GetScreen()->m_ZoomList[0] )
        bestzoom  = GetScreen()->m_ZoomList[0];

    return bestzoom;
}


void LIB_EDIT_FRAME::UpdateAliasSelectList()
{
    if( m_aliasSelectBox == NULL )
        return;

    m_aliasSelectBox->Clear();

    LIB_PART*      part = GetCurPart();

    if( !part )
        return;

    m_aliasSelectBox->Append( part->GetAliasNames() );
    m_aliasSelectBox->SetSelection( 0 );

    int index = m_aliasSelectBox->FindString( m_aliasName );

    if( index != wxNOT_FOUND )
        m_aliasSelectBox->SetSelection( index );
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

    m_partSelectBox->SetSelection( ( m_unit > 0 ) ? m_unit - 1 : 0 );
}


void LIB_EDIT_FRAME::OnUpdateEditingPart( wxUpdateUIEvent& aEvent )
{
    LIB_PART*      part = GetCurPart();

    aEvent.Enable( part != NULL );

    if( part && aEvent.GetEventObject() == m_drawToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void LIB_EDIT_FRAME::OnUpdateNotEditingPart( wxUpdateUIEvent& event )
{
    event.Enable( !GetCurPart() );
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


void LIB_EDIT_FRAME::OnUpdateSaveCurrentLib( wxUpdateUIEvent& event )
{
    PART_LIB* lib = GetCurLib();

    event.Enable( lib && !lib->IsReadOnly()
                  && ( lib->IsModified() || GetScreen()->IsModify() ) );
}


void LIB_EDIT_FRAME::OnUpdateViewDoc( wxUpdateUIEvent& event )
{
    bool enable = false;

    PART_LIB*    lib  = GetCurLib();
    LIB_PART*       part = GetCurPart();

    if( part && lib )
    {
        LIB_ALIAS* alias = part->GetAlias( m_aliasName );

        wxCHECK_RET( alias != NULL, wxT( "Alias <" ) + m_aliasName + wxT( "> not found." ) );

        enable = !alias->GetDocFileName().IsEmpty();
    }

    event.Enable( enable );
}


void LIB_EDIT_FRAME::OnUpdatePinByPin( wxUpdateUIEvent& event )
{
    LIB_PART*      part = GetCurPart();

    event.Enable( part && ( part->GetUnitCount() > 1 || m_showDeMorgan ) );

    event.Check( m_editPinsPerPartOrConvert );
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


void LIB_EDIT_FRAME::OnUpdateSelectAlias( wxUpdateUIEvent& event )
{
    if( m_aliasSelectBox == NULL )
        return;

    LIB_PART*      part = GetCurPart();

    // Using the typical event.Enable() call doesn't seem to work with wxGTK
    // so use the pointer to alias combobox to directly enable or disable.
    m_aliasSelectBox->Enable( part && part->GetAliasCount() > 1 );
}


void LIB_EDIT_FRAME::OnSelectAlias( wxCommandEvent& event )
{
    if( m_aliasSelectBox == NULL
        || ( m_aliasSelectBox->GetStringSelection().CmpNoCase( m_aliasName ) == 0)  )
        return;

    m_lastDrawItem = NULL;
    m_aliasName = m_aliasSelectBox->GetStringSelection();

    DisplayCmpDoc();
    m_canvas->Refresh();
}


void LIB_EDIT_FRAME::OnSelectPart( wxCommandEvent& event )
{
    int i = event.GetSelection();

    if( ( i == wxNOT_FOUND ) || ( ( i + 1 ) == m_unit ) )
        return;

    m_lastDrawItem = NULL;
    m_unit = i + 1;
    m_canvas->Refresh();
    DisplayCmpDoc();
}


void LIB_EDIT_FRAME::OnViewEntryDoc( wxCommandEvent& event )
{
    LIB_PART* part = GetCurPart();

    if( !part )
        return;

    wxString    fileName;
    LIB_ALIAS*  alias = part->GetAlias( m_aliasName );

    wxCHECK_RET( alias != NULL, wxT( "Alias not found." ) );

    fileName = alias->GetDocFileName();

    if( !fileName.IsEmpty() )
    {
        SEARCH_STACK* lib_search = Prj().SchSearchS();

        GetAssociatedDocument( this, fileName, lib_search );
    }
}


void LIB_EDIT_FRAME::OnSelectBodyStyle( wxCommandEvent& event )
{
    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    if( event.GetId() == ID_DE_MORGAN_NORMAL_BUTT )
        m_convert = 1;
    else
        m_convert = 2;

    m_lastDrawItem = NULL;
    m_canvas->Refresh();
}


void LIB_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int     id = event.GetId();
    wxPoint pos;

    m_canvas->SetIgnoreMouseEvents( true );

    wxGetMousePosition( &pos.x, &pos.y );
    pos.y += 20;

    switch( id )   // Stop placement commands before handling new command.
    {
    case ID_POPUP_LIBEDIT_END_CREATE_ITEM:
    case ID_LIBEDIT_EDIT_PIN:
    case ID_POPUP_LIBEDIT_BODY_EDIT_ITEM:
    case ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_COPY_BLOCK:
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
            m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );
        break;

    case ID_POPUP_LIBEDIT_DELETE_ITEM:
        m_canvas->EndMouseCapture();
        break;

    default:
        m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(),
                                    wxEmptyString );
        break;
    }

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    switch( id )
    {
    case ID_POPUP_LIBEDIT_CANCEL_EDITING:
        break;

    case ID_LIBEDIT_SELECT_CURRENT_LIB:
        SelectActiveLibrary();
        break;

    case ID_LIBEDIT_SAVE_CURRENT_PART:
        {
            LIB_PART*   part = GetCurPart();

            if( !part )
            {
                DisplayError( this, _( "No part to save." ) );
                break;
            }

            PART_LIB*   lib  = GetCurLib();

            if( !lib )
                SelectActiveLibrary();

            lib = GetCurLib();

            if( !lib )
            {
                DisplayError( this, _( "No library specified." ) );
                break;
            }

            SaveOnePart( lib );
        }
        break;

    case ID_LIBEDIT_EDIT_PIN_BY_PIN:
        m_editPinsPerPartOrConvert = m_mainToolBar->GetToolToggled( ID_LIBEDIT_EDIT_PIN_BY_PIN );
        break;

    case ID_POPUP_LIBEDIT_END_CREATE_ITEM:
        m_canvas->MoveCursorToCrossHair();
        if( m_drawItem )
        {
            EndDrawGraphicItem( &dc );
        }
        break;

    case ID_POPUP_LIBEDIT_BODY_EDIT_ITEM:
        if( m_drawItem )
        {
            m_canvas->CrossHairOff( &dc );

            switch( m_drawItem->Type() )
            {
            case LIB_ARC_T:
            case LIB_CIRCLE_T:
            case LIB_RECTANGLE_T:
            case LIB_POLYLINE_T:
                EditGraphicSymbol( &dc, m_drawItem );
                break;

            case LIB_TEXT_T:
                EditSymbolText( &dc, m_drawItem );
                break;

            default:
                ;
            }

            m_canvas->CrossHairOn( &dc );
        }
        break;

    case ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT:
        {
            // Delete the last created segment, while creating a polyline draw item
            if( m_drawItem == NULL )
                break;

            m_canvas->MoveCursorToCrossHair();
            STATUS_FLAGS oldFlags = m_drawItem->GetFlags();
            m_drawItem->ClearFlags();
            m_drawItem->Draw( m_canvas, &dc, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, g_XorMode, NULL,
                              DefaultTransform );
            ( (LIB_POLYLINE*) m_drawItem )->DeleteSegment( GetCrossHairPosition( true ) );
            m_drawItem->Draw( m_canvas, &dc, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, g_XorMode, NULL,
                              DefaultTransform );
            m_drawItem->SetFlags( oldFlags );
            m_lastDrawItem = NULL;
        }
        break;

    case ID_POPUP_LIBEDIT_DELETE_ITEM:
        if( m_drawItem )
            deleteItem( &dc );

        break;

    case ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST:
        if( m_drawItem == NULL )
            break;

        if( m_drawItem->Type() == LIB_PIN_T )
            StartMovePin( &dc );
        else
            StartMoveDrawSymbol( &dc );
        break;

    case ID_POPUP_LIBEDIT_MODIFY_ITEM:

        if( m_drawItem == NULL )
            break;

        m_canvas->MoveCursorToCrossHair();
        if( m_drawItem->Type() == LIB_RECTANGLE_T
            || m_drawItem->Type() == LIB_CIRCLE_T
            || m_drawItem->Type() == LIB_POLYLINE_T
            || m_drawItem->Type() == LIB_ARC_T
            )
        {
            StartModifyDrawSymbol( &dc );
        }

        break;

    case ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM:
        if( m_drawItem == NULL )
            break;

        m_canvas->CrossHairOff( &dc );

        if( m_drawItem->Type() == LIB_FIELD_T )
        {
            EditField( (LIB_FIELD*) m_drawItem );
        }

        m_canvas->MoveCursorToCrossHair();
        m_canvas->CrossHairOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
        {
            if( !m_drawItem || m_drawItem->Type() != LIB_PIN_T )
                break;

            LIB_PART*      part = GetCurPart();

            SaveCopyInUndoList( part );

            GlobalSetPins( (LIB_PIN*) m_drawItem, id );
            m_canvas->MoveCursorToCrossHair();
            m_canvas->Refresh();
        }
        break;

    case ID_POPUP_ZOOM_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_ZOOM );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_DELETE );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_COPY );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_SELECT_ITEMS_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_SELECT_ITEMS_ONLY );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_MIRROR_Y_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_MIRROR_Y );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_MIRROR_X_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_MIRROR_X );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_ROTATE_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_ROTATE );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_PLACE_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockPlace( &dc );
        break;

    default:
        DisplayError( this, wxT( "LIB_EDIT_FRAME::Process_Special_Functions error" ) );
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


PART_LIB* LIB_EDIT_FRAME::GetCurLib()
{
    wxString  name = Prj().GetRString( PROJECT::SCH_LIBEDIT_CUR_LIB );

    if( !!name )
    {
        PART_LIB* lib = Prj().SchLibs()->FindLibrary( name );

        if( !lib )
            Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, wxEmptyString );

        return lib;
    }

    return NULL;
}


PART_LIB* LIB_EDIT_FRAME::SetCurLib( PART_LIB* aLib )
{
    PART_LIB* old = GetCurLib();

    if( !aLib || !aLib->GetName() )
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, wxEmptyString );
    else
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, aLib->GetName() );

    return old;
}


LIB_PART* LIB_EDIT_FRAME::GetCurPart()
{
    if( !m_my_part )
    {
        wxString    name = Prj().GetRString( PROJECT::SCH_LIBEDIT_CUR_PART );
        LIB_PART*   part;

        if( !!name && ( part = Prj().SchLibs()->FindLibPart( name ) ) )
        {
            // clone it from the PART_LIB and own it.
            m_my_part = new LIB_PART( *part );
        }
        else
            Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_PART, wxEmptyString );
    }

    return m_my_part;
}


void LIB_EDIT_FRAME::SetCurPart( LIB_PART* aPart )
{
    delete m_my_part;
    m_my_part = aPart;      // take ownership here

    // retain in case this wxFrame is re-opened later on the same PROJECT
    Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_PART,
            aPart ? aPart->GetName() : wxString() );
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
        DrawItem->Draw( m_canvas, DC, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, g_XorMode, NULL,
                        DefaultTransform );

    DIALOG_LIB_EDIT_TEXT* frame = new DIALOG_LIB_EDIT_TEXT( this, (LIB_TEXT*) DrawItem );
    frame->ShowModal();
    frame->Destroy();
    OnModify();

    // Display new text
    if( DC && !DrawItem->InEditMode() )
        DrawItem->Draw( m_canvas, DC, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, GR_DEFAULT_DRAWMODE, NULL,
                        DefaultTransform );
}


void LIB_EDIT_FRAME::OnEditComponentProperties( wxCommandEvent& event )
{
    bool partLocked = GetCurPart()->UnitsLocked();

    DIALOG_EDIT_COMPONENT_IN_LIBRARY dlg( this );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( partLocked != GetCurPart()->UnitsLocked() )
    {
        // m_editPinsPerPartOrConvert is set to the better value, if m_UnitSelectionLocked
        // has changed
        m_editPinsPerPartOrConvert = GetCurPart()->UnitsLocked() ? true : false;
    }

    UpdateAliasSelectList();
    UpdatePartSelectList();
    DisplayLibInfos();
    DisplayCmpDoc();
    OnModify();
    m_canvas->Refresh();
}


void LIB_EDIT_FRAME::InstallDimensionsDialog( wxCommandEvent& event )
{
    DIALOG_LIBEDIT_DIMENSIONS dlg( this );
    dlg.ShowModal();
}


void LIB_EDIT_FRAME::OnCreateNewPartFromExisting( wxCommandEvent& event )
{
    LIB_PART*      part = GetCurPart();

    wxCHECK_RET( part, wxT( "Cannot create new part from non-existent current part." ) );

    INSTALL_UNBUFFERED_DC( dc, m_canvas );
    m_canvas->CrossHairOff( &dc );

    EditField( &part->GetValueField() );

    m_canvas->MoveCursorToCrossHair();
    m_canvas->CrossHairOn( &dc );
}


void LIB_EDIT_FRAME::OnSelectTool( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        m_lastDrawItem = NULL;

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(),
                               wxEmptyString );

    LIB_PART*      part = GetCurPart();

    switch( id )
    {
    case ID_NO_TOOL_SELECTED:
        SetToolID( id, m_canvas->GetDefaultCursor(), wxEmptyString );
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
            SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
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
        SetToolID( id, m_canvas->GetDefaultCursor(), _( "Import" ) );
        LoadOneSymbol();
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        break;

    case ID_LIBEDIT_EXPORT_BODY_BUTT:
        SetToolID( id, m_canvas->GetDefaultCursor(), _( "Export" ) );
        SaveOneSymbol();
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
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


void LIB_EDIT_FRAME::OnRotateItem( wxCommandEvent& aEvent )
{
    if( m_drawItem == NULL )
        return;

    if( !m_drawItem->InEditMode() )
    {
        LIB_PART*      part = GetCurPart();

        SaveCopyInUndoList( part );
        m_drawItem->SetUnit( m_unit );
    }

    m_drawItem->Rotate();
    OnModify();

    if( !m_drawItem->InEditMode() )
        m_drawItem->ClearFlags();

    m_canvas->Refresh();

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        m_lastDrawItem = NULL;
}


LIB_ITEM* LIB_EDIT_FRAME::LocateItemUsingCursor( const wxPoint& aPosition,
                                                 const KICAD_T aFilterList[] )
{
    LIB_PART*      part = GetCurPart();

    if( !part )
        return NULL;

    LIB_ITEM* item = locateItem( aPosition, aFilterList );

    if( item == NULL )
        return NULL;

    wxPoint pos = GetNearestGridPosition( aPosition );

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

    m_collectedItems.Collect( part->GetDrawItemList(), aFilterList, aPosition,
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
                          wxT( "Select item clarification context menu size limit exceeded." ) );

            wxMenu selectMenu;
            wxMenuItem* title = new wxMenuItem( &selectMenu, wxID_NONE, _( "Clarify Selection" ) );

            selectMenu.Append( title );
            selectMenu.AppendSeparator();

            for( int i = 0;  i < m_collectedItems.GetCount() && i < MAX_SELECT_ITEM_IDS;  i++ )
            {
                wxString    text = m_collectedItems[i]->GetSelectMenuText();
                BITMAP_DEF  xpm = m_collectedItems[i]->GetMenuImage();

                AddMenuItem( &selectMenu, ID_SELECT_ITEM_START + i, text, KiBitmap( xpm ) );
            }

            // Set to NULL in case user aborts the clarification context menu.
            m_drawItem = NULL;
            m_canvas->SetAbortRequest( true );   // Changed to false if an item is selected
            PopupMenu( &selectMenu );
            m_canvas->MoveCursorToCrossHair();
            item = m_drawItem;
        }
    }

    if( item )
    {
        MSG_PANEL_ITEMS items;
        item->GetMsgPanelInfo( items );
        SetMsgPanel( items );
    }
    else
    {
        ClearMsgPanel();
    }

    return item;
}


void LIB_EDIT_FRAME::deleteItem( wxDC* aDC )
{
    wxCHECK_RET( m_drawItem != NULL, wxT( "No drawing item selected to delete." ) );

    m_canvas->CrossHairOff( aDC );

    LIB_PART*      part = GetCurPart();

    SaveCopyInUndoList( part );

    if( m_drawItem->Type() == LIB_PIN_T )
    {
        LIB_PIN*    pin = (LIB_PIN*) m_drawItem;
        wxPoint     pos = pin->GetPosition();

        part->RemoveDrawItem( (LIB_ITEM*) pin, m_canvas, aDC );

        if( SynchronizePins() )
        {
            LIB_PIN* tmp = part->GetNextPin();

            while( tmp != NULL )
            {
                pin = tmp;
                tmp = part->GetNextPin( pin );

                if( pin->GetPosition() != pos )
                    continue;

                part->RemoveDrawItem( (LIB_ITEM*) pin );
            }
        }

        m_canvas->Refresh();
    }
    else
    {
        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->CallEndMouseCapture( aDC );
        }
        else
        {
            part->RemoveDrawItem( m_drawItem, m_canvas, aDC );
            m_canvas->Refresh();
        }
    }

    m_drawItem = NULL;
    m_lastDrawItem = NULL;
    OnModify();
    m_canvas->CrossHairOn( aDC );
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
        m_drawItem = item;
    }
}


bool LIB_EDIT_FRAME::SynchronizePins()
{
    LIB_PART*      part = GetCurPart();

    return !m_editPinsPerPartOrConvert && ( part &&
        ( part->HasConversion() || part->IsMulti() ) );
}
