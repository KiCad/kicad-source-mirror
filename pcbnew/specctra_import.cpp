/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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


#include <class_drawpanel.h>    // m_canvas
#include <confirm.h>            // DisplayError()
#include <gestfich.h>           // EDA_FileSelector()
#include <wxPcbStruct.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_drawsegment.h>

#include <specctra.h>


using namespace DSN;

void PCB_EDIT_FRAME::ImportSpecctraDesign( wxCommandEvent& event )
{
    /* @todo write this someday

    if( !Clear_Pcb( true ) )
        return;
    */
}


void PCB_EDIT_FRAME::ImportSpecctraSession( wxCommandEvent& event )
{
/*
    if( GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Board Modified: Continue ?" ) ) )
            return;
    }
*/

    wxString fullFileName = GetBoard()->GetFileName();
    wxString path;
    wxString name;
    wxString ext;

    wxString sessionExt( wxT( ".ses" ) );
    wxString mask = wxT( "*" ) + sessionExt;

    wxFileName::SplitPath( fullFileName, &path, &name, &ext );
    name += sessionExt;

    fullFileName = EDA_FileSelector( _( "Merge Specctra Session file:" ),
                            path,
                            name,
                            sessionExt,
                            mask,
                            this,
                            wxFD_OPEN,
                            false
                            );

    if( fullFileName == wxEmptyString )
        return;

    SPECCTRA_DB     db;
    LOCALE_IO       toggle;

    try
    {
        db.LoadSESSION( fullFileName );
        db.FromSESSION( GetBoard() );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = ioe.errorText;
        msg += '\n';
        msg += _("BOARD may be corrupted, do not save it.");
        msg += '\n';
        msg += _("Fix problem and try again.");

        DisplayError( this, msg );
        return;
    }

    OnModify();
    GetBoard()->m_Status_Pcb = 0;

    /* At this point we should call Compile_Ratsnest()
     * but this could be time consumming.
     * So if incorrect number of Connected and No connected pads is accepted
     * until Compile_Ratsnest() is called (when track tool selected for instance)
     * leave the next line commented
     * Otherwise uncomment this line
    */
    //Compile_Ratsnest( NULL, true );

    SetStatusText( wxString( _( "Session file imported and merged OK." ) ) );

    m_canvas->Refresh( true );
}


