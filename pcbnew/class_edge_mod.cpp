/****************************************************/
/* class_module.cpp : fonctions de la classe MODULE */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "wxstruct.h"
#include "common.h"
#include "trigo.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"

#ifdef PCBNEW
#include "pcbnew.h"
#include "autorout.h"
#include "drag.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "protos.h"

#define MAX_WIDTH 10000     // Epaisseur (en 1/10000 ") max raisonnable des traits, textes...

/******************************************/
/* class EDGE_MODULE ( contour de module ) */
/******************************************/

EDGE_MODULE::EDGE_MODULE( MODULE* parent ) :
    BOARD_ITEM( parent, TYPE_EDGE_MODULE )
{
    m_Width     = 0;
    m_Shape     = S_SEGMENT;
    m_Angle     = 0;
    m_Width     = 120;
}


EDGE_MODULE::~EDGE_MODULE()
{
}


/********************************************/
void EDGE_MODULE:: Copy( EDGE_MODULE* source )       // copy structure
/********************************************/
{
    if( source == NULL )
        return;

    m_Start  = source->m_Start;
    m_End    = source->m_End;
    m_Shape  = source->m_Shape;
    m_Start0 = source->m_Start0;    // coord relatives a l'ancre du point de depart(Orient 0)
    m_End0   = source->m_End0;      // coord relatives a l'ancre du point de fin (Orient 0)
    m_Angle  = source->m_Angle;     // pour les arcs de cercle: longueur de l'arc en 0,1 degres
    m_Layer  = source->m_Layer;
    m_Width  = source->m_Width;

    m_PolyPoints = source->m_PolyPoints;    // std::vector copy
}

/***********************************/
void EDGE_MODULE::SetDrawCoord()
/***********************************/
{
    MODULE* Module = (MODULE*) m_Parent;

    m_Start = m_Start0;
    m_End   = m_End0;

    if( Module )
    {
        RotatePoint( &m_Start.x, &m_Start.y, Module->m_Orient );
        RotatePoint( &m_End.x, &m_End.y, Module->m_Orient );
        m_Start += Module->m_Pos;
        m_End   += Module->m_Pos;
    }
}


/********************************************************************************/
void EDGE_MODULE::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                        int draw_mode, const wxPoint& offset )
/********************************************************************************/

/* Affichage d'un segment contour de module :
 *  Entree : ox, oy = offset de trace
 *  draw_mode = mode de trace ( GR_OR, GR_XOR, GR_AND)
 *      Les contours sont de differents type:
 *      - Segment
 *      - Cercles
 *      - Arcs
 */
{
    int                  ux0, uy0, dx, dy, rayon, StAngle, EndAngle;
    int                  color, type_trace;
    int                  typeaff;
    PCB_SCREEN*          screen;
    WinEDA_BasePcbFrame* frame;
    MODULE*              Module = NULL;

    if( m_Parent && (m_Parent->Type() == TYPE_MODULE) )
        Module = (MODULE*) m_Parent;

    color = g_DesignSettings.m_LayerColor[m_Layer];
    if( (color & ITEM_NOT_SHOW) != 0 )
        return;

    frame = (WinEDA_BasePcbFrame*) panel->m_Parent;

    screen = frame->GetScreen();

    type_trace = m_Shape;

    ux0 = m_Start.x - offset.x;
    uy0 = m_Start.y - offset.y;

    dx  = m_End.x - offset.x;
    dy  = m_End.y - offset.y;

    GRSetDrawMode( DC, draw_mode );
    typeaff = frame->m_DisplayModEdge;
    if( m_Layer <= LAST_COPPER_LAYER )
    {
        typeaff = frame->m_DisplayPcbTrackFill;
        if( !typeaff )
            typeaff = SKETCH;
    }
    if( panel->GetScreen()->Scale( m_Width ) < L_MIN_DESSIN )
        typeaff = FILAIRE;

    switch( type_trace )
    {
    case S_SEGMENT:
        if( typeaff == FILAIRE )
            GRLine( &panel->m_ClipBox, DC, ux0, uy0, dx, dy, 0, color );
        else if( typeaff == FILLED )
            GRLine( &panel->m_ClipBox, DC, ux0, uy0, dx, dy, m_Width, color );
        else
            // SKETCH Mode
            GRCSegm( &panel->m_ClipBox, DC, ux0, uy0, dx, dy, m_Width, color );
        break;

    case S_CIRCLE:
        rayon = (int) hypot( (double) (dx - ux0), (double) (dy - uy0) );
        if( typeaff == FILAIRE )
        {
            GRCircle( &panel->m_ClipBox, DC, ux0, uy0, rayon, color );
        }
        else
        {
            if( typeaff == FILLED )
            {
                GRCircle( &panel->m_ClipBox, DC, ux0, uy0, rayon,
                          m_Width, color );
            }
            else        // SKETCH Mode
            {
                GRCircle( &panel->m_ClipBox, DC, ux0, uy0,
                          rayon + (m_Width / 2), color );
                GRCircle( &panel->m_ClipBox, DC, ux0, uy0,
                          rayon - (m_Width / 2), color );
            }
        }
        break;

    case S_ARC:
        rayon    = (int) hypot( (double) (dx - ux0), (double) (dy - uy0) );
        StAngle  = (int) ArcTangente( dy - uy0, dx - ux0 );
        EndAngle = StAngle + m_Angle;
        if( StAngle > EndAngle )
            EXCHG( StAngle, EndAngle );
        if( typeaff == FILAIRE )
        {
            GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                   rayon, color );
        }
        else if( typeaff == FILLED )
        {
            GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle, rayon,
                   m_Width, color );
        }
        else        // SKETCH Mode
        {
            GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                   rayon + (m_Width / 2), color );
            GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                   rayon - (m_Width / 2), color );
        }
        break;

    case S_POLYGON:
        // We must compute true coordinates from m_PolyPoints
        // which are relative to module position, orientation 0

        std::vector<wxPoint>		points = m_PolyPoints;

        for( unsigned ii = 0; ii < points.size(); ii++ )
        {
            wxPoint& pt = points[ii];

            if( Module )
            {
                RotatePoint( &pt.x, &pt.y, Module->m_Orient );
                pt.x += Module->m_Pos.x;
                pt.y += Module->m_Pos.y;
            }

            pt.x   += m_Start0.x - offset.x;
            pt.y   += m_Start0.y - offset.y;
        }

        GRPoly( &panel->m_ClipBox, DC, points.size(), &points[0],
                TRUE, m_Width, color, color );
        break;
    }
}


