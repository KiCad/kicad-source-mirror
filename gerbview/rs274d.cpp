/********************************************************/
/**** Routine de lecture et visu d'un fichier GERBER ****/
/********************************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "protos.h"

#define IsNumber( x ) ( ( ( (x) >= '0' ) && ( (x) <='9' ) )   \
                       || ( (x) == '-' ) || ( (x) == '+' )  || ( (x) == '.' ) )

/* Format Gerber : NOTES :
 *  Fonctions preparatoires:
 *  Gn =
 *  G01			interpolation lineaire ( trace de droites )
 *  G02,G20,G21	Interpolation circulaire , sens trigo < 0
 *  G03,G30,G31	Interpolation circulaire , sens trigo > 0
 *  G04			commentaire
 *  G06			Interpolation parabolique
 *  G07			Interpolation cubique
 *  G10			interpolation lineaire ( echelle 10x )
 *  G11			interpolation lineaire ( echelle 0.1x )
 *  G12			interpolation lineaire ( echelle 0.01x )
 *  G52			plot symbole reference par Dnn code
 *  G53			plot symbole reference par Dnn ; symbole tourne de -90 degres
 *  G54			Selection d'outil
 *  G55			Mode exposition photo
 *  G56			plot symbole reference par Dnn A code
 *  G57			affiche le symbole reference sur la console
 *  G58			plot et affiche le symbole reference sur la console
 *  G60			interpolation lineaire ( echelle 100x )
 *  G70			Unites = Inches
 *  G71			Unites = Millimetres
 *  G74			supprime interpolation circulaire sur 360 degre, revient a G01
 *  G75			Active interpolation circulaire sur 360 degre
 *  G90			Mode Coordonnees absolues
 *  G91			Mode Coordonnees Relatives
 *
 *  Coordonnees X,Y
 *  X,Y sont suivies de + ou - et de m+n chiffres (non separes)
 *          m = partie entiere
 *          n = partie apres la virgule
 *           formats classiques : 	m = 2, n = 3 (format 2.3)
 *                                  m = 3, n = 4 (format 3.4)
 *  ex:
 *  G__ X00345Y-06123 D__*
 *
 *  Outils et D_CODES
 *  numero d'outil ( identification des formes )
 *  1 a 99 	(classique)
 *  1 a 999
 *  D_CODES:
 *
 *  D01 ... D9 = codes d'action:
 *  D01			= activation de lumiere (baisser de plume) lors du d�placement
 *  D02			= extinction de lumiere (lever de plume) lors du d�placement
 *  D03			= Flash
 *  D09			= VAPE Flash
 *  D51			= precede par G54 -> Select VAPE
 *
 *  D10 ... D255 = Indentification d'outils ( d'ouvertures )
 *              Ne sont pas tj dans l'ordre ( voir tableau dans PCBPLOT.H)
 */

// Type d'action du phototraceur:
#define GERB_ACTIVE_DRAW 1      // activation de lumiere ( baisser de plume)
#define GERB_STOP_DRAW   2      // extinction de lumiere ( lever de plume)
#define GERB_FLASH       3      // Flash

#define NEGATE( nb ) (nb) = -(nb)

/* Variables locales : */
static wxPoint LastPosition;

/* Routines Locales */

static void Append_1_Line_GERBER( int Dcode_index, WinEDA_GerberFrame* frame, wxDC* DC,
                                  const wxPoint& startpoint, const wxPoint& endpoint,
                                  int largeur, bool isDark );

static void Append_1_Flash_GERBER( int Dcode_index, WinEDA_GerberFrame* frame, wxDC* DC,
                                   const wxPoint& pos, const wxSize& size, int form, bool isDark );

static void Append_1_Flash_ROND_GERBER( int Dcode_index, WinEDA_GerberFrame* frame, wxDC* DC,
                                        const wxPoint& pos, int diameter, bool isDark );

static void Append_1_SEG_ARC_GERBER( int Dcode_index,
                                     WinEDA_GerberFrame* frame, wxDC* DC,
                                     const wxPoint& startpoint, const wxPoint& endpoint,
                                     const wxPoint& rel_center, int largeur,
                                     bool trigo_sens, bool multiquadrant, bool isDark );


