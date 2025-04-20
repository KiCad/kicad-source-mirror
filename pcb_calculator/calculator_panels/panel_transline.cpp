/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "transline_ident.h"
#include <bitmaps.h>
#include <calculator_panels/panel_transline.h>
#include <pcb_calculator_settings.h>
#include <widgets/std_bitmap_button.h>


PANEL_TRANSLINE::PANEL_TRANSLINE( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                  const wxSize& size, long style, const wxString& name ) :
        PANEL_TRANSLINE_BASE( parent, id, pos, size, style, name ),
        m_currTransLine( nullptr ),
        m_currTransLineType( DEFAULT_TYPE )
{
    m_bpButtonAnalyze->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_bpButtonSynthetize->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );


    // Populate transline list ordered like in dialog menu list
    const static TRANSLINE_TYPE_ID tltype_list[9] = { MICROSTRIP_TYPE,    C_MICROSTRIP_TYPE, STRIPLINE_TYPE,
                                                      C_STRIPLINE_TYPE,   CPW_TYPE,          GROUNDED_CPW_TYPE,
                                                      RECTWAVEGUIDE_TYPE, COAX_TYPE,         TWISTEDPAIR_TYPE };

    for( int ii = 0; ii < 9; ii++ )
        m_transline_list.push_back( new TRANSLINE_IDENT( tltype_list[ii] ) );

    m_EpsilonR_label->SetLabel( wxT( "εr" ) );
    m_substrate_prm3_labelUnit->SetLabel( wxT( "Ω ∙ m" ) );
}


PANEL_TRANSLINE::~PANEL_TRANSLINE()
{
    for( TRANSLINE_IDENT* transline : m_transline_list )
        delete transline;
}


void PANEL_TRANSLINE::ThemeChanged()
{
    // Update the bitmaps
    m_bpButtonAnalyze->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_bpButtonSynthetize->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_translineBitmap->SetBitmap( KiBitmapBundle( m_transline_list[m_currTransLineType]->m_BitmapName ) );
}


void PANEL_TRANSLINE::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    // Ensure parameters from current selection are up to date before saving
    if( m_currTransLine )
        TransfDlgDataToTranslineParams();

    aCfg->m_TransLine.type = m_currTransLineType;

    for( TRANSLINE_IDENT* transline : m_transline_list )
        transline->WriteConfig();
}


void PANEL_TRANSLINE::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_currTransLineType = static_cast<TRANSLINE_TYPE_ID>( aCfg->m_TransLine.type );

    for( TRANSLINE_IDENT* transline : m_transline_list )
        transline->ReadConfig();

    TranslineTypeSelection( m_currTransLineType );
    m_TranslineSelection->SetSelection( m_currTransLineType );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    // It also remove a minor cosmetic issue on wxWidgets 3.5 on MSW
    // Called here after the current selected transline bitmaps are enabled/disabled
    GetSizer()->SetSizeHints( this );
}



void PANEL_TRANSLINE::OnTranslineAnalyse( wxCommandEvent& event )
{
    if( m_currTransLine )
    {
        TransfDlgDataToTranslineParams();
        m_currTransLine->analyze();
    }
}


void PANEL_TRANSLINE::OnTranslineSynthetize( wxCommandEvent& event )
{
    if( m_currTransLine )
    {
        TransfDlgDataToTranslineParams();
        m_currTransLine->synthesize();
    }
}