// see class_edge_mod.h
void EDGE_MODULE::Display_Infos( WinEDA_DrawFrame* frame )
{
    wxString bufcar;

    MODULE* module = (MODULE*) m_Parent;
    if( !module )
        return;

    BOARD* board = (BOARD*) module->GetParent();
    if( !board )
        return;

    frame->MsgPanel->EraseMsgBox();

    Affiche_1_Parametre( frame, 1, _( "Seg" ), wxEmptyString, DARKCYAN );

    Affiche_1_Parametre( frame, 5, _( "Module" ), module->m_Reference->m_Text, DARKCYAN );
    Affiche_1_Parametre( frame, 14, _( "Value" ), module->m_Value->m_Text, BLUE );
    bufcar.Printf( wxT( "%8.8lX" ), module->m_TimeStamp );

    Affiche_1_Parametre( frame, 24, _( "TimeStamp" ), bufcar, BROWN );

    Affiche_1_Parametre( frame, 34, _( "Mod Layer" ), board->GetLayerName( module->GetLayer() ), RED );

    Affiche_1_Parametre( frame, 44, _( "Seg Layer" ), board->GetLayerName( module->GetLayer() ), RED );

    valeur_param( m_Width, bufcar );
    Affiche_1_Parametre( frame, 54, _( "Width" ), bufcar, BLUE );
}


/*******************************************/
bool EDGE_MODULE::Save( FILE* aFile ) const
/*******************************************/
{
    int ret = -1;

    switch( m_Shape )
    {
    case S_SEGMENT:
        ret = fprintf( aFile, "DS %d %d %d %d %d %d\n",
                 m_Start0.x, m_Start0.y,
                 m_End0.x, m_End0.y,
                 m_Width, m_Layer );
        break;

    case S_CIRCLE:
        ret = fprintf( aFile, "DC %d %d %d %d %d %d\n",
                 m_Start0.x, m_Start0.y,
                 m_End0.x, m_End0.y,
                 m_Width, m_Layer );
        break;

    case S_ARC:
        ret = fprintf( aFile, "DA %d %d %d %d %d %d %d\n",
                 m_Start0.x, m_Start0.y,
                 m_End0.x, m_End0.y,
                 m_Angle,
                 m_Width, m_Layer );
        break;

    case S_POLYGON:
        ret = fprintf( aFile, "DP %d %d %d %d %d %d %d\n",
                 m_Start0.x, m_Start0.y,
                 m_End0.x, m_End0.y,
                 m_PolyPoints.size(),
                 m_Width, m_Layer );

        for( unsigned i=0;  i<m_PolyPoints.size();  ++i )
            fprintf( aFile, "Dl %d %d\n",  m_PolyPoints[i].x, m_PolyPoints[i].y );
        break;

    default:
        // future: throw an exception here
#if defined(DEBUG)
        printf( "EDGE_MODULE::Save(): unexpected m_Shape: %d\n", m_Shape );
#endif
        break;
    }

    return (ret > 5);
}




/****************************************************************/
int EDGE_MODULE::ReadDescr( char* Line, FILE* File,
                            int* LineNum )
/***************************************************************/

