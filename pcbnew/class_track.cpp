/***********************************************************************/
/* Functions relatives to tracks, vias and segments used to fill zones */
/* (see class_track.h )                                                */
/***********************************************************************/

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


/**
 * Function ShowClearance
 * tests to see if the clearance border is drawn on the given track.
 * @return bool - true if should draw clearance, else false.
 */
static bool ShowClearance( const TRACK* aTrack )
{
    // maybe return true for tracks and vias, not for zone segments
    return !(aTrack->m_Flags & DRAW_ERASED)
           &&  DisplayOpt.DisplayTrackIsol
           &&  aTrack->GetLayer() <= LAST_COPPER_LAYER
           && ( aTrack->Type() == TYPE_TRACK || aTrack->Type() == TYPE_VIA );
}


/**********************************************************/
TRACK::TRACK( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_CONNECTED_ITEM( aParent, idtype )
/**********************************************************/
{
    m_Width = 0;
    m_Shape = S_SEGMENT;
    start   = end = NULL;
    SetDrillDefault();
    m_Param = 0;
}


/***************************/
wxString TRACK::ShowWidth()
/***************************/
{
    wxString msg;

    valeur_param( m_Width, msg );

    return msg;
}


SEGZONE::SEGZONE( BOARD_ITEM* aParent ) :
    TRACK( aParent, TYPE_ZONE )
{
}


SEGVIA::SEGVIA( BOARD_ITEM* aParent ) :
    TRACK( aParent, TYPE_VIA )
{
}


// Copy constructor
TRACK::TRACK( const TRACK& Source ) :
    BOARD_CONNECTED_ITEM( Source )
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
    if( Type() == TYPE_TRACK )
        return new TRACK( *this );

    if( Type() == TYPE_VIA )
        return new SEGVIA( (const SEGVIA &) * this );

    if( Type() == TYPE_ZONE )
        return new SEGZONE( (const SEGZONE &) * this );

    return NULL;    // should never happen
}


/**
 * Function GetDrillValue
 * calculate the drill value for vias (m-Drill if > 0, or default drill value for the board
 * @return real drill_value
 */
int TRACK::GetDrillValue() const
{
    if( Type() != TYPE_VIA )
        return 0;

    if( m_Drill >= 0 )
        return m_Drill;

    if( m_Shape == VIA_MICROVIA )
        return g_DesignSettings.m_MicroViaDrill;

    return g_DesignSettings.m_ViaDrill;
}


/***********************/
bool TRACK::IsNull()
/***********************/

