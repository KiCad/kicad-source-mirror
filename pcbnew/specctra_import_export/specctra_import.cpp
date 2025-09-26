/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


/*  This source is a complement to specctra.cpp and implements the import of
    a specctra session file (*.ses), and import of a specctra design file
    (*.dsn) file.  The specification for the grammar of the specctra dsn file
    used to develop this code is given here:
    http://tech.groups.yahoo.com/group/kicad-users/files/  then file "specctra.pdf"
    Also see the comments at the top of the specctra.cpp file itself.
*/

#include "specctra.h"

#include <confirm.h>            // DisplayErrorMessage()
#include <gestfich.h>           // EDA_FileSelector()
#include <fast_float/fast_float.h>
#include <pcb_edit_frame.h>
#include <locale_io.h>
#include <macros.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <connectivity/connectivity_data.h>
#include <view/view.h>
#include <math/util.h>      // for KiROUND
#include <pcbnew_settings.h>
#include <string_utils.h>

using namespace DSN;

bool PCB_EDIT_FRAME::ImportSpecctraSession( const wxString& fullFileName )
{
    // To avoid issues with undo/redo lists (dangling pointers) clear the lists
    // todo: use undo/redo feature
    ClearUndoRedoList();

    if( GetCanvas() )    // clear view:
    {
        for( PCB_TRACK* track : GetBoard()->Tracks() )
            GetCanvas()->GetView()->Remove( track );
    }

    try
    {
        DSN::ImportSpecctraSession( GetBoard(), fullFileName );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = _( "Board may be corrupted, do not save it.\n Fix problem and try again" );

        wxString extra = ioe.What();

        DisplayErrorMessage( this, msg, extra );
        return false;
    }

    OnModify();

    if( GetCanvas() )    // Update view:
    {
        // Update footprint positions

        // add imported tracks (previous tracks are removed, therefore all are new)
        for( PCB_TRACK* track : GetBoard()->Tracks() )
            GetCanvas()->GetView()->Add( track );
    }

    SetStatusText( wxString( _( "Session file imported and merged OK." ) ) );

    Refresh();

    return true;
}


