/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include <sch_draw_panel.h>

#include <wx/debug.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/window.h>
#include <wx/windowid.h>
#include <memory>
#include <stdexcept>

#include <confirm.h>
#include <eda_draw_frame.h>
#include <gal/definitions.h>
#include <gal/graphics_abstraction_layer.h>
#include <layer_ids.h>
#include <math/vector2d.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <view/wx_view_controls.h>

#include <sch_base_frame.h>
#include <sch_painter.h>
#include <zoom_defines.h>


SCH_DRAW_PANEL::SCH_DRAW_PANEL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                const wxPoint& aPosition, const wxSize& aSize,
                                KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType )
        : EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType )
{
    m_view = new KIGFX::SCH_VIEW( dynamic_cast<SCH_BASE_FRAME*>( GetParentEDAFrame() ) );
    m_view->SetGAL( m_gal );

    m_gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    m_painter.reset( new KIGFX::SCH_PAINTER( m_gal ) );

    COLOR_SETTINGS* cs = ::GetColorSettings( DEFAULT_THEME );

    if( SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( GetParentEDAFrame() ) )
        cs = frame->GetColorSettings();

    wxASSERT( cs );
    m_painter->GetSettings()->LoadColors( cs );

    m_view->SetPainter( m_painter.get() );

    // This fixes the zoom in and zoom out limits:
    m_view->SetScaleLimits( ZOOM_MAX_LIMIT_EESCHEMA, ZOOM_MIN_LIMIT_EESCHEMA );
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

    SetEvtHandlerEnabled( true );
    SetFocus();
    Show( true );
    Raise();
    StartDrawing();
}


SCH_DRAW_PANEL::~SCH_DRAW_PANEL()
{
}


void SCH_DRAW_PANEL::DisplaySymbol( LIB_SYMBOL* aSymbol )
{
    GetView()->DisplaySymbol( aSymbol );
}


void SCH_DRAW_PANEL::DisplaySheet( SCH_SCREEN *aScreen )
{
    GetView()->Clear();

    if( aScreen )
        GetView()->DisplaySheet( aScreen );
    else
        GetView()->Cleanup();
}


void SCH_DRAW_PANEL::setDefaultLayerOrder()
{
    for( int i = 0; (unsigned) i < sizeof( SCH_LAYER_ORDER ) / sizeof( int ); ++i )
    {
        int layer = SCH_LAYER_ORDER[i];
        wxASSERT( layer < KIGFX::VIEW::VIEW_MAX_LAYERS );

        m_view->SetLayerOrder( layer, i );
    }
}


bool SCH_DRAW_PANEL::SwitchBackend( GAL_TYPE aGalType )
{
    bool rv = EDA_DRAW_PANEL_GAL::SwitchBackend( aGalType );
    setDefaultLayerDeps();
    m_gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    Refresh();

    return rv;
}


void SCH_DRAW_PANEL::setDefaultLayerDeps()
{
    // caching makes no sense for Cairo and other software renderers
    auto target = m_backend == GAL_TYPE_OPENGL ? KIGFX::TARGET_CACHED : KIGFX::TARGET_NONCACHED;

    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; i++ )
        m_view->SetLayerTarget( i, target );

    m_view->SetLayerTarget( LAYER_SCHEMATIC_ANCHOR, KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( LAYER_SCHEMATIC_ANCHOR );

    // Bitmaps are draw on a non cached GAL layer:
    m_view->SetLayerTarget( LAYER_DRAW_BITMAPS, KIGFX::TARGET_NONCACHED );

    // Some draw layers need specific settings
    m_view->SetLayerTarget( LAYER_GP_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY ) ;

    m_view->SetLayerTarget( LAYER_SELECT_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_SELECT_OVERLAY ) ;

    m_view->SetLayerTarget( LAYER_DRAWINGSHEET, KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( LAYER_DRAWINGSHEET ) ;

    m_view->SetLayerTarget( LAYER_OP_VOLTAGES, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_OP_VOLTAGES );
    m_view->SetLayerTarget( LAYER_OP_CURRENTS, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_OP_CURRENTS );

    m_view->SetLayerTarget( LAYER_SELECTION_SHADOWS, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_SELECTION_SHADOWS ) ;

    m_view->SetLayerDisplayOnly( LAYER_NET_COLOR_HIGHLIGHT );
    m_view->SetLayerDisplayOnly( LAYER_DANGLING );
}


KIGFX::SCH_VIEW* SCH_DRAW_PANEL::GetView() const
{
    return static_cast<KIGFX::SCH_VIEW*>( m_view );
}


void SCH_DRAW_PANEL::OnShow()
{
    SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( GetParentEDAFrame() );

    try
    {
        // Check if the current rendering backend can be properly initialized
        m_view->UpdateItems();
    }
    catch( const std::runtime_error& e )
    {
        DisplayInfoMessage( frame, e.what() );

        // Use fallback if one is available
        if( GAL_FALLBACK != m_backend )
        {
            SwitchBackend( GAL_FALLBACK );

            if( frame )
                frame->ActivateGalCanvas();
        }
    }
}


void SCH_DRAW_PANEL::onPaint( wxPaintEvent& aEvent )
{
    // The first wxPaintEvent can be fired at startup before the GAL engine is fully initialized
    // (depending on platforms). Do nothing in this case
    if( !m_gal->IsInitialized() || !m_gal->IsVisible() )
        return;

    EDA_DRAW_PANEL_GAL::onPaint( aEvent );
}
