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

/**
 * @file gerbview_dialog_display_options_frame.cpp
 * Set some display options for GerbView
 */


#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <config_map.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_dialog_display_options_frame_base.h>

#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <gerbview_painter.h>
#include <gal/gal_display_options.h>
#include <widgets/gal_options_panel.h>


/*******************************************/
/* Dialog frame to select display options */
/*******************************************/
class DIALOG_DISPLAY_OPTIONS : public DIALOG_DISPLAY_OPTIONS_BASE
{
private:
    GERBVIEW_FRAME* m_Parent;
    GAL_OPTIONS_PANEL* m_galOptsPanel;
    int m_last_scale;

public:

    DIALOG_DISPLAY_OPTIONS( GERBVIEW_FRAME* parent );
    ~DIALOG_DISPLAY_OPTIONS() {};

protected:
    void OnScaleSlider( wxScrollEvent& aEvent ) override;
    void OnScaleAuto( wxCommandEvent& aEvent ) override;
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void OnOKBUttonClick( wxCommandEvent& event ) override;
    void OnCancelButtonClick( wxCommandEvent& event ) override;
    void initOptDialog( );
};


void GERBVIEW_FRAME::InstallGerberOptionsDialog( wxCommandEvent& event )
{
    DIALOG_DISPLAY_OPTIONS dlg( this );
    int opt = dlg.ShowModal();

    if( opt > 0 )
        m_canvas->Refresh();
}


DIALOG_DISPLAY_OPTIONS::DIALOG_DISPLAY_OPTIONS( GERBVIEW_FRAME *parent) :
    DIALOG_DISPLAY_OPTIONS_BASE( parent, wxID_ANY ),
    m_last_scale( -1 )
{
    m_Parent = parent;
    m_scaleSlider->SetStep( 25 );
    SetFocus();
    initOptDialog( );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Center();
    m_sdbSizer1OK->SetDefault();

    FinishDialogSettings();
}


void DIALOG_DISPLAY_OPTIONS::OnCancelButtonClick( wxCommandEvent& event )
{
    EndModal( 0 );
}


void DIALOG_DISPLAY_OPTIONS::initOptDialog( )
{
    KIGFX::GAL_DISPLAY_OPTIONS& galOptions = m_Parent->GetGalDisplayOptions();
    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, galOptions );
    m_UpperSizer->Add( m_galOptsPanel, 0, wxEXPAND, 0 );
    m_galOptsPanel->TransferDataToWindow();

    m_PolarDisplay->SetSelection( m_Parent->m_DisplayOptions.m_DisplayPolarCood ? 1 : 0 );
    m_BoxUnits->SetSelection( g_UserUnit ? 1 : 0 );

    // Show Option Draw Lines. We use DisplayPcbTrackFill as Lines draw option
    m_OptDisplayLines->SetSelection( m_Parent->m_DisplayOptions.m_DisplayLinesFill ? 1 : 0 );
    m_OptDisplayFlashedItems->SetSelection( m_Parent->m_DisplayOptions.m_DisplayFlashedItemsFill ? 1 : 0);

    // Show Option Draw polygons
    m_OptDisplayPolygons->SetSelection( m_Parent->m_DisplayOptions.m_DisplayPolygonsFill ? 1 : 0 );

    m_ShowPageLimits->SetSelection(0);

    if( m_Parent->GetShowBorderAndTitleBlock() )
    {
        wxString    curPaperType = m_Parent->GetPageSettings().GetType();

        for( unsigned i = 1;  i < DIM( g_GerberPageSizeList );  ++i )
        {
            if( g_GerberPageSizeList[i] == curPaperType )
            {
                m_ShowPageLimits->SetSelection( i );
                break;
            }
        }
    }

    m_OptDisplayDCodes->SetValue( m_Parent->IsElementVisible( LAYER_DCODES ) );
    m_OptZoomNoCenter->SetValue( m_Parent->GetCanvas()->GetEnableZoomNoCenter() );
    m_OptMousewheelPan->SetValue( m_Parent->GetCanvas()->GetEnableMousewheelPan() );
}


