/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@inpg.fr
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

#include <algorithm>
#include <netclass.h>
#include <macros.h>
#include <base_units.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/common/types/project_settings.pb.h>

// This will get mapped to "kicad_default" in the specctra_export.
const char NETCLASS::Default[] = "Default";

// Initial values for netclass initialization

// track to track and track to pads clearance.
const int DEFAULT_CLEARANCE        = pcbIUScale.mmToIU( 0.2 );
const int DEFAULT_VIA_DIAMETER     = pcbIUScale.mmToIU( 0.6 );
const int DEFAULT_VIA_DRILL        = pcbIUScale.mmToIU( 0.3 );
const int DEFAULT_UVIA_DIAMETER    = pcbIUScale.mmToIU( 0.3 );
const int DEFAULT_UVIA_DRILL       = pcbIUScale.mmToIU( 0.1 );
const int DEFAULT_TRACK_WIDTH      = pcbIUScale.mmToIU( 0.2 );
const int DEFAULT_DIFF_PAIR_WIDTH  = pcbIUScale.mmToIU( 0.2 );
const int DEFAULT_DIFF_PAIR_GAP    = pcbIUScale.mmToIU( 0.25 );
const int DEFAULT_DIFF_PAIR_VIAGAP = pcbIUScale.mmToIU( 0.25 );

const int DEFAULT_WIRE_WIDTH       = schIUScale.MilsToIU( 6 );
const int DEFAULT_BUS_WIDTH        = schIUScale.MilsToIU( 12 );

const int DEFAULT_LINE_STYLE       = 0; // solid


NETCLASS::NETCLASS( const wxString& aName, bool aInitWithDefaults ) : m_isDefault( false )
{
    m_constituents.push_back( this );

    SetName( aName );
    SetPriority( -1 );
    SetTuningProfile( wxEmptyString );

    // Colors are a special optional case - always set, but UNSPECIFIED used in place of optional
    SetPcbColor( COLOR4D::UNSPECIFIED );
    SetSchematicColor( COLOR4D::UNSPECIFIED );

    if( aInitWithDefaults )
    {
        SetClearance( DEFAULT_CLEARANCE );
        SetViaDrill( DEFAULT_VIA_DRILL );
        SetuViaDrill( DEFAULT_UVIA_DRILL );
        SetTrackWidth( DEFAULT_TRACK_WIDTH );
        SetViaDiameter( DEFAULT_VIA_DIAMETER );
        SetuViaDiameter( DEFAULT_UVIA_DIAMETER );
        SetDiffPairWidth( DEFAULT_DIFF_PAIR_WIDTH );
        SetDiffPairGap( DEFAULT_DIFF_PAIR_GAP );
        SetDiffPairViaGap( DEFAULT_DIFF_PAIR_VIAGAP );

        SetWireWidth( DEFAULT_WIRE_WIDTH );
        SetBusWidth( DEFAULT_BUS_WIDTH );
        SetLineStyle( DEFAULT_LINE_STYLE );
    }

    ResetParents();
}


void NETCLASS::ResetParents()
{
    SetClearanceParent( this );
    SetTrackWidthParent( this );
    SetViaDiameterParent( this );
    SetViaDrillParent( this );
    SetuViaDiameterParent( this );
    SetuViaDrillParent( this );
    SetDiffPairWidthParent( this );
    SetDiffPairGapParent( this );
    SetDiffPairViaGapParent( this );
    SetPcbColorParent( this );
    SetWireWidthParent( this );
    SetBusWidthParent( this );
    SetSchematicColorParent( this );
    SetLineStyleParent( this );
    SetTuningProfileParent( this );
}


void NETCLASS::ResetParameters()
{
    SetPcbColor( COLOR4D::UNSPECIFIED );
    SetSchematicColor( COLOR4D::UNSPECIFIED );
    SetClearance( std::optional<int>() );
    SetViaDrill( std::optional<int>() );
    SetuViaDrill( std::optional<int>() );
    SetTrackWidth( std::optional<int>() );
    SetViaDiameter( std::optional<int>() );
    SetuViaDiameter( std::optional<int>() );
    SetDiffPairWidth( std::optional<int>() );
    SetDiffPairGap( std::optional<int>() );
    SetDiffPairViaGap( std::optional<int>() );
    SetWireWidth( std::optional<int>() );
    SetBusWidth( std::optional<int>() );
    SetLineStyle( std::optional<int>() );

    ResetParents();
}


bool NETCLASS::operator==( const NETCLASS& other ) const
{
    return m_constituents == other.m_constituents;
}


