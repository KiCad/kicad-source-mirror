/**
 * @file class_pad.cpp
 * D_PAD class implementation.
 */

#include "fctsys.h"
#include "PolyLine.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "trigo.h"
#include "richio.h"
#include "wxstruct.h"
#include "macros.h"

#include "pcbnew.h"
#include "pcbnew_id.h"                      // ID_TRACK_BUTT

#include "class_board.h"
#include "class_module.h"


int D_PAD::m_PadSketchModePenSize = 0;      // Pen size used to draw pads in sketch mode


D_PAD::D_PAD( MODULE* parent ) : BOARD_CONNECTED_ITEM( parent, PCB_PAD_T )
{
    m_NumPadName = 0;

    m_Size[0] = m_Size[1] = FROM_LEGACY_LU( 500 );          // give it a reasonable size
    /// TODO: remove hardcoded constant
    m_Orient = 0;                       // Pad rotation in 1/10 degrees
    m_LengthDie = ZERO_LENGTH;

    if( m_Parent && (m_Parent->Type()  == PCB_MODULE_T) )
    {
        m_Pos = FROM_LEGACY_LU_VEC( ( (MODULE*) m_Parent )->GetPosition() );
    }

    m_PadShape = PAD_CIRCLE;                        // Shape: PAD_CIRCLE, PAD_RECT PAD_OVAL
                                                    // PAD_TRAPEZOID
    m_Attribut = PAD_STANDARD;                      // Type: NORMAL, PAD_SMD, PAD_CONN
    m_DrillShape     = PAD_CIRCLE;                  // Drill shape = circle
    m_LocalClearance = ZERO_LENGTH;
    m_LocalSolderMaskMargin  = ZERO_LENGTH;
    m_LocalSolderPasteMargin = ZERO_LENGTH;
    m_LocalSolderPasteMarginRatio = 0.0;
    m_layerMask = PAD_STANDARD_DEFAULT_LAYERS;      // set layers mask to
                                                    // default for a standard pad

    SetSubRatsnest( 0 );                            // used in ratsnest calculations
    ComputeShapeMaxRadius();
}


D_PAD::~D_PAD()
{
}


/* Calculate the radius of the circle containing the pad.
 */
int D_PAD::GetMaxRadius() const
{
    int x, y;
    int radius;

    switch( m_PadShape & 0x7F )
    {
    case PAD_CIRCLE:
        radius = TO_LEGACY_LU( m_Size.x() / 2 );
        break;

    case PAD_OVAL:
        radius = TO_LEGACY_LU( max( m_Size.x(), m_Size.y() ) / 2 );
        break;

    case PAD_RECT:
        x = TO_LEGACY_LU( m_Size.x() );
        y = TO_LEGACY_LU( m_Size.y() );
        radius = 1 + (int) ( sqrt( (double) y * y
                                   + (double) x * x ) / 2 );
        break;

    case PAD_TRAPEZOID:
        x = TO_LEGACY_LU( m_Size.x() + abs( m_DeltaSize.y() ) );   // Remember: m_DeltaSize.y is the m_Size.x change
        y = TO_LEGACY_LU( m_Size.y() + abs( m_DeltaSize.x() ) );   // Remember: m_DeltaSize.x is the m_Size.y change
        radius = 1 + (int) ( sqrt( (double) y * y + (double) x * x ) / 2 );
        break;

    default:
        radius = 0;     // quiet compiler
    }

    return radius;
}


/* Calculate the radius of the circle containing the pad.
 */
void D_PAD::ComputeShapeMaxRadius()
{
    m_ShapeMaxRadius = GetMaxRadius();
}


/**
 * Function GetBoundingBox
 * returns the bounding box of this pad
 * Mainly used to redraw the screen area occupied by the pad
 */
EDA_RECT D_PAD::GetBoundingBox() const
{
    EDA_RECT area;
    int radius = GetMaxRadius();     // Calculate the radius of the area, considered as a circle

    area.SetOrigin( TO_LEGACY_LU_WXP( m_Pos ) );
    area.Inflate( radius );

    return area;
}