/****************************************************************/
static void Append_1_Flash_ROND_GERBER( int Dcode_tool,
                                        WinEDA_GerberFrame* frame,
                                        wxDC* DC, const wxPoint& pos, int diameter, bool isDark )
/****************************************************************/

/* Trace 1 flash ROND en position pos
 */
{
    TRACK* track;

    track = new TRACK( frame->m_Pcb );

    track->Insert( frame->m_Pcb, NULL );

    track->SetLayer( frame->GetScreen()->m_Active_Layer );
    track->m_Width = diameter;
    track->m_Start = track->m_End = pos;
    NEGATE( track->m_Start.y );
    NEGATE( track->m_End.y );
    track->SetNet( Dcode_tool );
    track->m_Shape   = S_SPOT_CIRCLE;

    Trace_Segment( frame->DrawPanel, DC, track, GR_OR );
}


/**********************************************************************/
static void Append_1_Flash_GERBER( int Dcode_index,
                                   WinEDA_GerberFrame* frame, wxDC* DC,
                                   const wxPoint& pos, const wxSize& size, int forme, bool isDark )
/*********************************************************************/

/*
 *  Trace 1 flash rectangulaire ou ovale vertical ou horizontal
 *  donne par son centre et ses dimensions X et Y
 */
{
    TRACK* track;
    int    width, len;

    width = MIN( size.x, size.y );
    len   = MAX( size.x, size.y ) - width;

    track = new TRACK( frame->m_Pcb );

    track->Insert( frame->m_Pcb, NULL );

    track->SetLayer( frame->GetScreen()->m_Active_Layer );
    track->m_Width = width;
    track->m_Start = track->m_End = pos;
    NEGATE( track->m_Start.y );
    NEGATE( track->m_End.y );
    track->SetNet( Dcode_index );

    if( forme == PAD_OVAL )
        track->m_Shape = S_SPOT_OVALE;
    else
        track->m_Shape = S_SPOT_RECT; // donc rectangle ou carr�

    len >>= 1;
    if( size.x > size.y )  // ovale / rect horizontal
    {
        track->m_Start.x -= len;
        track->m_End.x   += len;
    }
    else    // ovale / rect vertical
    {
        track->m_Start.y -= len;
        track->m_End.y   += len;
    }

    Trace_Segment( frame->DrawPanel, DC, track, GR_OR );
}


/******************************************************************/
static void Append_1_Line_GERBER( int Dcode_index,
                                  WinEDA_GerberFrame* frame, wxDC* DC,
                                  const wxPoint& startpoint, const wxPoint& endpoint,
                                  int largeur, bool isDark )
/********************************************************************/
{
    TRACK* track;

    // @todo: need to handle isDark = false case.  This means using the background color.
    // Best way to do this?

    track = new TRACK( frame->m_Pcb );

    track->Insert( frame->m_Pcb, NULL );

    track->SetLayer( frame->GetScreen()->m_Active_Layer );
    track->m_Width = largeur;
    track->m_Start = startpoint;
    NEGATE( track->m_Start.y );
    track->m_End = endpoint;
    NEGATE( track->m_End.y );
    track->SetNet( Dcode_index );

    Trace_Segment( frame->DrawPanel, DC, track, GR_OR );
}


/*****************************************************************/
static void Append_1_SEG_ARC_GERBER( int Dcode_index,
                                     WinEDA_GerberFrame* frame, wxDC* DC,
                                     const wxPoint& startpoint, const wxPoint& endpoint,
                                     const wxPoint& rel_center, int largeur,
                                     bool trigo_sens, bool multiquadrant, bool isDark )
/*****************************************************************/

