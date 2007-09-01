/******************************************************************/
/* fonctions membres des classes TRACK et derivees (voir struct.h */
/******************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "trigo.h"
#include "protos.h"


/**************************************/
/* Classes pour Pistes, Vias et Zones */
/**************************************/

/* Constructeur des classes type pistes, vias et zones */

TRACK::TRACK( BOARD_ITEM* StructFather, KICAD_T idtype ) :
    BOARD_ITEM( StructFather, idtype )
{
    m_Width        = 0;
    m_Shape        = S_SEGMENT;
    start          = end = NULL;
    m_NetCode      = 0;
    m_Sous_Netcode = 0;
    m_Drill        = -1;
    m_Param        = 0;
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
    m_Shape     = Source.m_Shape;
    m_NetCode   = Source.m_NetCode;
    m_Flags     = Source.m_Flags;
    m_TimeStamp = Source.m_TimeStamp;
    SetStatus( Source.ReturnStatus() );
    m_Start = Source.m_Start;
    m_End   = Source.m_End;
    m_Width = Source.m_Width;
    m_Drill = Source.m_Drill;
    m_Sous_Netcode = Source.m_Sous_Netcode;
    m_Param = Source.m_Param;
}


/*  Because of the way SEGVIA is derived from TRACK and because there are 
    virtual functions being used, we can no longer simply copy a TRACK and
    expect it to be a via.  We must construct a true SEGVIA so its constructor
    can initialize the virtual function table properly.  So this constructor
    is being retired in favor of a factory type function called Copy()
    which can duplicate either a TRACK or a SEGVIA.
*/

TRACK* TRACK::Copy() const
{
    if( Type() == TYPETRACK )
        return new TRACK(*this);
    
    if( Type() == TYPEVIA )
        return new SEGVIA( (const SEGVIA&) *this );
    
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
    int dx, dy;
    int result = 0;

    if( min_dist < 0 )
        min_dist = m_Width / 2;

    dx = m_Start.x - point.x;
    dy = m_Start.y - point.y;
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
    KICAD_T     stype = *scanTypes;

#if 0 && defined(DEBUG)
    std::cout <<  GetClass().mb_str() << ' ';
#endif
    
    // If caller wants to inspect my type
    if( stype == Type() )
    {
        if( SEARCH_QUIT == inspector->Inspect( this, testData ) )
            return SEARCH_QUIT;
    }

    return SEARCH_CONTINUE;    
}


// see class_track.h
bool SEGVIA::IsOnLayer( int layer_number ) const
{
    int via_type = Shape();

    if( via_type == VIA_NORMALE )
    {
        if( layer_number <= LAYER_CMP_N )
            return true;
        else
            return false;
    }

    // VIA_BORGNE ou  VIA_ENTERREE:

    int bottom_layer, top_layer;
    
    ReturnLayerPair( &top_layer, &bottom_layer );
    
    if( bottom_layer <= layer_number && top_layer >= layer_number )
        return true;
    else
        return false;
}


/***********************************/
int TRACK::ReturnMaskLayer()
/***********************************/

/* Retourne le masque (liste bit a bit ) des couches occupees par le segment
 *  de piste pointe par PtSegm.
 *  Si PtSegm pointe une via, il y a plusieurs couches occupees
 */
{
    if( Type() == TYPEVIA )
    {
        int via_type = m_Shape & 15;
        if( via_type == VIA_NORMALE )
            return ALL_CU_LAYERS;

        // VIA_BORGNE ou  VIA_ENTERREE:
        int bottom_layer = (m_Layer >> 4) & 15;
        int top_layer    = m_Layer & 15;
        if( bottom_layer > top_layer )
            EXCHG( bottom_layer, top_layer );
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

/* Met a jour .m_Layer pour une via:
 *  m_Layer code les 2 couches limitant la via
 */
{
    int via_type = m_Shape & 255;

    if( via_type == VIA_NORMALE )
    {
        top_layer = LAYER_CMP_N; bottom_layer = LAYER_CUIVRE_N;
    }

    if( bottom_layer > top_layer )
        EXCHG( bottom_layer, top_layer );
    m_Layer = (top_layer & 15) + ( (bottom_layer & 15) << 4 );
}


/***************************************************************/
void SEGVIA::ReturnLayerPair( int* top_layer, int* bottom_layer ) const
/***************************************************************/

/* Retourne les 2 couches limitant la via
 *  les pointeurs top_layer et bottom_layer peuvent etre NULLs
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


/* supprime du chainage la structure Struct
 *  les structures arrieres et avant sont chainees directement
 */
void TRACK::UnLink()
{
    /* Modification du chainage arriere */
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
                    g_UnDeleteStack[g_UnDeleteStackPtr - 1] = (BOARD_ITEM*)Pnext;
            }
            else
            {
                if( Type() == TYPEZONE )
                {
                    ( (BOARD*) Pback )->m_Zone = (TRACK*) Pnext;
                }
                else
                {
                    ( (BOARD*) Pback )->m_Track = (TRACK*) Pnext;
                }
            }
        }
    }

    /* Modification du chainage avant */
    if( Pnext )
        Pnext->Pback = Pback;

    Pnext = Pback = NULL;
}


