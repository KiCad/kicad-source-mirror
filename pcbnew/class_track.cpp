/*******************************************************************/
/* Functions relatives to tracks, vias and zones(see class_track.h */
/*******************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"


#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "trigo.h"
#include "protos.h"


// #define RATSNET_DEBUG

#ifdef RATSNET_DEBUG

/**************************************/
void DbgDisplayTrackInfos( TRACK* track )
/**************************************/

/* Only for ratsnest debug
 */
{
    wxString msg;

    msg << wxT( "Netcode " ) << track->GetNet();
    msg << wxT( " - " ) << track->GetSubNet();
    msg << wxT( "\nptrS " ) << (unsigned) track->start;
    msg << wxT( " ptrE " ) << (unsigned) track->end;
    msg << wxT( " this " ) << (unsigned) track;

    wxMessageBox( msg );
}


#endif

/**********************************************************/
TRACK::TRACK( BOARD_ITEM* StructFather, KICAD_T idtype ) :
    BOARD_ITEM( StructFather, idtype )
/**********************************************************/
{
    m_Width = 0;
    m_Shape = S_SEGMENT;
    start   = end = NULL;
    SetNet( 0 );
    SetSubNet( 0 );
    m_Drill = -1;
    m_Param = 0;
}


/***************************/
wxString TRACK::ShowWidth()
/***************************/
{
    wxString msg;

#if 0
    double   value = To_User_Unit( g_UnitMetric, m_Width, PCB_INTERNAL_UNIT );

    if( g_UnitMetric == INCHES )  // Affichage en mils
        msg.Printf( wxT( "%.1f" ), value * 1000 );
    else
        msg.Printf( wxT( "%.3f" ), value );
#else

    valeur_param( m_Width, msg );

#endif

    return msg;
}


SEGZONE::SEGZONE( BOARD_ITEM* StructFather ) :
    TRACK( StructFather, TYPEZONE )
{
}


SEGVIA::SEGVIA( BOARD_ITEM* StructFather ) :
    TRACK( StructFather, TYPEVIA )
{
}


// Copy constructor
TRACK::TRACK( const TRACK& Source ) :
    BOARD_ITEM( Source )
{
    m_Shape = Source.m_Shape;
    SetNet( Source.GetNet() );
    m_Flags     = Source.m_Flags;
    m_TimeStamp = Source.m_TimeStamp;
    SetStatus( Source.ReturnStatus() );
    m_Start = Source.m_Start;
    m_End   = Source.m_End;
    m_Width = Source.m_Width;
    m_Drill = Source.m_Drill;
    SetSubNet( Source.GetSubNet() );
    m_Param = Source.m_Param;
}


/*  Because of the way SEGVIA and SEGZONE are derived from TRACK and because there are
 *  virtual functions being used, we can no longer simply copy a TRACK and
 *  expect it to be a via or zone.  We must construct a true SEGVIA or SEGZONE so its constructor
 *  can initialize the virtual function table properly.  This factory type of
 *  function called Copy() can duplicate either a TRACK, SEGVIA, or SEGZONE.
 */
TRACK* TRACK::Copy() const
{
    if( Type() == TYPETRACK )
        return new TRACK( *this );

    if( Type() == TYPEVIA )
        return new SEGVIA( (const SEGVIA &) * this );

    if( Type() == TYPEZONE )
        return new SEGZONE( (const SEGZONE &) * this );

    return NULL;    // should never happen
}


/***********************/
bool TRACK::IsNull()
/***********************/

// return TRUE if segment length = 0
{
    if( ( Type() != TYPEVIA ) && ( m_Start == m_End ) )
        return TRUE;
    else
        return FALSE;
}


/*************************************************************/
double TRACK::GetLength() const
/*************************************************************/
{
    int    dx = m_Start.x - m_End.x;
    int    dy = m_Start.y - m_End.y;

    double dist = ( (double) dx * dx ) + ( (double) dy * dy );

    dist = sqrt( dist );

    return dist;
}