namespace DSN {


/**
 * Function scale
 * converts a session file distance to KiCad units of deci-mils.
 * @param distance The session file length to convert.
 * @param aResolution The session UNIT_RES which holds the engineering unit specifier
 * @return int - The KiCad length in internal unit
 */
static int scale( double distance, UNIT_RES* aResolution )
{
    double  resValue = aResolution->GetValue();
    double  factor;

    switch( aResolution->GetEngUnits() )
    {
    default:
    case T_inch: factor = 25.4e6; break;     // nanometers per inch
    case T_mil:  factor = 25.4e3; break;     // nanometers per mil
    case T_cm:   factor = 1e7;    break;     // nanometers per cm
    case T_mm:   factor = 1e6;    break;     // nanometers per mm
    case T_um:   factor = 1e3;    break;     // nanometers per um
    }

    return KiROUND( factor * distance / resValue );
}


/**
 * Function mapPt
 * translates a point from the Specctra Session format coordinate system
 * to the KiCad coordinate system.
 * @param aPoint The session point to translate
 * @param aResolution - The amount to scale the point.
 * @return wxPoint - The KiCad coordinate system point.
 */
static VECTOR2I mapPt( const POINT& aPoint, UNIT_RES* aResolution )
{
    VECTOR2I ret( scale( aPoint.x, aResolution ),
                  -scale( aPoint.y, aResolution ) );    // negate y

    return ret;
}


PCB_TRACK* SPECCTRA_DB::makeTRACK( WIRE* wire, PATH* aPath, int aPointIndex, int aNetcode )
{
    int layerNdx = findLayerName( aPath->layer_id );

    if( layerNdx == -1 )
    {
        THROW_IO_ERROR( wxString::Format( _( "Session file uses invalid layer id '%s'." ),
                                          From_UTF8( aPath->layer_id.c_str() ) ) );
    }

    PCB_TRACK* track = new PCB_TRACK( m_sessionBoard );

    track->SetStart( mapPt( aPath->points[aPointIndex + 0], m_routeResolution ) );
    track->SetEnd( mapPt( aPath->points[aPointIndex + 1], m_routeResolution ) );
    track->SetLayer( m_pcbLayer2kicad[layerNdx] );
    track->SetWidth( scale( aPath->aperture_width, m_routeResolution ) );
    track->SetNetCode( aNetcode );

    // a track can be locked.
    // However specctra as 4 types, none is exactly the same as our locked option
    // wire->wire_type = T_fix, T_route, T_normal or T_protect
    // fix and protect could be used as lock option
    // but protect is returned for all tracks having initially the route or protect property
    if( wire->m_wire_type == T_fix )
        track->SetLocked( true );

    return track;
}


PCB_ARC* SPECCTRA_DB::makeARC( WIRE* wire, QARC* aQarc, int aNetcode )
{
    int layerNdx = findLayerName( aQarc->layer_id );

    if( layerNdx == -1 )
    {
        THROW_IO_ERROR( wxString::Format( _( "Session file uses invalid layer id '%s'." ),
                                          From_UTF8( aQarc->layer_id.c_str() ) ) );
    }

    PCB_ARC* arc = new PCB_ARC( m_sessionBoard );

    arc->SetStart( mapPt( aQarc->vertex[0], m_routeResolution ) );
    arc->SetEnd( mapPt( aQarc->vertex[1], m_routeResolution ) );
    arc->SetMid( CalcArcMid(arc->GetStart(), arc->GetEnd(),
                 mapPt( aQarc->vertex[2], m_routeResolution ) ) );
    arc->SetLayer( m_pcbLayer2kicad[layerNdx] );
    arc->SetWidth( scale( aQarc->aperture_width, m_routeResolution ) );
    arc->SetNetCode( aNetcode );

    // a track can be locked.
    // However specctra as 4 types, none is exactly the same as our locked option
    // wire->wire_type = T_fix, T_route, T_normal or T_protect
    // fix and protect could be used as lock option
    // but protect is returned for all tracks having initially the route or protect property
    if( wire->m_wire_type == T_fix )
        arc->SetLocked( true );

    return arc;
}


PCB_VIA* SPECCTRA_DB::makeVIA( WIRE_VIA* aVia, PADSTACK* aPadstack, const POINT& aPoint,
                               int aNetCode, int aViaDrillDefault )
{
    PCB_VIA* via = nullptr;
    SHAPE*   shape;
    int      shapeCount = aPadstack->Length();
    int      drill_diam_iu = -1;
    int      copperLayerCount = m_sessionBoard->GetCopperLayerCount();


    // The drill diameter is encoded in the padstack name if Pcbnew did the DSN export.
    // It is after the colon and before the last '_'
    size_t   drillStartNdx = aPadstack->m_padstack_id.find( ':' );

    if( drillStartNdx != std::string::npos )
    {
        ++drillStartNdx;    // skip over the ':'

        size_t drillEndNdx = aPadstack->m_padstack_id.rfind( '_' );

        if( drillEndNdx != std::string::npos )
        {
            std::string diam_txt( aPadstack->m_padstack_id, drillStartNdx,
                                  drillEndNdx-drillStartNdx );

            double drill_um{};
            fast_float::from_chars( diam_txt.data(), diam_txt.data() + diam_txt.size(), drill_um,
                                    fast_float::chars_format::skip_white_space );

            drill_diam_iu = static_cast<int>( drill_um * ( pcbIUScale.IU_PER_MM / 1000.0 ) );

            if( drill_diam_iu == aViaDrillDefault )
                drill_diam_iu = UNDEFINED_DRILL_DIAMETER;
        }
    }

    if( shapeCount == 0 )
    {
        THROW_IO_ERROR( _( "Session via padstack has no shapes" ) );
    }
    else if( shapeCount == 1 )
    {
        shape = static_cast<SHAPE*>( ( *aPadstack )[0] );
        DSN_T type = shape->shape->Type();

        if( type != T_circle )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unsupported via shape: %s." ),
                                              GetTokenString( type ) ) );
        }

        CIRCLE* circle = static_cast<CIRCLE*>( shape->shape );
        int viaDiam = scale( circle->diameter, m_routeResolution );

        via = new PCB_VIA( m_sessionBoard );
        via->SetPosition( mapPt( aPoint, m_routeResolution ) );
        via->SetDrill( drill_diam_iu );
        via->SetViaType( VIATYPE::THROUGH );
        via->SetWidth( ::PADSTACK::ALL_LAYERS, viaDiam );
        via->SetLayerPair( F_Cu, B_Cu );
    }
    else if( shapeCount == copperLayerCount )
    {
        shape = static_cast<SHAPE*>( ( *aPadstack )[0] );
        DSN_T type = shape->shape->Type();

        if( type != T_circle )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unsupported via shape: %s" ),
                                              GetTokenString( type ) ) );
        }

        CIRCLE* circle = static_cast<CIRCLE*>( shape->shape );
        int viaDiam = scale( circle->diameter, m_routeResolution );

        via = new PCB_VIA( m_sessionBoard );
        via->SetPosition( mapPt( aPoint, m_routeResolution ) );
        via->SetDrill( drill_diam_iu );
        via->SetViaType( VIATYPE::THROUGH );
        via->SetWidth( ::PADSTACK::ALL_LAYERS, viaDiam );
        via->SetLayerPair( F_Cu, B_Cu );
    }
    else    // VIA_MICROVIA or VIA_BLIND_BURIED
    {
        int topLayerNdx = -1;           // session layer detectors
        int botLayerNdx = INT_MAX;

        int viaDiam = -1;

        for( int i = 0; i < shapeCount; ++i )
        {
            shape = static_cast<SHAPE*>( ( *aPadstack )[i] );
            DSN_T type = shape->shape->Type();

            if( type != T_circle )
            {
                THROW_IO_ERROR( wxString::Format( _( "Unsupported via shape: %s" ),
                                                  GetTokenString( type ) ) );
            }

            CIRCLE* circle = static_cast<CIRCLE*>( shape->shape );

            int layerNdx = findLayerName( circle->layer_id );

            if( layerNdx == -1 )
            {
                wxString layerName = From_UTF8( circle->layer_id.c_str() );
                THROW_IO_ERROR( wxString::Format( _( "Session file uses invalid layer id '%s'" ),
                                                  layerName ) );
            }

            if( layerNdx > topLayerNdx )
                topLayerNdx = layerNdx;

            if( layerNdx < botLayerNdx )
                botLayerNdx = layerNdx;

            if( viaDiam == -1 )
                viaDiam = scale( circle->diameter, m_routeResolution );
        }

        via = new PCB_VIA( m_sessionBoard );
        via->SetPosition( mapPt( aPoint, m_routeResolution ) );
        via->SetDrill( drill_diam_iu );

        if( ( topLayerNdx == 0 && botLayerNdx == 1 )
            || ( topLayerNdx == copperLayerCount - 2 && botLayerNdx == copperLayerCount - 1 ) )
        {
            via->SetViaType( VIATYPE::MICROVIA );
        }
        else if( topLayerNdx > 0 && botLayerNdx < copperLayerCount - 1 )
        {
            via->SetViaType( VIATYPE::BURIED );
        }
        else
        {
            via->SetViaType( VIATYPE::BLIND );
        }

        wxCHECK2( topLayerNdx >= 0, topLayerNdx = 0 );

        via->SetWidth( ::PADSTACK::ALL_LAYERS, viaDiam );
        via->SetLayerPair( m_pcbLayer2kicad[ topLayerNdx ], m_pcbLayer2kicad[ botLayerNdx ] );
    }

    wxASSERT( via );

    via->SetNetCode( aNetCode );

    // a via can be locked.
    // However specctra as 4 types, none is exactly the same as our locked option
    // aVia->via_type = T_fix, T_route, T_normal or T_protect
    // fix and protect could be used as lock option
    // but protect is returned for all tracks having initially the route or protect property
    if( aVia->m_via_type == T_fix )
        via->SetLocked( true );

    return via;
}


