/***************************/
/**** Read GERBER files ****/
/***************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "macros.h"

#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"

#define DEFAULT_SIZE 100


/* Format Gerber: NOTES:
 * Features history:
 *   Gn =
 *   G01 linear interpolation (right trace)
 *   G02, G20, G21 Circular interpolation, meaning trig < 0
 *   G03, G30, G31 Circular interpolation, meaning trig > 0
 *   G04 review
 *   G06 parabolic interpolation
 *   G07 Cubic Interpolation
 *   G10 linear interpolation (scale x10)
 *   G11 linear interpolation (0.1x range)
 *   G12 linear interpolation (0.01x scale)
 *   G52 plot symbol reference code by Dnn
 *   G53 plot symbol reference by Dnn; symbol rotates from -90 degrees
 *   G54 Selection Tool
 *   G55 Fashion photo exhibition
 *   G56 plot symbol reference code for DNN
 *   G57 displays the symbol link to the console
 *   G58 plot displays the symbol and link to the console
 *   G60 linear interpolation (scale x100)
 *   G70 Units = Inches
 *   G71 Units = Millimeters
 *   G74 circular interpolation removes 360 degree, has returned G01
 *   G75 circular interpolation Active 360 degree
 *   G90 mode absolute coordinates
 *   G91 Fashion Related Contacts
 *
 * Coordinates X, Y
 *   X and Y are followed by + or - and m + n digits (not separated)
 *           m = integer part
 *           n = part after the comma
 *            conventional formats: m = 2, n = 3 (size 2.3)
 *                                  m = 3, n = 4 (size 3.4)
 *   eg
 *   G__ X00345Y-06123 * D__
 *
 * Tools and D_CODES
 *   tool number (identification of shapes)
 *   1 to 99 (Classical)
 *   1 to 999
 *
 * D_CODES:
 *   D01 ... D9 = action codes:
 *   D01 = activating light (lower pen) when di ¿½ placement
 *   D02 = light extinction (lift pen) when di ¿½ placement
 *   D03 Flash
 *   D09 = VAPE Flash
 *   D10 ... = Indentification Tool (Opening)
 */


GERBER::GERBER( int aLayer )
{
    m_Layer = aLayer;            // Layer Number

    m_Selected_Tool = FIRST_DCODE;

    ResetDefaultValues();

    for( unsigned ii = 0; ii < DIM( m_Aperture_List ); ii++ )
        m_Aperture_List[ii] = 0;

    m_Pcb = 0;
}


GERBER::~GERBER()
{
    for( unsigned ii = 0; ii < DIM( m_Aperture_List ); ii++ )
    {
        delete m_Aperture_List[ii];

        // m_Aperture_List[ii] = NULL;
    }

    delete m_Pcb;
}


D_CODE* GERBER::GetDCODE( int aDCODE, bool create )
{
    unsigned ndx = aDCODE - FIRST_DCODE;

    if( ndx < (unsigned) DIM( m_Aperture_List ) )
    {
        // lazily create the D_CODE if it does not exist.
        if( create )
        {
            if( m_Aperture_List[ndx] == NULL )
                m_Aperture_List[ndx] = new D_CODE( ndx + FIRST_DCODE );
        }

        return m_Aperture_List[ndx];
    }
    return NULL;
}


APERTURE_MACRO* GERBER::FindApertureMacro( const APERTURE_MACRO& aLookup )
{
    APERTURE_MACRO_SET::iterator iter = m_aperture_macros.find( aLookup );

    if( iter != m_aperture_macros.end() )
    {
        APERTURE_MACRO* pam = (APERTURE_MACRO*) &(*iter);
        return pam;
    }

    return NULL;    // not found
}