/*************************************************************/
int TRACK::IsPointOnEnds( const wxPoint& point, int min_dist )
/*************************************************************/

/* Return:
 *  STARTPOINT if point if near (dist = min_dist) star point
 *  ENDPOINT  if point if near (dist = min_dist) end point
 *  STARTPOINT|ENDPOINT  if point if near (dist = min_dist) both ends
 *  0 if no
 *  if min_dist < 0: min_dist = track_width/2
 */
{
    int result = 0;

    if( min_dist < 0 )
        min_dist = m_Width / 2;

    int dx = m_Start.x - point.x;
    int dy = m_Start.y - point.y;

    if( min_dist == 0 )
    {
        if( (dx == 0) && (dy == 0 ) )
            result |= STARTPOINT;
    }
    else
    {
        double dist = ( (double) dx * dx ) + ( (double) dy * dy );
        dist = sqrt( dist );
        if( min_dist >= (int) dist )
            result |= STARTPOINT;
    }

    dx = m_End.x - point.x;
    dy = m_End.y - point.y;
    if( min_dist == 0 )
    {
        if( (dx == 0) && (dy == 0 ) )
            result |= ENDPOINT;
    }
    else
    {
        double dist = ( (double) dx * dx ) + ( (double) dy * dy );
        dist = sqrt( dist );
        if( min_dist >= (int) dist )
            result |= ENDPOINT;
    }

    return result;
}


// see class_track.h
// SEGVIA and SEGZONE inherit this version
SEARCH_RESULT TRACK::Visit( INSPECTOR* inspector, const void* testData,
                            const KICAD_T scanTypes[] )
{
    KICAD_T stype = *scanTypes;

#if 0 && defined (DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    // If caller wants to inspect my type
    if( stype == Type() )
    {
        if( SEARCH_QUIT == inspector->Inspect( this, testData ) )
            return SEARCH_QUIT;
    }

    return SEARCH_CONTINUE;
}


/***********************************************/
bool SEGVIA::IsOnLayer( int layer_number ) const
/***********************************************/
{
/**
 *  @param layer_number = layer number to test
 *  @return true if the via is on the layer layer_number
 */

    int bottom_layer, top_layer;

    ReturnLayerPair( &top_layer, &bottom_layer );

    if( bottom_layer <= layer_number && layer_number <= top_layer )
        return true;
    else
        return false;
}


/***********************************/
int TRACK::ReturnMaskLayer()
/***********************************/

/* Return the mask layer for this.
 *  for a via, there is more than one layer used
 */
{
    if( Type() == TYPEVIA )
    {
        int via_type = Shape();

        if( via_type == VIA_THROUGH )
            return ALL_CU_LAYERS;

        // VIA_BLIND ou  VIA_BURIED:

        int bottom_layer, top_layer;

        // ReturnLayerPair() knows how layers are stored
        ( (SEGVIA*) this )->ReturnLayerPair( &top_layer, &bottom_layer );

        int layermask = 0;
        while( bottom_layer <= top_layer )
        {
            layermask |= g_TabOneLayerMask[bottom_layer++];
        }

        return layermask;
    }
    else
        return g_TabOneLayerMask[m_Layer];
}


/*********************************************************/
void SEGVIA::SetLayerPair( int top_layer, int bottom_layer )
/*********************************************************/

/** Set the .m_Layer member param:
 *  For a via m_Layer contains the 2 layers :
 * top layer and bottom layer used by the via.
 * The via connect all layers from top layer to bottom layer
 * 4 bits for the first layer and 4 next bits for the secaon layer
 * @param top_layer = first layer connected by the via
 * @param bottom_layer = last layer connected by the via
 */
{
    int via_type = Shape();

    if( via_type == VIA_THROUGH )
    {
        top_layer    = LAYER_CMP_N;
        bottom_layer = COPPER_LAYER_N;
    }

    if( bottom_layer > top_layer )
        EXCHG( bottom_layer, top_layer );

    m_Layer = (top_layer & 15) + ( (bottom_layer & 15) << 4 );
}


