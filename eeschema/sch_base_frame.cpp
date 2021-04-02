/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2020 KiCad Developers, see change_log.txt for contributors.
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

#include <base_units.h>
#include <kiway.h>
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <gal/graphics_abstraction_layer.h>
#include <confirm.h>
#include <preview_items/selection_area.h>
#include <class_library.h>
#include <eeschema_id.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <sch_base_frame.h>
#include <symbol_lib_table.h>
#include <dialog_configure_paths.h>

#include "dialogs/panel_sym_lib_table.h"



LIB_ALIAS* SchGetLibAlias( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aLibTable, PART_LIB* aCacheLib,
                           wxWindow* aParent, bool aShowErrorMsg )
{
    wxCHECK_MSG( aLibTable, nullptr, "Invalid symbol library table." );

    LIB_ALIAS* symbol = nullptr;

    try
    {
        symbol = aLibTable->LoadSymbol( aLibId );

        if( !symbol && aCacheLib )
        {
            wxCHECK_MSG( aCacheLib->IsCache(), nullptr, "Invalid cache library." );

            wxString cacheName = aLibId.GetLibNickname().wx_str();
            cacheName += "_" + aLibId.GetLibItemName();
            symbol = aCacheLib->FindAlias( cacheName );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        if( aShowErrorMsg )
        {
            wxString msg;

            msg.Printf( _( "Could not load symbol \"%s\" from library \"%s\"." ),
                        aLibId.GetLibItemName().wx_str(), aLibId.GetLibNickname().wx_str() );
            DisplayErrorMessage( aParent, msg, ioe.What() );
        }
    }

    return symbol;
}


LIB_PART* SchGetLibPart( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aLibTable, PART_LIB* aCacheLib,
                         wxWindow* aParent, bool aShowErrorMsg )
{
    LIB_ALIAS* alias = SchGetLibAlias( aLibId, aLibTable, aCacheLib, aParent, aShowErrorMsg );

    return ( alias ) ? alias->GetPart() : NULL;
}


// Static members:

SCH_BASE_FRAME::SCH_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent,
        FRAME_T aWindowType, const wxString& aTitle,
        const wxPoint& aPosition, const wxSize& aSize, long aStyle,
        const wxString& aFrameName ) :
    EDA_DRAW_FRAME( aKiway, aParent, aWindowType, aTitle, aPosition,
            aSize, aStyle, aFrameName )
{
    createCanvas();

    m_zoomLevelCoeff = 11.0;    // Adjusted to roughly displays zoom level = 1
                                // when the screen shows a 1:1 image
                                // obviously depends on the monitor,
                                // but this is an acceptable value
    m_repeatStep = wxPoint( DEFAULT_REPEAT_OFFSET_X, DEFAULT_REPEAT_OFFSET_Y );
    m_repeatDeltaLabel = DEFAULT_REPEAT_LABEL_INC;
}


SCH_BASE_FRAME::~SCH_BASE_FRAME()
{
}


void SCH_BASE_FRAME::OnUpdateSwitchCanvas( wxUpdateUIEvent& aEvent )
{
    wxMenuBar* menuBar = GetMenuBar();
    EDA_DRAW_PANEL_GAL* gal_canvas = GetGalCanvas();
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = gal_canvas->GetBackend();

    struct { int menuId; int galType; } menuList[] =
    {
        { ID_MENU_CANVAS_OPENGL,    EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL },
        { ID_MENU_CANVAS_CAIRO,     EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO },
    };

    for( auto ii: menuList )
    {
        wxMenuItem* item = menuBar->FindItem( ii.menuId );
        if( ii.galType == canvasType )
        {
            item->Check( true );
        }
    }
}


void SCH_BASE_FRAME::OnSwitchCanvas( wxCommandEvent& aEvent )
{
    auto new_type = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;

#ifndef __WXMAC__
    if( aEvent.GetId() == ID_MENU_CANVAS_CAIRO )
        new_type = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif

    if( m_canvasType == new_type )
        return;

    GetGalCanvas()->SwitchBackend( new_type );
    m_canvasType = new_type;
}