/************************************************************/
void TRACK::Insert( BOARD* Pcb, BOARD_ITEM* InsertPoint )
/************************************************************/
{
    TRACK* track;
    TRACK* NextS;

    /* Insertion du debut de la chaine a greffer */
    if( InsertPoint == NULL )
    {
        Pback = Pcb;
        
        if( Type() == TYPEZONE )    // put SEGZONE on front of m_Zone list
        {
            NextS = Pcb->m_Zone; 
            Pcb->m_Zone = this;
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

    /* Chainage de la fin de la liste a greffer */
    track = this;
    while( track->Pnext )
        track = (TRACK*) track->Pnext;

    /* Track pointe la fin de la chaine a greffer */
    track->Pnext = NextS;
    if( NextS )
        NextS->Pback = track;
}


/***********************************************/
TRACK* TRACK::GetBestInsertPoint( BOARD* Pcb )
/***********************************************/

/* Recherche du meilleur point d'insertion pour le nouveau segment de piste
 *  Retourne
 *      un pointeur sur le segment de piste APRES lequel l'insertion
 *           doit se faire ( dernier segment du net d'apartenance )
 *      NULL si pas de piste ( liste vide );
 */
{
    TRACK* track, * NextTrack;

    if( Type() == TYPEZONE )
        track = Pcb->m_Zone;
    else
        track = Pcb->m_Track;

    /* Traitement du debut de liste */
    if( track == NULL )
        return NULL;                    /* pas de piste ! */
    if( m_NetCode < track->m_NetCode )  /* insertion en tete de liste */
        return NULL;

    while( (NextTrack = (TRACK*) track->Pnext) != NULL )
    {
        if( NextTrack->m_NetCode > this->m_NetCode )
            break;
        track = NextTrack;
    }

    return track;
}


/* Recherche du debut du net
 *  ( les elements sont classes par net_code croissant )
 *  la recherche se fait a partir de this
 *  si net_code == -1 le netcode de this sera utilise
 *  Retourne un pointeur sur le debut du net, ou NULL si net non trouve
 */
TRACK* TRACK::GetStartNetCode( int NetCode )
{
    TRACK* Track = this;
    int    ii    = 0;

    if( NetCode == -1 )
        NetCode = m_NetCode;

    while( Track != NULL )
    {
        if( Track->m_NetCode > NetCode )
            break;
        if( Track->m_NetCode == NetCode )
        {
            ii++; break;
        }
        Track = (TRACK*) Track->Pnext;
    }

    if( ii )
        return Track;
    else
        return NULL;
}


/* Recherche de la fin du net
 *  Retourne un pointeur sur la fin du net, ou NULL si net non trouve
 */
TRACK* TRACK::GetEndNetCode( int NetCode )
{
    TRACK* NextS, * Track = this;
    int    ii = 0;

    if( Track == NULL )
        return NULL;

    if( NetCode == -1 )
        NetCode = m_NetCode;

    while( Track != NULL )
    {
        NextS = (TRACK*) Track->Pnext;
        if( Track->m_NetCode == NetCode )
            ii++;
        if( NextS == NULL )
            break;
        if( NextS->m_NetCode > NetCode )
            break;
        Track = NextS;
    }

    if( ii )
        return Track;
    else
        return NULL;
}


/**********************************/
TRACK* TRACK:: Copy( int NbSegm  )
/**********************************/

/* Copie d'un Element ou d'une chaine de n elements
 *  Retourne un pointeur sur le nouvel element ou le debut de la
 *  nouvelle chaine
 */
{
    TRACK* NewTrack, * FirstTrack, * OldTrack, * Source = this;
    int    ii;

    FirstTrack = NewTrack = new TRACK( *Source );

    for( ii = 1; ii < NbSegm; ii++ )
    {
        Source = Source->Next();
        if( Source == NULL )
            break;
        
        OldTrack = NewTrack;
        
        NewTrack = Source->Copy();
        
        NewTrack->Insert( NULL, OldTrack );
    }

    return FirstTrack;
}


/********************************************/
bool TRACK::WriteTrackDescr( FILE* File )
/********************************************/
{
    int type = 0;
    
    if( Type() == TYPEVIA )
        type = 1;

    if( GetState( DELETED ) )
        return FALSE;

    fprintf( File, "Po %d %d %d %d %d %d %d\n", m_Shape,
             m_Start.x, m_Start.y, m_End.x, m_End.y, m_Width, m_Drill );

    fprintf( File, "De %d %d %d %lX %X\n",
            m_Layer, type, m_NetCode,
            m_TimeStamp, ReturnStatus() );
    
    return TRUE;
}


/**********************************************************************/
void TRACK::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode )
/*********************************************************************/

/* routine de trace de 1 segment de piste.
 *  Parametres :
 *  draw_mode = mode ( GR_XOR, GR_OR..)
 */
{
    int l_piste;
    int color;
    int zoom;
    int rayon;
    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    if( Type() == TYPEZONE && (!DisplayOpt.DisplayZones) )
        return;

    GRSetDrawMode( DC, draw_mode );

    if( Type() == TYPEVIA ) /* VIA rencontree */
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

    if( Type() == TYPEVIA ) /* VIA rencontree */
    {
        rayon = l_piste; if( rayon < zoom )
            rayon = zoom;
        GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y, rayon, color );
        if( rayon > (4 * zoom) )
        {
            int drill_rayon, inner_rayon = rayon - (2 * zoom);
            GRCircle( &panel->m_ClipBox, DC, m_Start.x, m_Start.y,
                      inner_rayon, color );

            // Draw the via hole if the display option request it
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
            if( l_piste <= zoom ) /* trace simplifie si l_piste/zoom <= 1 */
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

    /* Trace de l'isolation (pour segments type CUIVRE et TRACK uniquement */
    if( (DisplayOpt.DisplayTrackIsol) && (m_Layer <= CMP_N )
       && ( Type() == TYPETRACK) )
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

    frame->MsgPanel->EraseMsgBox();

    switch( Type() )
    {
    case TYPEVIA:
        msg = g_ViaType_Name[m_Shape & 255];
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

    /* Affiche NetName pour les segments de piste type cuivre */
    text_pos += 15;
    if(  Type() == TYPETRACK 
      || Type() == TYPEZONE
      || Type() == TYPEVIA )
    {
        EQUIPOT* equipot = ((WinEDA_PcbFrame*)frame)->m_Pcb->FindNet( m_NetCode );
        if( equipot )
        {
            msg = equipot->m_Netname;
        }
        else
            msg = wxT( "<noname>" );
        Affiche_1_Parametre( frame, text_pos, _( "NetName" ), msg, RED );

        /* Affiche net code :*/
        msg.Printf( wxT( "%d .%d" ), m_NetCode, m_Sous_Netcode );
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

    /* Affiche les flags Status piste */
    msg = wxT( ". . " );
    if( GetState( SEGM_FIXE ) )
        msg[0] = 'F';
    
    if( GetState( SEGM_AR ) )
        msg[2] = 'A';
    
    text_pos = 42;
    Affiche_1_Parametre( frame, text_pos, _( "Stat" ), msg, MAGENTA );

    /* Affiche Layer(s) */
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

    /* Affiche Epaisseur */
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
    int     l_piste;          /* demi-largeur de la piste */
    int     dx, dy, spot_cX, spot_cY;
    int     ux0, uy0;
    
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


#if defined(DEBUG)
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
        " shape=\""     << m_Shape      << '"' <<
        " layer=\""     << m_Layer      << '"' <<
        " width=\""     << m_Width      << '"' <<
        " drill=\""     << m_Drill      << '"' <<
        " netcode=\""   << m_NetCode    << "\">" <<
        "<start"        << m_Start      << "/>" <<
        "<end"          << m_End        << "/>";
        
    os << "</" << GetClass().Lower().mb_str() << ">\n"; 
}

#endif


