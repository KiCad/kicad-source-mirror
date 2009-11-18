/*******************************************/
/* mounde.cpp - Microwave pcb layout code. */
/*******************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "trigo.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "protos.h"


#define COEFF_COUNT 6
static double* PolyEdges;
static int     PolyEdgesCount;
static double  ShapeScaleX, ShapeScaleY;
static wxSize  ShapeSize;
static int     PolyShapeType;


static void         Exit_Self( WinEDA_DrawPanel* Panel, wxDC* DC );
static EDGE_MODULE* gen_arc( MODULE*      aModule,
                             EDGE_MODULE* PtSegm,
                             int          cX,
                             int          cY,
                             int          angle );
static void         ShowCadreSelf( WinEDA_DrawPanel* panel,
                                   wxDC*             DC,
                                   bool              erase );


class SELFPCB
{
public:
    int     forme;                      // Shape: coil, spiral, etc ..
    int     orient;                     // 0..3600
    int     valeur;                     // Value.
    wxPoint m_Start;
    wxPoint m_End;
    wxSize  m_Size;
    D_PAD*  pt_pad_start, * pt_pad_end;
    int     lng;                        // Trace length.
    int     m_Width;
    int     nbrin;                      // Number of segments.
    int     lbrin;                      // Length of segments.
    int     rayon;                      // Radius between segments.
    int     delta;                      // distance between pads
};

static SELFPCB Mself;
static int     Self_On;
static int     Bl_X0, Bl_Y0, Bl_Xf, Bl_Yf;


/* ??? Routine d'affichage a l'ecran du cadre de la self */
static void ShowCadreSelf( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    int deltaX, deltaY;

    /* Calculate the orientation and size of the window:
     * - Orient = vertical or horizontal (maximum dimensions)
     * - Size.x = Size.y / 2
     */
    GRSetDrawMode( DC, GR_XOR );

    if( erase )
    {
        GRRect( &panel->m_ClipBox, DC, Bl_X0, Bl_Y0, Bl_Xf, Bl_Yf, YELLOW );
    }

    deltaX = ( panel->GetScreen()->m_Curseur.x - Mself.m_Start.x ) / 4;
    deltaY = ( panel->GetScreen()->m_Curseur.y - Mself.m_Start.y ) / 4;

    Mself.orient = 900;
    if( abs( deltaX ) > abs( deltaY ) )
        Mself.orient = 0;

    if( Mself.orient == 0 )
    {
        Bl_X0 = Mself.m_Start.x;
        Bl_Y0 = Mself.m_Start.y - deltaX;
        Bl_Xf = panel->GetScreen()->m_Curseur.x;
        Bl_Yf = Mself.m_Start.y + deltaX;
    }
    else
    {
        Bl_X0 = Mself.m_Start.x - deltaY;
        Bl_Y0 = Mself.m_Start.y;
        Bl_Xf = Mself.m_Start.x + deltaY;
        Bl_Yf = panel->GetScreen()->m_Curseur.y;
    }
    GRRect( &panel->m_ClipBox, DC, Bl_X0, Bl_Y0, Bl_Xf, Bl_Yf, YELLOW );
}


void Exit_Self( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    if( Self_On )
    {
        Self_On = 0;
        Panel->ManageCurseur( Panel, DC, 0 );
        Panel->ManageCurseur = NULL;
        Panel->ForceCloseManageCurseur = NULL;
    }
}


void WinEDA_PcbFrame::Begin_Self( wxDC* DC )
{
    if( Self_On )
    {
        Genere_Self( DC );
        return;
    }

    Mself.m_Start = GetScreen()->m_Curseur;

    Self_On = 1;

    /* Update the initial coordinates. */
    GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
    UpdateStatusBar();

    Bl_X0 = Mself.m_Start.x;
    Bl_Y0 = Mself.m_Start.y;

    Bl_Xf = Bl_X0;
    Bl_Yf = Bl_Y0;

    DrawPanel->ManageCurseur = ShowCadreSelf;
    DrawPanel->ForceCloseManageCurseur = Exit_Self;
    DrawPanel->ManageCurseur( DrawPanel, DC, 0 );
}


/* Create a self-shaped coil
 * - Length Mself.lng
 * - Extremities Mself.m_Start and Mself.m_End
 * - Constraint: m_Start.x = m_End.x (self Vertical)
 * Or m_Start.y = m_End.y (self Horizontal)
 *
 * We must determine:
 * Mself.nbrin = number of segments perpendicular to the direction
 * (The coil nbrin will demicercles + 1 + 2 1 / 4 circle)
 * Mself.lbrin = length of a strand
 * Mself.rayon = radius of rounded parts of the coil
 * Mself.delta = segments extremities connection between him and the coil even
 *
 * The equations are
 * Mself.m_Size.x = 2 * Mself.rayon + Mself.lbrin
 * Mself.m_Size.y * Mself.delta = 2 + 2 * Mself.nbrin * Mself.rayon
 * Mself.lng = 2 * Mself.delta / / connections to the coil
 + (Mself.nbrin-2) * Mself.lbrin / / length of the strands except 1st and last
 + (Mself.nbrin 1) * (PI * Mself.rayon) / / length of rounded
 * Mself.lbrin + / 2 - Melf.rayon * 2) / / length of 1st and last bit
 *
 * The constraints are:
 * Nbrin >= 2
 * Mself.rayon < Mself.m_Size.x
 * Mself.m_Size.y = Mself.rayon * 4 + 2 * Mself.raccord
 * Mself.lbrin> Mself.rayon * 2
 *
 * The calculation is conducted in the following way:
 * Initially:
 * Nbrin = 2
 * Radius = 4 * m_Size.x (arbitrarily fixed value)
 * Then:
 * Increasing the number of segments to the desired length
 * (Radius decreases if necessary)
 *
 */