// return TRUE if segment length = 0
{
    if( ( Type() != TYPE_VIA ) && ( m_Start == m_End ) )
        return TRUE;
    else
        return FALSE;
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


EDA_Rect TRACK::GetBoundingBox()
{
    // end of track is round, this is its radius, rounded up
    int radius = ( m_Width + 1 ) / 2;

    int ymax;
    int xmax;

    int ymin;
    int xmin;

    if( Type() == TYPE_VIA )
    {
        // Because vias are sometimes drawn larger than their m_Width would
        // provide, erasing them using a dirty rect must also compensate for this
        // possibility (that the via is larger on screen than its m_Width would provide).
        // Because it is cheap to return a larger BoundingBox, do it so that
        // the via gets erased properly.  Do not divide width by 2 for this reason.
        radius = m_Width;

        ymax = m_Start.y;
        xmax = m_Start.x;

        ymin = m_Start.y;
        xmin = m_Start.x;
    }
    else
    {
        radius = ( m_Width + 1 ) / 2;

        ymax = MAX( m_Start.y, m_End.y );
        xmax = MAX( m_Start.x, m_End.x );

        ymin = MIN( m_Start.y, m_End.y );
        xmin = MIN( m_Start.x, m_End.x );
    }

    if( ShowClearance( this ) )
    {
        // + 1 is for the clearance line itself.
        radius += g_DesignSettings.m_TrackClearence + 1;
    }

    ymax += radius;
    xmax += radius;

    ymin -= radius;
    xmin -= radius;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    EDA_Rect ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
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
    if( Type() == TYPE_VIA )
    {
        int via_type = Shape();

        if( via_type == VIA_THROUGH )
            return ALL_CU_LAYERS;

        // VIA_BLIND_BURIED or VIA_MICRVIA:

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

/**
 * Function ReturnLayerPair
 * Return the 2 layers used by  the via (the via actually uses
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


/***********************************************/
TRACK* TRACK::GetBestInsertPoint( BOARD* Pcb )
/***********************************************/

/**
 *  Search the "best" insertion point within the track linked list
 *  the best point is the of the corresponding net code section
 *  @return the item found in the linked list (or NULL if no track)
 */
{
    TRACK* track;

    if( Type() == TYPE_ZONE )
        track = Pcb->m_Zone;
    else
        track = Pcb->m_Track;

    for( ; track;  track = track->Next() )
    {
        if( GetNet() <= track->GetNet() )
            return track;
    }

    return NULL;
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


bool TRACK::Save( FILE* aFile ) const
{
    int type = 0;

    if( Type() == TYPE_VIA )
        type = 1;

    if( GetState( DELETED ) )
        return true;

    fprintf( aFile, "Po %d %d %d %d %d %d %d\n", m_Shape,
        m_Start.x, m_Start.y, m_End.x, m_End.y, m_Width, m_Drill );

    fprintf( aFile, "De %d %d %d %lX %X\n",
        m_Layer, type, GetNet(),
        m_TimeStamp, ReturnStatus() );

    return true;
}


/*********************************************************************/
void TRACK::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode, const wxPoint& notUsed )
/*********************************************************************/
{
    int l_piste;
    int color;
    int zoom;
    int rayon;
    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    if( Type() == TYPE_ZONE && DisplayOpt.DisplayZonesMode != 0 )
        return;

    if( m_Flags & DRAW_ERASED )   // draw in background color, used by classs TRACK in gerbview
    {
        color = g_DrawBgColor;
        D( printf( "DRAW_ERASED in Track::Draw, g_DrawBgColor=%04X\n", g_DrawBgColor ); )
    }
    else
    {
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

        SetAlpha( &color, 150 );
    }

    GRSetDrawMode( DC, draw_mode );

    zoom = panel->GetZoom();

    l_piste = m_Width >> 1;

    if( m_Shape == S_CIRCLE )
    {
        rayon = (int) hypot( (double) ( m_End.x - m_Start.x ),
            (double) ( m_End.y - m_Start.y ) );
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

    if( !DisplayOpt.DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
    {
        GRCSegm( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
            m_End.x, m_End.y, m_Width, color );
    }
    else
    {
        GRFillCSegm( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
            m_End.x, m_End.y, m_Width, color );
    }

    // Show clearance for tracks, not for zone segments
    if( ShowClearance( this ) )
    {
        GRCSegm( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
            m_End.x, m_End.y,
            m_Width + (g_DesignSettings.m_TrackClearence * 2), color );
    }

    /* Display the short netname for tracks, not for zone segments.
     *  we must filter tracks, to avoid a lot of texts.
     *  - only horizontal or vertical tracks are eligible
     *  - only  tracks with a length > 10 * thickness are eligible
     */
    if( Type() == TYPE_ZONE )
        return;

    #define THRESHOLD 10
    if( (m_End.x - m_Start.x) != 0 &&  (m_End.y - m_Start.y) != 0 )
        return;

    int len = ABS( (m_End.x - m_Start.x) + (m_End.y - m_Start.y) );

    if( len < THRESHOLD * m_Width )
        return;

    if( ( m_Width / zoom) < 6 )     // no room to display a text inside track
        return;

    if( GetNet() == 0 )
        return;
    EQUIPOT* net = ( (BOARD*) GetParent() )->FindNet( GetNet() );
    if( net == NULL )
        return;

    int textlen = net->GetShortNetname().Len();
    if( textlen > 0 )
    {
        // calculate a good size for the text
        int tsize = MIN( m_Width, len / textlen );
        wxPoint tpos = m_Start + m_End;
        tpos.x /= 2;
        tpos.y /= 2;

        // Calculate angle: if the track segment is vertical, angle = 90 degrees
        int angle = 0;
        if ( (m_End.x - m_Start.x) == 0 )   // Vertical segment
            angle = 900;    // angle is in 0.1 degree
        if( ( tsize / zoom) >= 6 )
        {
            tsize = (tsize * 8) / 10;           // small reduction to give a better look
            DrawGraphicText( panel, DC, tpos,
                WHITE, net->GetShortNetname(), angle, wxSize( tsize, tsize ),
                GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, tsize / 7 );
        }
    }
}


/*******************************************************************************************/
void SEGVIA::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode, const wxPoint& notUsed )
/*******************************************************************************************/
{
    int color;
    int zoom;
    int rayon;
    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    GRSetDrawMode( DC, draw_mode );

    color = g_DesignSettings.m_ViaColor[m_Shape];

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

    SetAlpha( &color, 150 );

    zoom = panel->GetZoom();

    rayon = m_Width >> 1;
    if( rayon < zoom )
        rayon = zoom;

    GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon, color );
    if( rayon <= (4 * zoom) )       // Size too small: cannot be drawn
        return;

    int drill_rayon = GetDrillValue() / 2;
    int inner_rayon = rayon - (2 * zoom);

    GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
        inner_rayon, color );

    // Draw the via hole if the display option allows it
    if( DisplayOpt.m_DisplayViaMode != VIA_HOLE_NOT_SHOW )
    {
        if( (DisplayOpt.m_DisplayViaMode == ALL_VIA_HOLE_SHOW)          // Display all drill holes requested
           || ( (drill_rayon > 0 ) && !IsDrillDefault() ) )             // Or Display non default holes requested
        {
            if( drill_rayon < inner_rayon )                             // We can show the via hole
            {
                GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
                    drill_rayon, color );
            }
        }
    }

    if( DisplayOpt.DisplayTrackIsol )
        GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
            rayon + g_DesignSettings.m_TrackClearence, color );

    // for Micro Vias, draw a partial cross :
    // X on component layer, or + on copper layer
    // (so we can see 2 superimposed microvias ):
    if( Shape() == VIA_MICROVIA )
    {
        int ax, ay, bx, by;

        if( IsOnLayer( COPPER_LAYER_N ) )
        {
            ax = rayon; ay = 0;
            bx = drill_rayon; by = 0;
        }
        else
        {
            ax = ay = (rayon * 707) / 1000;
            bx = by = (drill_rayon * 707) / 1000;
        }

        /* lines | or \ */
        GRLine( &panel->m_ClipBox, DC, m_Start.x - ax, m_Start.y - ay,
            m_Start.x - bx, m_Start.y - by, 0, color );
        GRLine( &panel->m_ClipBox, DC, m_Start.x + bx, m_Start.y + by,
            m_Start.x + ax, m_Start.y + ay, 0, color );

        /* lines - or / */
        GRLine( &panel->m_ClipBox, DC, m_Start.x + ay, m_Start.y - ax,
            m_Start.x + by, m_Start.y - bx, 0, color );
        GRLine( &panel->m_ClipBox, DC, m_Start.x - by, m_Start.y + bx,
            m_Start.x - ay, m_Start.y + ax, 0, color );
    }

    // for Buried Vias, draw a partial line :
    // orient depending on layer pair
    // (so we can see superimposed buried vias ):
    if( Shape() == VIA_BLIND_BURIED )
    {
        int ax = 0, ay = rayon, bx = 0, by = drill_rayon;
        int layer_top, layer_bottom;

        ( (SEGVIA*) this )->ReturnLayerPair( &layer_top, &layer_bottom );

        /* lines for the top layer */
        RotatePoint( &ax, &ay, layer_top * 3600 / g_DesignSettings.m_CopperLayerCount );
        RotatePoint( &bx, &by, layer_top * 3600 / g_DesignSettings.m_CopperLayerCount );
        GRLine( &panel->m_ClipBox, DC, m_Start.x - ax, m_Start.y - ay,
            m_Start.x - bx, m_Start.y - by, 0, color );

        /* lines for the bottom layer */
        ax = 0; ay = rayon; bx = 0; by = drill_rayon;
        RotatePoint( &ax, &ay, layer_bottom * 3600 / g_DesignSettings.m_CopperLayerCount );
        RotatePoint( &bx, &by, layer_bottom * 3600 / g_DesignSettings.m_CopperLayerCount );
        GRLine( &panel->m_ClipBox, DC, m_Start.x - ax, m_Start.y - ay,
            m_Start.x - bx, m_Start.y - by, 0, color );
    }

    // Display the short netname:
    if( GetNet() == 0 )
        return;
    EQUIPOT* net = ( (BOARD*) GetParent() )->FindNet( GetNet() );
    if( net == NULL )
        return;

    int len = net->GetShortNetname().Len();
    if( len > 0 )
    {
        // calculate a good size for the text
        int tsize = m_Width / len;
        if( ( tsize / zoom) >= 6 )
        {
            tsize = (tsize * 8) / 10;           // small reduction to give a better look, inside via
            DrawGraphicText( panel, DC, m_Start,
                WHITE, net->GetShortNetname(), 0, wxSize( tsize, tsize ),
                GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, tsize / 7 );
        }
    }
}


