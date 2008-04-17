/********************************************************/
/**** Routine de lecture et visu d'un fichier GERBER ****/
/********************************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "protos.h"

#define DEFAULT_SIZE 100

/*  Format Gerber : NOTES :
    Fonctions preparatoires:
    Gn =
    G01			interpolation lineaire ( trace de droites )
    G02,G20,G21	Interpolation circulaire , sens trigo < 0
    G03,G30,G31	Interpolation circulaire , sens trigo > 0
    G04			commentaire
    G06			Interpolation parabolique
    G07			Interpolation cubique
    G10			interpolation lineaire ( echelle 10x )
    G11			interpolation lineaire ( echelle 0.1x )
    G12			interpolation lineaire ( echelle 0.01x )
    G52			plot symbole reference par Dnn code
    G53			plot symbole reference par Dnn ; symbole tourne de -90 degres
    G54			Selection d'outil
    G55			Mode exposition photo
    G56			plot symbole reference par Dnn A code
    G57			affiche le symbole reference sur la console
    G58			plot et affiche le symbole reference sur la console
    G60			interpolation lineaire ( echelle 100x )
    G70			Unites = Inches
    G71			Unites = Millimetres
    G74			supprime interpolation circulaire sur 360 degre, revient a G01
    G75			Active interpolation circulaire sur 360 degre
    G90			Mode Coordonnees absolues
    G91			Mode Coordonnees Relatives

    Coordonnees X,Y
    X,Y sont suivies de + ou - et de m+n chiffres (non separes)
            m = partie entiere
            n = partie apres la virgule
             formats classiques : 	m = 2, n = 3 (format 2.3)
                                    m = 3, n = 4 (format 3.4)
    ex:
    G__ X00345Y-06123 D__*

    Outils et D_CODES
    numero d'outil ( identification des formes )
    1 a 99 	(classique)
    1 a 999
    D_CODES:

    D01 ... D9 = codes d'action:
    D01			= activation de lumiere (baisser de plume) lors du d�placement
    D02			= extinction de lumiere (lever de plume) lors du d�placement
    D03			= Flash
    D09			= VAPE Flash

    D10 ... = Indentification d'outils ( d'ouvertures )
*/

/*********************************/
/* class GERBER_Descr : Methodes */
/*********************************/

GERBER_Descr::GERBER_Descr( int layer )
{
    int ii;

    m_Layer = layer;            // Layer Number
    m_Selected_Tool = FIRST_DCODE;
    ResetDefaultValues();
    for( ii = 0; ii <= MAX_TOOLS; ii++ )
        m_Aperture_List[ii] = new D_CODE( ii + FIRST_DCODE );
}


GERBER_Descr::~GERBER_Descr()
{
    int ii;

    if( m_Aperture_List )
    {
        for( ii = 0; ii < MAX_TOOLS; ii++ )
        {
            delete m_Aperture_List[ii];
            m_Aperture_List[ii] = NULL;
        }
    }
}


/******************************************/
void GERBER_Descr::ResetDefaultValues()
/******************************************/
{
    m_Parent = NULL;
    m_Pback  = NULL;
    m_Pnext  = NULL;
    m_FileName.Empty();
    m_Name = wxT( "no name" );          // Layer name
    m_LayerNegative = FALSE;            // TRUE = Negative Layer
    m_ImageNegative = FALSE;            // TRUE = Negative image
    m_GerbMetric    = FALSE;            // FALSE = Inches, TRUE = metric
    m_Relative = FALSE;                 // FALSE = absolute Coord, RUE = relative Coord
    m_NoTrailingZeros = FALSE;          // True: zeros a droite supprim�s
    m_MirorA   = FALSE;                 // True: miror / axe A (X)
    m_MirorB   = FALSE;                 // True: miror / axe B (Y)
    m_As_DCode = FALSE;                 // TRUE = DCodes in file (FALSE = no DCode->
                                // separate DCode file
    m_Offset.x = m_Offset.y = 0;        // Coord Offset

    m_FmtScale.x = m_FmtScale.y = g_Default_GERBER_Format % 10;
    m_FmtLen.x   = m_FmtLen.y = m_FmtScale.x + (g_Default_GERBER_Format / 10);

    m_LayerScale.x = m_LayerScale.y = 1.0;          // scale (X et Y) pour cette layer
    m_Rotation      = 0;
    m_Iterpolation  = GERB_INTERPOL_LINEAR_1X;      // Linear, 90 arc, Circ.
    m_360Arc_enbl   = FALSE;                        // 360 deg circular interpolation disable
    m_Current_Tool  = 0;                            // Current Tool (Dcode) number selected
    m_CommandState  = 0;                            // donne l'etat de l'analyse des commandes gerber
    m_CurrentPos.x  = m_CurrentPos.y = 0;           // current specified coord for plot
    m_PreviousPos.x = m_PreviousPos.y = 0;          // old current specified coord for plot
    m_IJPos.x              = m_IJPos.y = 0;         // current centre coord for plot arcs & circles
    m_Current_File         = NULL;                  // File to read
    m_FilesPtr             = 0;
    m_Transform[0][0]      = m_Transform[1][1] = 1;
    m_Transform[0][1]      = m_Transform[1][0] = 0; // Rotation/mirror = Normal
    m_PolygonFillMode      = FALSE;
    m_PolygonFillModeState = 0;
}


