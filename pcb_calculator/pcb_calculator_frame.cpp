/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 jean-pierre.charras
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/msgdlg.h>
#include <bitmaps.h>
#include <bitmap_store.h>
#include <geometry/shape_poly_set.h>
#include <kiface_base.h>
#include "attenuators/attenuator_classes.h"
#include "pcb_calculator_frame.h"
#include "pcb_calculator_settings.h"


PCB_CALCULATOR_FRAME::PCB_CALCULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    PCB_CALCULATOR_FRAME_BASE( aParent ),
    m_lastNotebookPage( -1 ),
    m_macHack( true )
{
    SetKiway( this, aKiway );

    SHAPE_POLY_SET dummy;   // A ugly trick to force the linker to include
                            // some methods in code and avoid link errors

    LoadSettings( config() );

    m_panelRegulators->ReadDataFile();

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    GetSizer()->SetSizeHints( this );

    // Set previous size and position
    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    if( m_framePos == wxDefaultPosition )
        Centre();

    Bind( wxEVT_SYS_COLOUR_CHANGED,
          wxSysColourChangedEventHandler( PCB_CALCULATOR_FRAME::onThemeChanged ), this );
}


PCB_CALCULATOR_FRAME::~PCB_CALCULATOR_FRAME()
{
    // This needed for OSX: avoids further OnDraw processing after this destructor and before
    // the native window is destroyed
    this->Freeze();
}


void PCB_CALCULATOR_FRAME::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    // Force the bitmaps to refresh
    GetBitmapStore()->ThemeChanged();

    // Update the panels
    m_panelTransline->ThemeChanged();
    m_panelRegulators->ThemeChanged();
    m_panelAttenuators->ThemeChanged();
    m_panelColorCode->ThemeChanged();
    m_panelViaSize->ThemeChanged();
    m_panelTrackWidth->ThemeChanged();
    m_panelElectricalSpacing->ThemeChanged();
    m_panelBoardClass->ThemeChanged();

    aEvent.Skip();
}


void PCB_CALCULATOR_FRAME::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_Notebook->GetSelection() != m_lastNotebookPage )
    {
        // Kick all the things that wxWidgets can't seem to redraw on its own.
        // This is getting seriously ridiculous....

        wxCommandEvent event2( wxEVT_RADIOBUTTON );
        event2.SetEventObject( m_panelTransline->GetTranslineSelector() );
        event2.SetInt( m_panelTransline->GetCurrTransLineType() );
         m_panelTransline->GetTranslineSelector()->Command( event2 );

        for( int i = 0; i < m_panelAttenuators->m_AttenuatorList.size(); ++i )
        {
            if( m_panelAttenuators->m_AttenuatorList[i] == m_panelAttenuators->m_CurrAttenuator )
            {
                event2.SetEventObject( m_panelAttenuators->GetAttenuatorsSelector() );
                event2.SetInt( i );
                m_panelAttenuators->GetAttenuatorsSelector()->Command( event2 );
                break;
            }
        }

        m_panelAttenuators->UpdateUI();

       	m_panelViaSize->Layout();

       	m_panelRegulators->Layout();

        // Until it's shown on screen the above won't work; but doing it anyway at least keeps
        // putting new OnUpdateUI events into the queue until it *is* shown on screen.
        if( m_Notebook->IsShownOnScreen() )
        {
            // Work around an OSX bug where the wxGrid children don't get placed correctly until
            // the first resize event.
#ifdef __WXMAC__
            if( m_macHack )
            {
                wxSize pageSize = m_panelElectricalSpacing->GetSize();

                pageSize.x -= 100;
                m_panelElectricalSpacing->SetSize( pageSize );
                m_panelElectricalSpacing->Layout();

                pageSize.x += 100;
                m_panelElectricalSpacing->SetSize( pageSize );
                m_panelElectricalSpacing->Layout();

                m_macHack = false;
            }
#endif

            m_lastNotebookPage = m_Notebook->GetSelection();
        }
    }
}


void PCB_CALCULATOR_FRAME::OnClosePcbCalc( wxCloseEvent& event )
{
    if( m_panelRegulators->m_RegulatorListChanged )
    {
        wxString msg;
        wxString title = _( "Write Data Failed" );

        if( m_panelRegulators->GetDataFilename().IsEmpty() )
        {
            msg = _( "No data filename to save modifications.\n"
                     "Do you want to exit and abandon your changes?" );

            if( wxMessageBox( msg, title, wxYES_NO | wxICON_QUESTION ) == wxNO )
                return;
        }
        else
        {
            if( !m_panelRegulators->WriteDataFile() )
            {
                msg.Printf( _( "Unable to write file '%s'\n"
                               "Do you want to exit and abandon your changes?"),
                            m_panelRegulators->GetDataFilename() );

                if( wxMessageBox( msg, title, wxYES_NO | wxICON_ERROR ) == wxNO )
                    return;
            }
        }
    }

    event.Skip();
}


void PCB_CALCULATOR_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    if( aCfg == nullptr )
        return;

    EDA_BASE_FRAME::LoadSettings( aCfg );

    PCB_CALCULATOR_SETTINGS* cfg = static_cast<PCB_CALCULATOR_SETTINGS*>( aCfg );

    m_Notebook->ChangeSelection( cfg->m_LastPage );

    m_panelTransline->LoadSettings( cfg );

    // Attenuators panel config:
    m_panelAttenuators->LoadSettings( cfg );

    // Regul panel config:
    m_panelRegulators->LoadSettings( cfg );

    // color panel config:
    m_panelColorCode->LoadSettings( cfg );

    m_panelViaSize->LoadSettings( cfg );
    m_panelTrackWidth->LoadSettings( cfg );
    m_panelElectricalSpacing->LoadSettings( cfg );
    m_panelBoardClass->LoadSettings( cfg );
    m_panelESeries->LoadSettings( cfg );
}


void PCB_CALCULATOR_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    if( aCfg == nullptr )
        return;

    EDA_BASE_FRAME::SaveSettings( aCfg );

    // Save current parameters values in config.
    auto cfg = dynamic_cast<PCB_CALCULATOR_SETTINGS*>( Kiface().KifaceSettings() );

    if( cfg )
    {
        cfg->m_LastPage = m_Notebook->GetSelection();

        m_panelTransline->SaveSettings( cfg );
        m_panelRegulators->Regulators_WriteConfig( cfg );
        m_panelAttenuators->SaveSettings( cfg );
        m_panelColorCode->SaveSettings( cfg );
        m_panelViaSize->SaveSettings( cfg );
        m_panelTrackWidth->SaveSettings( cfg );
        m_panelElectricalSpacing->SaveSettings( cfg );
        m_panelBoardClass->SaveSettings( cfg );
    }

}