namespace DSN {


/**
 * Function scale
 * converts a session file distance to KiCad units of deci-mils.
 * @param distance The session file length to convert.
 * @param aResolution The session UNIT_RES which holds the engineering unit
 *  specifier
 * @return int - The KiCad length in deci-mils
 */
static int scale( double distance, UNIT_RES* aResolution )
{
    double  resValue = aResolution->GetValue();
    double  factor;

    switch( aResolution->GetEngUnits() )
    {
    default:
    case T_inch:
        factor = 25.4e6;        // nanometers per inch
        break;
    case T_mil:
        factor = 25.4e3;        // nanometers per mil
        break;
    case T_cm:
        factor = 1e7;           // nanometers per cm
        break;
    case T_mm:
        factor = 1e6;           // nanometers per mm
        break;
    case T_um:
        factor = 1e3;           // nanometers per um
        break;
    }

    int ret = KiROUND( factor * distance / resValue );

    return ret;
}


/**
 * Function mapPt
 * translates a point from the Specctra Session format coordinate system
 * to the KiCad coordinate system.
 * @param aPoint The session point to translate
 * @param aResolution - The amount to scale the point.
 * @return wxPoint - The KiCad coordinate system point.
 */
static wxPoint mapPt( const POINT& aPoint, UNIT_RES* aResolution )
{
    wxPoint ret(  scale( aPoint.x, aResolution ),
                 -scale( aPoint.y, aResolution ) );    // negate y

    return ret;
}


TRACK* SPECCTRA_DB::makeTRACK( PATH* aPath, int aPointIndex, int aNetcode ) throw( IO_ERROR )
{
    int layerNdx = findLayerName( aPath->layer_id );

    if( layerNdx == -1 )
    {
        wxString layerName = FROM_UTF8( aPath->layer_id.c_str() );
        ThrowIOError( _("Session file uses invalid layer id \"%s\""),
                        GetChars( layerName ) );
    }

    TRACK* track = new TRACK( sessionBoard );

    track->SetStart( mapPt( aPath->points[aPointIndex+0], routeResolution ) );
    track->SetEnd( mapPt( aPath->points[aPointIndex+1], routeResolution ) );
    track->SetLayer( pcbLayer2kicad[layerNdx] );
    track->SetWidth( scale( aPath->aperture_width, routeResolution ) );
    track->SetNetCode( aNetcode );

    return track;
}


::VIA* SPECCTRA_DB::makeVIA( PADSTACK* aPadstack, const POINT& aPoint,
            int aNetCode, int aViaDrillDefault )
    throw( IO_ERROR )
{
    ::VIA*  via = 0;
    SHAPE*  shape;

    int     shapeCount = aPadstack->Length();
    int     drill_diam_iu = -1;
    int     copperLayerCount = sessionBoard->GetCopperLayerCount();


    // The drill diameter is encoded in the padstack name if Pcbnew did the DSN export.
    // It is after the colon and before the last '_'
    int     drillStartNdx = aPadstack->padstack_id.find( ':' );

    if( drillStartNdx != -1 )
    {
        ++drillStartNdx;    // skip over the ':'

        int drillEndNdx = aPadstack->padstack_id.rfind( '_' );
        if( drillEndNdx != -1 )
        {
            std::string diam_txt( aPadstack->padstack_id,
                            drillStartNdx, drillEndNdx-drillStartNdx );

            double drill_um = strtod( diam_txt.c_str(), 0 );

            drill_diam_iu = int( drill_um * (IU_PER_MM / 1000.0) );

            if( drill_diam_iu == aViaDrillDefault )
                drill_diam_iu = UNDEFINED_DRILL_DIAMETER;
        }
    }

    if( shapeCount == 0 )
    {
        ThrowIOError( _( "Session via padstack has no shapes" ) );
    }
    else if( shapeCount == 1 )
    {
        shape = (SHAPE*) (*aPadstack)[0];
        DSN_T type = shape->shape->Type();
        if( type != T_circle )
            ThrowIOError( _( "Unsupported via shape: %s"),
                     GetChars( GetTokenString( type ) ) );

        CIRCLE* circle = (CIRCLE*) shape->shape;
        int viaDiam = scale( circle->diameter, routeResolution );

        via = new ::VIA( sessionBoard );
        via->SetPosition( mapPt( aPoint, routeResolution ) );
        via->SetDrill( drill_diam_iu );
        via->SetViaType( VIA_THROUGH );
        via->SetWidth( viaDiam );
        via->SetLayerPair( F_Cu, B_Cu );
    }
    else if( shapeCount == copperLayerCount )
    {
        shape = (SHAPE*) (*aPadstack)[0];
        DSN_T type = shape->shape->Type();
        if( type != T_circle )
            ThrowIOError( _( "Unsupported via shape: %s"),
                     GetChars( GetTokenString( type ) ) );

        CIRCLE* circle = (CIRCLE*) shape->shape;
        int viaDiam = scale( circle->diameter, routeResolution );

        via = new ::VIA( sessionBoard );
        via->SetPosition( mapPt( aPoint, routeResolution ) );
        via->SetDrill( drill_diam_iu );
        via->SetViaType( VIA_THROUGH );
        via->SetWidth( viaDiam );
        via->SetLayerPair( F_Cu, B_Cu );
    }
    else    // VIA_MICROVIA or VIA_BLIND_BURIED
    {
        int topLayerNdx = -1;           // session layer detectors
        int botLayerNdx = INT_MAX;

        int viaDiam = -1;

        for( int i=0; i<shapeCount;  ++i )
        {
            shape = (SHAPE*) (*aPadstack)[i];
            DSN_T type = shape->shape->Type();
            if( type != T_circle )
                ThrowIOError( _( "Unsupported via shape: %s"),
                         GetChars( GetTokenString( type ) ) );

            CIRCLE* circle = (CIRCLE*) shape->shape;

            int layerNdx = findLayerName( circle->layer_id );
            if( layerNdx == -1 )
            {
                wxString layerName = FROM_UTF8( circle->layer_id.c_str() );
                ThrowIOError( _("Session file uses invalid layer id \"%s\""),
                                GetChars( layerName ) );
            }

            if( layerNdx > topLayerNdx )
                topLayerNdx = layerNdx;

            if( layerNdx < botLayerNdx )
                botLayerNdx = layerNdx;

            if( viaDiam == -1 )
                viaDiam = scale( circle->diameter, routeResolution );
        }

        via = new ::VIA( sessionBoard );
        via->SetPosition( mapPt( aPoint, routeResolution ) );
        via->SetDrill( drill_diam_iu );

        if( (topLayerNdx==0 && botLayerNdx==1)
         || (topLayerNdx==copperLayerCount-2 && botLayerNdx==copperLayerCount-1))
            via->SetViaType( VIA_MICROVIA );
        else
            via->SetViaType( VIA_BLIND_BURIED );

        via->SetWidth( viaDiam );

        LAYER_ID topLayer = pcbLayer2kicad[topLayerNdx];
        LAYER_ID botLayer = pcbLayer2kicad[botLayerNdx];

        via->SetLayerPair( topLayer, botLayer );
    }

    if( via )
        via->SetNetCode( aNetCode );

    return via;
}


// no UI code in this function, throw exception to report problems to the
// UI handler: void PCB_EDIT_FRAME::ImportSpecctraSession( wxCommandEvent& event )

void SPECCTRA_DB::FromSESSION( BOARD* aBoard ) throw( IO_ERROR )
{
    sessionBoard = aBoard;      // not owned here

    if( !session )
        ThrowIOError( _("Session file is missing the \"session\" section") );

    /* Dick 16-Jan-2012: session need not have a placement section.
    if( !session->placement )
        ThrowIOError( _("Session file is missing the \"placement\" section") );
    */

    if( !session->route )
        ThrowIOError( _("Session file is missing the \"routes\" section") );

    if( !session->route->library )
        ThrowIOError( _("Session file is missing the \"library_out\" section") );

    // delete all the old tracks and vias
    aBoard->m_Track.DeleteAll();

    aBoard->DeleteMARKERs();

    buildLayerMaps( aBoard );

    if( session->placement )
    {
        // Walk the PLACEMENT object's COMPONENTs list, and for each PLACE within
        // each COMPONENT, reposition and re-orient each component and put on
        // correct side of the board.
        COMPONENTS& components = session->placement->components;
        for( COMPONENTS::iterator comp=components.begin();  comp!=components.end();  ++comp )
        {
            PLACES& places = comp->places;
            for( unsigned i=0; i<places.size();  ++i )
            {
                PLACE* place = &places[i];  // '&' even though places[] holds a pointer!

                wxString reference = FROM_UTF8( place->component_id.c_str() );
                MODULE* module = aBoard->FindModuleByReference( reference );
                if( !module )
                {
                    ThrowIOError(
                       _("Session file has 'reference' to non-existent component \"%s\""),
                       GetChars( reference ) );
                }

                if( !place->hasVertex )
                    continue;

                UNIT_RES* resolution = place->GetUnits();
                wxASSERT( resolution );

                wxPoint newPos = mapPt( place->vertex, resolution );
                module->SetPosition( newPos );

                if( place->side == T_front )
                {
                    // convert from degrees to tenths of degrees used in KiCad.
                    int orientation = KiROUND( place->rotation * 10.0 );

                    if( module->GetLayer() != F_Cu )
                    {
                        // module is on copper layer (back)
                        module->Flip( module->GetPosition() );
                    }

                    module->SetOrientation( orientation );
                }
                else if( place->side == T_back )
                {
                    int orientation = KiROUND( (place->rotation + 180.0) * 10.0 );

                    if( module->GetLayer() != B_Cu )
                    {
                        // module is on component layer (front)
                        module->Flip( module->GetPosition() );
                    }

                    module->SetOrientation( orientation );
                }
                else
                {
                    // as I write this, the PARSER *is* catching this, so we should never see below:
                    wxFAIL_MSG( wxT("DSN::PARSER did not catch an illegal side := 'back|front'") );
                }
            }
        }
    }

    routeResolution = session->route->GetUnits();

    // Walk the NET_OUTs and create tracks and vias anew.
    NET_OUTS& net_outs = session->route->net_outs;
    for( NET_OUTS::iterator net=net_outs.begin();  net!=net_outs.end();  ++net )
    {
        int         netCode = 0;

        // page 143 of spec says wire's net_id is optional
        if( net->net_id.size() )
        {
            wxString netName = FROM_UTF8( net->net_id.c_str() );

            NETINFO_ITEM* net = aBoard->FindNet( netName );
            if( net )
                netCode = net->GetNet();
            else  // else netCode remains 0
            {
                // int breakhere = 1;
            }
        }

        WIRES& wires = net->wires;
        for( unsigned i=0;  i<wires.size();  ++i )
        {
            WIRE*   wire  = &wires[i];
            DSN_T   shape = wire->shape->Type();

            if( shape != T_path )
            {
                /*  shape == T_polygon is expected from freerouter if you have
                    a zone on a non "power" type layer, i.e. a T_signal layer
                    and the design does a round trip back in as session here.
                    We kept our own zones in the BOARD, so ignore this so called
                    'wire'.

                wxString netId = FROM_UTF8( wire->net_id.c_str() );
                ThrowIOError(
                    _("Unsupported wire shape: \"%s\" for net: \"%s\""),
                    DLEX::GetTokenString(shape).GetData(),
                    netId.GetData()
                    );
                */
            }
            else
            {
                PATH*   path = (PATH*) wire->shape;
                for( unsigned pt=0;  pt<path->points.size()-1;  ++pt )
                {
                    /* a debugging aid, may come in handy
                    if( path->points[pt].x == 547800
                    &&  path->points[pt].y == -380250 )
                    {
                        int breakhere = 1;
                    }
                    */

                    TRACK* track = makeTRACK( path, pt, netCode );
                    aBoard->Add( track );
                }
            }
        }

        WIRE_VIAS& wire_vias = net->wire_vias;
        LIBRARY& library = *session->route->library;
        for( unsigned i=0;  i<wire_vias.size();  ++i )
        {
            int         netCode = 0;

            // page 144 of spec says wire_via's net_id is optional
            if( net->net_id.size() )
            {
                wxString netName = FROM_UTF8( net->net_id.c_str() );

                NETINFO_ITEM* net = aBoard->FindNet( netName );
                if( net )
                    netCode = net->GetNet();

                // else netCode remains 0
            }

            WIRE_VIA* wire_via = &wire_vias[i];

            // example: (via Via_15:8_mil 149000 -71000 )

            PADSTACK* padstack = library.FindPADSTACK( wire_via->GetPadstackId() );
            if( !padstack )
            {
                // Dick  Feb 29, 2008:
                // Freerouter has a bug where it will not round trip all vias.
                // Vias which have a (use_via) element will be round tripped.
                // Vias which do not, don't come back in in the session library,
                // even though they may be actually used in the pre-routed,
                // protected wire_vias. So until that is fixed, create the
                // padstack from its name as a work around.


                // Could use a STRING_FORMATTER here and convert the entire
                // wire_via to text and put that text into the exception.
                wxString psid( FROM_UTF8( wire_via->GetPadstackId().c_str() ) );

                ThrowIOError( _("A wire_via references a missing padstack \"%s\""),
                             GetChars( psid ) );
            }

            NETCLASSPTR netclass = aBoard->GetDesignSettings().m_NetClasses.GetDefault();

            int via_drill_default = netclass->GetViaDrill();

            for( unsigned v=0;  v<wire_via->vertexes.size();  ++v )
            {
                ::VIA* via = makeVIA( padstack, wire_via->vertexes[v], netCode, via_drill_default );
                aBoard->Add( via );
            }
        }
    }
}


} // namespace DSN

