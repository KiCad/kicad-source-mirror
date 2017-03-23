/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_shim.h>
#include <kiway.h>
#include <kiway_player.h>
#include <make_unique.h>
#include <project.h>
#include <widgets/footprint_choice.h>
#include <widgets/footprint_select_widget.h>

#include <functional>
#include <wx/combo.h>
#include <wx/gauge.h>
#include <wx/odcombo.h>
#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx/wupdlock.h>


/**
 * Fixed positions for standard items in the list
 */
enum
{
    POS_DEFAULT,
    POS_OTHER,
    POS_SEPARATOR
};


/**
 * Page numbers in the wxSimplebook
 */
enum
{
    PAGE_PROGRESS,
    PAGE_SELECT
};


wxDEFINE_EVENT( EVT_FOOTPRINT_SELECTED, wxCommandEvent );


FOOTPRINT_SELECT_WIDGET::FOOTPRINT_SELECT_WIDGET( wxWindow* aParent,
        FOOTPRINT_ASYNC_LOADER& aLoader, std::unique_ptr<FOOTPRINT_LIST>& aFpList, bool aUpdate,
        int aMaxItems )
        : wxPanel( aParent ),
          m_kiway( nullptr ),
          m_update( aUpdate ),
          m_finished_loading( false ),
          m_max_items( aMaxItems ),
          m_last_item( 0 ),
          m_fp_loader( aLoader ),
          m_fp_list( aFpList )
{
    m_sizer = new wxBoxSizer( wxVERTICAL );
    m_progress_timer = std::make_unique<wxTimer>( this );
    m_book = new wxSimplebook( this, wxID_ANY );
    m_progress_ctrl = new wxGauge( m_book, wxID_ANY, 100 );
    m_fp_sel_ctrl = new FOOTPRINT_CHOICE( m_book, wxID_ANY );

    m_book->SetEffect( wxSHOW_EFFECT_BLEND );
    m_book->AddPage( m_progress_ctrl, "", true );
    m_book->AddPage( m_fp_sel_ctrl, "", false );
    m_sizer->Add( m_book, 1, wxEXPAND | wxALL, 5 );

    SetSizer( m_sizer );
    Layout();
    m_sizer->Fit( this );

    Bind( wxEVT_TIMER, &FOOTPRINT_SELECT_WIDGET::OnProgressTimer, this, m_progress_timer->GetId() );
    m_fp_sel_ctrl->Bind( wxEVT_COMBOBOX, &FOOTPRINT_SELECT_WIDGET::OnComboBox, this );
    m_fp_sel_ctrl->Bind(
            EVT_INTERACTIVE_CHOICE, &FOOTPRINT_SELECT_WIDGET::OnComboInteractive, this );
}


void FOOTPRINT_SELECT_WIDGET::Load( KIWAY& aKiway, PROJECT& aProject )
{
    m_kiway = &aKiway;
    auto fp_lib_table = aProject.PcbFootprintLibs( aKiway );

    if( m_fp_loader.GetProgress() == 0 || !m_fp_loader.IsSameTable( fp_lib_table ) )
    {
        m_fp_list = FOOTPRINT_LIST::GetInstance( aKiway );
        m_fp_loader.SetList( &*m_fp_list );
        m_fp_loader.Start( fp_lib_table );
    }

    m_progress_timer->Start( 200 );
}


void FOOTPRINT_SELECT_WIDGET::OnProgressTimer( wxTimerEvent& aEvent )
{
    int prog = m_fp_loader.GetProgress();
    m_progress_ctrl->SetValue( prog );

    if( prog == 100 )
    {
        wxBusyCursor busy;

        m_fp_loader.Join();
        m_fp_filter.SetList( *m_fp_list );
        m_progress_timer->Stop();

        m_book->SetSelection( PAGE_SELECT );
        m_finished_loading = true;

        if( m_update )
            UpdateList();
    }
}


void FOOTPRINT_SELECT_WIDGET::OnComboBox( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( EVT_FOOTPRINT_SELECTED );
    int            sel = m_fp_sel_ctrl->GetSelection();

    switch( sel )
    {
    case wxNOT_FOUND: return;

    case POS_SEPARATOR:
        // User somehow managed to select the separator. This should not be
        // possible, but just in case... deselect it
        m_fp_sel_ctrl->SetSelection( m_last_item );
        break;

    case POS_OTHER:
        // When POS_OTHER is selected, a dialog should be shown. However, we don't want to
        // do this ALL the time, as some times (e.g. when moving around with the arrow keys)
        // it could be very annoying. Therefore showing the picker is done from the custom
        // "interactive select" event on FOOTPRINT_CHOICE, which only fires for more direct
        // choice actions.
        break;

    default:
    {
        wxStringClientData* clientdata =
                static_cast<wxStringClientData*>( m_fp_sel_ctrl->GetClientObject( sel ) );
        wxASSERT( clientdata );

        evt.SetString( clientdata->GetData() );
        wxPostEvent( this, evt );
    }
    }
}