void SCH_BASE_FRAME::OnOpenLibraryViewer( wxCommandEvent& event )
{
    LIB_VIEW_FRAME* viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, true );

    viewlibFrame->PushPreferences( m_canvas );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( viewlibFrame->IsIconized() )
        viewlibFrame->Iconize( false );

    viewlibFrame->Show( true );
    viewlibFrame->Raise();
}


SCH_SCREEN* SCH_BASE_FRAME::GetScreen() const
{
    return (SCH_SCREEN*) EDA_DRAW_FRAME::GetScreen();
}


const wxString SCH_BASE_FRAME::GetZoomLevelIndicator() const
{
    return EDA_DRAW_FRAME::GetZoomLevelIndicator();
}


void SCH_BASE_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    GetScreen()->SetPageSettings( aPageSettings );
}


const PAGE_INFO& SCH_BASE_FRAME::GetPageSettings () const
{
    return GetScreen()->GetPageSettings();
}


const wxSize SCH_BASE_FRAME::GetPageSizeIU() const
{
    // GetSizeIU is compile time dependent:
    return GetScreen()->GetPageSettings().GetSizeIU();
}


const wxPoint& SCH_BASE_FRAME::GetAuxOrigin() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetAuxOrigin();
}


void SCH_BASE_FRAME::SetAuxOrigin( const wxPoint& aPosition )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetAuxOrigin( aPosition );
}


const TITLE_BLOCK& SCH_BASE_FRAME::GetTitleBlock() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetTitleBlock();
}


void SCH_BASE_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetTitleBlock( aTitleBlock );
}


void SCH_BASE_FRAME::UpdateStatusBar()
{
    wxString        line;
    BASE_SCREEN*    screen = GetScreen();

    if( !screen )
        return;

    EDA_DRAW_FRAME::UpdateStatusBar();

    // Display absolute coordinates:
    double dXpos = To_User_Unit( GetUserUnits(), GetCrossHairPosition().x );
    double dYpos = To_User_Unit( GetUserUnits(), GetCrossHairPosition().y );

    if ( GetUserUnits() == MILLIMETRES )
    {
        dXpos = RoundTo0( dXpos, 100.0 );
        dYpos = RoundTo0( dYpos, 100.0 );
    }

    wxString absformatter;
    wxString locformatter;

    switch( GetUserUnits() )
    {
    case INCHES:
        absformatter = "X %.3f  Y %.3f";
        locformatter = "dx %.3f  dy %.3f  dist %.3f";
        break;

    case MILLIMETRES:
        absformatter = "X %.2f  Y %.2f";
        locformatter = "dx %.2f  dy %.2f  dist %.2f";
        break;

    case UNSCALED_UNITS:
        absformatter = "X %f  Y %f";
        locformatter = "dx %f  dy %f  dist %f";
        break;

    case PERCENT:
    case DEGREES:
        wxASSERT( false );
        break;
    }

    line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( line, 2 );

    // Display relative coordinates:
    double dx = (double)GetCrossHairPosition().x - (double)screen->m_O_Curseur.x;
    double dy = (double)GetCrossHairPosition().y - (double)screen->m_O_Curseur.y;

    dXpos = To_User_Unit( GetUserUnits(), dx );
    dYpos = To_User_Unit( GetUserUnits(), dy );

    if( GetUserUnits() == MILLIMETRES )
    {
        dXpos = RoundTo0( dXpos, 100.0 );
        dYpos = RoundTo0( dYpos, 100.0 );
    }

    // We already decided the formatter above
    line.Printf( locformatter, dXpos, dYpos, hypot( dXpos, dYpos ) );
    SetStatusText( line, 3 );

    // refresh grid display
    DisplayGridMsg();

    // refresh units display
    DisplayUnitsMsg();
}


void SCH_BASE_FRAME::OnConfigurePaths( wxCommandEvent& aEvent )
{
    DIALOG_CONFIGURE_PATHS dlg( this, nullptr );
    dlg.ShowModal();
}


void SCH_BASE_FRAME::OnEditSymbolLibTable( wxCommandEvent& aEvent )
{
    InvokeSchEditSymbolLibTable( &Kiway(), this );
}


