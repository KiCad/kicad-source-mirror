/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <gerbview_painter.h>
#include <gal/gal_display_options.h>
#include <widgets/gal_options_panel.h>
#include "panel_gerbview_display_options.h"


PANEL_GERBVIEW_DISPLAY_OPTIONS::PANEL_GERBVIEW_DISPLAY_OPTIONS( GERBVIEW_FRAME *aFrame,
                                                                wxWindow* aWindow ) :
    PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE( aWindow, wxID_ANY ),
    m_Parent( aFrame )
{
    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, m_Parent );
    m_galOptionsSizer->Add( m_galOptsPanel, 0, wxEXPAND | wxLEFT, 5 );
}


bool PANEL_GERBVIEW_DISPLAY_OPTIONS::TransferDataToWindow( )
{
    m_galOptsPanel->TransferDataToWindow();

    // Show Option Draw Lines. We use DisplayPcbTrackFill as Lines draw option
    m_OptDisplayLines->SetValue( !m_Parent->GetDisplayOptions().m_DisplayLinesFill );
    m_OptDisplayFlashedItems->SetValue( !m_Parent->GetDisplayOptions().m_DisplayFlashedItemsFill );

    // Show Option Draw polygons
    m_OptDisplayPolygons->SetValue( !m_Parent->GetDisplayOptions().m_DisplayPolygonsFill );

    m_OptDisplayDCodes->SetValue( m_Parent->IsElementVisible( LAYER_DCODES ) );

    return true;
}


bool PANEL_GERBVIEW_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    GBR_DISPLAY_OPTIONS displayOptions = m_Parent->GetDisplayOptions();

    bool needs_repaint = false, option;

    option = !m_OptDisplayLines->GetValue();

    if( option != displayOptions.m_DisplayLinesFill )
        needs_repaint = true;

    displayOptions.m_DisplayLinesFill = option;

    option = !m_OptDisplayFlashedItems->GetValue();

    if( option != m_Parent->GetDisplayOptions().m_DisplayFlashedItemsFill )
        needs_repaint = true;

    displayOptions.m_DisplayFlashedItemsFill = option;

    option = !m_OptDisplayPolygons->GetValue();

    if( option != displayOptions.m_DisplayPolygonsFill )
        needs_repaint = true;

    displayOptions.m_DisplayPolygonsFill = option;

    m_Parent->SetElementVisibility( LAYER_DCODES, m_OptDisplayDCodes->GetValue() );

    m_galOptsPanel->TransferDataFromWindow();

    // Apply changes to the GAL
    auto view = m_Parent->GetCanvas()->GetView();
    auto painter = static_cast<KIGFX::GERBVIEW_PAINTER*>( view->GetPainter() );
    auto settings = painter->GetSettings();
    settings->LoadDisplayOptions( displayOptions );
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    if( needs_repaint )
        view->UpdateAllItems( KIGFX::REPAINT );

    m_Parent->GetCanvas()->Refresh();

    return true;
}