MODULE* WinEDA_PcbFrame::Genere_Self( wxDC* DC )
{
    EDGE_MODULE* PtSegm, * LastSegm, * FirstSegm, * newedge;
    MODULE*      Module;
    D_PAD*       PtPad;
    int          ii, ll, lextbrin;
    double       fcoeff;
    bool         abort = FALSE;
    wxString     msg;

    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;

    if( Self_On == 0 )
    {
        DisplayError( this, wxT( "Starting point not init.." ) );
        return NULL;
    }

    Self_On = 0;

    Mself.m_End = GetScreen()->m_Curseur;

    /* Fitting of parameters to simplify the calculation:
     * The starting point must be coord departure from the end point */
    if( Mself.orient == 0 )    // Horizontal
    {
        Mself.m_End.y = Mself.m_Start.y;
        if( Mself.m_Start.x > Mself.m_End.x )
            EXCHG( Mself.m_Start.x, Mself.m_End.x );
        Mself.m_Size.y = Mself.m_End.x - Mself.m_Start.x;
        Mself.lng = Mself.m_Size.y;
    }
    else                       // Vertical
    {
        Mself.m_End.x = Mself.m_Start.x;
        if( Mself.m_Start.y > Mself.m_End.y )
            EXCHG( Mself.m_Start.y, Mself.m_End.y );
        Mself.m_Size.y = Mself.m_End.y - Mself.m_Start.y;
        Mself.lng = Mself.m_Size.y;
    }

    /* Enter the desired length. */
    if( !g_UnitMetric )
    {
        fcoeff = 10000.0;
        msg.Printf( wxT( "%1.4f" ), Mself.lng / fcoeff );
        abort = Get_Message( _( "Length(inch):" ), _( "Length" ), msg, this );
    }
    else
    {
        fcoeff = 10000.0 / 25.4;
        msg.Printf( wxT( "%2.3f" ), Mself.lng / fcoeff );
        abort = Get_Message( _( "Length(mm):" ), _( "Length" ), msg, this );
    }
    if( abort )
        return NULL;

    double fval;
    if( !msg.ToDouble( &fval ) )
    {
        DisplayError( this, _( "Incorrect number, abort" ) );
        return NULL;
    }
    Mself.lng = wxRound( fval * fcoeff );

    /* Control values (ii = minimum length) */
    if( Mself.lng < Mself.m_Size.y )
    {
        DisplayError( this, _( "Requested length < minimum length" ) );
        return NULL;
    }

    /* Calculate the elements. */
    Mself.m_Width  = GetBoard()->GetCurrentTrackWidth();
    Mself.m_Size.x = Mself.m_Size.y / 2;

    // Choose a reasonable starting value for the radius of the arcs.
    Mself.rayon = MIN( Mself.m_Width * 5, Mself.m_Size.x / 4 );

    for( Mself.nbrin = 2; ; Mself.nbrin++ )
    {
        Mself.delta =
            ( Mself.m_Size.y - ( Mself.rayon * 2 * Mself.nbrin ) ) / 2;
        if( Mself.delta < Mself.m_Size.y / 10 ) // Reduce radius.
        {
            Mself.delta = Mself.m_Size.y / 10;
            Mself.rayon =
                ( Mself.m_Size.y - 2 * Mself.delta ) / ( 2 * Mself.nbrin );
            if( Mself.rayon < Mself.m_Width ) // Radius too small.
            {
                Affiche_Message( _( "Unable to create line: Requested length is too big" ) );
                return NULL;
            }
        }
        Mself.lbrin = Mself.m_Size.x - ( Mself.rayon * 2 );
        lextbrin    = ( Mself.lbrin / 2 ) - Mself.rayon;
        ll  = 2 * lextbrin;                     // Length of first and last
                                                // segment.
        ll += 2 * Mself.delta;                  // Length of coil connections.
        ll += Mself.nbrin * ( Mself.lbrin - 2 );  // Length of other segments.
        ll += ( ( Mself.nbrin + 1 ) * 314 * Mself.rayon ) / 100;

        msg.Printf( _( "Segment count = %d, length = " ), Mself.nbrin );
        wxString stlen;
        valeur_param( ll, stlen );
        msg += stlen;
        Affiche_Message( msg );
        if( ll >= Mself.lng )
            break;
    }

    /* Generate module. */
    Module = Create_1_Module( DC, wxEmptyString );
    if( Module == NULL )
        return NULL;

    // here the Module is already in the BOARD, Create_1_Module() does that.
    Module->m_LibRef    = wxT( "MuSelf" );
    Module->m_Attributs = MOD_VIRTUAL | MOD_CMS;
    Module->m_Flags     = 0;

    Module->Draw( DrawPanel, DC, GR_XOR );

    /* Generate special features. */
    FirstSegm = PtSegm = new EDGE_MODULE( Module );
    Module->m_Drawings.PushBack( PtSegm );

    PtSegm->m_Start = Mself.m_Start;
    PtSegm->m_End.x = Mself.m_Start.x;
    PtSegm->m_End.y = PtSegm->m_Start.y + Mself.delta;
    PtSegm->m_Width = Mself.m_Width;
    PtSegm->SetLayer( Module->GetLayer() );
    PtSegm->m_Shape = S_SEGMENT;

    newedge = new EDGE_MODULE( Module );
    newedge->Copy( PtSegm );

    Module->m_Drawings.PushBack( newedge );

    PtSegm = newedge;
    PtSegm->m_Start = PtSegm->m_End;

    PtSegm = gen_arc( Module,
                      PtSegm,
                      PtSegm->m_End.x - Mself.rayon,
                      PtSegm->m_End.y,
                      -900 );

    if( lextbrin )
    {
        newedge = new EDGE_MODULE( Module );
        newedge->Copy( PtSegm );

        Module->m_Drawings.PushBack( newedge );

        PtSegm = newedge;
        PtSegm->m_Start  = PtSegm->m_End;
        PtSegm->m_End.x -= lextbrin;
    }

    /* Create coil. */
    for( ii = 1; ii < Mself.nbrin; ii++ )
    {
        int arc_angle;
        newedge = new EDGE_MODULE( Module );
        newedge->Copy( PtSegm );

        Module->m_Drawings.PushBack( newedge );

        PtSegm = newedge;
        PtSegm->m_Start = PtSegm->m_End;

        if( ii & 1 ) /* odd order arcs are greater than 0 */
            arc_angle = 1800;
        else
            arc_angle = -1800;

        PtSegm = gen_arc( Module, PtSegm, PtSegm->m_End.x,
                          PtSegm->m_End.y + Mself.rayon, arc_angle );

        if( ii < Mself.nbrin - 1 )
        {
            newedge = new EDGE_MODULE( Module );
            newedge->Copy( PtSegm );

            Module->m_Drawings.Insert( newedge, PtSegm->Next() );

            PtSegm = newedge;
            PtSegm->m_Start = PtSegm->m_End;
            if( ii & 1 )
                PtSegm->m_End.x += Mself.lbrin;
            else
                PtSegm->m_End.x -= Mself.lbrin;
        }
    }

    /* Create last segment. */
    if( ii & 1 )
    {
        if( lextbrin )
        {
            newedge = new EDGE_MODULE( Module );
            newedge->Copy( PtSegm );

            Module->m_Drawings.Insert( newedge, PtSegm->Next() );

            PtSegm = newedge;
            PtSegm->m_Start  = PtSegm->m_End;
            PtSegm->m_End.x -= lextbrin;
        }

        newedge = new EDGE_MODULE( Module );
        newedge->Copy( PtSegm );
        Module->m_Drawings.PushBack( newedge );

        PtSegm = newedge;
        PtSegm->m_Start.x = PtSegm->m_End.x; PtSegm->m_Start.y =
            PtSegm->m_End.y;
        PtSegm = gen_arc( Module,
                          PtSegm,
                          PtSegm->m_End.x,
                          PtSegm->m_End.y + Mself.rayon,
                          900 );
    }
    else
    {
        if( lextbrin )
        {
            newedge = new EDGE_MODULE( Module );
            newedge->Copy( PtSegm );

            Module->m_Drawings.Insert( newedge, PtSegm->Next() );

            PtSegm = newedge;
            PtSegm->m_Start  = PtSegm->m_End;
            PtSegm->m_End.x += lextbrin;
        }
        newedge = new EDGE_MODULE( Module );
        newedge->Copy( PtSegm );

        Module->m_Drawings.PushBack( newedge );
        PtSegm = newedge;
        PtSegm->m_Start = PtSegm->m_End;
        PtSegm = gen_arc( Module,
                          PtSegm,
                          PtSegm->m_End.x,
                          PtSegm->m_End.y + Mself.rayon,
                          -900 );
    }

    newedge = new EDGE_MODULE( Module );
    newedge->Copy( PtSegm );
    Module->m_Drawings.Insert( newedge, PtSegm->Next() );
    PtSegm = newedge;

    PtSegm->m_Start = PtSegm->m_End;
    PtSegm->m_End   = Mself.m_End;

    /* Rotate the coil if it has a horizontal orientation. */
    LastSegm = PtSegm;
    if( Mself.orient == 0 )
    {
        for( PtSegm = FirstSegm;
            PtSegm != NULL;
            PtSegm = (EDGE_MODULE*) PtSegm->Next() )
        {
            RotatePoint( &PtSegm->m_Start.x, &PtSegm->m_Start.y,
                         FirstSegm->m_Start.x, FirstSegm->m_Start.y, 900 );
            if( PtSegm != LastSegm )
                RotatePoint( &PtSegm->m_End.x, &PtSegm->m_End.y,
                             FirstSegm->m_Start.x, FirstSegm->m_Start.y, 900 );
        }
    }

    Module->m_Pos = LastSegm->m_End;

    /* Place pad on each end of coil. */
    PtPad = new D_PAD( Module );

    Module->m_Pads.PushFront( PtPad );

    PtPad->SetPadName( wxT( "1" ) );
    PtPad->m_Pos = LastSegm->m_End;
    PtPad->m_Pos0 = PtPad->m_Pos - Module->m_Pos;
    PtPad->m_Size.x = PtPad->m_Size.y = LastSegm->m_Width;
    PtPad->m_Masque_Layer = g_TabOneLayerMask[LastSegm->GetLayer()];
    PtPad->m_Attribut     = PAD_SMD;
    PtPad->m_PadShape     = PAD_CIRCLE;
    PtPad->m_Rayon = PtPad->m_Size.x / 2;

    D_PAD* newpad = new D_PAD( Module );
    newpad->Copy( PtPad );

    Module->m_Pads.Insert( newpad, PtPad->Next() );

    PtPad = newpad;
    PtPad->SetPadName( wxT( "2" ) );
    PtPad->m_Pos = FirstSegm->m_Start;
    PtPad->m_Pos0 = PtPad->m_Pos - Module->m_Pos;

    /* Modify text positions. */
    Module->DisplayInfo( this );
    Module->m_Value->m_Pos.x = Module->m_Reference->m_Pos.x =
        ( FirstSegm->m_Start.x + LastSegm->m_End.x ) / 2;
    Module->m_Value->m_Pos.y = Module->m_Reference->m_Pos.y =
        ( FirstSegm->m_Start.y + LastSegm->m_End.y ) / 2;

    Module->m_Reference->m_Pos.y -= Module->m_Reference->m_Size.y;
    Module->m_Value->m_Pos.y += Module->m_Value->m_Size.y;
    Module->m_Reference->m_Pos0 = Module->m_Reference->m_Pos - Module->m_Pos;
    Module->m_Value->m_Pos0 = Module->m_Value->m_Pos - Module->m_Pos;

    /* Initial segment coordinates. */
    for( PtSegm = FirstSegm; PtSegm; PtSegm = PtSegm->Next() )
    {
        PtSegm->m_Start0 = PtSegm->m_Start - Module->m_Pos;
        PtSegm->m_End0   = PtSegm->m_End - Module->m_Pos;
    }

    Module->Set_Rectangle_Encadrement();
    Module->Draw( DrawPanel, DC, GR_OR );

    return Module;
}