/* Read a description line like:
 *  DS 2600 0 2600 -600 120 21
 *  this description line is in Line
 *  EDGE_MODULE type can be:
 *  - Circle,
 *  - Segment (line)
 *  - Arc
 *  - Polygon
 *
 */
{
    int  ii;
    int  error = 0;
    char Buf[1024];

    switch( Line[1] )
    {
    case 'S':
        m_Shape = S_SEGMENT;
        break;

    case 'C':
        m_Shape = S_CIRCLE;
        break;

    case 'A':
        m_Shape = S_ARC;
        break;

    case 'P':
        m_Shape = S_POLYGON;
        break;

    default:
        wxString msg;
        msg.Printf( wxT( "Unknown EDGE_MODULE type <%s>" ), Line );
        DisplayError( NULL, msg );
        error = 1;
        break;
    }

    switch( m_Shape )
    {
    case  S_ARC:
        sscanf( Line + 3, "%d %d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &m_Angle, &m_Width, &m_Layer );
        break;

    case S_SEGMENT:
    case S_CIRCLE:
        sscanf( Line + 3, "%d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &m_Width, &m_Layer );
        break;

    case S_POLYGON:
        int pointCount;
        sscanf( Line + 3, "%d %d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &pointCount, &m_Width, &m_Layer );

        (*LineNum)++;
        m_PolyPoints.clear();
        m_PolyPoints.reserve( pointCount );
        for( ii = 0;  ii<pointCount;  ii++ )
        {
            if( GetLine( File, Buf, LineNum, sizeof(Buf) - 1 ) != NULL )
            {
                if( strncmp( Buf, "Dl", 2 ) != 0 )
                {
                    error = 1;
                    break;
                }

                int x;
                int y;
                sscanf( Buf + 3, "%d %d\n", &x, &y );

                m_PolyPoints.push_back( wxPoint(x,y) );

                (*LineNum)++;
            }
            else
            {
                error = 1;
                break;
            }
        }

        break;

    default:
        sscanf( Line + 3, "%d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &m_Width, &m_Layer );
        break;
    }

    // Check for a reasonnable width:
    if( m_Width <= 1 )
        m_Width = 1;
    if( m_Width > MAX_WIDTH )
        m_Width = MAX_WIDTH;

    // Check for a reasonnable layer:
    // m_Layer must be >= FIRST_NON_COPPER_LAYER, but because microwave footprints
    // can use the copper layers m_Layer < FIRST_NON_COPPER_LAYER is allowed.
    // @todo: changes use of EDGE_MODULE these footprints and allows only m_Layer >= FIRST_NON_COPPER_LAYER
    if ( (m_Layer < 0) || (m_Layer > LAST_NON_COPPER_LAYER) )
        m_Layer = SILKSCREEN_N_CMP;
    return error;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool EDGE_MODULE::HitTest( const wxPoint& ref_pos )
{
    int             uxf, uyf;
    int             rayon, dist;
    int             dx, dy, spot_cX, spot_cY;
    int             ux0, uy0;

    ux0 = m_Start.x;
    uy0 = m_Start.y;

    uxf = m_End.x;
    uyf = m_End.y;

    switch( m_Shape )
    {
    case S_SEGMENT:
        /* recalcul des coordonnees avec ux0,uy0 = origine des coord. */
        spot_cX = ref_pos.x - ux0;
        spot_cY = ref_pos.y - uy0;

        dx = uxf - ux0;
        dy = uyf - uy0;
        if( DistanceTest( m_Width/2, dx, dy, spot_cX, spot_cY ) )
            return true;
        break;

    case S_CIRCLE:
        rayon = (int) hypot( (double) (uxf - ux0), (double) (uyf - uy0) );
        dist  = (int) hypot( (double) (ref_pos.x - ux0), (double) (ref_pos.y - uy0) );
        if( abs( rayon - dist ) <= m_Width )
            return true;
        break;

    case S_ARC:
        rayon = (int) hypot( (double) (uxf - ux0), (double) (uyf - uy0) );
        dist  = (int) hypot( (double) (ref_pos.x - ux0), (double) (ref_pos.y - uy0) );

        if( abs( rayon - dist ) > m_Width )
            break;

        /* pour un arc, controle complementaire */
        int mouseAngle = (int) ArcTangente( ref_pos.y - uy0, ref_pos.x - ux0 );
        int stAngle    = (int) ArcTangente( uyf - uy0, uxf - ux0 );
        int endAngle   = stAngle + m_Angle;

        if( endAngle > 3600 )
        {
            stAngle  -= 3600;
            endAngle -= 3600;
        }

        if( (mouseAngle >= stAngle) && (mouseAngle <= endAngle) )
            return true;

        break;
    }

    return false;       // an unknown m_Shape also returns false
}


#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void EDGE_MODULE::Show( int nestLevel, std::ostream& os )
{
    wxString shape = ShowShape( (Track_Shapes) m_Shape );

    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
        " type=\"" << CONV_TO_UTF8(shape) << "\">";

    os << " <start" << m_Start0 << "/>";
    os << " <end" << m_End0 << "/>";

    os << " </" << GetClass().Lower().mb_str() << ">\n";
}

#endif