/* creation d'un arc:
 *  si multiquadrant == TRUE arc de 0 a 360 degres
 *      et rel_center est la coordonn�e du centre relativement au startpoint
 *
 *  si multiquadrant == FALSE arc de 0 � 90 entierement contenu dans le meme quadrant
 *      et rel_center est la coordonn�e du centre relativement au startpoint,
 *      mais en VALEUR ABSOLUE et le signe des valeurs x et y de rel_center doit
 *      etre deduit de cette limite de 90 degres
 *
 */
{
    TRACK*  track;
    wxPoint center, delta;

    track = new TRACK( frame->m_Pcb );

    track->Insert( frame->m_Pcb, NULL );

    track->m_Shape = S_ARC;
    track->SetLayer( frame->GetScreen()->m_Active_Layer );
    track->m_Width = largeur;

    if( multiquadrant )
    {
        center.x = startpoint.x + rel_center.x;
        center.y = startpoint.y - rel_center.y;

        if( !trigo_sens )
        {
            track->m_Start = startpoint;
            track->m_End   = endpoint;
        }
        else
        {
            track->m_Start = endpoint;
            track->m_End   = startpoint;
        }
    }
    else
    {
        center  = rel_center;
        delta.x = endpoint.x - startpoint.x;
        delta.y = endpoint.y - startpoint.y;

        // il faut corriger de signe de rel_center.x et rel_center.y
        // selon le quadrant ou on se trouve
        if( (delta.x >= 0) && (delta.y >= 0) ) // 1er quadrant
        {
            center.x = -center.x;
        }
        else if( (delta.x < 0) && (delta.y >= 0) ) // 2eme quadrant
        {
            center.x = -center.x;
            center.y = -center.y;
        }
        else if( (delta.x < 0) && (delta.y < 0) )  // 3eme quadrant
        {
            center.y = -center.y;
        }
        else    // 4eme qadrant: les 2 coord sont >= 0!
        {
        }

        center.x += startpoint.x;
        center.y  = startpoint.y + center.y;

        if(  trigo_sens )
        {
            track->m_Start = startpoint;
            track->m_End   = endpoint;
        }
        else
        {
            track->m_Start = endpoint;
            track->m_End   = startpoint;
        }
    }

    track->SetNet( Dcode_index );
    track->m_Param        = center.x;
    track->SetSubNet( center.y );

    NEGATE( track->m_Start.y );
    NEGATE( track->m_End.y );

    //NEGATE( track->GetSubNet() );
    track->SetSubNet( -track->GetSubNet() );

    Trace_Segment( frame->DrawPanel, DC, track, GR_OR );
}


/**************************************************/
/* Routines utilis�es en lecture de ficher gerber */
/**************************************************/

/* ces routines lisent la chaine de texte point�e par Text.
 *  Apres appel, Text pointe le debut de la sequence non lue
 */

/***********************************************/
wxPoint GERBER::ReadXYCoord( char*& Text )
/***********************************************/