/* Generate an arc EDGE_MODULE:
 * Center cX, cY
 * Angle "angle"
 * Starting point gives the structure pointed to by PtSegm, which must
 * Returns a pointer to the structure EDGE_MODULE generated.
 */
static EDGE_MODULE* gen_arc( MODULE*      aModule,
                             EDGE_MODULE* PtSegm,
                             int          cX,
                             int          cY,
                             int          angle )
{
    int          ii, nb_seg;
    double       alpha, beta, fsin, fcos;
    int          x0, xr0, y0, yr0;
    EDGE_MODULE* newedge;

    angle = -angle;
    y0    = PtSegm->m_Start.x - cX;
    x0    = PtSegm->m_Start.y - cY;

    nb_seg = ( abs( angle ) ) / 225;

    if( nb_seg == 0 )
        nb_seg = 1;

    alpha = ( (double) angle * 3.14159 / 1800 ) / nb_seg;

    for( ii = 1; ii <= nb_seg; ii++ )
    {
        if( ii > 1 )
        {
            newedge = new EDGE_MODULE( aModule );

            newedge->Copy( PtSegm );

            aModule->m_Drawings.PushBack( newedge );

            PtSegm = newedge;

            PtSegm->m_Start = PtSegm->m_End;
        }

        beta = ( alpha * ii );
        fcos = cos( beta ); fsin = sin( beta );

        xr0 = (int) ( x0 * fcos + y0 * fsin );
        yr0 = (int) ( y0 * fcos - x0 * fsin );

        PtSegm->m_End.x = cX + yr0;
        PtSegm->m_End.y = cY + xr0;
    }

    return PtSegm;
}