// Returns the position of the pad.
const wxPoint D_PAD::ReturnShapePos()
{
    if( m_Offset[0] == ZERO_LENGTH && m_Offset[0] == ZERO_LENGTH )
        return TO_LEGACY_LU_WXP( m_Pos );

    wxPoint shape_pos;
    int     dX, dY;

    dX = TO_LEGACY_LU( m_Offset[0] );
    dY = TO_LEGACY_LU( m_Offset[1] );

    RotatePoint( &dX, &dY, m_Orient );

    shape_pos.x = TO_LEGACY_LU( m_Pos.x() ) + dX;
    shape_pos.y = TO_LEGACY_LU( m_Pos.y() ) + dY;

    return shape_pos;
}


/* Return pad name as string in a wxString
 */
wxString D_PAD::ReturnStringPadName() const
{
    wxString name;

    ReturnStringPadName( name );
    return name;
}


/* Return pad name as string in a buffer
 */
void D_PAD::ReturnStringPadName( wxString& text ) const
{
    int ii;

    text.Empty();

    for( ii = 0; ii < 4; ii++ )
    {
        if( m_Padname[ii] == 0 )
            break;

        text.Append( m_Padname[ii] );
    }
}


// Change pad name
void D_PAD::SetPadName( const wxString& name )
{
    int ii, len;

    len = name.Length();

    if( len > 4 )
        len = 4;

    for( ii = 0; ii < len; ii++ )
        m_Padname[ii] = name.GetChar( ii );

    for( ii = len; ii < 4; ii++ )
        m_Padname[ii] = 0;
}


/**
 * Function SetNetname
 * @param aNetname: the new netname
 */
void D_PAD::SetNetname( const wxString& aNetname )
{
    m_Netname = aNetname;
    m_ShortNetname = m_Netname.AfterLast( '/' );
}


void D_PAD::Copy( D_PAD* source )
{
    if( source == NULL )
        return;

    m_Pos = source->m_Pos;
    m_layerMask = source->m_layerMask;

    m_NumPadName = source->m_NumPadName;
    SetNet( source->GetNet() );
    m_Drill = source->m_Drill;
    m_DrillShape = source->m_DrillShape;
    m_Offset     = source->m_Offset;
    m_Size = source->m_Size;
    m_DeltaSize = source->m_DeltaSize;
    m_Pos0     = source->m_Pos0;
    m_ShapeMaxRadius    = source->m_ShapeMaxRadius;
    m_PadShape = source->m_PadShape;
    m_Attribut = source->m_Attribut;
    m_Orient   = source->m_Orient;
    m_LengthDie = source->m_LengthDie;
    m_LocalClearance = source->m_LocalClearance;
    m_LocalSolderMaskMargin  = source->m_LocalSolderMaskMargin;
    m_LocalSolderPasteMargin = source->m_LocalSolderPasteMargin;
    m_LocalSolderPasteMarginRatio = source->m_LocalSolderPasteMarginRatio;

    SetSubRatsnest( 0 );
    SetSubNet( 0 );
    m_Netname = source->m_Netname;
    m_ShortNetname = source->m_ShortNetname;
}


/**
 * Function GetClearance (virtual)
 * returns the clearance in internal units.  If \a aItem is not NULL then the
 * returned clearance is the greater of this object's clearance and
 * aItem's clearance.  If \a aItem is NULL, then this object clearance is returned.
 * @param aItem is another BOARD_CONNECTED_ITEM or NULL
 * @return int - the clearance in internal units.
 */
int D_PAD::GetClearance( BOARD_CONNECTED_ITEM* aItem ) const
{
    // A pad can have specific clearance parameters that
    // overrides its NETCLASS clearance value
    int clearance = TO_LEGACY_LU( m_LocalClearance );

    if( clearance == 0 )
    {   // If local clearance is 0, use the parent footprint clearance value
        /// @BUG unsafe type cast style
        if( GetParent() && ( (MODULE*) GetParent() )->m_LocalClearance )
            clearance = ( (MODULE*) GetParent() )->m_LocalClearance;
    }

    if( clearance == 0 )   // If the parent footprint clearance value = 0, use NETCLASS value
        return BOARD_CONNECTED_ITEM::GetClearance( aItem );

    // We have a specific clearance.
    // if aItem, return the biggest clearance
    if( aItem )
    {
        int hisClearance = aItem->GetClearance();
        return max( hisClearance, clearance );
    }

    // Return the specific clearance.
    return clearance;
}