void DIALOG_DISPLAY_OPTIONS::OnOKBUttonClick( wxCommandEvent& event )
{
    TransferDataFromWindow();
    auto displayOptions = (GBR_DISPLAY_OPTIONS*) m_Parent->GetDisplayOptions();

    bool needs_repaint = false, option;

    m_Parent->m_DisplayOptions.m_DisplayPolarCood =
        (m_PolarDisplay->GetSelection() == 0) ? false : true;
    g_UserUnit  = (m_BoxUnits->GetSelection() == 0) ? INCHES : MILLIMETRES;

    option = ( m_OptDisplayLines->GetSelection() == 1 );

    if( option != m_Parent->m_DisplayOptions.m_DisplayLinesFill )
        needs_repaint = true;

    m_Parent->m_DisplayOptions.m_DisplayLinesFill = option;

    option = ( m_OptDisplayFlashedItems->GetSelection() == 1 );

    if( option != m_Parent->m_DisplayOptions.m_DisplayFlashedItemsFill )
        needs_repaint = true;

    m_Parent->m_DisplayOptions.m_DisplayFlashedItemsFill = option;

    option = ( m_OptDisplayPolygons->GetSelection() == 1 );

    if( option != m_Parent->m_DisplayOptions.m_DisplayPolygonsFill )
        needs_repaint = true;

    m_Parent->m_DisplayOptions.m_DisplayPolygonsFill = option;

    m_Parent->SetElementVisibility( LAYER_DCODES, m_OptDisplayDCodes->GetValue() );

    int idx = m_ShowPageLimits->GetSelection();

    m_Parent->SetShowBorderAndTitleBlock( idx > 0  ?  true : false );

    PAGE_INFO   pageInfo( g_GerberPageSizeList[idx] );

    m_Parent->SetPageSettings( pageInfo );

    m_Parent->GetCanvas()->SetEnableZoomNoCenter( m_OptZoomNoCenter->GetValue() );
    m_Parent->GetCanvas()->SetEnableMousewheelPan( m_OptMousewheelPan->GetValue() );

    m_galOptsPanel->TransferDataFromWindow();

    // Apply changes to the GAL
    auto view = m_Parent->GetGalCanvas()->GetView();
    auto painter = static_cast<KIGFX::GERBVIEW_PAINTER*>( view->GetPainter() );
    auto settings = static_cast<KIGFX::GERBVIEW_RENDER_SETTINGS*>( painter->GetSettings() );
    settings->LoadDisplayOptions( displayOptions );
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    if( needs_repaint )
        view->UpdateAllItems( KIGFX::REPAINT );

    m_Parent->GetCanvas()->Refresh();

    EndModal( 1 );
}


bool DIALOG_DISPLAY_OPTIONS::TransferDataToWindow()
{
    const auto parent = static_cast<GERBVIEW_FRAME*>( GetParent() );
    const int scale_fourths = parent->GetIconScale();

    if( scale_fourths <= 0 )
    {
        m_scaleAuto->SetValue( true );
        m_scaleSlider->SetValue( 25 * KiIconScale( parent ) );
    }
    else
    {
        m_scaleAuto->SetValue( false );
        m_scaleSlider->SetValue( scale_fourths * 25 );
    }

    return true;
}


bool DIALOG_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    const auto parent = static_cast<GERBVIEW_FRAME*>( GetParent() );
    const int scale_fourths = m_scaleAuto->GetValue() ? -1 : m_scaleSlider->GetValue() / 25;

    if( parent->GetIconScale() != scale_fourths )
        parent->SetIconScale( scale_fourths );

    return true;
}


void DIALOG_DISPLAY_OPTIONS::OnScaleSlider( wxScrollEvent& aEvent )
{
    m_scaleAuto->SetValue( false );
    aEvent.Skip();
}


void DIALOG_DISPLAY_OPTIONS::OnScaleAuto( wxCommandEvent& aEvent )
{
    if( m_scaleAuto->GetValue() )
    {
        m_last_scale = m_scaleSlider->GetValue();
        m_scaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        if( m_last_scale >= 0 )
            m_scaleSlider->SetValue( m_last_scale );
    }
}