/* Retourne la coord courante pointee par Text (XnnnnYmmmm)
 */
{
    wxPoint pos = m_CurrentPos;
    int     type_coord = 0, current_coord, nbchar;
    bool    is_float   = FALSE;
    char*   text;
    char    line[256];


    if( m_Relative )
        pos.x = pos.y = 0;
    else
        pos = m_CurrentPos;

    if( Text == NULL )
        return pos;

    text = line;
    while( *Text )
    {
        if( (*Text == 'X') || (*Text == 'Y') )
        {
            type_coord = *Text;
            Text++;
            text = line;
            nbchar = 0;
            while( IsNumber( *Text ) )
            {
                if( *Text == '.' )
                    is_float = TRUE;
                *(text++) = *(Text++);
                if( (*Text >= '0') && (*Text <='9') )
                    nbchar++;
            }

            *text = 0;
            if( is_float )
            {
                if( m_GerbMetric )
                    current_coord = (int) round( atof( line ) / 0.00254 );
                else
                    current_coord = (int) round( atof( line ) * PCB_INTERNAL_UNIT );
            }
            else
            {
                int    fmt_scale = (type_coord == 'X') ? m_FmtScale.x : m_FmtScale.y;
                if( m_NoTrailingZeros )
                {
                    int min_digit = (type_coord == 'X') ? m_FmtLen.x : m_FmtLen.y;
                    while( nbchar < min_digit )
                    {
                        *(text++) = '0';
                        nbchar++;
                    }

                    *text = 0;
                }
                current_coord = atoi( line );
                double real_scale = 1.0;

                switch( fmt_scale )
                {
                case 0:
                    real_scale = 10000.0;
                    break;

                case 1:
                    real_scale = 1000.0;
                    break;

                case 2:
                    real_scale = 100.0;
                    break;

                case 3:
                    real_scale = 10.0;
                    break;

                case 4:
                    break;

                case 5:
                    real_scale = 0.1;
                    break;

                case 6:
                    real_scale = 0.01;
                    break;

                case 7:
                    real_scale = 0.001;
                    break;

                case 8:
                    real_scale = 0.0001;
                    break;

                case 9:
                    real_scale = 0.00001;
                    break;
                }

                if( m_GerbMetric )
                    real_scale = real_scale / 25.4;

                current_coord = (int) round( current_coord * real_scale );
            }

            if( type_coord == 'X' )
                pos.x = current_coord;
            else if( type_coord == 'Y' )
                pos.y = current_coord;

            continue;
        }
        else
            break;
    }

    if( m_Relative )
    {
        pos.x += m_CurrentPos.x;
        pos.y += m_CurrentPos.y;
    }

    m_CurrentPos = pos;
    return pos;
}


/************************************************/
wxPoint GERBER::ReadIJCoord( char*& Text )
/************************************************/

/* Retourne la coord type InnJnn courante pointee par Text (InnnnJmmmm)
 *  Ces coordonn�es sont relatives, donc si une coord est absente, sa valeur
 *  par defaut est 0
 */
{
    wxPoint pos( 0, 0 );

    int   type_coord = 0, current_coord, nbchar;
    bool  is_float   = FALSE;
    char* text;
    char  line[256];

    if( Text == NULL )
        return pos;

    text = line;
    while( *Text )
    {
        if( (*Text == 'I') || (*Text == 'J') )
        {
            type_coord = *Text;
            Text++;
            text = line;
            nbchar = 0;
            while( IsNumber( *Text ) )
            {
                if( *Text == '.' )
                    is_float = TRUE;
                *(text++) = *(Text++);
                if( (*Text >= '0') && (*Text <='9') )
                    nbchar++;
            }

            *text = 0;
            if( is_float )
            {
                if( m_GerbMetric )
                    current_coord = (int) round( atof( line ) / 0.00254 );
                else
                    current_coord = (int) round( atof( line ) * PCB_INTERNAL_UNIT );
            }
            else
            {
                int    fmt_scale = (type_coord == 'I') ? m_FmtScale.x : m_FmtScale.y;
                if( m_NoTrailingZeros )
                {
                    int min_digit = (type_coord == 'I') ? m_FmtLen.x : m_FmtLen.y;
                    while( nbchar < min_digit )
                    {
                        *(text++) = '0';
                        nbchar++;
                    }

                    *text = 0;
                }
                current_coord = atoi( line );
                double real_scale = 1.0;

                switch( fmt_scale )
                {
                case 0:
                    real_scale = 10000.0;
                    break;

                case 1:
                    real_scale = 1000.0;
                    break;

                case 2:
                    real_scale = 100.0;
                    break;

                case 3:
                    real_scale = 10.0;
                    break;

                case 4:
                    break;

                case 5:
                    real_scale = 0.1;
                    break;

                case 6:
                    real_scale = 0.01;
                    break;

                case 7:
                    real_scale = 0.001;
                    break;

                case 8:
                    real_scale = 0.0001;
                    break;

                case 9:
                    real_scale = 0.00001;
                    break;
                }

                if( m_GerbMetric )
                    real_scale = real_scale / 25.4;
                current_coord = (int) round( current_coord * real_scale );
            }
            if( type_coord == 'I' )
                pos.x = current_coord;
            else if( type_coord == 'J' )
                pos.y = current_coord;
            continue;
        }
        else
            break;
    }

    m_IJPos = pos;
    return pos;
}