void FOOTPRINT_SELECT_WIDGET::OnComboInteractive( wxCommandEvent& aEvent )
{
    if( aEvent.GetInt() == POS_OTHER && !m_fp_sel_ctrl->IsPopupShown() )
    {
        DoOther();
    }
}


void FOOTPRINT_SELECT_WIDGET::DoOther()
{
    wxCommandEvent evt( EVT_FOOTPRINT_SELECTED );

    wxString fpname = ShowPicker();
    m_other_footprint = fpname;
    UpdateList();
    m_fp_sel_ctrl->SetSelection( POS_OTHER );
    m_last_item = POS_OTHER;

    evt.SetString( m_other_footprint );
    wxPostEvent( this, evt );
}


wxString FOOTPRINT_SELECT_WIDGET::ShowPicker()
{
    wxString     fpname;
    wxWindow*    parent = ::wxGetTopLevelParent( this );
    DIALOG_SHIM* dsparent = dynamic_cast<DIALOG_SHIM*>( parent );

    // Only quasimodal dialogs can launch modal kiface dialogs. Otherwise the
    // event loop goes all silly.
    wxASSERT( !dsparent || dsparent->IsQuasiModal() );

    auto frame = m_kiway->Player( FRAME_PCB_MODULE_VIEWER_MODAL, true );

    if( !frame->ShowModal( &fpname, parent ) )
    {
        fpname = wxEmptyString;
    }

    frame->Destroy();

    return fpname;
}


void FOOTPRINT_SELECT_WIDGET::ClearFilters()
{
    m_fp_filter.ClearFilters();
    m_default_footprint.Clear();
    m_other_footprint.Clear();
    m_zero_filter = false;
}


void FOOTPRINT_SELECT_WIDGET::FilterByPinCount( int aPinCount )
{
    m_fp_filter.FilterByPinCount( aPinCount );
}


void FOOTPRINT_SELECT_WIDGET::FilterByFootprintFilters(
        wxArrayString const& aFilters, bool aZeroFilters )
{
    if( aZeroFilters && aFilters.size() == 0 )
        m_zero_filter = true;
    else
        m_zero_filter = false;

    m_fp_filter.FilterByFootprintFilters( aFilters );
}


void FOOTPRINT_SELECT_WIDGET::SetDefaultFootprint( wxString const& aFp )
{
    m_default_footprint = aFp;
}


bool FOOTPRINT_SELECT_WIDGET::UpdateList()
{
    int n_items = 0;

    if( !m_fp_list || !m_finished_loading )
        return false;

    wxWindowUpdateLocker lock( m_fp_sel_ctrl );
    m_fp_sel_ctrl->Clear();

    // Be careful adding items! "Default" must occupy POS_DEFAULT,
    // "Other" must occupy POS_OTHER, and the separator must occupy POS_SEPARATOR.

    m_fp_sel_ctrl->Append( m_default_footprint.IsEmpty() ?
                                   _( "No default footprint" ) :
                                   "[" + _( "Default" ) + "] " + m_default_footprint,
            new wxStringClientData( m_default_footprint ) );

    m_fp_sel_ctrl->Append( m_other_footprint.IsEmpty() ?
                                   _( "Other..." ) :
                                   "[" + _( "Other..." ) + "] " + m_other_footprint,
            new wxStringClientData( m_other_footprint ) );

    m_fp_sel_ctrl->Append( "", new wxStringClientData( "" ) );

    if( !m_zero_filter )
    {
        for( auto& fpinfo : m_fp_filter )
        {
            wxString display_name( fpinfo.GetNickname() + ":" + fpinfo.GetFootprintName() );

            m_fp_sel_ctrl->Append( display_name, new wxStringClientData( display_name ) );
            ++n_items;

            if( n_items >= m_max_items )
                break;
        }
    }

    SelectDefault();
    return true;
}


void FOOTPRINT_SELECT_WIDGET::SelectDefault()
{
    m_fp_sel_ctrl->SetSelection( POS_DEFAULT );
}


bool FOOTPRINT_SELECT_WIDGET::Enable( bool aEnable )
{
    return m_fp_sel_ctrl->Enable( aEnable );
}