// Mask margins handling:

/**
 * Function GetSolderMaskMargin
 * @return the margin for the solder mask layer
 * usually > 0 (mask shape bigger than pad
 * value is
 * 1 - the local value
 * 2 - if null, the parent footprint value
 * 1 - if null, the global value
 */
int D_PAD::GetSolderMaskMargin()
{
    int margin = TO_LEGACY_LU( m_LocalSolderMaskMargin );
    MODULE * module = (MODULE*) GetParent();

    if( module )
    {
        if( margin == 0 )
        {
            if( module->m_LocalSolderMaskMargin )
                margin = module->m_LocalSolderMaskMargin;
        }

        if( margin == 0 )
        {
            BOARD * brd = GetBoard();
            margin = brd->GetBoardDesignSettings()->m_SolderMaskMargin;
        }
    }

    // ensure mask have a size always >= 0
    if( margin < 0 )
    {
        int minsize = TO_LEGACY_LU( -min( m_Size.x(), m_Size.y() ) / 2 );

        if( margin < minsize )
            minsize = minsize;
    }

    return margin;
}


/**
 * Function GetSolderPasteMargin
 * @return the margin for the solder mask layer
 * usually < 0 (mask shape smaller than pad
 * value is
 * 1 - the local value
 * 2 - if null, the parent footprint value
 * 1 - if null, the global value
 */
wxSize D_PAD::GetSolderPasteMargin()
{
    int margin = TO_LEGACY_LU( m_LocalSolderPasteMargin );
    double mratio = m_LocalSolderPasteMarginRatio;
    MODULE * module = (MODULE*) GetParent();

    if( module )
    {
        if( margin == 0  )
            margin = module->m_LocalSolderPasteMargin;

        BOARD * brd = GetBoard();

        if( margin == 0  )
            margin = brd->GetBoardDesignSettings()->m_SolderPasteMargin;

        if( mratio == 0.0 )
            mratio = module->m_LocalSolderPasteMarginRatio;

        if( mratio == 0.0 )
        {
           mratio = brd->GetBoardDesignSettings()->m_SolderPasteMarginRatio;
        }
    }

    wxSize pad_margin;
    pad_margin.x = margin + wxRound( TO_LEGACY_LU_DBL( m_Size.x() ) * mratio );
    pad_margin.y = margin + wxRound( TO_LEGACY_LU_DBL( m_Size.y() ) * mratio );

    // ensure mask have a size always >= 0
    if( pad_margin.x < TO_LEGACY_LU( -m_Size.x() / 2 ) )
        pad_margin.x = TO_LEGACY_LU( -m_Size.x() / 2 );

    if( pad_margin.y < TO_LEGACY_LU( -m_Size.y() / 2 ) )
        pad_margin.y = TO_LEGACY_LU( -m_Size.y() / 2 );

    return pad_margin;
}


/* Read pad from file.
 * The 1st line of descr ($PAD) is assumed to be already read
 * Syntax:
 * $PAD
 * Sh "N1" C 550 550 0 0 1800
 * Dr 310 0 0
 * At STD N 00C0FFFF
 * Do 3 "netname"
 * Po 6000 -6000
 * $EndPAD
 */