/********************************************/
int GERBER_Descr::ReturnUsedDcodeNumber()
/********************************************/
{
    int ii, jj;

    jj = 0;
    if( m_Aperture_List )
    {
        for( ii = 0; ii < MAX_TOOLS; ii++ )
        {
            if( m_Aperture_List[ii]->m_InUse || m_Aperture_List[ii]->m_Defined )
                jj++;
        }
    }
    return jj;
}


/******************************/
void GERBER_Descr::InitToolTable()
/******************************/

/* Creation du tableau des MAX_TOOLS DCodes utilisables, si il n'existe pas,
  * et Init des DCodes � une valeur raisonnable
 */
{
    int count;

    /* Init du buffer des D_CODES a des valeurs raisonnables */
    for( count = 0; count < MAX_TOOLS; count++ )
    {
        if( m_Aperture_List[count] == NULL )
            continue;
        m_Aperture_List[count]->m_Num_Dcode = count + FIRST_DCODE;
        m_Aperture_List[count]->Clear_D_CODE_Data();
    }
}


/*************************/
/* Class DCODE: methodes */
/*************************/

/* Variables locales : */

/* Routines Locales */


D_CODE::D_CODE( int num_dcode )
{
    m_Num_Dcode = num_dcode;
    Clear_D_CODE_Data();
}


D_CODE::~D_CODE()
{
}


void D_CODE::Clear_D_CODE_Data()
{
    m_Size.x     = DEFAULT_SIZE;
    m_Size.y     = DEFAULT_SIZE;
    m_Shape      = GERB_CIRCLE;
    m_Drill.x    = m_Drill.y = 0;
    m_DrillShape = 0;
    m_InUse   = FALSE;
    m_Defined = FALSE;
}


/******************************************************************************/
int WinEDA_GerberFrame::Read_D_Code_File( const wxString& D_Code_FullFileName )
/******************************************************************************/