/* Create a footprint with pad_count pads for micro wave applications
 *  This footprint has pad_count pads:
 *  PAD_SMD, rectangular, H size = V size = current track width.
 */
MODULE* WinEDA_PcbFrame::Create_MuWaveBasicShape( const wxString& name,
                                                  int             pad_count )
{
    MODULE*  Module;
    int      pad_num = 1;
    wxString Line;

    Module = Create_1_Module( NULL, name );
    if( Module == NULL )
        return NULL;

    Module->m_TimeStamp = GetTimeStamp();
    Module->m_Value->m_Size = wxSize( 30, 30 );
    Module->m_Value->m_Pos0.y     = -30;
    Module->m_Value->m_Pos.y     += Module->m_Value->m_Pos0.y;
    Module->m_Reference->m_Size   = wxSize( 30, 30 );
    Module->m_Reference->m_Pos0.y = 30;
    Module->m_Reference->m_Pos.y += Module->m_Reference->m_Pos0.y;

    /* Create dots forming the gap. */
    while( pad_count-- )
    {
        D_PAD* pad = new D_PAD( Module );

        Module->m_Pads.PushFront( pad );

        pad->m_Size.x = pad->m_Size.y = GetBoard()->GetCurrentTrackWidth();
        pad->m_Pos = Module->m_Pos;
        pad->m_PadShape     = PAD_RECT;
        pad->m_Attribut     = PAD_SMD;
        pad->m_Masque_Layer = CMP_LAYER;
        Line.Printf( wxT( "%d" ), pad_num );
        pad->SetPadName( Line );
        pad_num++;
    }

    return Module;
}


