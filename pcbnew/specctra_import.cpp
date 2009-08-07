/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 Kicad Developers, see change_log.txt for contributors.
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
#include "common.h"             // IsOK() & EDA_FileSelector()
#include "class_drawpanel.h"    // DrawPanel
#include "confirm.h"            // DisplayError()
#include "gestfich.h"           // EDA_FileSelector()
#include "wxPcbStruct.h"



using namespace DSN;

void WinEDA_PcbFrame::ImportSpecctraDesign( wxCommandEvent& event )
{
    /* @todo write this someday

    if( !Clear_Pcb( true ) )
        return;
    */
}


void WinEDA_PcbFrame::ImportSpecctraSession( wxCommandEvent& event )
{
/*
    if( GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Board Modified: Continue ?" ) ) )
            return;
    }
*/

    wxString fullFileName = GetScreen()->m_FileName;
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
                            FALSE
                            );

    if( fullFileName == wxEmptyString )
        return;

    SPECCTRA_DB     db;

    SetLocaleTo_C_standard( );    // Switch the locale to standard C

    try
    {
        db.LoadSESSION( fullFileName );
        db.FromSESSION( GetBoard() );
    }
    catch( IOError ioe )
    {
        SetLocaleTo_Default( );    // revert to the current locale

        ioe.errorText += '\n';
        ioe.errorText += _("BOARD may be corrupted, do not save it.");
        ioe.errorText += '\n';
        ioe.errorText += _("Fix problem and try again.");

        DisplayError( this, ioe.errorText );
        return;
    }

    SetLocaleTo_Default( );    // revert to the current locale

    m_SelTrackWidthBox_Changed = TRUE;
    m_SelClrWidthBox_Changed = TRUE;
    m_SelViaSizeBox_Changed    = TRUE;

    GetScreen()->SetModify();
    GetBoard()->m_Status_Pcb = 0;

    /* At this point we should call Compile_Ratsnest()
     * but this could be time consumming.
     * So if incorrect number of Connecred and No connected pads is accepted
     * until Compile_Ratsnest() is called (when track tool selected for instance)
     * leave the next line commented
     * Otherwise uncomment this line
    */
    //Compile_Ratsnest( NULL, true );

    Affiche_Message( wxString( _("Session file imported and merged OK.")) );

    DrawPanel->Refresh( TRUE );
}


