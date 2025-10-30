/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_draw_frame.h>
#include <kiway.h>
#include <kiway_player.h>
#include <project.h>
#include <project_pcb.h>
#include <widgets/footprint_choice.h>
#include <widgets/footprint_select_widget.h>
#include <widgets/wx_progress_reporters.h>
#include <progress_reporter.h>
#include <footprint_info_impl.h>
#include <wx/wupdlock.h>


extern FOOTPRINT_LIST_IMPL GFootprintList;        // KIFACE scope.

wxDEFINE_EVENT( EVT_FOOTPRINT_SELECTED, wxCommandEvent );

FOOTPRINT_SELECT_WIDGET::FOOTPRINT_SELECT_WIDGET( EDA_DRAW_FRAME* aFrame, wxWindow* aParent,
                                                  FOOTPRINT_LIST* aFpList, bool aUpdate,
                                                  int aMaxItems ) :
          wxPanel( aParent ),
          m_update( aUpdate ),
          m_max_items( aMaxItems ),
          m_fp_list( aFpList ),
          m_frame( aFrame )
{
    m_zero_filter = true;
    m_sizer = new wxBoxSizer( wxVERTICAL );
    m_fp_sel_ctrl = new FOOTPRINT_CHOICE( this, wxID_ANY );
    m_sizer->Add( m_fp_sel_ctrl, 1, wxEXPAND, 5 );

    SetSizer( m_sizer );
    Layout();
    m_sizer->Fit( this );

    m_fp_sel_ctrl->Bind( wxEVT_COMBOBOX, &FOOTPRINT_SELECT_WIDGET::OnComboBox, this );
}


void FOOTPRINT_SELECT_WIDGET::Load( KIWAY& aKiway, PROJECT& aProject )
{
    m_fp_list = FOOTPRINT_LIST::GetInstance( aKiway );
    wxCHECK_MSG( m_fp_list, /* void */, "Failed to get the footprint list from the KiWay" );

    if( m_fp_list->GetCount() == 0 )
    {
        WX_PROGRESS_REPORTER progressReporter( m_frame, _( "Load Footprint Libraries" ), 1,
                                               PR_CAN_ABORT );

        // If the fp-info-cache is empty (or, more likely, hasn't been created in a new
        // project yet), load footprints the hard way.
        FOOTPRINT_LIBRARY_ADAPTER* footprints = aProject.FootprintLibAdapter( aKiway );
        FOOTPRINT_LIST_IMPL& fpList = static_cast<FOOTPRINT_LIST_IMPL&>( *m_fp_list );

        fpList.ReadFootprintFiles( footprints, nullptr, &progressReporter );
    }

    m_fp_filter.SetList( *m_fp_list );

    if( m_update )
        UpdateList();
}


void FOOTPRINT_SELECT_WIDGET::OnComboBox( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( EVT_FOOTPRINT_SELECTED );
    int            sel = m_fp_sel_ctrl->GetSelection();

    if( sel == wxNOT_FOUND )
        return;

    wxStringClientData* clientdata =
            static_cast<wxStringClientData*>( m_fp_sel_ctrl->GetClientObject( sel ) );
    wxASSERT( clientdata );

    evt.SetString( clientdata->GetData() );
    wxPostEvent( this, evt );
}


void FOOTPRINT_SELECT_WIDGET::ClearFilters()
{
    m_fp_filter.ClearFilters();
    m_default_footprint.Clear();
    m_zero_filter = false;
}


void FOOTPRINT_SELECT_WIDGET::FilterByPinCount( int aPinCount )
{
    m_fp_filter.FilterByPinCount( aPinCount );
}


void FOOTPRINT_SELECT_WIDGET::FilterByFootprintFilters( wxArrayString const& aFilters,
                                                        bool aZeroFilters )
{
    m_zero_filter = ( aZeroFilters && aFilters.size() == 0 );
    m_fp_filter.FilterByFootprintFilters( aFilters );
}


void FOOTPRINT_SELECT_WIDGET::SetDefaultFootprint( wxString const& aFp )
{
    m_default_footprint = aFp;
}


bool FOOTPRINT_SELECT_WIDGET::UpdateList()
{
    int n_items = 0;

    if( !m_fp_list )
        return false;

    wxWindowUpdateLocker lock( m_fp_sel_ctrl );
    m_fp_sel_ctrl->Clear();

    // Be careful adding items! "Default" must occupy POS_DEFAULT,
    // "Other" must occupy POS_OTHER, and the separator must occupy POS_SEPARATOR.
    m_fp_sel_ctrl->Append( m_default_footprint.IsEmpty() ?
                                   _( "No default footprint" ) :
                                   wxS( "[" ) + _( "Default" ) + wxS( "] " ) + m_default_footprint,
                           new wxStringClientData( m_default_footprint ) );

    if( !m_zero_filter )
    {
        for( FOOTPRINT_INFO& fpinfo : m_fp_filter )
        {
            wxString display_name( fpinfo.GetLibNickname() + wxS( ":" ) +
                                   fpinfo.GetFootprintName() );

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
    m_fp_sel_ctrl->SetSelection( 0 );
}


bool FOOTPRINT_SELECT_WIDGET::Enable( bool aEnable )
{
    return m_fp_sel_ctrl->Enable( aEnable );
}