#if 0
static void Exit_Muonde( WinEDA_DrawFrame* frame, wxDC* DC )
{
    MODULE* Module = (MODULE*) frame->GetScreen()->GetCurItem();

    if( Module )
    {
        if( Module->m_Flags & IS_NEW )
        {
            Module->Draw( frame->DrawPanel, DC, GR_XOR );
            Module->DeleteStructure();
        }
        else
        {
            Module->Draw( frame->DrawPanel, DC, GR_XOR );
        }
    }

    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;
    frame->SetCurItem( NULL );
}


#endif


/* Create a module "GAP" or "STUB"
 *  This a "gap" or  "stub" used in micro wave designs
 *  This module has 2 pads:
 *  PAD_SMD, rectangular, H size = V size = current track width.
 *  the "gap" is isolation created between this 2 pads
 */
MODULE* WinEDA_PcbFrame::Create_MuWaveComponent(  int shape_type )
{
    int      oX;
    float    fcoeff;
    D_PAD*   pad;
    MODULE*  Module;
    wxString msg, cmp_name;
    int      pad_count = 2;
    int      angle     = 0;
    bool     abort;

    /* Enter the size of the gap or stub*/
    int      gap_size = GetBoard()->GetCurrentTrackWidth();

    switch( shape_type )
    {
    case 0:
        msg = _( "Gap" );
        cmp_name = wxT( "GAP" );
        break;

    case 1:
        msg = _( "Stub" );
        cmp_name  = wxT( "STUB" );
        pad_count = 2;
        break;

    case 2:
        msg = _( "Arc Stub" );
        cmp_name  = wxT( "ASTUB" );
        pad_count = 1;
        break;

    default:
        msg = wxT( "???" );
        break;
    }

    wxString value;
    if( g_UnitMetric )
    {
        fcoeff = 10000.0f / 25.4f;
        value.Printf( wxT( "%2.4f" ), gap_size / fcoeff );
        msg += _( " (mm):" );
    }
    else
    {
        fcoeff = 10000.0;
        value.Printf( wxT( "%2.3f" ), gap_size / fcoeff );
        msg += _( " (inch):" );
    }
    abort = Get_Message( msg, _( "Create microwave module" ), value, this );

    double fval;
    if( !value.ToDouble( &fval ) )
    {
        DisplayError( this, _( "Incorrect number, abort" ) );
        abort = TRUE;
    }
    gap_size = ABS( wxRound( fval * fcoeff ) );

    if( !abort && ( shape_type == 2 ) )
    {
        fcoeff = 10.0;
        value.Printf( wxT( "%3.1f" ), angle / fcoeff );
        msg   = _( "Angle (0.1deg):" );
        abort = Get_Message( msg, _( "Create microwave module" ), value, this );
        if( !value.ToDouble( &fval ) )
        {
            DisplayError( this, _( "Incorrect number, abort" ) );
            abort = TRUE;
        }
        angle = ABS( wxRound( fval * fcoeff ) );
        if( angle > 1800 )
            angle = 1800;
    }

    if( abort )
    {
        DrawPanel->MouseToCursorSchema();
        return NULL;
    }

    Module = Create_MuWaveBasicShape( cmp_name, pad_count );
    pad    = Module->m_Pads;

    switch( shape_type )
    {
    case 0:     //Gap :
        oX = pad->m_Pos0.x = -( gap_size + pad->m_Size.x ) / 2;
        pad->m_Pos.x += pad->m_Pos0.x;
        pad = pad->Next();
        pad->m_Pos0.x = oX + gap_size + pad->m_Size.x;
        pad->m_Pos.x += pad->m_Pos0.x;
        break;

    case 1:     //Stub :
        pad->SetPadName( wxT( "1" ) );
        pad = pad->Next();
        pad->m_Pos0.y = -( gap_size + pad->m_Size.y ) / 2;
        pad->m_Size.y = gap_size;
        pad->m_Pos.y += pad->m_Pos0.y;
        break;

    case 2:     // Arc Stub created by a polygonal approach:
    {
        EDGE_MODULE* edge = new EDGE_MODULE( Module );
        Module->m_Drawings.PushFront( edge );

        edge->m_Shape = S_POLYGON;
        edge->SetLayer( LAYER_CMP_N );

        int numPoints = angle / 50 + 3;     // Note: angles are in 0.1 degrees
        edge->m_PolyPoints.reserve( numPoints );

        edge->m_Start0.y = -pad->m_Size.y / 2;

        edge->m_PolyPoints.push_back( wxPoint( 0, 0 ) );

        int theta = -angle / 2;
        for( int ii = 1; ii<numPoints - 1; ii++ )
        {
            wxPoint pt( 0, -gap_size );

            RotatePoint( &pt.x, &pt.y, theta );

            edge->m_PolyPoints.push_back( pt );

            theta += 50;
            if( theta > angle / 2 )
                theta = angle / 2;
        }

        // Close the polygon:
        edge->m_PolyPoints.push_back( edge->m_PolyPoints[0] );
    }
    break;

    default:
        break;
    }

    Module->Set_Rectangle_Encadrement();
    GetBoard()->m_Status_Pcb = 0;
    GetScreen()->SetModify();
    return Module;
}


