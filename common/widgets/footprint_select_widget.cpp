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
#include <kiface_ids.h>
#include <widgets/footprint_choice.h>
#include <widgets/footprint_select_widget.h>
#include <nlohmann/json.hpp>
#include <wx/wupdlock.h>


wxDEFINE_EVENT( EVT_FOOTPRINT_SELECTED, wxCommandEvent );

FOOTPRINT_SELECT_WIDGET::FOOTPRINT_SELECT_WIDGET( EDA_DRAW_FRAME* aFrame, wxWindow* aParent,
                                                  int aMaxItems ) :
        wxPanel( aParent ),
        m_max_items( aMaxItems ),
        m_pin_count( 0 ),
        m_zero_filter( true ),
        m_kiway( nullptr )
{
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
    m_kiway = &aKiway;
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
    m_pin_count = 0;
    m_filters.Clear();
    m_default_footprint.Clear();
    m_zero_filter = false;
}


void FOOTPRINT_SELECT_WIDGET::FilterByPinCount( int aPinCount )
{
    m_pin_count = aPinCount;
}


void FOOTPRINT_SELECT_WIDGET::FilterByFootprintFilters( wxArrayString const& aFilters,
                                                        bool aZeroFilters )
{
    m_zero_filter = ( aZeroFilters && aFilters.size() == 0 );
    m_filters = aFilters;
}


void FOOTPRINT_SELECT_WIDGET::SetDefaultFootprint( wxString const& aFp )
{
    m_default_footprint = aFp;
}


bool FOOTPRINT_SELECT_WIDGET::UpdateList()
{
    if( !m_kiway )
        return false;

    wxWindowUpdateLocker lock( m_fp_sel_ctrl );
    m_fp_sel_ctrl->Clear();

    // Add the default footprint entry at the top
    wxString defaultLabel = m_default_footprint.IsEmpty()
                                    ? _( "No default footprint" )
                                    : wxS( "[" ) + _( "Default" ) + wxS( "] " ) + m_default_footprint;

    m_fp_sel_ctrl->Append( defaultLabel, new wxStringClientData( m_default_footprint ) );

    // If zero_filter is set and we have no filters, show no footprints
    if( m_zero_filter && m_filters.IsEmpty() && m_pin_count == 0 )
    {
        SelectDefault();
        return true;
    }

    // Build JSON request for pcbnew
    using json = nlohmann::json;
    json request;
    request["pin_count"] = m_pin_count;
    request["zero_filters"] = m_zero_filter;
    request["max_results"] = m_max_items;

    json filtersArray = json::array();

    for( const wxString& filter : m_filters )
        filtersArray.push_back( filter.ToStdString() );

    request["filters"] = filtersArray;

    // Get the filter function from pcbnew via KIWAY
    try
    {
        KIFACE* kiface = m_kiway->KiFACE( KIWAY::FACE_PCB );

        if( !kiface )
        {
            SelectDefault();
            return true;
        }

        void* funcPtr = kiface->IfaceOrAddress( KIFACE_FILTER_FOOTPRINTS );

        if( !funcPtr )
        {
            SelectDefault();
            return true;
        }

        // Call the filter function
        using FilterFunc = wxString ( * )( const wxString& );
        FilterFunc filterFootprints = reinterpret_cast<FilterFunc>( funcPtr );

        wxString requestStr = wxString::FromUTF8( request.dump() );
        wxString responseStr = filterFootprints( requestStr );

        // Parse the response
        json response = json::parse( responseStr.ToStdString() );

        if( response.is_array() )
        {
            for( const auto& item : response )
            {
                if( item.is_string() )
                {
                    wxString fpName = wxString::FromUTF8( item.get<std::string>() );
                    m_fp_sel_ctrl->Append( fpName, new wxStringClientData( fpName ) );
                }
            }
        }
    }
    catch( const std::exception& e )
    {
        // JSON parsing or other error - just show default
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