// see class_track.h
void TRACK::Display_Infos( WinEDA_DrawFrame* frame )
{
    wxString msg;
    int      text_pos;
    BOARD*   board = ( (WinEDA_PcbFrame*) frame )->GetBoard();

#ifdef RATSNET_DEBUG
    DbgDisplayTrackInfos( this );
#endif

    frame->MsgPanel->EraseMsgBox();

    switch( Type() )
    {
    case TYPE_VIA:
        msg = g_ViaType_Name[Shape()];
        break;

    case TYPE_TRACK:
        msg = _( "Track" );
        break;

    case TYPE_ZONE:
        msg = _( "Zone" ); break;

    default:
        msg = wxT( "????" ); break;
    }

    text_pos = 1;

    Affiche_1_Parametre( frame, text_pos, _( "Type" ), msg, DARKCYAN );
    text_pos += 10;


    if(  Type() == TYPE_TRACK
         || Type() == TYPE_ZONE
         || Type() == TYPE_VIA )
    {
        /* Display NetName pour les segments de piste type cuivre */

        EQUIPOT* equipot = board->FindNet( GetNet() );

        if( equipot )
            msg = equipot->GetNetname();
        else
            msg = wxT( "<noname>" );

        Affiche_1_Parametre( frame, text_pos, _( "NetName" ), msg, RED );
        text_pos += 20;

        /* Display net code : (usefull in test or debug) */
        msg.Printf( wxT( "%d .%d" ), GetNet(), GetSubNet() );

        Affiche_1_Parametre( frame, text_pos, _( "NetCode" ), msg, RED );
        text_pos += 8;
    }
    else
    {
        Affiche_1_Parametre( frame, text_pos, _( "Segment" ), wxEmptyString, RED );
        if( m_Shape == S_CIRCLE )
            Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Circle" ), RED );
        else
            Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Standard" ), RED );
        text_pos += 8;
    }

#if defined(DEBUG)

    /* Display the flags */
    msg.Printf( wxT( "0x%08X" ), m_Flags );
    Affiche_1_Parametre( frame, text_pos, _( "Flags" ), msg, BLUE );
    text_pos += 8;

#endif

    /* Display the State member */
    msg = wxT( ". . " );
    if( GetState( SEGM_FIXE ) )
        msg[0] = 'F';

    if( GetState( SEGM_AR ) )
        msg[2] = 'A';

    Affiche_1_Parametre( frame, text_pos, _( "Stat" ), msg, MAGENTA );
    text_pos += 6;

    /* Display layer or layer pair) */
    if( Type() == TYPE_VIA )
    {
        SEGVIA* Via = (SEGVIA*) this;
        int     top_layer, bottom_layer;

        Via->ReturnLayerPair( &top_layer, &bottom_layer );
        msg = board->GetLayerName( top_layer ) + wxT( "/" )
              + board->GetLayerName( bottom_layer );
    }
    else
        msg = board->GetLayerName( m_Layer );

    Affiche_1_Parametre( frame, text_pos, _( "Layer" ), msg, BROWN );
    text_pos += 15;

    /* Display width */
    valeur_param( (unsigned) m_Width, msg );

    if( Type() == TYPE_VIA )      // Display Diam and Drill values
    {
        Affiche_1_Parametre( frame, text_pos, _( "Diam" ), msg, DARKCYAN );
        text_pos += 8;

        int drill_value = GetDrillValue();

        valeur_param( (unsigned) drill_value, msg );

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
    int radius = m_Width >> 1;

    // (dx, dy) is a vector from m_Start to m_End (an origin of m_Start)
    int dx = m_End.x - m_Start.x;
    int dy = m_End.y - m_Start.y;

    // (spot_cX, spot_cY) is a vector from m_Start to ref_pos (an origin of m_Start)
    int spot_cX = ref_pos.x - m_Start.x;
    int spot_cY = ref_pos.y - m_Start.y;

    if( Type() == TYPE_VIA )   /* VIA rencontree */
    {
        return (double) spot_cX * spot_cX + (double) spot_cY * spot_cY <=
               (double) radius * radius;
    }
    else
    {
        if( DistanceTest( radius, dx, dy, spot_cX, spot_cY ) )
            return true;
    }

    return false;
}


/**
 * Function HitTest (overlayed)
 * tests if the given EDA_Rect intersect this object.
 * For now, an ending point must be inside this rect.
 * @param refArea : the given EDA_Rect
 * @return bool - true if a hit, else false
 */
bool TRACK::HitTest( EDA_Rect& refArea )
{
    if( refArea.Inside( m_Start ) )
        return true;
    if( refArea.Inside( m_End ) )
        return true;
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
    " addr=\"" << std::hex << this << std::dec << '"' <<
    " layer=\"" << m_Layer << '"' <<
    " width=\"" << m_Width << '"' <<
    " flags=\"" << m_Flags << '"' <<
    " status=\"" << GetState( -1 ) << '"' <<

//        " drill=\""     << GetDrillValue()   << '"' <<
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

    case VIA_BLIND_BURIED:
        cp = "blind/buried";
        break;

    case VIA_MICROVIA:
        cp = "micro via";
        break;

    default:
    case VIA_NOT_DEFINED:
        cp = "undefined";
        break;
    }

    int    topLayer;
    int    botLayer;
    BOARD* board = (BOARD*) m_Parent;


    ReturnLayerPair( &topLayer, &botLayer );

    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " type=\"" << cp << '"';

    if( board )
        os << " layers=\"" << board->GetLayerName( topLayer ).Trim().mb_str() << ","
           << board->GetLayerName( botLayer ).Trim().mb_str() << '"';
    os <<
    " width=\"" << m_Width << '"' <<
    " drill=\"" << GetDrillValue() << '"' <<
    " netcode=\"" << GetNet() << "\">" <<
    "<pos" << m_Start << "/>";

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}


wxString TRACK::ShowState( int stateBits )
{
    wxString ret;

    if( stateBits & CHAIN )
        ret << wxT( " | CHAIN" );
    if( stateBits & SEGM_AR )
        ret << wxT( " | SEGM_AR" );
    if( stateBits & SEGM_FIXE )
        ret << wxT( " | SEGM_FIXE" );
    if( stateBits & EDIT )
        ret << wxT( " | EDIT" );
    if( stateBits & DRAG )
        ret << wxT( " | DRAG" );
    if( stateBits & SURBRILL )
        ret << wxT( " | SURBRILL" );
    if( stateBits & NO_TRACE )
        ret << wxT( " | NO_TRACE" );
    if( stateBits & DELETED )
        ret << wxT( " | DELETED" );
    if( stateBits & BUSY )
        ret << wxT( " | BUSY" );
    if( stateBits & END_ONPAD )
        ret << wxT( " | END_ONPAD" );
    if( stateBits & BEGIN_ONPAD )
        ret << wxT( " | BEGIN_ONPAD" );
    if( stateBits & FLAG0 )
        ret << wxT( " | FLAG0" );
    if( stateBits & FLAG1 )
        ret << wxT( " | FLAG1" );

    return ret;
}


#endif