/**************** Polygon Shapes ***********************/

enum id_mw_cmd {
    ID_READ_SHAPE_FILE = 1000
};


/* Setting polynomial form parameters
 */
class WinEDA_SetParamShapeFrame : public wxDialog
{
private:
    WinEDA_PcbFrame* m_Parent;
    wxRadioBox*      m_ShapeOptionCtrl;
    WinEDA_SizeCtrl* m_SizeCtrl;

public: WinEDA_SetParamShapeFrame( WinEDA_PcbFrame* parent, const wxPoint& pos );
    ~WinEDA_SetParamShapeFrame() { };

private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void ReadDataShapeDescr( wxCommandEvent& event );
    void AcceptOptions( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE( WinEDA_SetParamShapeFrame, wxDialog )
    EVT_BUTTON( wxID_OK, WinEDA_SetParamShapeFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_SetParamShapeFrame::OnCancelClick )
    EVT_BUTTON( ID_READ_SHAPE_FILE,
                WinEDA_SetParamShapeFrame::ReadDataShapeDescr )
END_EVENT_TABLE()

WinEDA_SetParamShapeFrame::WinEDA_SetParamShapeFrame( WinEDA_PcbFrame* parent,
                                                      const wxPoint&   framepos ) :
    wxDialog( parent, -1, _( "Complex shape" ), framepos, wxSize( 350, 280 ),
              DIALOG_STYLE )
{
    m_Parent = parent;

    if( PolyEdges )
        free( PolyEdges );
    PolyEdges = NULL;
    PolyEdgesCount = 0;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    wxButton* Button = new wxButton( this, wxID_OK, _( "OK" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button =
        new wxButton( this, ID_READ_SHAPE_FILE,
                      _( "Read Shape Description File..." ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    wxString shapelist[3] = { _( "Normal" ), _( "Symmetrical" ),
                              _( "Mirrored" ) };
    m_ShapeOptionCtrl = new wxRadioBox( this, -1, _( "Shape Option" ),
                                        wxDefaultPosition, wxDefaultSize, 3,
                                        shapelist, 1,
                                        wxRA_SPECIFY_COLS );
    LeftBoxSizer->Add( m_ShapeOptionCtrl, 0, wxGROW | wxALL, 5 );

    m_SizeCtrl = new WinEDA_SizeCtrl( this, _( "Size" ), ShapeSize,
                                      g_UnitMetric, LeftBoxSizer,
                                      PCB_INTERNAL_UNIT );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void WinEDA_SetParamShapeFrame::OnCancelClick( wxCommandEvent& WXUNUSED( event ) )
{
    if( PolyEdges )
        free( PolyEdges );
    PolyEdges = NULL;
    PolyEdgesCount = 0;
    EndModal( -1 );
}


void WinEDA_SetParamShapeFrame::OnOkClick( wxCommandEvent& event )
{
    ShapeSize     = m_SizeCtrl->GetValue();
    PolyShapeType = m_ShapeOptionCtrl->GetSelection();
    EndModal( 1 );
}


/* Read a description shape file
 *  File format is
 *  Unit=MM
 *  XScale=271.501
 *  YScale=1.00133
 *
 *  $COORD
 *  0                      0.6112600148417837
 *  0.001851851851851852   0.6104800531118608
 *  ....
 *  $ENDCOORD
 *
 *  Each line is the X Y coord (normalized units from 0 to 1)
 */
void WinEDA_SetParamShapeFrame::ReadDataShapeDescr( wxCommandEvent& event )
{
    wxString FullFileName;
    wxString ext, mask;
    FILE*    File;
    char     Line[1024];
    double   unitconv = 10000;
    char*    param1, * param2;
    int      bufsize;
    double*  ptbuf;

    ext  = wxT( ".txt" );
    mask = wxT( "*" ) + ext;
    FullFileName = EDA_FileSelector( _( "Read descr shape file" ),
                                     wxEmptyString,
                                     FullFileName,
                                     ext,
                                     mask,
                                     this,
                                     wxFD_OPEN,
                                     TRUE );
    if( FullFileName.IsEmpty() )
        return;

    File = wxFopen( FullFileName, wxT( "rt" ) );

    if( File == NULL )
    {
        DisplayError( this, _( "File not found" ) );
        return;
    }


    bufsize = 100;
    ptbuf   = PolyEdges = (double*) MyZMalloc( bufsize * 2 * sizeof(double) );

    SetLocaleTo_C_standard();
    int LineNum = 0;
    while( GetLine( File, Line, &LineNum, sizeof(Line) - 1 ) != NULL )
    {
        param1 = strtok( Line, " =\n\r" );
        param2 = strtok( NULL, " \t\n\r" );

        if( strnicmp( param1, "Unit", 4 ) == 0 )
        {
            if( strnicmp( param2, "inch", 4 ) == 0 )
                unitconv = 10000;
            if( strnicmp( param2, "mm", 2 ) == 0 )
                unitconv = 10000 / 25.4;
        }
        if( strnicmp( param1, "$ENDCOORD", 8 ) == 0 )
            break;
        if( strnicmp( param1, "$COORD", 6 ) == 0 )
        {
            while( GetLine( File, Line, &LineNum, sizeof(Line) - 1 ) != NULL )
            {
                param1 = strtok( Line, " \t\n\r" );
                param2 = strtok( NULL, " \t\n\r" );
                if( strnicmp( param1, "$ENDCOORD", 8 ) == 0 )
                    break;
                if( bufsize <= PolyEdgesCount )
                {
                    int index = ptbuf - PolyEdges;
                    bufsize *= 2;
                    ptbuf    = PolyEdges = (double*) realloc(
                                    PolyEdges, bufsize * 2 *
                                   sizeof(double) );
                    ptbuf += index;
                }
                *ptbuf = atof( param1 );
                ptbuf++;
                *ptbuf = atof( param2 );
                ptbuf++;
                PolyEdgesCount++;
            }
        }
        if( strnicmp( Line, "XScale", 6 ) == 0 )
        {
            ShapeScaleX = atof( param2 );
        }
        if( strnicmp( Line, "YScale", 6 ) == 0 )
        {
            ShapeScaleY = atof( param2 );
        }
    }

    if( PolyEdgesCount == 0 )
    {
        free( PolyEdges );
        PolyEdges = NULL;
    }
    fclose( File );
    SetLocaleTo_Default();       // revert to the current locale

    ShapeScaleX *= unitconv;
    ShapeScaleY *= unitconv;

    m_SizeCtrl->SetValue( (int) ShapeScaleX, (int) ShapeScaleY );
}


MODULE* WinEDA_PcbFrame::Create_MuWavePolygonShape()
{
    D_PAD*       pad1, * pad2;
    MODULE*      Module;
    wxString     cmp_name;
    int          pad_count = 2;
    EDGE_MODULE* edge;
    int          ii, npoints;

    WinEDA_SetParamShapeFrame* frame = new WinEDA_SetParamShapeFrame(
         this, wxPoint( -1, -1 ) );

    int ok = frame->ShowModal();

    frame->Destroy();

    DrawPanel->MouseToCursorSchema();

    if( ok != 1 )
    {
        if( PolyEdges )
            free( PolyEdges );
        PolyEdges = NULL;
        PolyEdgesCount = 0;
        return NULL;
    }

    if( PolyShapeType == 2 )  // mirrored
        ShapeScaleY = -ShapeScaleY;

    ShapeSize.x = wxRound( ShapeScaleX );
    ShapeSize.y = wxRound( ShapeScaleY );

    if( ( ShapeSize.x ) == 0 || ( ShapeSize.y == 0 ) )
    {
        DisplayError( this, _( "Shape has a null size!" ) );
        return NULL;
    }
    if( PolyEdgesCount == 0 )
    {
        DisplayError( this, _( "Shape has no points!" ) );
        return NULL;
    }

    cmp_name = wxT( "POLY" );

    Module = Create_MuWaveBasicShape( cmp_name, pad_count );
    pad1   = Module->m_Pads;

    pad1->m_Pos0.x = -ShapeSize.x / 2;
    pad1->m_Pos.x += pad1->m_Pos0.x;

    pad2 = (D_PAD*) pad1->Next();
    pad2->m_Pos0.x = pad1->m_Pos0.x + ShapeSize.x;
    pad2->m_Pos.x += pad2->m_Pos0.x;

    edge = new EDGE_MODULE( Module );

    Module->m_Drawings.PushFront( edge );

    edge->m_Shape = S_POLYGON;
    edge->SetLayer( LAYER_CMP_N );
    npoints = PolyEdgesCount;

    edge->m_PolyPoints.reserve( 2 * PolyEdgesCount + 2 );

    // Init start point coord:
    edge->m_PolyPoints.push_back( wxPoint( pad1->m_Pos0.x, 0 ) );

    double* dptr = PolyEdges;
    wxPoint first_coordinate, last_coordinate;
    for( ii = 0; ii < npoints; ii++ )  // Copy points
    {
        last_coordinate.x = wxRound( *dptr++ *ShapeScaleX ) + pad1->m_Pos0.x;
        last_coordinate.y = -wxRound( *dptr++ *ShapeScaleY );
        edge->m_PolyPoints.push_back( last_coordinate );
    }

    first_coordinate.y = edge->m_PolyPoints[1].y;

    switch( PolyShapeType )
    {
    case 0:     // Single
    case 2:     // Single mirrored
        // Init end point coord:
        pad2->m_Pos0.x = last_coordinate.x;
        edge->m_PolyPoints.push_back( wxPoint( last_coordinate.x, 0 ) );

        pad1->m_Size.x = pad1->m_Size.y = ABS( first_coordinate.y );
        pad2->m_Size.x = pad2->m_Size.y = ABS( last_coordinate.y );
        pad1->m_Pos0.y = first_coordinate.y / 2;
        pad2->m_Pos0.y = last_coordinate.y / 2;
        pad1->m_Pos.y  = pad1->m_Pos0.y + Module->m_Pos.y;
        pad2->m_Pos.y  = pad2->m_Pos0.y + Module->m_Pos.y;
        break;

    case 1:     // Symmetric
        for( int ndx = edge->m_PolyPoints.size() - 1; ndx>=0; --ndx )
        {
            wxPoint pt = edge->m_PolyPoints[ndx];

            pt.y = -pt.y;   // mirror about X axis

            edge->m_PolyPoints.push_back( pt );
        }

        pad1->m_Size.x = pad1->m_Size.y = 2 * ABS( first_coordinate.y );
        pad2->m_Size.x = pad2->m_Size.y = 2 * ABS( last_coordinate.y );
        break;
    }

    free( PolyEdges );
    PolyEdgesCount = 0;
    PolyEdges = NULL;

    Module->Set_Rectangle_Encadrement();
    GetBoard()->m_Status_Pcb = 0;
    GetScreen()->SetModify();
    return Module;
}


/*
 * Edit the GAP module, if it has changed the position and/or size
 * Pads that form the gap to get a new value of the gap.
 */
void WinEDA_PcbFrame::Edit_Gap( wxDC* DC, MODULE* Module )
{
    int      gap_size, oX;
    float    fcoeff;
    D_PAD*   pad, * next_pad;
    wxString msg;

    if( Module == NULL )
        return;

    /* Test if module is a gap type (name begins with GAP, and has 2 pads). */
    msg = Module->m_Reference->m_Text.Left( 3 );
    if( msg != wxT( "GAP" ) )
        return;

    pad = Module->m_Pads;
    if( pad == NULL )
    {
        DisplayError( this, _( "No pad for this module" ) );
        return;
    }
    next_pad = (D_PAD*) pad->Next();
    if( next_pad == NULL )
    {
        DisplayError( this, _( "Only one pad for this module" ) );
        return;
    }

    Module->Draw( DrawPanel, DC, GR_XOR );

    /* Calculate the current dimension. */
    gap_size = next_pad->m_Pos0.x - pad->m_Pos0.x - pad->m_Size.x;

    /* Entrance to the desired length of the gap. */
    if( g_UnitMetric )
    {
        fcoeff = 10000.0f / 25.4f;
        msg.Printf( wxT( "%2.3f" ), gap_size / fcoeff );
        Get_Message( _( "Gap (mm):" ), _( "Create Microwave Gap" ), msg, this );
    }
    else
    {
        fcoeff = 10000.0;
        msg.Printf( wxT( "%2.4f" ), gap_size / fcoeff );
        Get_Message( _( "Gap (inch):" ), _( "Create Microwave Gap" ), msg,
                     this );
    }

    if( !msg.IsEmpty() )
    {
        double fval;
        if( msg.ToDouble( &fval ) )
            gap_size = (int) ( fval * fcoeff );
    }

    /* Updating sizes of pads forming the gap. */
    pad->m_Size.x = pad->m_Size.y = GetBoard()->GetCurrentTrackWidth();
    pad->m_Pos0.y = 0;
    oX = pad->m_Pos0.x = -( (gap_size + pad->m_Size.x) / 2 );
    pad->m_Pos.x = pad->m_Pos0.x + Module->m_Pos.x;
    pad->m_Pos.y = pad->m_Pos0.y + Module->m_Pos.y;
    RotatePoint( &pad->m_Pos.x, &pad->m_Pos.y,
                 Module->m_Pos.x, Module->m_Pos.y, Module->m_Orient );

    next_pad->m_Size.x = next_pad->m_Size.y = GetBoard()->GetCurrentTrackWidth();
    next_pad->m_Pos0.y = 0;
    next_pad->m_Pos0.x = oX + gap_size + next_pad->m_Size.x;
    next_pad->m_Pos.x  = next_pad->m_Pos0.x + Module->m_Pos.x;
    next_pad->m_Pos.y  = next_pad->m_Pos0.y + Module->m_Pos.y;
    RotatePoint( &next_pad->m_Pos.x, &next_pad->m_Pos.y,
                 Module->m_Pos.x, Module->m_Pos.y, Module->m_Orient );

    Module->Draw( DrawPanel, DC, GR_OR );
}
