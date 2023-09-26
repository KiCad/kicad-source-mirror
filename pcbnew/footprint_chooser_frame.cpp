/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <kiway.h>
#include <kiway_express.h>
#include <wx/button.h>
#include <kiplatform/ui.h>
#include <widgets/panel_footprint_chooser.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include <footprint_chooser_frame.h>


static wxArrayString s_FootprintHistoryList;
static unsigned      s_FootprintHistoryMaxCount = 8;

static void AddFootprintToHistory( const wxString& aName )
{
    // Remove duplicates
    for( int ii = s_FootprintHistoryList.GetCount() - 1; ii >= 0; --ii )
    {
        if( s_FootprintHistoryList[ ii ] == aName )
            s_FootprintHistoryList.RemoveAt((size_t) ii );
    }

    // Add the new name at the beginning of the history list
    s_FootprintHistoryList.Insert( aName, 0 );

    // Remove extra names
    while( s_FootprintHistoryList.GetCount() >= s_FootprintHistoryMaxCount )
        s_FootprintHistoryList.RemoveAt( s_FootprintHistoryList.GetCount() - 1 );
}


BEGIN_EVENT_TABLE( FOOTPRINT_CHOOSER_FRAME, PCB_BASE_FRAME )
    EVT_MENU( wxID_CLOSE, FOOTPRINT_CHOOSER_FRAME::CloseFootprintChooser )
    EVT_BUTTON( wxID_OK, FOOTPRINT_CHOOSER_FRAME::OnOK )
    EVT_BUTTON( wxID_CANCEL, FOOTPRINT_CHOOSER_FRAME::CloseFootprintChooser )
    EVT_PAINT( FOOTPRINT_CHOOSER_FRAME::OnPaint )
END_EVENT_TABLE()


#define PARENT_STYLE ( wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN \
                       | wxWANTS_CHARS | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT )
#define MODAL_STYLE  ( wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN \
                      | wxWANTS_CHARS | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP )


FOOTPRINT_CHOOSER_FRAME::FOOTPRINT_CHOOSER_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    PCB_BASE_FRAME( aKiway, aParent, FRAME_FOOTPRINT_CHOOSER, _( "Footprint Chooser" ),
                    wxDefaultPosition, wxDefaultSize, aParent ? PARENT_STYLE : MODAL_STYLE,
                    FOOTPRINT_CHOOSER_FRAME_NAME ),
    m_comp( LIB_ID(), wxEmptyString, wxEmptyString, KIID_PATH(), {} )
{
    SetModal( true );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    m_chooserPanel = new PANEL_FOOTPRINT_CHOOSER( this, this, s_FootprintHistoryList,
                                                  [this]()
                                                  {
                                                      wxCommandEvent dummy;
                                                      OnOK( dummy );
                                                  } );

    sizer->Add( m_chooserPanel, 1, wxEXPAND, 5 );

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( this, wxID_OK );
    wxButton*               cancelButton = new wxButton( this, wxID_CANCEL );
    sdbSizer->AddButton( okButton );
    sdbSizer->AddButton( cancelButton );
    sdbSizer->Realize();

    sizer->Add( sdbSizer, 0, wxEXPAND | wxALL, 5 );
    SetSizer( sizer );

    SetTitle( GetTitle() + wxString::Format( _( " (%d items loaded)" ),
                                             m_chooserPanel->GetItemCount() ) );

    Layout();
    m_chooserPanel->FinishSetup();
}


void FOOTPRINT_CHOOSER_FRAME::doCloseWindow()
{
    // Only dismiss a modal frame once, so that the return values set by
    // the prior DismissModal() are not bashed for ShowModal().
    if( !IsDismissed() )
        DismissModal( false );

    // window to be destroyed by the caller of KIWAY_PLAYER::ShowModal()
}


WINDOW_SETTINGS* FOOTPRINT_CHOOSER_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxCHECK_MSG( cfg, nullptr, wxT( "config not existing" ) );

    return &cfg->m_FootprintViewer;
}


COLOR_SETTINGS* FOOTPRINT_CHOOSER_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    auto* settings = Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();

    if( settings )
        return Pgm().GetSettingsManager().GetColorSettings( settings->m_ColorTheme );
    else
        return Pgm().GetSettingsManager().GetColorSettings();
}


void FOOTPRINT_CHOOSER_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    // JEY TODO: don't delete this just yet.  We can use it to pass in symbol info so that we
    // can filter on footprint filters, symbol pin count, etc.

    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_SYMBOL_NETLIST:
    {
        /*
         * Symbol netlist format:
         *   library:footprint
         *   reference
         *   value
         *   pinName,netName,pinFunction,pinType
         *   pinName,netName,pinFunction,pinType
         *   ...
         */
        std::vector<std::string> strings = split( payload, "\r" );
        LIB_ID libid;

        if( strings.size() >= 3 )
        {
            libid.Parse( strings[0] );

            m_comp.SetFPID( libid );
            m_comp.SetReference( strings[1] );
            m_comp.SetValue( strings[2] );

            m_comp.ClearNets();

            for( size_t ii = 3; ii < strings.size(); ++ii )
            {
                std::vector<std::string> pinData = split( strings[ii], "," );
                m_comp.AddNet( pinData[0], pinData[1], pinData[2], pinData[3] );
            }
        }

        break;
    }

    default:
        break;
    }
}


bool FOOTPRINT_CHOOSER_FRAME::ShowModal( wxString* aFootprint, wxWindow* aParent )
{
    if( aFootprint && !aFootprint->IsEmpty() )
    {
        LIB_ID fpid;

        fpid.Parse( *aFootprint, true );

        if( fpid.IsValid() )
            m_chooserPanel->SetPreselect( fpid );
    }

    return KIWAY_PLAYER::ShowModal( aFootprint, aParent );
}


void FOOTPRINT_CHOOSER_FRAME::OnPaint( wxPaintEvent& aEvent )
{
    if( m_firstPaintEvent )
    {
        KIPLATFORM::UI::FixupCancelButtonCmdKeyCollision( this );
        KIPLATFORM::UI::ForceFocus( m_chooserPanel->GetFocusTarget() );

        m_firstPaintEvent = false;
    }

    aEvent.Skip();
}


void FOOTPRINT_CHOOSER_FRAME::OnOK( wxCommandEvent& aEvent )
{
    LIB_ID fpID = m_chooserPanel->GetSelectedLibId();

    if( fpID.IsValid() )
    {
        wxString footprint = fpID.Format();

        AddFootprintToHistory( footprint );
        DismissModal( true, footprint );
    }
    else
    {
        DismissModal( false );
    }
}


void FOOTPRINT_CHOOSER_FRAME::CloseFootprintChooser( wxCommandEvent& aEvent )
{
    Close( false );
}