int D_PAD::ReadDescr( LINE_READER* aReader )
{
    char* Line;
    char  BufLine[1024], BufCar[256];
    char* PtLine;
    int   nn, ll;
    ARG_LENLD_TYPE sx, sy, ox, oy, dr, dx, dy;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( Line[0] == '$' )
            return 0;

        PtLine = Line + 3;

        /* Decode the first code and read the corresponding data
         */
        switch( Line[0] )
        {
        case 'S': // = Sh
            /* Read pad name */
            nn = 0;

            while( (*PtLine != '"') && *PtLine )
                PtLine++;

            if( *PtLine )
                PtLine++;

            memset( m_Padname, 0, sizeof(m_Padname) );

            while( (*PtLine != '"') && *PtLine )
            {
                if( nn < (int) sizeof(m_Padname) )
                {
                    if( *PtLine > ' ' )
                    {
                        m_Padname[nn] = *PtLine; nn++;
                    }
                }
                PtLine++;
            }

            if( *PtLine == '"' )
                PtLine++;

            nn = sscanf( PtLine, " %s "FM_LENLD" "FM_LENLD" "FM_LENLD" "FM_LENLD" %d",
                         BufCar, &sx, &sy,
                         &dx, &dy,
                         &m_Orient );
            m_Size.x() = LENGTH_LOAD_TMP( sx );
            m_Size.y() = LENGTH_LOAD_TMP( sy );
            m_DeltaSize.x() = LENGTH_LOAD_TMP( dx );
            m_DeltaSize.y() = LENGTH_LOAD_TMP( dy );
            ll = 0xFF & BufCar[0];

            /* Read pad shape */
            m_PadShape = PAD_CIRCLE;

            switch( ll )
            {
            case 'C':
                m_PadShape = PAD_CIRCLE; break;

            case 'R':
                m_PadShape = PAD_RECT; break;

            case 'O':
                m_PadShape = PAD_OVAL; break;

            case 'T':
                m_PadShape = PAD_TRAPEZOID; break;
            }

            ComputeShapeMaxRadius();
            break;

        case 'D':
            BufCar[0] = 0;
            nn = sscanf( PtLine, FM_LENLD" "FM_LENLD" "FM_LENLD" %s "FM_LENLD" "FM_LENLD, &dr,
                         &ox, &oy, BufCar, &dx, &dy );
            m_Offset.x()              = LENGTH_LOAD_TMP( ox );
            m_Offset.y()              = LENGTH_LOAD_TMP( oy );
            m_Drill.y() = m_Drill.x() = LENGTH_LOAD_TMP( dr );
            m_DrillShape = PAD_CIRCLE;

            if( nn >= 6 )       // Drill shape = OVAL ?
            {
                if( BufCar[0] == 'O' )
                {
                    m_Drill.x()  = FROM_LEGACY_LU( dx );
                    m_Drill.y()  = FROM_LEGACY_LU( dy );
                    m_DrillShape = PAD_OVAL;
                }
            }

            break;

        case 'A':
            nn = sscanf( PtLine, "%s %s %X", BufLine, BufCar, /// @BUG Stack overflow vulnerability!
                         &m_layerMask );

            /* BufCar is not used now update attributes */
            m_Attribut = PAD_STANDARD;
            if( strncmp( BufLine, "SMD", 3 ) == 0 )
                m_Attribut = PAD_SMD;

            if( strncmp( BufLine, "CONN", 4 ) == 0 )
                m_Attribut = PAD_CONN;

            if( strncmp( BufLine, "HOLE", 4 ) == 0 )
                m_Attribut = PAD_HOLE_NOT_PLATED;

            break;

        case 'N':       /* Read Netname */
            int netcode;
            nn = sscanf( PtLine, "%d", &netcode );
            SetNet( netcode );

            /* read Netname */
            ReadDelimitedText( BufLine, PtLine, sizeof(BufLine) );
            SetNetname( FROM_UTF8( StrPurge( BufLine ) ) );
        break;

        case 'P':
            nn    = sscanf( PtLine, FM_LENLD" "FM_LENLD, &ox, &oy );
            m_Pos0 = VECTOR_PCB::fromXY( LENGTH_LOAD_TMP( ox ), LENGTH_LOAD_TMP( oy ) );
            m_Pos = m_Pos0;
            break;

        case 'L':
            nn    = sscanf( PtLine, FM_LENLD, &ox );
            m_LengthDie = LENGTH_LOAD_TMP( ox );
            break;

        case '.':    /* Read specific data */
            if( strnicmp( Line, ".SolderMask ", 12 ) == 0 )
                m_LocalSolderMaskMargin = FROM_LEGACY_LU( atof( Line + 12 ) );
            else if( strnicmp( Line, ".SolderPaste ", 13 )  == 0 )
                m_LocalSolderPasteMargin = FROM_LEGACY_LU( atoi( Line + 13 ) );
            else if( strnicmp( Line, ".SolderPasteRatio ", 18 ) == 0 )
                m_LocalSolderPasteMarginRatio = atoi( Line + 18 );
            else if( strnicmp( Line, ".LocalClearance ", 16 ) == 0 )
                m_LocalClearance = FROM_LEGACY_LU( atof( Line + 16 ) );
            break;

        default:
            DisplayError( NULL, wxT( "Err Pad: Id inconnu" ) );
            return 1;
        }
    }

    return 2;   /* error : EOF */
}