/*********************************************************************/
void SEGVIA::ReturnLayerPair( int* top_layer, int* bottom_layer ) const
/*********************************************************************/

/* Return the 2 layers used by  the via (the via actually uses
 * all layers between these 2 layers)
 *  @param top_layer = pointer to the first layer (can be null)
 *  @param bottom_layer = pointer to the last layer (can be null)
 */
{
    int b_layer = (m_Layer >> 4) & 15;
    int t_layer = m_Layer & 15;

    if( b_layer > t_layer )
        EXCHG( b_layer, t_layer );

    if( top_layer )
        *top_layer = t_layer;

    if( bottom_layer )
        *bottom_layer = b_layer;
}


/* Remove this from the track linked list
 */
void TRACK::UnLink()
{
    /* Remove the back link */
    if( Pback )
    {
        if( Pback->Type() != TYPEPCB )
        {
            Pback->Pnext = Pnext;
        }
        else                                /* Le chainage arriere pointe sur la structure "Pere" */
        {
            if( GetState( DELETED ) )       // A REVOIR car Pback = NULL si place en undelete
            {
                if( g_UnDeleteStack )
                    g_UnDeleteStack[g_UnDeleteStackPtr - 1] = (BOARD_ITEM*) Pnext;
            }
            else
            {
                if( Type() == TYPEZONE )
                {
                    ( (BOARD*) Pback )->m_Zone = (SEGZONE*) Pnext;
                }
                else
                {
                    ( (BOARD*) Pback )->m_Track = (TRACK*) Pnext;
                }
            }
        }
    }

    /* Remove the forward link */
    if( Pnext )
        Pnext->Pback = Pback;

    Pnext = Pback = NULL;
}


/************************************************************/
void TRACK::Insert( BOARD* Pcb, BOARD_ITEM* InsertPoint )
/************************************************************/

/* insert this (and its linked segments is exists)
 * in the track linked list
 * @param InsertPoint = insert point within the linked list
 * if NULL: insert as first element of Pcb->m_Tracks
 */
{
    TRACK* track;
    TRACK* NextS;

    if( InsertPoint == NULL )
    {
        Pback = Pcb;

        if( Type() == TYPEZONE )    // put SEGZONE on front of m_Zone list
        {
            NextS = Pcb->m_Zone;
            Pcb->m_Zone = (SEGZONE*) this;
        }
        else    // put TRACK or SEGVIA on front of m_Track list
        {
            NextS = Pcb->m_Track;
            Pcb->m_Track = this;
        }
    }
    else
    {
        NextS = (TRACK*) InsertPoint->Pnext;
        Pback = InsertPoint;
        InsertPoint->Pnext = this;
    }

    /* Set the forward link */
    track = this;
    while( track->Pnext )  // Search the end of added chain
        track = (TRACK*) track->Pnext;

    /* Link the end of chain */
    track->Pnext = NextS;
    if( NextS )
        NextS->Pback = track;
}


/***********************************************/
TRACK* TRACK::GetBestInsertPoint( BOARD* Pcb )
/***********************************************/

/**
 *  Search the "best" insertion point within the track linked list
 *  the best point is the of the corresponding net code section
 *  @return the item found in the linked list (or NULL if no track)
 */
{
    TRACK* track, * NextTrack;

    if( Type() == TYPEZONE )
        track = Pcb->m_Zone;
    else
        track = Pcb->m_Track;

    /* Traitement du debut de liste */
    if( track == NULL )
        return NULL;                    /* No tracks ! */
    if( GetNet() < track->GetNet() )    /* no net code or net code = 0 (track not connected) */
        return NULL;

    while( (NextTrack = (TRACK*) track->Pnext) != NULL )
    {
        if( NextTrack->GetNet() > this->GetNet() )
            break;
        track = NextTrack;
    }

    return track;
}


/*******************************************/
TRACK* TRACK::GetStartNetCode( int NetCode )
/*******************************************/

