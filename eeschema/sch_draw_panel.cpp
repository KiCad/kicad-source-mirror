/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <view/wx_view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <sch_edit_frame.h>
#include <preview_items/selection_area.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <functional>
#include <sch_sheet.h>
#include <pgm_base.h>
#include <tools/ee_selection_tool.h>


SCH_DRAW_PANEL::SCH_DRAW_PANEL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                const wxPoint& aPosition, const wxSize& aSize,
                                KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
    EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType ),
    m_parent( aParentWindow )
{
    m_currentCursor = wxCURSOR_ARROW;
    m_view = new KIGFX::SCH_VIEW( true, dynamic_cast<SCH_BASE_FRAME*>( aParentWindow ) );
    m_view->SetGAL( m_gal );

    m_gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    m_painter.reset( new KIGFX::SCH_PAINTER( m_gal ) );

    m_view->SetPainter( m_painter.get() );
    m_view->SetScaleLimits( 50.0, 0.05 );    // This fixes the zoom in and zoom out limits
    m_view->SetMirror( false, false );

    // Early initialization of the canvas background color,
    // before any OnPaint event is fired for the canvas using a wrong bg color
    auto settings = m_painter->GetSettings();
    m_gal->SetClearColor( settings->GetBackgroundColor() );

    setDefaultLayerOrder();
    setDefaultLayerDeps();

    GetView()->UpdateAllLayersOrder();

    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    m_viewControls->SetSnapping( true );

    SetEvtHandlerEnabled( true );
    SetFocus();
    Show( true );
    Raise();
    StartDrawing();
}


SCH_DRAW_PANEL::~SCH_DRAW_PANEL()
{
}


void SCH_DRAW_PANEL::DisplayComponent( const LIB_PART* aComponent )
{
    GetView()->Clear();
    GetView()->DisplayComponent( const_cast<LIB_PART*>(aComponent) );

}


void SCH_DRAW_PANEL::DisplaySheet( const SCH_SCREEN *aScreen )
{
    GetView()->Clear();

    if( aScreen )
        GetView()->DisplaySheet( const_cast<SCH_SCREEN*>( aScreen ) );
}


void SCH_DRAW_PANEL::setDefaultLayerOrder()
{
    for( LAYER_NUM i = 0; (unsigned) i < sizeof( SCH_LAYER_ORDER ) / sizeof( LAYER_NUM ); ++i )
    {
        LAYER_NUM layer = SCH_LAYER_ORDER[i];
        wxASSERT( layer < KIGFX::VIEW::VIEW_MAX_LAYERS );

        m_view->SetLayerOrder( layer, i );
    }
}


bool SCH_DRAW_PANEL::SwitchBackend( GAL_TYPE aGalType )
{
    VECTOR2D grid_size = m_gal->GetGridSize();
    bool rv = EDA_DRAW_PANEL_GAL::SwitchBackend( aGalType );
    setDefaultLayerDeps();
    m_gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    // Keep grid size and grid visibility:
    m_gal->SetGridSize( grid_size );
    SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( GetParent() );

    if( frame )
        m_gal->SetGridVisibility( frame->IsGridVisible() );

    Refresh();

    return rv;
}


void SCH_DRAW_PANEL::setDefaultLayerDeps()
{
    // caching makes no sense for Cairo and other software renderers
    auto target = m_backend == GAL_TYPE_OPENGL ? KIGFX::TARGET_CACHED : KIGFX::TARGET_NONCACHED;

    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; i++ )
        m_view->SetLayerTarget( i, target );

    // Bitmaps are draw on a non cached GAL layer:
    m_view->SetLayerTarget( LAYER_DRAW_BITMAPS, KIGFX::TARGET_NONCACHED );

    // Some draw layers need specific settings
    m_view->SetLayerTarget( LAYER_GP_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY ) ;

    m_view->SetLayerTarget( LAYER_SELECT_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_SELECT_OVERLAY ) ;

    m_view->SetLayerTarget( LAYER_WORKSHEET, KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( LAYER_WORKSHEET ) ;

//    m_view->SetLayerTarget( LAYER_SELECTION_SHADOWS, KIGFX::TARGET_NONCACHED );
//    m_view->SetLayerDisplayOnly( LAYER_SELECTION_SHADOWS ) ;
}


KIGFX::SCH_VIEW* SCH_DRAW_PANEL::GetView() const
{
    return static_cast<KIGFX::SCH_VIEW*>( m_view );
}


void SCH_DRAW_PANEL::onPaint( wxPaintEvent& aEvent )
{
    // The first wxPaintEvent can be fired at startup before the GAL engine is fully initialized
    // (depending on platforms). Do nothing in this case
    if( !m_gal->IsInitialized() || !m_gal->IsVisible() )
        return;

    if( m_painter )
        static_cast<KIGFX::SCH_PAINTER*>( m_painter.get() )->GetSettings()->ImportLegacyColors( nullptr );

    EDA_DRAW_PANEL_GAL::onPaint( aEvent );
}