bool D_PAD::Save( FILE* aFile ) const
{
    int         cshape;
    const char* texttype;

    // check the return values for first and last fprints() in this function
    if( fprintf( aFile, "$PAD\n" ) != sizeof("$PAD\n") - 1 )
        return false;

    switch( m_PadShape )
    {
    case PAD_CIRCLE:
        cshape = 'C'; break;

    case PAD_RECT:
        cshape = 'R'; break;

    case PAD_OVAL:
        cshape = 'O'; break;

    case PAD_TRAPEZOID:
        cshape = 'T'; break;

    default:
        cshape = 'C';
        DisplayError( NULL, _( "Unknown pad shape" ) );
        break;
    }

    fprintf( aFile, "Sh \"%.4s\" %c "FM_LENSV" "FM_LENSV" "FM_LENSV" "FM_LENSV" %d\n", // TODO: pad name length limit!
             m_Padname, cshape, ARG_LENSV( m_Size.x() ), ARG_LENSV( m_Size.y() ),
             ARG_LENSV( m_DeltaSize.x() ), ARG_LENSV( m_DeltaSize.y() ), m_Orient );

    fprintf( aFile, "Dr "FM_LENSV" "FM_LENSV" "FM_LENSV, ARG_LENSV( m_Drill.x() ), ARG_LENSV( m_Offset.x() ), ARG_LENSV( m_Offset.y() ) );

    if( m_DrillShape == PAD_OVAL )
    {
        fprintf( aFile, " %c "FM_LENSV" "FM_LENSV, 'O', ARG_LENSV( m_Drill.x() ), ARG_LENSV( m_Drill.y() ) );
    }

    fprintf( aFile, "\n" );

    switch( m_Attribut )
    {
    case PAD_STANDARD:
        texttype = "STD"; break;

    case PAD_SMD:
        texttype = "SMD"; break;

    case PAD_CONN:
        texttype = "CONN"; break;

    case PAD_HOLE_NOT_PLATED:
        texttype = "HOLE"; break;

    default:
        texttype = "STD";
        DisplayError( NULL, wxT( "Invalid Pad attribute" ) );
        break;
    }

    fprintf( aFile, "At %s N %8.8X\n", texttype, m_layerMask );

    fprintf( aFile, "Ne %d %s\n", GetNet(), EscapedUTF8( m_Netname ).c_str() );

    fprintf( aFile, "Po "FM_LENSV" "FM_LENSV"\n", ARG_LENSV( m_Pos0.x() ), ARG_LENSV( m_Pos0.y() ) );

    if( m_LengthDie != ZERO_LENGTH )
        fprintf( aFile, "Le "FM_LENSV"\n", ARG_LENSV( m_LengthDie ) );

    if( m_LocalSolderMaskMargin != ZERO_LENGTH )
        fprintf( aFile, ".SolderMask "FM_LENSV"\n", ARG_LENSV( m_LocalSolderMaskMargin ) );

    if( m_LocalSolderPasteMargin != ZERO_LENGTH )
        fprintf( aFile, ".SolderPaste "FM_LENSV"\n", ARG_LENSV( m_LocalSolderPasteMargin ) );

    if( m_LocalSolderPasteMarginRatio != 0 )
        fprintf( aFile, ".SolderPasteRatio %g\n", m_LocalSolderPasteMarginRatio );

    if( m_LocalClearance != ZERO_LENGTH )
        fprintf( aFile, ".LocalClearance "FM_LENSV"\n", ARG_LENSV( m_LocalClearance ) );

    if( fprintf( aFile, "$EndPAD\n" ) != sizeof("$EndPAD\n") - 1 )
        return false;

    return true;
}