/* Search (within the track linked list) the first segment matching the netcode
 * ( the linked list is always sorted by net codes )
 */
{
    TRACK* Track = this;
    int    ii    = 0;

    if( NetCode == -1 )
        NetCode = GetNet();

    while( Track != NULL )
    {
        if( Track->GetNet() > NetCode )
            break;

        if( Track->GetNet() == NetCode )
        {
            ii++;
            break;
        }

        Track = (TRACK*) Track->Pnext;
    }

    if( ii )
        return Track;
    else
        return NULL;
}


/*****************************************/
TRACK* TRACK::GetEndNetCode( int NetCode )
/*****************************************/

/* Search (within the track linked list) the last segment matching the netcode
 * ( the linked list is always sorted by net codes )
 */
{
    TRACK* NextS, * Track = this;
    int    ii = 0;

    if( Track == NULL )
        return NULL;

    if( NetCode == -1 )
        NetCode = GetNet();

    while( Track != NULL )
    {
        NextS = (TRACK*) Track->Pnext;
        if( Track->GetNet() == NetCode )
            ii++;

        if( NextS == NULL )
            break;

        if( NextS->GetNet() > NetCode )
            break;

        Track = NextS;
    }

    if( ii )
        return Track;
    else
        return NULL;
}


/********************************************/
bool TRACK::WriteTrackDescr( FILE* File )
/********************************************/
/* write a via description on file
*/
{
    int type = 0;

    if( Type() == TYPEVIA )
        type = 1;

    if( GetState( DELETED ) )
        return FALSE;

    fprintf( File, "Po %d %d %d %d %d %d %d\n", m_Shape,
             m_Start.x, m_Start.y, m_End.x, m_End.y, m_Width, m_Drill );

    fprintf( File, "De %d %d %d %lX %X\n",
            m_Layer, type, GetNet(),
            m_TimeStamp, ReturnStatus() );

    return TRUE;
}


/*********************************************************************/
void TRACK::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode )
/*********************************************************************/

/** Draws the segment.
 *  @param panel = current panel
 *  @param DC = current device context
 *  @param draw_mode = GR_XOR, GR_OR..
 */
{
    int l_piste;
    int color;
    int zoom;
    int rayon;
    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    if( Type() == TYPEZONE && !DisplayOpt.DisplayZones )
        return;

    GRSetDrawMode( DC, draw_mode );

    if( Type() == TYPEVIA )
        color = g_DesignSettings.m_ViaColor[m_Shape];
    else
        color = g_DesignSettings.m_LayerColor[m_Layer];

    if( ( color & (ITEM_NOT_SHOW | HIGHT_LIGHT_FLAG) ) == ITEM_NOT_SHOW )
        return;

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
        {
            color &= ~MASKCOLOR;
            color |= DARKDARKGRAY;
        }
    }

    if( draw_mode & GR_SURBRILL )
    {
        if( draw_mode & GR_AND )
            color &= ~HIGHT_LIGHT_FLAG;
        else
            color |= HIGHT_LIGHT_FLAG;
    }
    if( color & HIGHT_LIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    zoom = panel->GetZoom();

    l_piste = m_Width >> 1;

    if( Type() == TYPEVIA ) /* The via is drawn as a circle */
    {
        rayon = l_piste;
        if( rayon < zoom )
            rayon = zoom;
        GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon, color );
        if( rayon > (4 * zoom) )
        {
            int drill_rayon, inner_rayon = rayon - (2 * zoom);
            GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
                      inner_rayon, color );

            // Draw the via hole if the display option allows it
            if( DisplayOpt.m_DisplayViaMode != VIA_HOLE_NOT_SHOW )
            {
                if( (DisplayOpt.m_DisplayViaMode == ALL_VIA_HOLE_SHOW)
                   || ( m_Drill > 0 ) )
                {
                    if( m_Drill > 0 )
                        drill_rayon = m_Drill / 2;
                    else
                        drill_rayon = g_DesignSettings.m_ViaDrill / 2;
                    if( drill_rayon < inner_rayon ) // We can show the via hole
                    {
                        GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
                                  drill_rayon, color );
                    }
                }
            }

            if( DisplayOpt.DisplayTrackIsol )
                GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
                          rayon + g_DesignSettings.m_TrackClearence, color );
        }
        return;
    }

    if( m_Shape == S_CIRCLE )
    {
        rayon = (int) hypot( (double) (m_End.x - m_Start.x),
                            (double) (m_End.y - m_Start.y) );
        if( (l_piste / zoom) < L_MIN_DESSIN )
        {
            GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon, color );
        }
        else
        {
            if( l_piste <= zoom ) /* Sketch mode if l_piste/zoom <= 1 */
            {
                GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon, color );
            }
            else if( ( !DisplayOpt.DisplayPcbTrackFill) || GetState( FORCE_SKETCH ) )
            {
                GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon - l_piste, color );
                GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon + l_piste, color );
            }
            else
            {
                GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon,
                          m_Width, color );
            }
        }
        return;
    }

    if( (l_piste / zoom) < L_MIN_DESSIN )
    {
        GRLine( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
                m_End.x, m_End.y, 0, color );
        return;
    }

    if( (!DisplayOpt.DisplayPcbTrackFill) || GetState( FORCE_SKETCH ) )
    {
        GRCSegm( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
                 m_End.x, m_End.y, m_Width, color );
    }
    else
    {
        GRFillCSegm( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
                     m_End.x, m_End.y, m_Width, color );
    }

    /* Shows clearance (for tracks and vias, not for zone segments */
    if( DisplayOpt.DisplayTrackIsol && ( m_Layer <= CMP_N )
       && ( Type() == TYPETRACK || Type() == TYPEVIA) )
    {
        GRCSegm( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
                 m_End.x, m_End.y,
                 m_Width + (g_DesignSettings.m_TrackClearence * 2), color );
    }
}