// no UI code in this function, throw exception to report problems to the
// UI handler: void PCB_EDIT_FRAME::ImportSpecctraSession( wxCommandEvent& event )

void SPECCTRA_DB::FromSESSION( BOARD* aBoard )
{
    m_sessionBoard = aBoard;      // not owned here

    if( !m_session )
        THROW_IO_ERROR( _("Session file is missing the \"session\" section") );

    if( !m_session->route )
        THROW_IO_ERROR( _("Session file is missing the \"routes\" section") );

    if( !m_session->route->library )
        THROW_IO_ERROR( _("Session file is missing the \"library_out\" section") );

    // delete the old tracks and vias but save locked tracks/vias; they will be re-added later
    std::vector<PCB_TRACK*> locked;
    TRACKS tracks = aBoard->Tracks();
    aBoard->RemoveAll( { PCB_TRACE_T } );

    for( PCB_TRACK* track : tracks )
    {
        if( track->IsLocked() )
        {
            locked.push_back( track );
        }
        else
        {
            if( EDA_GROUP* group = track->GetParentGroup() )
                group->RemoveItem( track );

            delete track;
        }
    }

    aBoard->DeleteMARKERs();

    buildLayerMaps( aBoard );

    // Add locked tracks: because they are exported as Fix tracks, they are not
    // in .ses file.
    for( PCB_TRACK* track : locked )
        aBoard->Add( track );

    if( m_session->placement )
    {
        // Walk the PLACEMENT object's COMPONENTs list, and for each PLACE within
        // each COMPONENT, reposition and re-orient each component and put on
        // correct side of the board.
        COMPONENTS& components = m_session->placement->m_components;

        for( COMPONENTS::iterator comp = components.begin(); comp != components.end(); ++comp )
        {
            PLACES& places = comp->m_places;

            for( unsigned i = 0; i < places.size(); ++i )
            {
                PLACE* place = &places[i];  // '&' even though places[] holds a pointer!

                wxString   reference = From_UTF8( place->m_component_id.c_str() );
                FOOTPRINT* footprint = aBoard->FindFootprintByReference( reference );

                if( !footprint )
                {
                    THROW_IO_ERROR( wxString::Format( _( "Reference '%s' not found." ),
                                                      reference ) );
                }

                if( !place->m_hasVertex )
                    continue;

                UNIT_RES* resolution = place->GetUnits();
                wxASSERT( resolution );

                VECTOR2I newPos = mapPt( place->m_vertex, resolution );
                footprint->SetPosition( newPos );

                if( place->m_side == T_front )
                {
                    // convert from degrees to tenths of degrees used in KiCad.
                    EDA_ANGLE orientation( place->m_rotation, DEGREES_T );

                    if( footprint->GetLayer() != F_Cu )
                    {
                        // footprint is on copper layer (back)
                        footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );
                    }

                    footprint->SetOrientation( orientation );
                }
                else if( place->m_side == T_back )
                {
                    EDA_ANGLE orientation( place->m_rotation + 180.0, DEGREES_T );

                    if( footprint->GetLayer() != B_Cu )
                    {
                        // footprint is on component layer (front)
                        footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );
                    }

                    footprint->SetOrientation( orientation );
                }
                else
                {
                    // as I write this, the PARSER *is* catching this, so we should never see below:
                    wxFAIL_MSG( wxT("DSN::PARSER did not catch an illegal side := 'back|front'") );
                }
            }
        }
    }

    m_routeResolution = m_session->route->GetUnits();

    // Walk the NET_OUTs and create tracks and vias anew.
    NET_OUTS& net_outs = m_session->route->net_outs;

    for( NET_OUTS::iterator net = net_outs.begin(); net!=net_outs.end(); ++net )
    {
        int netoutCode = 0;

        // page 143 of spec says wire's net_id is optional
        if( net->net_id.size() )
        {
            wxString netName = From_UTF8( net->net_id.c_str() );
            NETINFO_ITEM* netinfo = aBoard->FindNet( netName );

            if( netinfo )
                netoutCode = netinfo->GetNetCode();
        }

        WIRES& wires = net->wires;

        for( unsigned i = 0; i<wires.size(); ++i )
        {
            WIRE*   wire  = &wires[i];
            DSN_T   shape = wire->m_shape->Type();

            if( shape == T_path )
            {
                PATH* path = static_cast<PATH*>( wire->m_shape );

                for( unsigned pt = 0; pt < path->points.size() - 1; ++pt )
                {
                    PCB_TRACK* track;
                    track = makeTRACK( wire, path, pt, netoutCode );
                    aBoard->Add( track );
                }
            }
            else if ( shape == T_qarc )
            {
                QARC* qarc = static_cast<QARC*>( wire->m_shape );

                PCB_ARC* arc = makeARC( wire, qarc, netoutCode );
                aBoard->Add( arc );
            }
            else
            {
                /*
                 * shape == T_polygon is expected from freerouter if you have a zone on a non-
                 * "power" type layer, i.e. a T_signal layer and the design does a round-trip
                 * back in as session here.  We kept our own zones in the BOARD, so ignore this
                 * so called 'wire'.

                wxString netId = From_UTF8( wire->net_id.c_str() );
                THROW_IO_ERROR( wxString::Format( _( "Unsupported wire shape: '%s' for net: '%s'" ),
                                                    DLEX::GetTokenString(shape).GetData(),
                                                    netId.GetData() ) );
                */
            }
        }

        WIRE_VIAS& wire_vias = net->wire_vias;
        LIBRARY& library = *m_session->route->library;

        for( unsigned i = 0; i < wire_vias.size(); ++i )
        {
            int netCode = 0;

            // page 144 of spec says wire_via's net_id is optional
            if( net->net_id.size() )
            {
                wxString netName = From_UTF8( net->net_id.c_str() );
                NETINFO_ITEM* netvia = aBoard->FindNet( netName );

                if( netvia )
                    netCode = netvia->GetNetCode();
            }

            WIRE_VIA* wire_via = &wire_vias[i];

            // example: (via Via_15:8_mil 149000 -71000 )

            PADSTACK* padstack = library.FindPADSTACK( wire_via->GetPadstackId() );

            if( !padstack )
            {
                // Dick  Feb 29, 2008:
                // Freerouter has a bug where it will not round trip all vias.  Vias which have
                // a (use_via) element will be round tripped.  Vias which do not, don't come back
                // in in the session library, even though they may be actually used in the
                // pre-routed, protected wire_vias. So until that is fixed, create the padstack
                // from its name as a work around.
                wxString psid( From_UTF8( wire_via->GetPadstackId().c_str() ) );

                THROW_IO_ERROR( wxString::Format( _( "A wire_via refers to missing padstack '%s'." ),
                                                  psid ) );
            }

            std::shared_ptr<NET_SETTINGS>& netSettings = aBoard->GetDesignSettings().m_NetSettings;

            int via_drill_default = netSettings->GetDefaultNetclass()->GetViaDrill();

            for( unsigned v = 0; v < wire_via->m_vertexes.size(); ++v )
            {
                PCB_VIA* via = makeVIA( wire_via, padstack, wire_via->m_vertexes[v], netCode,
                                        via_drill_default );
                aBoard->Add( via );
            }
        }
    }
}


bool ImportSpecctraSession( BOARD* aBoard, const wxString& fullFileName )
{
    SPECCTRA_DB db;
    LOCALE_IO   toggle;

    db.LoadSESSION( fullFileName );
    db.FromSESSION( aBoard );

    aBoard->GetConnectivity()->ClearRatsnest();
    aBoard->BuildConnectivity();

    return true;
}
} // namespace DSN