/* Routine de Lecture d'un fichier de D Codes.
  * Accepte format standard ou ALSPCB
  *     un ';' demarre un commentaire.
 *
  * Format Standard:
  * tool,	 Horiz,		  Vert,	  drill, vitesse, acc. ,Type ; [DCODE (commentaire)]
  * ex:	   1,		  12,		12,		0,		  0,	 0,	  3 ; D10
 *
  * Format:
  * Ver  ,  Hor , Type , Tool [,Drill]
  * ex:	0.012, 0.012,  L   , D10
 *
  * Classe les caract en buf_tmp sous forme de tableau de structures D_CODE.
  * Retourne:
  *     < 0 si erreur:
  *         -1 = Fichier non trouve
  *         -2 = Erreur lecture fichier
  *     0 si pas de nom de fichier (inits seules)
  *     1 si OK
 */
{
    int      current_Dcode, ii, dcode_scale;
    char*    ptcar;
    int      dimH, dimV, drill, type_outil, dummy;
    float    fdimH, fdimV, fdrill;
    char     c_type_outil[256];
    char     Line[2000];
    wxString msg;
    D_CODE*  pt_Dcode;
    FILE*    dest;
    int      layer = GetScreen()->m_Active_Layer;
    D_CODE** ListeDCode;


    if( g_GERBER_Descr_List[layer] == NULL )
    {
        g_GERBER_Descr_List[layer] = new GERBER_Descr( layer );
    }

    /* Mise a jour de l'echelle gerber : */
    dcode_scale   = 10; /* ici unit dcode = mil, unit interne = 0.1 mil
                          *  -> 1 unite dcode = 10 unit PCB */
    current_Dcode = 0;

    if( D_Code_FullFileName.IsEmpty() )
        return 0;

    dest = wxFopen( D_Code_FullFileName, wxT( "rt" ) );

    if( dest == 0 )
    {
        msg = _( "File " ) + D_Code_FullFileName + _( " not found" );
        DisplayError( this, msg, 10 );
        return -1;
    }

    g_GERBER_Descr_List[layer]->InitToolTable();

    ListeDCode = g_GERBER_Descr_List[layer]->m_Aperture_List;

    while( fgets( Line, sizeof(Line) - 1, dest ) != NULL )
    {
        if( *Line == ';' )
            continue;                       /* Commentaire */
        if( strlen( Line ) < 10 )
            continue;                       /* Probablemant ligne vide */

        pt_Dcode = NULL; current_Dcode = 0;
        /* Determination du type de fichier D_Code */
        ptcar = Line; ii = 0;
        while( *ptcar )
            if( *(ptcar++) == ',' )
                ii++;

        if( ii >= 6 )   /* valeurs en mils */
        {
            sscanf( Line, "%d,%d,%d,%d,%d,%d,%d", &ii,
                &dimH, &dimV, &drill,
                &dummy, &dummy,
                &type_outil );
            dimH  = (int) ( (dimH * dcode_scale) + 0.5 );
            dimV  = (int) ( (dimV * dcode_scale) + 0.5 );
            drill = (int) ( (drill * dcode_scale) + 0.5 );
            if( ii < 1 )
                ii = 1;
            current_Dcode = ii - 1 + FIRST_DCODE;
        }
        else        /* valeurs en inches a convertir en mils */
        {
            fdrill = 0; current_Dcode = 0; fdrill = 0;
            sscanf( Line, "%f,%f,%1s", &fdimV, &fdimH, c_type_outil );
            ptcar = Line;
            while( *ptcar )
            {
                if( *ptcar == 'D' )
                {
                    sscanf( ptcar + 1, "%d,%f", &current_Dcode, &fdrill ); break;
                }
                else
                    ptcar++;
            }

            dimH       = (int) ( (fdimH * dcode_scale * 1000) + 0.5 );
            dimV       = (int) ( (fdimV * dcode_scale * 1000) + 0.5 );
            drill      = (int) ( (fdrill * dcode_scale * 1000) + 0.5 );
            type_outil = -1;
            if( c_type_outil[0] == 'L' )
                type_outil = GERB_LINE;
            if( c_type_outil[0] == 'R' )
                type_outil = GERB_RECT;
            if( c_type_outil[0] == 'C' )
                type_outil = GERB_CIRCLE;
            if( c_type_outil[0] == 'O' )
                type_outil = GERB_OVALE;
            if( type_outil == -1 )
            {
                fclose( dest ); return -2;
            }
        }
        /* Mise a jour de la liste des d_codes si valeurs lues coherentes*/
        if( current_Dcode < FIRST_DCODE )
            continue;
        if( current_Dcode >= MAX_TOOLS )
            continue;
        pt_Dcode = ReturnToolDescr( layer, current_Dcode );
        pt_Dcode->m_Size.x  = dimH;
        pt_Dcode->m_Size.y  = dimV;
        pt_Dcode->m_Shape   = type_outil;
        pt_Dcode->m_Drill.x = pt_Dcode->m_Drill.y = drill;
        pt_Dcode->m_Defined = TRUE;
    }

    fclose( dest );

    return 1;
}


/***************************************************/
void WinEDA_GerberFrame::CopyDCodesSizeToItems()
/***************************************************/

