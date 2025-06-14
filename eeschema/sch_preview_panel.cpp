/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


#include <view/wx_view_controls.h>
#include <drawing_sheet/ds_proxy_view_item.h>

#include <gal/graphics_abstraction_layer.h>

#include <sch_preview_panel.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <sch_edit_frame.h>
#include <settings/settings_manager.h>
#include <preview_items/selection_area.h>
#include <zoom_defines.h>

#include <functional>

#include <sch_sheet.h>
#include <pgm_base.h>

using namespace std::placeholders;

SCH_PREVIEW_PANEL::SCH_PREVIEW_PANEL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                      const wxPoint& aPosition, const wxSize& aSize,
                                      KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
    EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType )
{
    m_view = new KIGFX::SCH_VIEW( nullptr );
    m_view->SetGAL( m_gal );

    m_gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    m_painter.reset( new KIGFX::SCH_PAINTER( m_gal ) );

    SCH_RENDER_SETTINGS* renderSettings = GetRenderSettings();
    renderSettings->LoadColors( ::GetColorSettings( DEFAULT_THEME ) );
    renderSettings->m_ShowPinsElectricalType = false;
    renderSettings->m_ShowPinNumbers = false;
    renderSettings->m_TextOffsetRatio = 0.35;

    m_view->SetPainter( m_painter.get() );
    // This fixes the zoom in and zoom out limits:
    m_view->SetScaleLimits( ZOOM_MAX_LIMIT_EESCHEMA_PREVIEW, ZOOM_MIN_LIMIT_EESCHEMA_PREVIEW );
    m_view->SetMirror( false, false );

    setDefaultLayerOrder();
    setDefaultLayerDeps();

    view()->UpdateAllLayersOrder();
    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    m_gal->SetGridColor( m_painter->GetSettings()->GetLayerColor( LAYER_SCHEMATIC_GRID ) );
    m_gal->SetCursorEnabled( false );
    m_gal->SetGridSize( VECTOR2D( schIUScale.MilsToIU( 100.0 ), schIUScale.MilsToIU( 100.0 ) ) );

    SetEvtHandlerEnabled( true );
    SetFocus();
    Show( true );
    Raise();
    StartDrawing();
}


SCH_PREVIEW_PANEL::~SCH_PREVIEW_PANEL()
{
}


SCH_RENDER_SETTINGS* SCH_PREVIEW_PANEL::GetRenderSettings() const
{
    return static_cast<SCH_RENDER_SETTINGS*>( m_painter->GetSettings() );
}


void SCH_PREVIEW_PANEL::OnShow()
{
    //m_view->RecacheAllItems();
}


void SCH_PREVIEW_PANEL::setDefaultLayerOrder()
{
    for( int i = 0; (unsigned) i < sizeof( SCH_LAYER_ORDER ) / sizeof( int ); ++i )
    {
        int layer = SCH_LAYER_ORDER[i];
        wxASSERT( layer < KIGFX::VIEW::VIEW_MAX_LAYERS );

        m_view->SetLayerOrder( layer, i );
    }
}


void SCH_PREVIEW_PANEL::setDefaultLayerDeps()
{
    // An alias's fields don't know how to substitute in their parent's values, so we
    // don't let them draw themselves.  This means no caching.
    auto target = KIGFX::TARGET_NONCACHED;

    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; i++ )
        m_view->SetLayerTarget( i, target );

    m_view->SetLayerTarget( LAYER_GP_OVERLAY , KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY ) ;

    m_view->SetLayerTarget( LAYER_SELECT_OVERLAY , KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_SELECT_OVERLAY ) ;

    m_view->SetLayerTarget( LAYER_DRAWINGSHEET , KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( LAYER_DRAWINGSHEET ) ;
}


KIGFX::SCH_VIEW* SCH_PREVIEW_PANEL::view() const
{
    return static_cast<KIGFX::SCH_VIEW*>( m_view );
}


void SCH_PREVIEW_PANEL::Refresh( bool aEraseBackground, const wxRect* aRect )
{
    EDA_DRAW_PANEL_GAL::Refresh( aEraseBackground, aRect );
}


void SCH_PREVIEW_PANEL::onPaint( wxPaintEvent& aEvent )
{
    if( IsShownOnScreen() )
        EDA_DRAW_PANEL_GAL::onPaint( aEvent );
}