void GERBER::ResetDefaultValues()
{
    m_FileName.Empty();
    m_Name = wxT( "no name" );          // Layer name
    m_LayerNegative = FALSE;            // TRUE = Negative Layer
    m_ImageNegative = FALSE;            // TRUE = Negative image
    m_GerbMetric    = FALSE;            // FALSE = Inches, TRUE = metric
    m_Relative = FALSE;                 // FALSE = absolute Coord, RUE =
                                        // relative Coord
    m_NoTrailingZeros = FALSE;          // True: trailing zeros deleted
    m_MirorA    = FALSE;                // True: miror / axe A (X)
    m_MirorB    = FALSE;                // True: miror / axe B (Y)
    m_Has_DCode = FALSE;                // TRUE = DCodes in file (FALSE = no
                                        // DCode->
                                        // separate DCode file

    m_Offset.x = m_Offset.y = 0;        // Coord Offset

    m_FmtScale.x = m_FmtScale.y = g_Default_GERBER_Format % 10;
    m_FmtLen.x   = m_FmtLen.y = m_FmtScale.x + (g_Default_GERBER_Format / 10);

    m_LayerScale.x  = m_LayerScale.y = 1.0;         // scale (X and Y) this
                                                    // layer
    m_Rotation      = 0;
    m_Iterpolation  = GERB_INTERPOL_LINEAR_1X;      // Linear, 90 arc, Circ.
    m_360Arc_enbl   = FALSE;                        // 360 deg circular
                                                    // interpolation disable
    m_Current_Tool  = 0;                            // Current Tool (Dcode)
                                                    // number selected
    m_CommandState  = 0;                            // gives tate of the
                                                    // stacking order analysis
    m_CurrentPos.x  = m_CurrentPos.y = 0;           // current specified coord
                                                    // for plot
    m_PreviousPos.x = m_PreviousPos.y = 0;          // old current specified
                                                    // coord for plot
    m_IJPos.x = m_IJPos.y = 0;                      // current centre coord for
                                                    // plot arcs & circles
    m_Current_File    = NULL;                       // File to read
    m_FilesPtr        = 0;
    m_Transform[0][0] = m_Transform[1][1] = 1;
    m_Transform[0][1] = m_Transform[1][0] = 0;      // Rotation/mirror = Normal
    m_PolygonFillMode = FALSE;
    m_PolygonFillModeState = 0;
}


int GERBER::ReturnUsedDcodeNumber()
{
    int count = 0;

    for( unsigned ii = 0; ii < DIM( m_Aperture_List ); ii++ )
    {
        if( m_Aperture_List[ii] )
            if( m_Aperture_List[ii]->m_InUse || m_Aperture_List[ii]->m_Defined )
                ++count;
    }

    return count;
}


void GERBER::InitToolTable()
{
    for( int count = 0; count < MAX_TOOLS; count++ )
    {
        if( m_Aperture_List[count] == NULL )
            continue;

        m_Aperture_List[count]->m_Num_Dcode = count + FIRST_DCODE;
        m_Aperture_List[count]->Clear_D_CODE_Data();
    }
}


/***************/
/* Class DCODE */
/***************/


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
    m_Shape      = APT_CIRCLE;
    m_Drill.x    = m_Drill.y = 0;
    m_DrillShape = 0;
    m_InUse      = FALSE;
    m_Defined    = FALSE;
    m_Macro      = 0;
}


const wxChar* D_CODE::ShowApertureType( APERTURE_T aType )
{
    const wxChar* ret;

    switch( aType )
    {
    case APT_CIRCLE:
        ret = wxT( "Round" );   break;

    case APT_LINE:
        ret = wxT( "Line" );    break;

    case APT_RECT:
        ret = wxT( "Rect" );    break;

    case APT_OVAL:
        ret = wxT( "Oval" );    break;

    case APT_POLYGON:
        ret = wxT( "Poly" );    break;

    case APT_MACRO:
        ret = wxT( "Macro" );   break;

    default:
        ret = wxT( "???" );     break;
    }

    return ret;
}