/* Set Size Items (Lines, Flashes) from DCodes List
 */
{
    TRACK*  track;
    D_CODE* pt_Dcode; /* Pointeur sur le D code*/

    track = m_Pcb->m_Track;
    for( ; track != NULL; track = (TRACK*) track->Pnext )
    {
        pt_Dcode = ReturnToolDescr( track->GetLayer(), track->GetNet() );
        pt_Dcode->m_InUse = TRUE;

        if(                                     // Line Item
            (track->m_Shape == S_SEGMENT )      /* segment rectiligne */
            || (track->m_Shape == S_RECT )      /* segment forme rect (i.e. bouts non arrondis) */
            || (track->m_Shape == S_ARC )       /* segment en arc de cercle (bouts arrondis)*/
            || (track->m_Shape == S_CIRCLE )    /* segment en cercle (anneau)*/
            || (track->m_Shape == S_ARC_RECT )  /* segment en arc de cercle (bouts droits) (GERBER)*/
            )
        {
            track->m_Width = pt_Dcode->m_Size.x;
        }
        else        // Spots ( Flashed Items )
        {
            int    width, len;
            wxSize size = pt_Dcode->m_Size;

            width = MIN( size.x, size.y );
            len   = MAX( size.x, size.y ) - width;

            track->m_Width = width;

            track->m_Start.x = (track->m_Start.x + track->m_End.x) / 2;
            track->m_Start.y = (track->m_Start.y + track->m_End.y) / 2;
            track->m_End = track->m_Start;  // m_Start = m_End = Spot center

            switch( pt_Dcode->m_Shape )
            {
            case GERB_LINE:             // ne devrait pas etre utilis� ici
            case GERB_CIRCLE:           /* spot rond (for GERBER)*/
                track->m_Shape = S_SPOT_CIRCLE;
                break;

            case GERB_OVALE:        /* spot ovale (for GERBER)*/
                track->m_Shape = S_SPOT_OVALE;
                break;

            default:                /* spot rect (for GERBER)*/
                track->m_Shape = S_SPOT_RECT;
                break;
            }

            len >>= 1;
            if( size.x > size.y )
            {
                track->m_Start.x -= len;
                track->m_End.x   += len;
            }
            else
            {
                track->m_Start.y -= len;
                track->m_End.y   += len;
            }
        }
    }
}


/*********************************************************/
D_CODE* ReturnToolDescr( int layer, int Dcode, int* index )
/*********************************************************/

/* Retourne un pointeur sur la description de l'outil DCode de reference Dcode
  * (rappel : Dcode >= 10)
 */
{
    GERBER_Descr* DcodeList = g_GERBER_Descr_List[layer];
    D_CODE*       pt_dcode  = NULL;

    if( index )
        *index = 0;
    if( DcodeList && Dcode >= FIRST_DCODE && Dcode < MAX_TOOLS )
    {
        pt_dcode = DcodeList->m_Aperture_List[Dcode - FIRST_DCODE];
        if( index )
            *index = Dcode - FIRST_DCODE + 1;
    }
    return pt_dcode;
}


/************************************************/
void WinEDA_GerberFrame::Liste_D_Codes( wxDC* DC )
/************************************************/
{
    int               ii, jj;
    D_CODE*           pt_D_code;
    wxString          Line;
    WinEDA_TextFrame* List;
    int               scale      = 10000;
    int               curr_layer = GetScreen()->m_Active_Layer;
    int               layer;
    GERBER_Descr*     DcodeList;

    /* Construction de la liste des messages */
    List = new WinEDA_TextFrame( this, _( "List D codes" ) );

    for( layer = 0; layer < 32; layer++ )
    {
        DcodeList = g_GERBER_Descr_List[layer];
        if( DcodeList == NULL )
            continue;
        if( DcodeList->ReturnUsedDcodeNumber() == 0 )
            continue;
        if( layer == curr_layer )
            Line.Printf( wxT( "*** Active layer (%2.2d) ***" ), layer + 1 );
        else
            Line.Printf( wxT( "*** layer %2.2d  ***" ), layer + 1 );
        List->Append( Line );

        for( ii = 0, jj = 1; ii < MAX_TOOLS; ii++ )
        {
            pt_D_code = DcodeList->m_Aperture_List[ii];
            if( pt_D_code == NULL )
                continue;
            if( !pt_D_code->m_InUse && !pt_D_code->m_Defined )
                continue;
            Line.Printf( wxT(
                    "tool %2.2d:   D%2.2d  V %2.4f  H %2.4f  %s" ),
                jj,
                pt_D_code->m_Num_Dcode,
                (float) pt_D_code->m_Size.y / scale,
                (float) pt_D_code->m_Size.x / scale,
                g_GERBER_Tool_Type[pt_D_code->m_Shape] );

            if( !pt_D_code->m_Defined )
                Line += wxT( " ?" );

            if( !pt_D_code->m_InUse )
                Line += wxT( " *" );

            List->Append( Line );
            jj++;
        }
    }

    ii = List->ShowModal(); List->Destroy();
    if( ii < 0 )
        return;

#if 0

    // Mise en surbrillance des �l�ments correspondant au DCode s�lectionn�
    if( Etat_Surbrillance )
        Hight_Light( DrawPanel, DC );
    net_code_Surbrillance = (GetScreen()->m_Active_Layer << 16) + ii;
    Hight_Light( DrawPanel, DC );
#endif
}
