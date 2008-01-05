/****************************************************/
/* Gestion des composants specifiques aux microndes */
/****************************************************/

/* Fichier MUONDE.CPP */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "trigo.h"
#include "pcbnew.h"
#include "autorout.h"

#include "drag.h"

#include <string.h>
#include "protos.h"

/* fonctions importees */

/* Fonctions locales */

//static void Exit_Muonde(WinEDA_DrawFrame * frame, wxDC *DC);

/* Variables locales : */
#define COEFF_COUNT 6
double* PolyEdges;
int     PolyEdgesCount;
double  ShapeScaleX, ShapeScaleY;
wxSize  ShapeSize;
int     PolyShapeType;

#include "gen_self.h"

/***************************************************************************/
MODULE* WinEDA_PcbFrame::Create_MuWaveBasicShape( wxDC* DC,
                                                  const wxString& name, int pad_count )
/***************************************************************************/

/* Create a footprint with pad_count pads for micro wave applications
 *  This footprint has pad_count pads:
 *  PAD_SMD, rectangular, H size = V size = current track width.
 */
{
    MODULE*  Module;
    int      pad_num = 1;
    wxString Line;

    Module = Create_1_Module( DC, name );
    if( Module == NULL )
        return NULL;

    Module->m_TimeStamp           = GetTimeStamp();
    Module->m_Value->m_Size       = wxSize( 30, 30 );
    Module->m_Value->m_Pos0.y     = -30;
    Module->m_Value->m_Pos.y     += Module->m_Value->m_Pos0.y;
    Module->m_Reference->m_Size   = wxSize( 30, 30 );
    Module->m_Reference->m_Pos0.y = 30;
    Module->m_Reference->m_Pos.y += Module->m_Reference->m_Pos0.y;

    /* Creation des pastilles formant le gap */
    while( pad_count-- )
    {
        D_PAD* pad;
        pad = new D_PAD( Module );
        pad->Pback = Module;
        if( Module->m_Pads == NULL )
        {
            Module->m_Pads = pad;
        }
        else
        {
            Module->m_Pads->Pback = pad;
            pad->Pnext     = Module->m_Pads;
            Module->m_Pads = pad;
        }
        pad->m_Size.x       = pad->m_Size.y = g_DesignSettings.m_CurrentTrackWidth;
        pad->m_Pos          = Module->m_Pos;
        pad->m_PadShape     = PAD_RECT;
        pad->m_Attribut     = PAD_SMD;
        pad->m_Masque_Layer = CMP_LAYER;
        Line.Printf( wxT( "%d" ), pad_num );
        pad->SetPadName( Line );
        pad_num++;
    }

    if( DC )
        Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    return Module;
}