/*****************************************************/
int GERBER::ReturnGCodeNumber( char*& Text )
/*****************************************************/

/* Lit la sequence Gnn et retourne la valeur nn
 */
{
    int   ii = 0;
    char* text;
    char  line[1024];

    if( Text == NULL )
        return 0;
    Text++;
    text = line;
    while( IsNumber( *Text ) )
    {
        *(text++) = *(Text++);
    }

    *text = 0;
    ii    = atoi( line );
    return ii;
}


/**************************************************/
int GERBER::ReturnDCodeNumber( char*& Text )
/**************************************************/

/* Lit la sequence Dnn et retourne la valeur nn
 */
{
    int   ii = 0;
    char* text;
    char  line[1024];

    if( Text == NULL )
        return 0;
    Text++;
    text = line;
    while( IsNumber( *Text ) )
        *(text++) = *(Text++);

    *text = 0;
    ii    = atoi( line );
    return ii;
}


/******************************************************************/
bool GERBER::Execute_G_Command( char*& text, int G_commande )
/******************************************************************/

{
    switch( G_commande )
    {
    case GC_LINEAR_INTERPOL_1X:
        m_Iterpolation = GERB_INTERPOL_LINEAR_1X;
        break;

    case GC_CIRCLE_NEG_INTERPOL:
        m_Iterpolation = GERB_INTERPOL_ARC_NEG;
        break;

    case GC_CIRCLE_POS_INTERPOL:
        m_Iterpolation = GERB_INTERPOL_ARC_POS;
        break;

    case GC_COMMENT:
        text = NULL;
        break;

    case GC_LINEAR_INTERPOL_10X:
        m_Iterpolation = GERB_INTERPOL_LINEAR_10X;
        break;

    case GC_LINEAR_INTERPOL_0P1X:
        m_Iterpolation = GERB_INTERPOL_LINEAR_01X;
        break;

    case GC_LINEAR_INTERPOL_0P01X:
        m_Iterpolation = GERB_INTERPOL_LINEAR_001X;
        break;

    case GC_SELECT_TOOL:
    {
        int D_commande = ReturnDCodeNumber( text );
        if( D_commande < FIRST_DCODE )
            return FALSE;
        if( D_commande > (MAX_TOOLS - 1) )
            D_commande = MAX_TOOLS - 1;
        m_Current_Tool = D_commande;
        D_CODE* pt_Dcode = GetDCODE( D_commande, false );
        if( pt_Dcode )
            pt_Dcode->m_InUse = TRUE;
        break;
    }

    case GC_SPECIFY_INCHES:
        m_GerbMetric = FALSE;           // FALSE = Inches, TRUE = metric
        break;

    case GC_SPECIFY_MILLIMETERS:
        m_GerbMetric = TRUE;            // FALSE = Inches, TRUE = metric
        break;

    case GC_TURN_OFF_360_INTERPOL:
        m_360Arc_enbl = FALSE;
        break;

    case GC_TURN_ON_360_INTERPOL:
        m_360Arc_enbl = TRUE;
        break;

    case GC_SPECIFY_ABSOLUES_COORD:
        m_Relative = FALSE;         // FALSE = absolute Coord, RUE = relative Coord
        break;

    case GC_SPECIFY_RELATIVEES_COORD:
        m_Relative = TRUE;          // FALSE = absolute Coord, RUE = relative Coord
        break;

    case GC_TURN_ON_POLY_FILL:
        m_PolygonFillMode = TRUE;
        break;

    case GC_TURN_OFF_POLY_FILL:
        m_PolygonFillMode      = FALSE;
        m_PolygonFillModeState = 0;
        break;

    case GC_MOVE:       // Non existant
    default:
    {
        wxString msg; msg.Printf( wxT( "G%.2d command not handled" ), G_commande );
        DisplayError( NULL, msg );
        return FALSE;
    }
    }


    return TRUE;
}


/**
 * Function scale
 * converts a distance given in floating point to our deci-mils
 */