void D_PAD::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    MODULE*     module;
    wxString    Line;
    BOARD*      board;

    frame->EraseMsgBox();

    module = (MODULE*) m_Parent;

    if( module )
    {
        wxString msg = module->GetReference();
        frame->AppendMsgPanel( _( "Module" ), msg, DARKCYAN );
        ReturnStringPadName( Line );
        frame->AppendMsgPanel( _( "RefP" ), Line, BROWN );
    }

    frame->AppendMsgPanel( _( "Net" ), m_Netname, DARKCYAN );

    /* For test and debug only: display m_physical_connexion and
     * m_logical_connexion */
#if 1   // Used only to debug connectivity calculations
    Line.Printf( wxT( "%d-%d-%d " ), GetSubRatsnest(), GetSubNet(), m_ZoneSubnet );
    frame->AppendMsgPanel( wxT( "L-P-Z" ), Line, DARKGREEN );
#endif

    board = GetBoard();

    wxString layerInfo;

    if( (m_layerMask & ALL_CU_LAYERS) == 0 )     // pad is not on any copper layers
    {
        switch( m_layerMask & ~ALL_CU_LAYERS )
        {
        case ADHESIVE_LAYER_BACK:
            layerInfo = board->GetLayerName( ADHESIVE_N_BACK );
            break;

        case ADHESIVE_LAYER_FRONT:
            layerInfo = board->GetLayerName( ADHESIVE_N_FRONT );
            break;

        case SOLDERPASTE_LAYER_BACK:
            layerInfo = board->GetLayerName( SOLDERPASTE_N_BACK );
            break;

        case SOLDERPASTE_LAYER_FRONT:
            layerInfo = board->GetLayerName( SOLDERPASTE_N_FRONT );
            break;

        case SILKSCREEN_LAYER_BACK:
            layerInfo = board->GetLayerName( SILKSCREEN_N_BACK );
            break;

        case SILKSCREEN_LAYER_FRONT:
            layerInfo = board->GetLayerName( SILKSCREEN_N_FRONT );
            break;

        case SOLDERMASK_LAYER_BACK:
            layerInfo = board->GetLayerName( SOLDERMASK_N_BACK );
            break;

        case SOLDERMASK_LAYER_FRONT:
            layerInfo = board->GetLayerName( SOLDERMASK_N_FRONT );
            break;

        case DRAW_LAYER:
            layerInfo = board->GetLayerName( DRAW_N );
            break;

        case COMMENT_LAYER:
            layerInfo = board->GetLayerName( COMMENT_N );
            break;

        case ECO1_LAYER:
            layerInfo = board->GetLayerName( ECO1_N );
            break;

        case ECO2_LAYER:
            layerInfo = board->GetLayerName( ECO2_N );
            break;

        case EDGE_LAYER:
            layerInfo = board->GetLayerName( EDGE_N );
            break;

        default:
            layerInfo = _( "Non-copper" );
            break;
        }
    }
    else
    {
#define INTERIOR_COPPER     (ALL_CU_LAYERS & ~(LAYER_BACK | LAYER_FRONT))

        static const wxChar* andInternal = _( " & int" );

        if( (m_layerMask & (LAYER_BACK | LAYER_FRONT)) == LAYER_BACK )
        {
            layerInfo = board->GetLayerName( LAYER_N_BACK );

            if( m_layerMask & INTERIOR_COPPER )
                layerInfo += andInternal;
        }

        else if( (m_layerMask & (LAYER_BACK | LAYER_FRONT)) == (LAYER_BACK | LAYER_FRONT) )
        {
            layerInfo = board->GetLayerName( LAYER_N_BACK ) + wxT(", ") +
                        board->GetLayerName( LAYER_N_FRONT );

            if( m_layerMask & INTERIOR_COPPER )
                layerInfo += andInternal;
        }

        else if( (m_layerMask & (LAYER_BACK | LAYER_FRONT)) == LAYER_FRONT )
        {
            layerInfo = board->GetLayerName( LAYER_N_FRONT );

            if( m_layerMask & INTERIOR_COPPER )
                layerInfo += andInternal;
        }
        else // necessarily true: if( m_layerMask & INTERIOR_COPPER )
        {
            layerInfo = _( "internal" );
        }
    }

    frame->AppendMsgPanel( _( "Layer" ), layerInfo, DARKGREEN );

    frame->AppendMsgPanel( ShowPadShape(), ShowPadAttr(), DARKGREEN );

    valeur_param( TO_LEGACY_LU( m_Size.x() ), Line );
    frame->AppendMsgPanel( _( "H Size" ), Line, RED );

    valeur_param( TO_LEGACY_LU( m_Size.y() ), Line );
    frame->AppendMsgPanel( _( "V Size" ), Line, RED );

    valeur_param( (unsigned)(int) TO_LEGACY_LU( m_Drill.x() ), Line );

    if( m_DrillShape == PAD_CIRCLE )
    {
        frame->AppendMsgPanel( _( "Drill" ), Line, RED );
    }
    else
    {
        valeur_param( (unsigned) TO_LEGACY_LU( m_Drill.x() ), Line );
        wxString msg;
        valeur_param( (unsigned) TO_LEGACY_LU( m_Drill.y() ), msg );
        Line += wxT( " / " ) + msg;
        frame->AppendMsgPanel( _( "Drill X / Y" ), Line, RED );
    }

    int module_orient = module ? module->m_Orient : 0;

    if( module_orient )
        Line.Printf( wxT( "%3.1f(+%3.1f)" ),
                     (float) ( m_Orient - module_orient ) / 10,
                     (float) module_orient / 10 );
    else
        Line.Printf( wxT( "%3.1f" ), (float) m_Orient / 10 );

    frame->AppendMsgPanel( _( "Orient" ), Line, LIGHTBLUE );

    valeur_param( TO_LEGACY_LU( m_Pos.x() ), Line );
    frame->AppendMsgPanel( _( "X Pos" ), Line, LIGHTBLUE );

    valeur_param( TO_LEGACY_LU( m_Pos.y() ), Line );
    frame->AppendMsgPanel( _( "Y pos" ), Line, LIGHTBLUE );

    if( m_LengthDie != ZERO_LENGTH )
    {
        valeur_param( TO_LEGACY_LU( m_LengthDie ), Line );
        frame->AppendMsgPanel( _( "Length on die" ), Line, CYAN );
    }
}