int WinEDA_GerberFrame::Read_D_Code_File( const wxString& D_Code_FullFileName )
{
    int      current_Dcode, ii, dcode_scale;
    char*    ptcar;
    int      dimH, dimV, drill, dummy;
    float    fdimH, fdimV, fdrill;
    char     c_type_outil[256];
    char     line[GERBER_BUFZ];
    wxString msg;
    D_CODE*  dcode;
    FILE*    dest;
    int      layer = GetScreen()->m_Active_Layer;
    int      type_outil;

    if( g_GERBER_List[layer] == NULL )
    {
        g_GERBER_List[layer] = new GERBER( layer );
    }

    GERBER* gerber = g_GERBER_List[layer];


    /* Updating gerber scale: */
    dcode_scale   = 10; /* By uniting dCode = mil, internal unit = 0.1 mil
                         * -> 1 unite dcode = 10 unit PCB */
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

    gerber->InitToolTable();

    while( fgets( line, sizeof(line) - 1, dest ) != NULL )
    {
        if( *line == ';' )
            continue;

        if( strlen( line ) < 10 )
            continue;                       /* Skip blank line. */

        dcode = NULL;
        current_Dcode = 0;

        /* Determine of the type of file from D_Code. */
        ptcar = line;
        ii    = 0;
        while( *ptcar )
            if( *(ptcar++) == ',' )
                ii++;

        if( ii >= 6 )   /* valeurs en mils */
        {
            sscanf( line, "%d,%d,%d,%d,%d,%d,%d", &ii,
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
        else        /* Values in inches are converted to mils. */
        {
            fdrill = 0;
            current_Dcode = 0;

            sscanf( line, "%f,%f,%1s", &fdimV, &fdimH, c_type_outil );
            ptcar = line;
            while( *ptcar )
            {
                if( *ptcar == 'D' )
                {
                    sscanf( ptcar + 1, "%d,%f", &current_Dcode, &fdrill );
                    break;
                }
                else
                    ptcar++;
            }

            dimH  = (int) ( (fdimH * dcode_scale * 1000) + 0.5 );
            dimV  = (int) ( (fdimV * dcode_scale * 1000) + 0.5 );
            drill = (int) ( (fdrill * dcode_scale * 1000) + 0.5 );

            if( strchr( "CLROP", c_type_outil[0] ) )
                type_outil = (APERTURE_T) c_type_outil[0];
            else
            {
                fclose( dest );
                return -2;
            }
        }

        /* Update the list of d_codes if consistant. */
        if( current_Dcode < FIRST_DCODE )
            continue;

        if( current_Dcode >= MAX_TOOLS )
            continue;

        dcode = gerber->GetDCODE( current_Dcode );
        dcode->m_Size.x  = dimH;
        dcode->m_Size.y  = dimV;
        dcode->m_Shape   = (APERTURE_T) type_outil;
        dcode->m_Drill.x = dcode->m_Drill.y = drill;
        dcode->m_Defined = TRUE;
    }

    fclose( dest );

    return 1;
}


/* Set Size Items (Lines, Flashes) from DCodes List
 */
void WinEDA_GerberFrame::CopyDCodesSizeToItems()
{
    static D_CODE dummy( 999 );   //Used if D_CODE not found in list

    for( TRACK* track = GetBoard()->m_Track; track; track = track->Next() )
    {
        GERBER* gerber = g_GERBER_List[track->GetLayer()];
        wxASSERT( gerber );

        D_CODE* dcode = gerber->GetDCODE( track->GetNet(), false );
        wxASSERT( dcode );
        if( dcode == NULL )
            dcode = &dummy;

        dcode->m_InUse = TRUE;

        if(                                     // Line Item
            (track->m_Shape == S_SEGMENT )      /* rectilinear segment */
            || (track->m_Shape == S_RECT )      /* rect segment form (i.e.
                                                 * non-rounded ends) */
            || (track->m_Shape == S_ARC )       /* segment arc (rounded tips) */
            || (track->m_Shape == S_CIRCLE )    /* segment in a circle (ring) */
            || (track->m_Shape == S_ARC_RECT )  /* segment arc (stretches)
                                                 * (GERBER)*/
            )
        {
            track->m_Width = dcode->m_Size.x;
        }
        else        // Spots ( Flashed Items )
        {
            int    width, len;
            wxSize size = dcode->m_Size;

            width = MIN( size.x, size.y );
            len   = MAX( size.x, size.y ) - width;

            track->m_Width = width;

            track->m_Start.x = (track->m_Start.x + track->m_End.x) / 2;
            track->m_Start.y = (track->m_Start.y + track->m_End.y) / 2;
            track->m_End     = track->m_Start; // m_Start = m_End = Spot center

            switch( dcode->m_Shape )
            {
            case APT_LINE:          // might not appears here, but some broken
                                    // gerber files use it
            case APT_CIRCLE:        /* spot round (for GERBER) */
                track->m_Shape = S_SPOT_CIRCLE;
                break;

            case APT_OVAL:          /* spot oval (for GERBER)*/
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


void WinEDA_GerberFrame::Liste_D_Codes( )
{
    int               ii, jj;
    D_CODE*           pt_D_code;
    wxString          Line;
    WinEDA_TextFrame* List;
    int               scale = 10000;
    int               curr_layer = GetScreen()->m_Active_Layer;

    List = new WinEDA_TextFrame( this, _( "List D codes" ) );

    for( int layer = 0; layer < 32; layer++ )
    {
        GERBER* gerber = g_GERBER_List[layer];
        if( gerber == NULL )
            continue;

        if( gerber->ReturnUsedDcodeNumber() == 0 )
            continue;

        if( layer == curr_layer )
            Line.Printf( wxT( "*** Active layer (%2.2d) ***" ), layer + 1 );
        else
            Line.Printf( wxT( "*** layer %2.2d  ***" ), layer + 1 );
        List->Append( Line );

        for( ii = 0, jj = 1; ii < MAX_TOOLS; ii++ )
        {
            pt_D_code = gerber->GetDCODE( ii + FIRST_DCODE, false );
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
                         D_CODE::ShowApertureType( pt_D_code->m_Shape )
                         );

            if( !pt_D_code->m_Defined )
                Line += wxT( " ?" );

            if( !pt_D_code->m_InUse )
                Line += wxT( " *" );

            List->Append( Line );
            jj++;
        }
    }

    ii = List->ShowModal();
    List->Destroy();
    if( ii < 0 )
        return;
}
