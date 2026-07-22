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

#include <mdw/mdw_event_ingress.h>
#include <mdw/client/mdw_events.h>
#include <mdw/mdw_proto_convert.h>

#include <kinng.h>
#include <mdw_runtime/mdw_backend_server.h>

#include <api/mdw/mdw_envelope.pb.h>
#include <api/workspace/workspace_types.pb.h>

#include <wx/datetime.h>

static MDW_EVENT eventFromEnvelope( const kiapi::workspace::EventEnvelope& aEnvelope )
{
    MDW_EVENT event;
    event.type = MdwFromProto( aEnvelope.type() );
    event.workspace_id = MdwFromUtf8( aEnvelope.workspace_id() );
    event.item_id = MdwFromUtf8( aEnvelope.item_id() );
    event.path_hint = MdwFromUtf8( aEnvelope.path_hint() );
    event.client_identity = MdwFromUtf8( aEnvelope.client_identity() );
    event.payload = MdwFromUtf8( aEnvelope.payload() );
    event.backend_event_type = MdwFromUtf8( aEnvelope.backend_event_type() );
    event.operation_id = MdwFromUtf8( aEnvelope.operation_id() );
    event.change_id = MdwFromUtf8( aEnvelope.change_id() );
    event.seq = aEnvelope.seq();
    event.progress_done = aEnvelope.progress().done();
    event.progress_total = aEnvelope.progress().total();
    event.progress_phase = MdwFromUtf8( aEnvelope.progress().phase() );

    if( aEnvelope.timestamp_unix() != 0 )
        event.timestamp = wxDateTime( static_cast<time_t>( aEnvelope.timestamp_unix() ) );

    return event;
}


MDW_EVENT_INGRESS::MDW_EVENT_INGRESS( SINK aSink, const wxString& aChannelToken ) :
        m_sink( std::move( aSink ) ),
        m_channelToken( aChannelToken.utf8_string() )
{
}


MDW_EVENT_INGRESS::~MDW_EVENT_INGRESS()
{
    Stop();
}


bool MDW_EVENT_INGRESS::Start( const wxString& aUrl )
{
    if( m_server )
        return true;

    // Owned before it starts, because the first message's acknowledgement goes through m_server
    m_server = std::make_unique<KINNG_REQUEST_SERVER>( aUrl.utf8_string(), MDW_CONTROL_MAX_BYTES );
    m_server->SetCallback( [this]( std::string* aMessage ) { onMessage( aMessage ); } );

    if( !m_server->Start() )
    {
        m_server.reset();
        return false;
    }

    return true;
}


void MDW_EVENT_INGRESS::Stop()
{
    // Stopping joins the listen thread, so no callback is in flight afterwards and the sink is safe
    // to destroy
    if( m_server )
        m_server->Stop();

    m_server.reset();
}


void MDW_EVENT_INGRESS::onMessage( std::string* aMessage )
{
    kiapi::mdw::PublishEvent publish;

    // An empty reply never confirms delivery and never hands the token to a sender that lacks it
    if( !aMessage || !publish.ParseFromString( *aMessage ) || publish.channel_token() != m_channelToken )
    {
        m_server->Reply( std::string() );
        return;
    }

    MDW_EVENT event = eventFromEnvelope( publish.event() );

    // Acknowledged first, so a subscriber that throws or blocks cannot make the backend resend an accepted event
    kiapi::mdw::PublishEventAck ack;
    ack.set_channel_token( m_channelToken );
    ack.set_seq( event.seq );
    m_server->Reply( ack.SerializeAsString() );

    if( m_sink )
        m_sink( event );
}