LIB_ALIAS* SCH_BASE_FRAME::GetLibAlias( const LIB_ID& aLibId, bool aUseCacheLib, bool aShowError )
{
    PART_LIB* cache = ( aUseCacheLib ) ? Prj().SchLibs()->GetCacheLibrary() : NULL;

    return SchGetLibAlias( aLibId, Prj().SchSymbolLibTable(), cache, this, aShowError );
}


LIB_PART* SCH_BASE_FRAME::GetLibPart( const LIB_ID& aLibId, bool aUseCacheLib, bool aShowErrorMsg )
{
    PART_LIB* cache = ( aUseCacheLib ) ? Prj().SchLibs()->GetCacheLibrary() : NULL;

    return SchGetLibPart( aLibId, Prj().SchSymbolLibTable(), cache, this, aShowErrorMsg );
}


bool SCH_BASE_FRAME::saveSymbolLibTables( bool aGlobal, bool aProject )
{
    wxString msg;
    bool success = true;

    if( aGlobal )
    {
        try
        {
            SYMBOL_LIB_TABLE::GetGlobalLibTable().Save( SYMBOL_LIB_TABLE::GetGlobalTableFileName() );
        }
        catch( const IO_ERROR& ioe )
        {
            success = false;
            msg.Printf( _( "Error saving global symbol library table:\n\n%s" ), ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    if( aProject && !Prj().GetProjectName().IsEmpty() )
    {
        wxFileName fn( Prj().GetProjectPath(), SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        try
        {
            Prj().SchSymbolLibTable()->Save( fn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            success = false;
            msg.Printf( _( "Error saving project-specific symbol library table:\n\n%s" ), ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    return success;
}


// Set the zoom level to show the contents of the view.
void SCH_BASE_FRAME::Zoom_Automatique( bool aWarpPointer )
{
    EDA_DRAW_PANEL_GAL* galCanvas = GetGalCanvas();
    KIGFX::VIEW* view = GetGalCanvas()->GetView();

    BOX2I bBox = GetDocumentExtents();

    VECTOR2D scrollbarSize = VECTOR2D( galCanvas->GetSize() - galCanvas->GetClientSize() );
    VECTOR2D screenSize = view->ToWorld( galCanvas->GetClientSize(), false );

    if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
    {
        bBox = galCanvas->GetDefaultViewBBox();
    }

    VECTOR2D vsize = bBox.GetSize();
    double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                                                fabs( vsize.y / screenSize.y ) );

    // Reserve a 10% margin around component bounding box.
    double margin_scale_factor = 1.1;

    // Leave 20% for library editors & viewers
    if( IsType( FRAME_PCB_MODULE_VIEWER ) || IsType( FRAME_PCB_MODULE_VIEWER_MODAL )
            || IsType( FRAME_SCH_VIEWER ) || IsType( FRAME_SCH_VIEWER_MODAL )
            || IsType( FRAME_SCH_LIB_EDITOR ) || IsType( FRAME_PCB_MODULE_EDITOR ) )
    {
        margin_scale_factor = 1.2;
    }

    scale /= margin_scale_factor;

    GetScreen()->SetScalingFactor( 1 / scale );

    view->SetScale( scale );
    view->SetCenter( bBox.Centre() );

    // Take scrollbars into account
    VECTOR2D worldScrollbarSize = view->ToWorld( scrollbarSize, false );
    view->SetCenter( view->GetCenter() + worldScrollbarSize / 2.0 );
    galCanvas->Refresh();
}


// Set the zoom level to show the area of aRect
void SCH_BASE_FRAME::Window_Zoom( EDA_RECT& aRect )
{
    KIGFX::VIEW* view = GetGalCanvas()->GetView();
    BOX2I selectionBox ( aRect.GetPosition(), aRect.GetSize() );

    VECTOR2D screenSize = view->ToWorld( GetGalCanvas()->GetClientSize(), false );

    if( selectionBox.GetWidth() == 0 || selectionBox.GetHeight() == 0 )
        return;

    VECTOR2D vsize = selectionBox.GetSize();
    double scale;
    double ratio = std::max( fabs( vsize.x / screenSize.x ),
                             fabs( vsize.y / screenSize.y ) );

    scale = view->GetScale() / ratio;

    GetScreen()->SetScalingFactor( 1 / scale );

    view->SetScale( scale );
    view->SetCenter( selectionBox.Centre() );
    GetGalCanvas()->Refresh();
}


void SCH_BASE_FRAME::RedrawScreen( const wxPoint& aCenterPoint, bool aWarpPointer )
{
    KIGFX::GAL* gal = GetCanvas()->GetGAL();

    double selectedZoom = GetScreen()->GetZoom();
    double zoomFactor = gal->GetWorldScale() / gal->GetZoomFactor();
    double scale = 1.0 / ( zoomFactor * selectedZoom );

    if( aCenterPoint != wxPoint( 0, 0 ) )
        GetCanvas()->GetView()->SetScale( scale, aCenterPoint );
    else
        GetCanvas()->GetView()->SetScale( scale );

    if( aWarpPointer )
        GetCanvas()->GetViewControls()->CenterOnCursor();

    GetCanvas()->Refresh();
}


void SCH_BASE_FRAME::RedrawScreen2( const wxPoint& posBefore )
{
    KIGFX::GAL* gal = GetCanvas()->GetGAL();

    double selectedZoom = GetScreen()->GetZoom();
    double zoomFactor = gal->GetWorldScale() / gal->GetZoomFactor();
    double scale = 1.0 / ( zoomFactor * selectedZoom );

    GetCanvas()->GetView()->SetScale( scale );

    GetGalCanvas()->Refresh();
}


void SCH_BASE_FRAME::CenterScreen( const wxPoint& aCenterPoint, bool aWarpPointer )
{
    GetCanvas()->GetView()->SetCenter( aCenterPoint );

    if( aWarpPointer )
        GetCanvas()->GetViewControls()->WarpCursor( aCenterPoint, true );

    GetGalCanvas()->Refresh();
}


void SCH_BASE_FRAME::HardRedraw()
{
    // Currently: just refresh the screen
    GetCanvas()->Refresh();
}


SCH_DRAW_PANEL* SCH_BASE_FRAME::GetCanvas() const
{
    return static_cast<SCH_DRAW_PANEL*>( GetGalCanvas() );
}


KIGFX::SCH_RENDER_SETTINGS* SCH_BASE_FRAME::GetRenderSettings()
{
    KIGFX::PAINTER* painter = GetGalCanvas()->GetView()->GetPainter();
    return static_cast<KIGFX::SCH_RENDER_SETTINGS*>( painter->GetSettings() );
}


bool SCH_BASE_FRAME::HandleBlockBegin( wxDC* aDC, EDA_KEY aKey, const wxPoint& aPosition,
                                       int aExplicitCommand )
{
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    if( ( block->GetCommand() != BLOCK_IDLE ) || ( block->GetState() != STATE_NO_BLOCK ) )
        return false;

    if( aExplicitCommand == 0 )
        block->SetCommand( (BLOCK_COMMAND_T) BlockCommand( aKey ) );
    else
        block->SetCommand( (BLOCK_COMMAND_T) aExplicitCommand );

    if( block->GetCommand() == 0 )
        return false;

    switch( block->GetCommand() )
    {
    case BLOCK_IDLE:
        break;

    case BLOCK_MOVE:                // Move
    case BLOCK_DRAG:                // Drag (block defined)
    case BLOCK_DRAG_ITEM:           // Drag from a drag item command
    case BLOCK_DUPLICATE:           // Duplicate
    case BLOCK_DUPLICATE_AND_INCREMENT: // Duplicate and increment relevant references
    case BLOCK_DELETE:              // Delete
    case BLOCK_COPY:                // Copy
    case BLOCK_FLIP:                // Flip
    case BLOCK_ROTATE:              // Rotate
    case BLOCK_ZOOM:                // Window Zoom
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:            // mirror
    case BLOCK_PRESELECT_MOVE:      // Move with preselection list
        block->InitData( m_canvas, aPosition );
        GetCanvas()->GetView()->ShowSelectionArea();
        break;

    case BLOCK_PASTE:
    {
        block->InitData( m_canvas, aPosition );
        InitBlockPasteInfos();

        wxRect bounds( 0, 0, 0, 0 );

        for( unsigned i = 0; i < block->GetCount(); ++i )
            bounds.Union( block->GetItem( i )->GetBoundingBox() );

        block->SetOrigin( bounds.GetPosition() );
        block->SetSize( bounds.GetSize() );
        block->SetLastCursorPosition( wxPoint( 0, 0 ) );

        if( block->GetCount() == 0 )      // No data to paste
        {
            DisplayError( this, _( "Nothing to paste" ), 20 );
            GetScreen()->m_BlockLocate.SetCommand( BLOCK_IDLE );
            m_canvas->SetMouseCaptureCallback( NULL );
            block->SetState( STATE_NO_BLOCK );
            block->SetMessageBlock( this );
            return true;
        }

        if( !m_canvas->IsMouseCaptured() )
        {
            block->ClearItemsList();
            wxFAIL_MSG( "SCH_BASE_FRAME::HandleBlockBegin() error: m_mouseCaptureCallback NULL" );
            block->SetState( STATE_NO_BLOCK );
            block->SetMessageBlock( this );
            return true;
        }

        block->SetState( STATE_BLOCK_MOVE );
        block->SetFlags( IS_MOVED );
        m_canvas->CallMouseCapture( aDC, aPosition, false );
        m_canvas->Refresh();
    }
        break;

    default:
        wxFAIL_MSG( wxString::Format( "SCH_BASE_FRAME::HandleBlockBegin() unknown command: %d",
                                      block->GetCommand() ) );
        break;
    }

    block->SetMessageBlock( this );
    return true;
}

void SCH_BASE_FRAME::createCanvas()
{
    m_canvasType = LoadCanvasTypeSetting();

    // Allows only a CAIRO or OPENGL canvas:
    if( m_canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL &&
        m_canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO )
        m_canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;

    m_canvas = new SCH_DRAW_PANEL( this, wxID_ANY, wxPoint( 0, 0 ), m_FrameSize,
                                   GetGalDisplayOptions(), m_canvasType );

    m_useSingleCanvasPane = true;

    SetGalCanvas( static_cast<SCH_DRAW_PANEL*> (m_canvas) );
    UseGalCanvas( true );
}


void SCH_BASE_FRAME::RefreshItem( SCH_ITEM* aItem, bool isAddOrDelete )
{
    EDA_ITEM* parent = aItem->GetParent();

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // Sheet pins aren't in the view.  Refresh their parent.
        if( parent )
            GetCanvas()->GetView()->Update( parent );
    }
    else
    {
        if( !isAddOrDelete )
            GetCanvas()->GetView()->Update( aItem );

        // Component children are drawn from their parents.  Mark them for re-paint.
        if( parent && parent->Type() == SCH_COMPONENT_T )
            GetCanvas()->GetView()->Update( parent, KIGFX::REPAINT );
    }

    GetCanvas()->Refresh();
}


void SCH_BASE_FRAME::AddToScreen( SCH_ITEM* aItem )
{
    GetScreen()->Append( aItem );
    GetCanvas()->GetView()->Add( aItem );
    RefreshItem( aItem, true );           // handle any additional parent semantics
}


void SCH_BASE_FRAME::AddToScreen( DLIST<SCH_ITEM>& aItems )
{
    for( SCH_ITEM* item = aItems.begin(); item; item = item->Next() )
    {
        GetCanvas()->GetView()->Add( item );
        RefreshItem( item, true );        // handle any additional parent semantics
    }

    GetScreen()->Append( aItems );
}


void SCH_BASE_FRAME::RemoveFromScreen( SCH_ITEM* aItem )
{
    GetCanvas()->GetView()->Remove( aItem );
    GetScreen()->Remove( aItem );
    RefreshItem( aItem, true );           // handle any additional parent semantics
}


void SCH_BASE_FRAME::SyncView()
{
    auto screen = GetScreen();
    auto gal = GetGalCanvas()->GetGAL();

    auto gs = screen->GetGridSize();
    gal->SetGridSize( VECTOR2D( gs.x, gs.y ));
    GetGalCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
}