#if 0
/**********************************************************/
static void Exit_Muonde( WinEDA_DrawFrame* frame, wxDC* DC )
/**********************************************************/
{
    MODULE* Module = (MODULE*) frame->m_CurrentScreen->GetCurItem();

    if( Module )
    {
        if( Module->m_Flags & IS_NEW )
        {
            Module->Draw( frame->DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            Module ->DeleteStructure();
        }
        else
        {
            Module->Draw( frame->DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
        }
    }

    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;
    frame->SetCurItem( NULL );
}


#endif


/***************************************************************************/
MODULE* WinEDA_PcbFrame::Create_MuWaveComponent( wxDC* DC, int shape_type )
/***************************************************************************/

/* Create a module "GAP" or "STUB"
 *  This a "gap" or  "stub" used in micro wave designs
 *  This modue has 2 pads:
 *  PAD_SMD, rectangular, H size = V size = current track width.
 *  the "gap" is isolation created between this 2 pads
 */
{
    int      gap_size, oX, ii;
    float    fcoeff;
    D_PAD*   pt_pad;
    MODULE*  Module;
    wxString msg, cmp_name;
    int      pad_count = 2;
    int      angle = 0;
    bool     abort;

    /* Entree de la longueur desiree du gap*/
    gap_size = g_DesignSettings.m_CurrentTrackWidth;        // Valeur raisonnable

    switch( shape_type )
    {
    case 0:
        msg      = _( "Gap" );
        cmp_name = wxT( "GAP" );
        break;

    case 1:
        msg       = _( "Stub" );
        cmp_name  = wxT( "STUB" );
        pad_count = 2;
        break;

    case 2:
        msg       = _( "Arc Stub" );
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
        fcoeff = 10000.0 / 25.4;
        value.Printf( wxT( "%2.4f" ), gap_size / fcoeff );
        msg  += _( " (mm):" );
        abort = Get_Message( msg, value, this );
    }
    else
    {
        fcoeff = 10000.0;
        value.Printf( wxT( "%2.3f" ), gap_size / fcoeff );
        msg  += _( " (inch):" );
        abort = Get_Message( msg, value, this );
    }

    double   fval;
    if( !value.ToDouble( &fval ) )
    {
        DisplayError( this, _( "Incorrect number, abort" ) );
        abort = TRUE;
    }
    gap_size = ABS( (int) round( fval * fcoeff ) );

    if( !abort && (shape_type == 2) )
    {
        fcoeff = 10.0;
        value.Printf( wxT( "%3.1f" ), angle / fcoeff );
        msg   = _( "Angle (0.1deg):" );
        abort = Get_Message( msg, value, this );
        if( !value.ToDouble( &fval ) )
        {
            DisplayError( this, _( "Incorrect number, abort" ) );
            abort = TRUE;
        }
        angle = ABS( (int) round( fval * fcoeff ) );
        if( angle > 1800 )
            angle = 1800;
    }

    if( abort )
    {
        DrawPanel->MouseToCursorSchema();
        return NULL;
    }

    Module = Create_MuWaveBasicShape( NULL, cmp_name, pad_count );
    pt_pad = Module->m_Pads;

    switch( shape_type )
    {
    case 0:     //Gap :
        oX = pt_pad->m_Pos0.x = -(gap_size + pt_pad->m_Size.x) / 2;
        pt_pad->m_Pos.x += pt_pad->m_Pos0.x;

        pt_pad = (D_PAD*) pt_pad->Pnext;
        pt_pad->m_Pos0.x = oX + gap_size + pt_pad->m_Size.x;
        pt_pad->m_Pos.x += pt_pad->m_Pos0.x;
        break;

    case 1:     //Stub :
        pt_pad->SetPadName( wxT( "1" ) );
        pt_pad = (D_PAD*) pt_pad->Pnext;
        pt_pad->m_Pos0.y = -(gap_size + pt_pad->m_Size.y) / 2;
        pt_pad->m_Size.y = gap_size;
        pt_pad->m_Pos.y += pt_pad->m_Pos0.y;
        break;

    case 2:     //Arc Stub :
    {
        EDGE_MODULE* edge; int* ptr, theta;
        ii   = angle / 50;
        edge = new EDGE_MODULE( Module );
        Module->m_Drawings = edge;
        edge->Pback       = Module;
        edge->m_Shape     = S_POLYGON;
        edge->SetLayer( LAYER_CMP_N );
        edge->m_PolyCount = ii + 3;
        edge->m_PolyList  = (int*) MyMalloc( edge->m_PolyCount * 2 * sizeof(int) );
        ptr = edge->m_PolyList;
        edge->m_Start0.y = -pt_pad->m_Size.y / 2;

        *ptr  = 0; ptr++;
        *ptr  = 0; ptr++;
        theta = -angle / 2;
        for( ii = 1; ii < edge->m_PolyCount - 1; ii++ )
        {
            int x, y;
            x = 0; y = -gap_size;
            RotatePoint( &x, &y, theta );
            *ptr   = x; ptr++; *ptr = y; ptr++;
            theta += 50;
            if( theta > angle / 2 )
                theta = angle / 2;
        }

        *ptr = edge->m_PolyList[0]; ptr++;
        *ptr = edge->m_PolyList[1];
        break;
    }

    default:
        break;
    }

    Module->Set_Rectangle_Encadrement();
    Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    DrawPanel->MouseToCursorSchema();
    m_Pcb->m_Status_Pcb = 0;
    m_CurrentScreen->SetModify();
    return Module;
}


/**************** Polygon Shapes ***********************/

enum id_mw_cmd {
    ID_READ_SHAPE_FILE = 1000
};

/*************************************************/
class WinEDA_SetParamShapeFrame : public wxDialog
/*************************************************/

/* Reglages des parametres des forme polynomiales
 */
{
private:
    WinEDA_PcbFrame* m_Parent;
    wxRadioBox*      m_ShapeOptionCtrl;
    WinEDA_SizeCtrl* m_SizeCtrl;

public:

    // Constructor and destructor
    WinEDA_SetParamShapeFrame( WinEDA_PcbFrame* parent, const wxPoint& pos );
    ~WinEDA_SetParamShapeFrame() { };

private:
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );
    void    ReadDataShapeDescr( wxCommandEvent& event );
    void    AcceptOptions( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
/* Construction de la table des evenements pour WinEDA_SetParamShapeFrame */
BEGIN_EVENT_TABLE( WinEDA_SetParamShapeFrame, wxDialog )
EVT_BUTTON( wxID_OK, WinEDA_SetParamShapeFrame::OnOkClick )
EVT_BUTTON( wxID_CANCEL, WinEDA_SetParamShapeFrame::OnCancelClick )
EVT_BUTTON( ID_READ_SHAPE_FILE, WinEDA_SetParamShapeFrame::ReadDataShapeDescr )
END_EVENT_TABLE()


/*************************************************/
/* Constructeur de WinEDA_SetParamShapeFrame */
/************************************************/

WinEDA_SetParamShapeFrame::WinEDA_SetParamShapeFrame( WinEDA_PcbFrame* parent,
                                                      const wxPoint&   framepos ) :
    wxDialog( parent, -1, _( "Complex shape" ), framepos, wxSize( 350, 280 ),
              DIALOG_STYLE )
{
    m_Parent = parent;
    SetFont( *g_DialogFont );

    if( PolyEdges )
        free( PolyEdges );
    PolyEdges      = NULL;
    PolyEdgesCount = 0;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    wxButton* Button = new wxButton( this, wxID_OK, _( "OK" ) );
    Button->SetForegroundColour( *wxRED );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    Button->SetForegroundColour( *wxBLUE );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, ID_READ_SHAPE_FILE, _( "Read Shape Descr File..." ) );
    Button->SetForegroundColour( wxColor( 0, 100, 0 ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    wxString shapelist[3] = { _( "Normal" ), _( "Symmetrical" ), _( "Mirrored" ) };
    m_ShapeOptionCtrl = new wxRadioBox( this, -1, _(
                                            "Shape Option" ),
                                        wxDefaultPosition, wxDefaultSize, 3, shapelist, 1,
                                        wxRA_SPECIFY_COLS );
    LeftBoxSizer->Add( m_ShapeOptionCtrl, 0, wxGROW | wxALL, 5 );

    m_SizeCtrl = new WinEDA_SizeCtrl( this, _( "Size" ),
                                      ShapeSize,
                                      g_UnitMetric, LeftBoxSizer, PCB_INTERNAL_UNIT );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/**********************************************************************/
void WinEDA_SetParamShapeFrame::OnCancelClick( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    if( PolyEdges )
        free( PolyEdges );
    PolyEdges      = NULL;
    PolyEdgesCount = 0;
    EndModal( -1 );
}


/*******************************************************************/
void WinEDA_SetParamShapeFrame::OnOkClick( wxCommandEvent& event )
/*******************************************************************/
{
    ShapeSize     = m_SizeCtrl->GetValue();
    PolyShapeType = m_ShapeOptionCtrl->GetSelection();
    EndModal( 1 );
}


/************************************************************************/
void WinEDA_SetParamShapeFrame::ReadDataShapeDescr( wxCommandEvent& event )
/************************************************************************/

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
 *  Each line is the X Y coord (normalised units from 0 to 1)
 */
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
                                     wxEmptyString, /* Chemin par defaut */
                                     FullFileName,  /* nom fichier par defaut */
                                     ext,           /* extension par defaut */
                                     mask,          /* Masque d'affichage */
                                     this,
                                     wxFD_OPEN,
                                     TRUE /* ne change pas de repertoire courant */
                   );
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

    setlocale( LC_NUMERIC, "C" );
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
                    ptbuf    = PolyEdges = (double*) realloc( PolyEdges, bufsize * 2 *
                                                             sizeof(double) );
                    ptbuf   += index;
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
    setlocale( LC_NUMERIC, "" );      // revert to the current  locale

    ShapeScaleX *= unitconv;
    ShapeScaleY *= unitconv;

    m_SizeCtrl->SetValue( (int) ShapeScaleX, (int) ShapeScaleY );
}


/*************************************************************/
MODULE* WinEDA_PcbFrame::Create_MuWavePolygonShape( wxDC* DC )
/*************************************************************/
{
    D_PAD*       pad1, * pad2;
    MODULE*      Module;
    wxString     cmp_name;
    int          pad_count = 2;
    EDGE_MODULE* edge; int* ptr;
    int          ii, npoints;

    WinEDA_SetParamShapeFrame* frame = new WinEDA_SetParamShapeFrame( this, wxPoint( -1, -1 ) );
    int          ok = frame->ShowModal(); frame->Destroy();

    DrawPanel->MouseToCursorSchema();

    if( ok != 1 )
    {
        if( PolyEdges )
            free( PolyEdges );
        PolyEdges      = NULL;
        PolyEdgesCount = 0;
        return NULL;
    }

    if( PolyShapeType == 2 )  // mirrored
        ShapeScaleY = -ShapeScaleY;
    ShapeSize.x = (int) round( ShapeScaleX );
    ShapeSize.y = (int) round( ShapeScaleY );

    if( (ShapeSize.x) == 0 || (ShapeSize.y == 0) )
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

    Module = Create_MuWaveBasicShape( NULL, cmp_name, pad_count );
    pad1   = Module->m_Pads;

    pad1->m_Pos0.x = -ShapeSize.x / 2;
    pad1->m_Pos.x += pad1->m_Pos0.x;

    pad2 = (D_PAD*) pad1->Pnext;
    pad2->m_Pos0.x = pad1->m_Pos0.x + ShapeSize.x;
    pad2->m_Pos.x += pad2->m_Pos0.x;

    edge = new EDGE_MODULE( Module );
    Module->m_Drawings = edge;
    edge->Pback   = Module;
    edge->m_Shape = S_POLYGON;
    edge->SetLayer( LAYER_CMP_N );
    npoints = PolyEdgesCount;

    switch( PolyShapeType )
    {
    case 0:     // Single
    case 2:     // Single, mirrored
        edge->m_PolyCount = PolyEdgesCount + 2;
        break;

    case 1:     // Symetric
        edge->m_PolyCount = (2 * PolyEdgesCount) + 2;
        break;
    }

    edge->m_PolyList = (int*) MyMalloc( edge->m_PolyCount * 2 * sizeof(int) );

    ptr = edge->m_PolyList;

    // Init start point coord:
    *ptr = pad1->m_Pos0.x; ptr++;
    *ptr = 0; ptr++;

    double* dptr = PolyEdges;
    wxPoint first_cordinate, last_cordinate;
    for( ii = 0; ii < npoints; ii++ )  // Copy points
    {
        last_cordinate.x = *ptr = (int) round( *dptr * ShapeScaleX ) + pad1->m_Pos0.x;
        dptr++; ptr++;
        last_cordinate.y = *ptr = -(int) round( *dptr * ShapeScaleY );
        dptr++; ptr++;
    }

    first_cordinate.y = edge->m_PolyList[3];

    switch( PolyShapeType )
    {
        int* ptr1;

    case 0:     // Single
    case 2:     // Single mirrored
        // Init end point coord:
        *ptr = pad2->m_Pos0.x = last_cordinate.x; ptr++;
        *ptr = 0;
        pad1->m_Size.x = pad1->m_Size.y = ABS( first_cordinate.y );
        pad2->m_Size.x = pad2->m_Size.y = ABS( last_cordinate.y );
        pad1->m_Pos0.y = first_cordinate.y / 2;
        pad2->m_Pos0.y = last_cordinate.y / 2;
        pad1->m_Pos.y  = pad1->m_Pos0.y + Module->m_Pos.y;
        pad2->m_Pos.y  = pad2->m_Pos0.y + Module->m_Pos.y;
        break;

    case 1:     // Symetric
        ptr1 = ptr - 2;
        for( ii = 0; ii <= npoints; ii++ )
        {
            *ptr = *ptr1;           // Copy X coord
            ptr++;
            *ptr  = -*(ptr1 + 1);   // Copy Y coord, mirror X axis
            ptr1 -= 2; ptr++;
        }

        pad1->m_Size.x = pad1->m_Size.y = 2*    ABS( first_cordinate.y );

        pad2->m_Size.x = pad2->m_Size.y = 2*    ABS( last_cordinate.y );

        break;
    }

    free( PolyEdges );
    PolyEdgesCount = 0;
    PolyEdges = NULL;

    Module->Set_Rectangle_Encadrement();
    Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    m_Pcb->m_Status_Pcb = 0;
    m_CurrentScreen->SetModify();
    return Module;
}


/***************************************************************/
void WinEDA_PcbFrame::Edit_Gap( wxDC* DC, MODULE* Module )
/***************************************************************/

/*
 *  Edit le module GAP, c'est a dire modifie la position et la taille
 *  des pastilles formant le gap pour obtenir une nouvelle valeur du gap
 */
{
    int      gap_size, oX;
    float    fcoeff;
    D_PAD*   pad, * next_pad;
    wxString msg;

    if( Module == NULL )
        return;                     /* Module non trouve */

    /* Test si module = gap ( nom commence par GAP, et 2 pastilles) */
    msg = Module->m_Reference->m_Text.Left( 3 );
    if( msg != wxT( "GAP" ) )
        return;

    pad = Module->m_Pads;
    if( pad == NULL )
    {
        DisplayError( this, _( "No pad for this module" ) ); return;
    }
    next_pad = (D_PAD*) pad->Pnext;
    if( next_pad == NULL )
    {
        DisplayError( this, _( "Only one pad for this module" ) ); return;
    }

    /* Effacement du module: */
    Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );

    /* Calcul de la dimension actuelle */
    gap_size = next_pad->m_Pos0.x - pad->m_Pos0.x - pad->m_Size.x;

    /* Entree de la longueur desiree du gap*/
    if( g_UnitMetric )
    {
        fcoeff = 10000.0 / 25.4;
        msg.Printf( wxT( "%2.3f" ), gap_size / fcoeff );
        Get_Message( _( "Gap (mm):" ), msg, this );
    }
    else
    {
        fcoeff = 10000.0;
        msg.Printf( wxT( "%2.4f" ), gap_size / fcoeff );
        Get_Message( _( "Gap (inch):" ), msg, this );
    }

    if( !msg.IsEmpty() )
    {
        double fval;
        if( msg.ToDouble( &fval ) )
            gap_size = (int) ( fval * fcoeff );
    }

    /* Mise a jour des tailles des pastilles formant le gap */
    pad->m_Size.x = pad->m_Size.y = g_DesignSettings.m_CurrentTrackWidth;
    pad->m_Pos0.y = 0;
    oX = pad->m_Pos0.x = -( (gap_size + pad->m_Size.x) / 2 );
    pad->m_Pos.x = pad->m_Pos0.x + Module->m_Pos.x;
    pad->m_Pos.y = pad->m_Pos0.y + Module->m_Pos.y;
    RotatePoint( &(pad->m_Pos.x), &(pad->m_Pos.y),
                 Module->m_Pos.x, Module->m_Pos.y, Module->m_Orient );

    next_pad->m_Size.x = next_pad->m_Size.y = g_DesignSettings.m_CurrentTrackWidth;
    next_pad->m_Pos0.y = 0;
    next_pad->m_Pos0.x = oX + gap_size + next_pad->m_Size.x;
    next_pad->m_Pos.x  = next_pad->m_Pos0.x + Module->m_Pos.x;
    next_pad->m_Pos.y  = next_pad->m_Pos0.y + Module->m_Pos.y;
    RotatePoint( &(next_pad->m_Pos.x), &(next_pad->m_Pos.y),
                 Module->m_Pos.x, Module->m_Pos.y, Module->m_Orient );

    Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
}