namespace DSN {


/**
 * Function scale
 * converts a session file distance to Kicad units of deci-mils.
 * @param distance The session file length to convert.
 * @param aResolution The session UNIT_RES which holds the engineering unit
 *  specifier
 * @return int - The Kicad length in deci-mils
 */
static int scale( double distance, UNIT_RES* aResolution )
{
    double  resValue = aResolution->GetValue();

    double  factor;     // multiply this times session value to get mils for Kicad.

    switch( aResolution->GetEngUnits() )
    {
    default:
    case T_inch:
        factor = 1000.0;
        break;
    case T_mil:
        factor = 1.0;
        break;
    case T_cm:
        factor = 1000.0/2.54;
        break;
    case T_mm:
        factor = 1000.0/25.4;
        break;
    case T_um:
        factor = 1.0/25.4;
        break;
    }

    // the factor of 10.0 is used to convert mils to deci-mils, the units
    // used within Kicad.
    factor *= 10.0;

    int ret = wxRound( factor * distance / resValue );
    return ret;
}


/**
 * Function mapPt
 * translates a point from the Specctra Session format coordinate system
 * to the Kicad coordinate system.
 * @param aPoint The session point to translate
 * @return wxPoint - The Kicad coordinate system point.
 */
static wxPoint mapPt( const POINT& aPoint, UNIT_RES* aResolution )
{
    wxPoint ret(  scale( aPoint.x, aResolution ),
                 -scale( aPoint.y, aResolution ) );    // negate y

    return ret;
}


TRACK* SPECCTRA_DB::makeTRACK( PATH* aPath, int aPointIndex, int aNetcode ) throw( IOError )
{
    int layerNdx = findLayerName( aPath->layer_id );

    if( layerNdx == -1 )
    {
        wxString layerName = CONV_FROM_UTF8( aPath->layer_id.c_str() );
        ThrowIOError( _("Session file uses invalid layer id \"%s\""),
                        GetChars(layerName) );
    }

    TRACK* track = new TRACK( sessionBoard );

    track->m_Start   = mapPt( aPath->points[aPointIndex+0], routeResolution );
    track->m_End     = mapPt( aPath->points[aPointIndex+1], routeResolution );
    track->SetLayer( pcbLayer2kicad[layerNdx] );
    track->m_Width   = scale( aPath->aperture_width, routeResolution );
    track->SetNet( aNetcode );

    return track;
}


SEGVIA* SPECCTRA_DB::makeVIA( PADSTACK* aPadstack, const POINT& aPoint, int aNetCode ) throw( IOError )
{
    SEGVIA* via = 0;
    SHAPE*  shape;

    int     shapeCount = aPadstack->Length();
    int     drillDiam = -1;
    int     copperLayerCount = sessionBoard->GetCopperLayerCount();


    // The drill diameter is encoded in the padstack name if PCBNEW did the DSN export.
    // It is in mils and is after the colon and before the last '_'
    int     drillStartNdx = aPadstack->padstack_id.find( ':' );

    if( drillStartNdx != -1 )
    {
        ++drillStartNdx;    // skip over the ':'
        int drillEndNdx = aPadstack->padstack_id.rfind( '_' );
        if( drillEndNdx != -1 )
        {
            std::string diamTxt( aPadstack->padstack_id, drillStartNdx, drillEndNdx-drillStartNdx );
            const char* sdiamTxt = diamTxt.c_str();
            double drillMils = strtod( sdiamTxt, 0 );

            // drillMils is not in the session units, but actual mils so we don't use scale()
            drillDiam = (int) (drillMils * 10);

            if( drillDiam == g_DesignSettings.m_ViaDrill )      // default
                drillDiam = -1;         // import as default
        }
    }

    if( shapeCount == 0 )
    {
        ThrowIOError( _( "Session via padstack has no shapes") );
    }
    else if( shapeCount == 1 )
    {
        shape = (SHAPE*) (*aPadstack)[0];
        DSN_T type = shape->shape->Type();
        if( type != T_circle )
            ThrowIOError( _( "Unsupported via shape: \"%s\""),
                     GetChars( LEXER::GetTokenString( type ) ) );

        CIRCLE* circle = (CIRCLE*) shape->shape;
        int viaDiam = scale( circle->diameter, routeResolution );

        via = new SEGVIA( sessionBoard );
        via->SetPosition( mapPt( aPoint, routeResolution ) );
        via->SetDrillValue( drillDiam );
        via->m_Shape = VIA_THROUGH;
        via->m_Width = viaDiam;
        via->SetLayerPair( CMP_N, COPPER_LAYER_N );
    }
    else if( shapeCount == copperLayerCount )
    {
        shape = (SHAPE*) (*aPadstack)[0];
        DSN_T type = shape->shape->Type();
        if( type != T_circle )
            ThrowIOError( _( "Unsupported via shape: \"%s\""),
                     GetChars( LEXER::GetTokenString( type ) ) );

        CIRCLE* circle = (CIRCLE*) shape->shape;
        int viaDiam = scale( circle->diameter, routeResolution );

        via = new SEGVIA( sessionBoard );
        via->SetPosition( mapPt( aPoint, routeResolution ) );
        via->SetDrillValue( drillDiam );
        via->m_Shape = VIA_THROUGH;
        via->m_Width = viaDiam;
        via->SetLayerPair( CMP_N, COPPER_LAYER_N );
    }
    else    // VIA_MICROVIA or VIA_BLIND_BURIED
    {
        int topLayerNdx = -1;
        int botLayerNdx = 7000;
        int viaDiam = -1;

        for( int i=0; i<shapeCount;  ++i )
        {
            shape = (SHAPE*) (*aPadstack)[i];
            DSN_T type = shape->shape->Type();
            if( type != T_circle )
                ThrowIOError( _( "Unsupported via shape: \"%s\""),
                         GetChars( LEXER::GetTokenString( type ) ) );

            CIRCLE* circle = (CIRCLE*) shape->shape;

            int layerNdx = findLayerName( circle->layer_id );
            if( layerNdx == -1 )
            {
                wxString layerName = CONV_FROM_UTF8( circle->layer_id.c_str() );
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

        via = new SEGVIA( sessionBoard );
        via->SetPosition( mapPt( aPoint, routeResolution ) );
        via->SetDrillValue( drillDiam );

        if( (topLayerNdx==0 && botLayerNdx==1)
         || (topLayerNdx==copperLayerCount-2 && botLayerNdx==copperLayerCount-1))
            via->m_Shape = VIA_MICROVIA;
        else
            via->m_Shape = VIA_BLIND_BURIED;

        via->m_Width = viaDiam;

        topLayerNdx = pcbLayer2kicad[topLayerNdx];
        botLayerNdx = pcbLayer2kicad[botLayerNdx];

        via->SetLayerPair( topLayerNdx, botLayerNdx );
    }

    if( via )
        via->SetNet( aNetCode );

    return via;
}



// no UI code in this function, throw exception to report problems to the
// UI handler: void WinEDA_PcbFrame::ImportSpecctraSession( wxCommandEvent& event )

void SPECCTRA_DB::FromSESSION( BOARD* aBoard ) throw( IOError )
{
    sessionBoard = aBoard;      // not owned here

    if( !session )
        ThrowIOError( _("Session file is missing the \"session\" section") );

    if( !session->placement )
        ThrowIOError( _("Session file is missing the \"placement\" section") );

    if( !session->route )
        ThrowIOError( _("Session file is missing the \"routes\" section") );

    if( !session->route->library )
        ThrowIOError( _("Session file is missing the \"library_out\" section") );

    // delete all the old tracks and vias
    aBoard->m_Track.DeleteAll();

    aBoard->DeleteMARKERs();

    buildLayerMaps( aBoard );

#if 1
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

            wxString reference = CONV_FROM_UTF8( place->component_id.c_str() );
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
                // convert from degrees to tenths of degrees used in Kicad.
                int orientation = (int) (place->rotation * 10.0);
                if( module->GetLayer() != CMP_N )
                {
                    // module is on copper layer (back)
                    module->Flip( module->m_Pos );
                }
                module->SetOrientation( orientation );
            }
            else if( place->side == T_back )
            {
                int orientation = (int) ((place->rotation + 180.0) * 10.0);
                if( module->GetLayer() != COPPER_LAYER_N )
                {
                    // module is on component layer (front)
                    module->Flip( module->m_Pos );
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
#endif

    routeResolution = session->route->GetUnits();

    // Walk the NET_OUTs and create tracks and vias anew.
    NET_OUTS& net_outs = session->route->net_outs;
    for( NET_OUTS::iterator net=net_outs.begin();  net!=net_outs.end();  ++net )
    {
        int         netCode = 0;

        // page 143 of spec says wire's net_id is optional
        if( net->net_id.size() )
        {
            wxString netName = CONV_FROM_UTF8( net->net_id.c_str() );

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

                wxString netId = CONV_FROM_UTF8( wire->net_id.c_str() );
                ThrowIOError(
                    _("Unsupported wire shape: \"%s\" for net: \"%s\""),
                    LEXER::GetTokenString(shape).GetData(),
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
                wxString netName = CONV_FROM_UTF8( net->net_id.c_str() );

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


                // Could use a STRINGFORMATTER here and convert the entire
                // wire_via to text and put that text into the exception.
                wxString psid( CONV_FROM_UTF8( wire_via->GetPadstackId().c_str() ) );

                ThrowIOError( _("A wire_via references a missing padstack \"%s\""),
                             GetChars( psid ) );
            }

            for( unsigned v=0;  v<wire_via->vertexes.size();  ++v )
            {
                SEGVIA* via = makeVIA( padstack, wire_via->vertexes[v], netCode );
                aBoard->Add( via );
            }
        }
    }
}


} // namespace DSN