// see class_track.h
void TRACK::Display_Infos( WinEDA_DrawFrame* frame )
{
    wxString msg;
    int      text_pos;

#ifdef RATSNET_DEBUG
    DbgDisplayTrackInfos( this );
#endif

    frame->MsgPanel->EraseMsgBox();

    switch( Type() )
    {
    case TYPEVIA:
        msg = g_ViaType_Name[Shape()];
        break;

    case TYPETRACK:
        msg = _( "Track" );
        break;

    case TYPEZONE:
        msg = _( "Zone" ); break;

    default:
        msg = wxT( "????" ); break;
    }

    text_pos = 1;
    Affiche_1_Parametre( frame, text_pos, _( "Type" ), msg, DARKCYAN );

    /* Display NetName pour les segments de piste type cuivre */
    text_pos += 15;
    if(  Type() == TYPETRACK
         || Type() == TYPEZONE
         || Type() == TYPEVIA )
    {
        EQUIPOT* equipot = ( (WinEDA_PcbFrame*) frame )->m_Pcb->FindNet( GetNet() );

        if( equipot )
            msg = equipot->m_Netname;
        else
            msg = wxT( "<noname>" );

        Affiche_1_Parametre( frame, text_pos, _( "NetName" ), msg, RED );

        /* Display net code : (usefull in test or debug) */
        msg.Printf( wxT( "%d .%d" ), GetNet(), GetSubNet() );
        text_pos += 18;
        Affiche_1_Parametre( frame, text_pos, _( "NetCode" ), msg, RED );
    }
    else
    {
        Affiche_1_Parametre( frame, text_pos, _( "Segment" ), wxEmptyString, RED );
        if( m_Shape == S_CIRCLE )
            Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Circle" ), RED );
        else
            Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Standard" ), RED );
    }

    /* Display the Status flags */
    msg = wxT( ". . " );
    if( GetState( SEGM_FIXE ) )
        msg[0] = 'F';

    if( GetState( SEGM_AR ) )
        msg[2] = 'A';

    text_pos = 42;
    Affiche_1_Parametre( frame, text_pos, _( "Stat" ), msg, MAGENTA );

    /* Display layer or layer pair) */
    if( Type() == TYPEVIA )
    {
        SEGVIA* Via = (SEGVIA*) this;
        int     top_layer, bottom_layer;

        Via->ReturnLayerPair( &top_layer, &bottom_layer );
        msg = ReturnPcbLayerName( top_layer, TRUE ) + wxT( "/" )
              + ReturnPcbLayerName( bottom_layer, TRUE );
    }
    else
        msg = ReturnPcbLayerName( m_Layer );

    text_pos += 5;
    Affiche_1_Parametre( frame, text_pos, _( "Layer" ), msg, BROWN );

    /* Display width */
    valeur_param( (unsigned) m_Width, msg );
    text_pos += 11;

    if( Type() == TYPEVIA )      // Display Diam and Drill values
    {
        Affiche_1_Parametre( frame, text_pos, _( "Diam" ), msg, DARKCYAN );

        int drill_value = m_Drill >= 0  ?
                          m_Drill : g_DesignSettings.m_ViaDrill;

        valeur_param( (unsigned) drill_value, msg );

        text_pos += 8;
        wxString title = _( "Drill" );

        if( g_DesignSettings.m_ViaDrill >= 0 )
            title += wxT( "*" );

        Affiche_1_Parametre( frame, text_pos, _( "Drill" ), msg, RED );
    }
    else
        Affiche_1_Parametre( frame, text_pos, _( "Width" ), msg, DARKCYAN );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param ref_pos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool TRACK::HitTest( const wxPoint& ref_pos )
{
    int l_piste;          /* demi-largeur de la piste */
    int dx, dy, spot_cX, spot_cY;
    int ux0, uy0;

    /* calcul des coordonnees du segment teste */
    l_piste = m_Width >> 1;  /* l_piste = demi largeur piste */

    ux0 = m_Start.x;
    uy0 = m_Start.y;         /* coord de depart */

    dx = m_End.x;
    dy = m_End.y;            /* coord d'arrivee */

    /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
    dx -= ux0;
    dy -= uy0;

    spot_cX = ref_pos.x - ux0;
    spot_cY = ref_pos.y - uy0;

    if( Type() == TYPEVIA )   /* VIA rencontree */
    {
        if( (abs( spot_cX ) <= l_piste ) && (abs( spot_cY ) <=l_piste) )
            return true;
        else
            return false;
    }
    else
    {
        if( DistanceTest( l_piste, dx, dy, spot_cX, spot_cY ) )
            return true;
    }

    return false;
}


#if defined (DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void TRACK::Show( int nestLevel, std::ostream& os )
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<

//        " shape=\""     << m_Shape      << '"' <<
    " layer=\"" << m_Layer << '"' <<
    " width=\"" << m_Width << '"' <<

//        " drill=\""     << m_Drill      << '"' <<
    " netcode=\"" << GetNet() << "\">" <<
    "<start" << m_Start << "/>" <<
    "<end" << m_End << "/>";

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}


/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void SEGVIA::Show( int nestLevel, std::ostream& os )
{
    const char* cp;

    switch( Shape() )
    {
    case VIA_THROUGH:
        cp = "through";
        break;

    case VIA_BURIED:
        cp = "blind";
        break;

    case VIA_BLIND:
        cp = "buried";
        break;

    default:
    case VIA_NOT_DEFINED:
        cp = "undefined";
        break;
    }

    int         topLayer;
    int         botLayer;

    ReturnLayerPair( &topLayer, &botLayer );

    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " type=\"" << cp << '"' <<
    " layers=\"" << ReturnPcbLayerName( topLayer ).Trim().mb_str() << ","
                                 << ReturnPcbLayerName( botLayer ).Trim().mb_str() << '"' <<
    " width=\"" << m_Width << '"' <<
    " drill=\"" << m_Drill << '"' <<
    " netcode=\"" << GetNet() << "\">" <<
    "<pos" << m_Start << "/>";

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}


#endif