// see class_pad.h
bool D_PAD::IsOnLayer( int aLayer ) const
{
    return (1 << aLayer) & m_layerMask;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool D_PAD::HitTest( const wxPoint& refPos )
{
    int     dx, dy;
    double  dist;

    wxPoint shape_pos = ReturnShapePos();

    wxPoint delta = refPos - shape_pos;

    /* Quick test: a test point must be inside the circle. */
    if( ( abs( delta.x ) > m_ShapeMaxRadius ) || ( abs( delta.y ) > m_ShapeMaxRadius ) )
        return false;

    dx = TO_LEGACY_LU( m_Size.x() / 2 ); // dx also is the radius for rounded pads
    dy = TO_LEGACY_LU( m_Size.y() / 2 );

    switch( m_PadShape & 0x7F )
    {
    case PAD_CIRCLE:
        dist = hypot( delta.x, delta.y );

        if( wxRound( dist ) <= dx )
            return true;

        break;

    case PAD_TRAPEZOID:
    {
        wxPoint poly[4];
        BuildPadPolygon( poly, wxSize(0,0), 0 );
        RotatePoint( &delta, -m_Orient );
        return TestPointInsidePolygon( poly, 4, delta );
    }

    default:
        RotatePoint( &delta, -m_Orient );

        if( (abs( delta.x ) <= dx ) && (abs( delta.y ) <= dy) )
            return true;

        break;
    }

    return false;
}


int D_PAD::Compare( const D_PAD* padref, const D_PAD* padcmp )
{
    int diff;

    if( ( diff = padref->m_PadShape - padcmp->m_PadShape) )
        return diff;

    if( ( diff = TO_LEGACY_LU( padref->m_Size.x() - padcmp->m_Size.x() ) ) )
        return diff;

    if( ( diff = TO_LEGACY_LU( padref->m_Size.y() - padcmp->m_Size.y() ) ) )
        return diff;

    if( ( diff = TO_LEGACY_LU( padref->m_Offset.x() - padcmp->m_Offset.x() ) ) )
        return diff;

    if( ( diff = TO_LEGACY_LU( padref->m_Offset.y() - padcmp->m_Offset.y() ) ) )
        return diff;

    if( ( diff = TO_LEGACY_LU( padref->m_DeltaSize.x() - padcmp->m_DeltaSize.x() ) ) )
        return diff;

    if( ( diff = TO_LEGACY_LU( padref->m_DeltaSize.y() - padcmp->m_DeltaSize.y() ) ) )
        return diff;

    // @todo check if export_gencad still works:
    // specctra_export needs this, but maybe export_gencad does not.  added on
    // Jan 24 2008 by Dick.
    if( ( diff = padref->m_layerMask - padcmp->m_layerMask ) )
        return diff;

    return 0;
}


wxString D_PAD::ShowPadShape() const
{
    switch( m_PadShape )
    {
    case PAD_CIRCLE:
        return _( "Circle" );

    case PAD_OVAL:
        return _( "Oval" );

    case PAD_RECT:
        return _( "Rect" );

    case PAD_TRAPEZOID:
        return _( "Trap" );

    default:
        return wxT( "??Unknown??" );
    }
}


wxString D_PAD::ShowPadAttr() const
{
    switch( m_Attribut & 0x0F )
    {
    case PAD_STANDARD:
        return _( "Std" );

    case PAD_SMD:
        return _( "Smd" );

    case PAD_CONN:
        return _( "Conn" );

    case PAD_HOLE_NOT_PLATED:
        return _( "Not Plated" );

    default:
        return wxT( "??Unkown??" );
    }
}


wxString D_PAD::GetSelectMenuText() const
{
    wxString text;
    wxString padlayers;
    BOARD * board = GetBoard();


    if ( (m_layerMask & ALL_CU_LAYERS) == ALL_CU_LAYERS )
        padlayers = _("all copper layers");
    else if( (m_layerMask & LAYER_BACK ) == LAYER_BACK )
        padlayers = board->GetLayerName(LAYER_N_BACK);
    else if( (m_layerMask & LAYER_FRONT) == LAYER_FRONT )
        padlayers = board->GetLayerName(LAYER_N_FRONT);
    else
        padlayers = _( "???" );

    text.Printf( _( "Pad [%s] (%s) of %s" ),
                 GetChars(ReturnStringPadName() ), GetChars( padlayers ),
                 GetChars(( (MODULE*) GetParent() )->GetReference() ) );

    return text;
}

#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void D_PAD::Show( int nestLevel, std::ostream& os )
{
    char padname[5] = { m_Padname[0], m_Padname[1], m_Padname[2], m_Padname[3], 0 };

    char layerMask[16];

    sprintf( layerMask, "0x%08X", m_layerMask );

    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " shape=\"" << ShowPadShape() << '"' <<
    " attr=\"" << ShowPadAttr( ) << '"' <<
    " num=\"" << padname << '"' <<
    " net=\"" << m_Netname.mb_str() << '"' <<
    " netcode=\"" << GetNet() << '"' <<
    " layerMask=\"" << layerMask << '"' << m_Pos << "/>\n";

//    NestedSpace( nestLevel+1, os ) << m_Text.mb_str() << '\n';

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str()
//    << ">\n";
}


#endif