static int scale( double aCoord, bool isMetric )
{
    int ret;

    if( isMetric )
        ret = (int) round( aCoord / 0.00254 );
    else
        ret = (int) round( aCoord * PCB_INTERNAL_UNIT );

    return ret;
}


/**
 * Function mapPt
 * translates a point from the aperture macro coordinate system to our
 * deci-mils coordinate system.
 * @return wxSize - The gerbview coordinate system vector.
 */
static wxSize mapPt( double x, double y, bool isMetric )
{
    wxSize  ret(  scale( x, isMetric ),
                  scale( y, isMetric ) );

    return ret;
}


static bool mapExposure( int param1, bool curExposure )
{
    bool    exposure;

    switch( param1 )
    {
    case 0:
        exposure = false;
        break;
    default:
    case 1:
        exposure = true;
        break;
    case 2:
        exposure = !curExposure;
    }

    return exposure;
}


/*****************************************************************************/
bool GERBER::Execute_DCODE_Command( WinEDA_GerberFrame* frame, wxDC* DC,
                                          char*& text, int D_commande )
/*****************************************************************************/
{
    wxSize      size( 15, 15 );

    APERTURE_T  aperture = APT_CIRCLE;

    int         dcode = 0;
    D_CODE*     tool = NULL;
    wxString    msg;

    if( D_commande >= FIRST_DCODE )  // This is a "Set tool" command
    {
        if( D_commande > (MAX_TOOLS - 1) )
            D_commande = MAX_TOOLS - 1;

        // remember which tool it selected, nothing is done with it in this call
        m_Current_Tool = D_commande;

        D_CODE* pt_Dcode = GetDCODE( D_commande, false );
        if( pt_Dcode )
            pt_Dcode->m_InUse = TRUE;

        return TRUE;
    }
    else // D_commande = 0..9:	this is a pen command (usualy D1, D2 or D3)
    {
        m_Last_Pen_Command = D_commande;
    }

    if( m_PolygonFillMode )    // Enter a polygon description:
    {
        switch( D_commande )
        {
        case 1:     // code D01 Draw line, exposure ON
        {
            m_Exposure = true;
            SEGZONE* edge_poly, * last;

            edge_poly = new SEGZONE( frame->m_Pcb );

            last = (SEGZONE*) frame->m_Pcb->m_Zone;
            if( last )
                while( last->Pnext )
                    last = (SEGZONE*) last->Pnext;

            edge_poly->Insert( frame->m_Pcb, last );

            edge_poly->SetLayer( frame->GetScreen()->m_Active_Layer );
            edge_poly->m_Width = 1;
            edge_poly->m_Start = m_PreviousPos;
            NEGATE( edge_poly->m_Start.y );
            edge_poly->m_End = m_CurrentPos;
            NEGATE( edge_poly->m_End.y );
            edge_poly->SetNet( m_PolygonFillModeState );
            m_PreviousPos = m_CurrentPos;
            m_PolygonFillModeState = 1;
            break;
        }

        case 2:     // code D2: exposure OFF (i.e. "move to")
            m_Exposure = false;
            m_PreviousPos = m_CurrentPos;
            m_PolygonFillModeState = 0;
            break;

        default:
            return FALSE;
        }
    }
    else
        switch( D_commande )
        {
        case 1:     // code D01 Draw line, exposure ON
            m_Exposure = true;

            tool = GetDCODE( m_Current_Tool, false );
            if( tool )
            {
                size     = tool->m_Size;
                dcode    = tool->m_Num_Dcode;
                aperture = tool->m_Shape;
            }

            switch( m_Iterpolation )
            {
            case GERB_INTERPOL_LINEAR_1X:
                Append_1_Line_GERBER( dcode,
                                      frame, DC,
                                      m_PreviousPos, m_CurrentPos,
                                      size.x, m_Exposure ^ m_ImageNegative );
                break;

            case GERB_INTERPOL_LINEAR_01X:
            case GERB_INTERPOL_LINEAR_001X:
            case GERB_INTERPOL_LINEAR_10X:
                wxBell();
                break;

            case GERB_INTERPOL_ARC_NEG:
                Append_1_SEG_ARC_GERBER( dcode,
                                         frame, DC,
                                         m_PreviousPos, m_CurrentPos, m_IJPos,
                                         size.x, FALSE, m_360Arc_enbl,
                                         m_Exposure ^ m_ImageNegative );
                break;

            case GERB_INTERPOL_ARC_POS:
                Append_1_SEG_ARC_GERBER( dcode,
                                         frame, DC,
                                         m_PreviousPos, m_CurrentPos, m_IJPos,
                                         size.x, TRUE, m_360Arc_enbl,
                                         m_Exposure ^ m_ImageNegative );
                break;

            default:
                msg.Printf( wxT( "Execute_DCODE_Command: interpol error (type %X)" ),
                            m_Iterpolation );
                DisplayError( frame, msg );
                break;
            }

            m_PreviousPos = m_CurrentPos;
            break;

        case 2:     // code D2: exposure OFF (i.e. "move to")
            m_Exposure = false;
            m_PreviousPos = m_CurrentPos;
            break;

        case 3:     // code D3: flash aperture
            tool = GetDCODE( m_Current_Tool, false );
            if( tool )
            {
                size     = tool->m_Size;
                dcode    = tool->m_Num_Dcode;
                aperture = tool->m_Shape;
            }

            switch( aperture )
            {
            case APT_LINE:
            case APT_CIRCLE:
                Append_1_Flash_ROND_GERBER( dcode,
                                            frame, DC,
                                            m_CurrentPos,
                                            size.x, true ^ m_ImageNegative );
                break;

            case APT_OVAL:
                Append_1_Flash_GERBER( dcode,
                                       frame, DC, m_CurrentPos,
                                       size,
                                       PAD_OVAL, true ^ m_ImageNegative );
                break;

            case APT_RECT:
                Append_1_Flash_GERBER( dcode,
                                       frame, DC, m_CurrentPos,
                                       size,
                                       PAD_RECT, true ^ m_ImageNegative );
                break;

            case APT_MACRO:
                {
                    wxPoint curPos = m_CurrentPos;
                    APERTURE_MACRO* macro = tool->GetMacro();
                    wxASSERT( macro );

                    // split the macro primitives up into multiple normal TRACK elements
                    for( AM_PRIMITIVES::iterator p=macro->primitives.begin();  p!=macro->primitives.end();  ++p )
                    {
                        bool    exposure;

                        switch( p->primitive_id )
                        {
                        case AMP_CIRCLE:
                            exposure = mapExposure( p->GetExposure(), m_Exposure );
                            curPos += mapPt( p->params[2].GetValue( tool ), p->params[3].GetValue( tool ), m_GerbMetric );
                            Append_1_Flash_ROND_GERBER( dcode, frame, DC,
                                            curPos,
                                            scale( p->params[1].GetValue( tool ), m_GerbMetric ),   // diameter
                                            exposure ^ m_ImageNegative
                                            );
                            break;

                        case AMP_LINE2:
                        case AMP_LINE20:
                            break;
                        case AMP_LINE_CENTER:
                            break;

                        case AMP_LINE_LOWER_LEFT:
                            exposure = mapExposure( p->GetExposure(), m_Exposure );
                            curPos += mapPt( p->params[3].GetValue( tool ), p->params[4].GetValue( tool ), m_GerbMetric );
                            size = mapPt( p->params[1].GetValue( tool ), p->params[2].GetValue( tool ), m_GerbMetric );
                            Append_1_Flash_GERBER( dcode, frame, DC, curPos,
                                           size,
                                           PAD_RECT, exposure ^ m_ImageNegative );
                            break;

                        case AMP_EOF:
                        case AMP_OUTLINE:
                        case AMP_POLYGON:
                        case AMP_MOIRE:
                        case AMP_THERMAL:
                        default:
                            break;
                        }
                    }
                }
                break;

            default:
                break;
            }

            m_PreviousPos = m_CurrentPos;
            break;

        default:
            return FALSE;
        }

    return TRUE;
}