void NETCLASS::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::common;
    project::NetClass nc;

    nc.set_name( m_Name.ToUTF8() );
    nc.set_priority( m_Priority );

    nc.set_type( m_constituents.empty() ? project::NCT_EXPLICIT : project::NCT_IMPLICIT );

    for( NETCLASS* member : m_constituents )
        nc.add_constituents( member->GetName() );

    project::NetClassBoardSettings* board = nc.mutable_board();

    if( m_Clearance )
        board->mutable_clearance()->set_value_nm( *m_Clearance );

    if( m_TrackWidth )
        board->mutable_track_width()->set_value_nm( *m_TrackWidth );

    if( m_diffPairWidth )
        board->mutable_diff_pair_track_width()->set_value_nm( *m_diffPairWidth );

    if( m_diffPairGap )
        board->mutable_diff_pair_gap()->set_value_nm( *m_diffPairGap );

    if( m_diffPairViaGap )
        board->mutable_diff_pair_via_gap()->set_value_nm( *m_diffPairViaGap );

    if( m_ViaDia )
    {
        kiapi::board::types::PadStackLayer* layer = board->mutable_via_stack()->add_copper_layers();
        layer->set_shape( kiapi::board::types::PSS_CIRCLE );
        layer->set_layer( kiapi::board::types::BoardLayer::BL_F_Cu );
        PackVector2( *layer->mutable_size(), { *m_ViaDia, *m_ViaDia } );
    }

    if( m_ViaDrill )
    {
        PackVector2( *board->mutable_via_stack()->mutable_drill()->mutable_diameter(),
                     { *m_ViaDrill, *m_ViaDrill } );
    }

    if( m_pcbColor != COLOR4D::UNSPECIFIED )
        PackColor( *board->mutable_color(), m_pcbColor );

    if( m_tuningProfile != wxEmptyString )
        board->set_tuning_profile( m_tuningProfile.ToUTF8() );

    project::NetClassSchematicSettings* schematic = nc.mutable_schematic();

    if( m_wireWidth )
        schematic->mutable_wire_width()->set_value_nm( *m_wireWidth );

    if( m_busWidth )
        schematic->mutable_bus_width()->set_value_nm( *m_busWidth );

    if( m_schematicColor != COLOR4D::UNSPECIFIED )
        PackColor( *schematic->mutable_color(), m_schematicColor );

    if( m_lineStyle )
    {
        // TODO(JE) resolve issues with moving to kicommon
        // schematic->set_line_style( ToProtoEnum<LINE_STYLE, types::StrokeLineStyle>(
        //         static_cast<LINE_STYLE>( *m_lineStyle ) ) );
    }

    aContainer.PackFrom( nc );
}


bool NETCLASS::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::common;
    project::NetClass nc;

    if( !aContainer.UnpackTo( &nc ) )
        return false;

    m_Name = wxString::FromUTF8( nc.name() );
    m_Priority = nc.priority();

    // We don't allow creating implicit classes directly
    if( nc.type() == project::NCT_IMPLICIT )
        return false;

    SetConstituentNetclasses( {} );

    if( nc.board().has_clearance() )
        m_Clearance = nc.board().clearance().value_nm();

    if( nc.board().has_track_width() )
        m_TrackWidth = nc.board().track_width().value_nm();

    if( nc.board().has_diff_pair_track_width() )
        m_diffPairWidth = nc.board().diff_pair_track_width().value_nm();

    if( nc.board().has_diff_pair_gap() )
        m_diffPairGap = nc.board().diff_pair_gap().value_nm();

    if( nc.board().has_diff_pair_via_gap() )
        m_diffPairViaGap = nc.board().diff_pair_via_gap().value_nm();

    if( nc.board().has_via_stack() )
    {
        if( nc.board().via_stack().copper_layers_size() > 0 )
            m_ViaDia = nc.board().via_stack().copper_layers().at( 0 ).size().x_nm();

        if( nc.board().via_stack().has_drill() )
            m_ViaDrill = nc.board().via_stack().drill().diameter().x_nm();
    }

    if( nc.board().has_color() )
        m_pcbColor = UnpackColor( nc.board().color() );

    if( nc.board().has_tuning_profile() )
        m_tuningProfile = wxString::FromUTF8( nc.board().tuning_profile() );

    if( nc.schematic().has_wire_width() )
        m_wireWidth = nc.schematic().wire_width().value_nm();

    if( nc.schematic().has_bus_width() )
        m_busWidth = nc.schematic().bus_width().value_nm();

    if( nc.schematic().has_color() )
        m_schematicColor = UnpackColor( nc.schematic().color() );

    // TODO(JE) resolve issues with moving to kicommon
    // if( nc.schematic().has_line_style() )
    //     m_lineStyle = static_cast<int>( FromProtoEnum<LINE_STYLE>( nc.schematic().line_style() ) );

    return true;
}


const std::vector<NETCLASS*>& NETCLASS::GetConstituentNetclasses() const
{
    return m_constituents;
}


void NETCLASS::SetConstituentNetclasses( std::vector<NETCLASS*>&& constituents )
{
    m_constituents = std::move( constituents );
}


bool NETCLASS::ContainsNetclassWithName( const wxString& netclass ) const
{
    return std::any_of( m_constituents.begin(), m_constituents.end(),
                        [&netclass]( const NETCLASS* nc )
                        {
                            return nc && nc->GetName().Matches( netclass );
                        } );
}


const wxString NETCLASS::GetHumanReadableName() const
{
    if( m_constituents.size() == 1 )
        return m_Name;

    wxASSERT( m_constituents.size() >= 2 );

    wxString name;

    if( m_constituents.size() == 2 )
    {
        name.Printf( _( "%s and %s" ),
                     m_constituents[0]->GetName(),
                     m_constituents[1]->GetName() );
    }
    else if( m_constituents.size() == 3 )
    {
        name.Printf( _( "%s, %s and %s" ),
                     m_constituents[0]->GetName(),
                     m_constituents[1]->GetName(),
                     m_constituents[2]->GetName() );
    }
    else if( m_constituents.size() > 3 )
    {
        name.Printf( _( "%s, %s and %d more" ),
                     m_constituents[0]->GetName(),
                     m_constituents[1]->GetName(),
                     static_cast<int>( m_constituents.size() - 2 ) );
    }

    return name;
}


const wxString NETCLASS::GetName() const
{
    if( m_constituents.size() == 1 )
        return m_Name;

    wxASSERT( m_constituents.size() >= 2 );

    wxString name = m_constituents[0]->m_Name;

    for( std::size_t i = 1; i < m_constituents.size(); ++i )
    {
        name += ",";
        name += m_constituents[i]->m_Name;
    }

    return name;
}
