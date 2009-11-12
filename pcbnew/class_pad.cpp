/***********************************************/
/* class_pad.cpp : D_PAD class implementation. */
/***********************************************/

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"

#include "pcbnew.h"
#include "trigo.h"
#include "pcbnew_id.h"             // ID_TRACK_BUTT
#include "class_board_design_settings.h"


D_PAD::D_PAD( MODULE* parent ) : BOARD_CONNECTED_ITEM( parent, TYPE_PAD )
{
    m_NumPadName = 0;

    m_Size.x = m_Size.y = 500;          // give it a reasonable size
    m_Orient = 0;                       // Pad rotation in 1/10 degrees

    if( m_Parent && (m_Parent->Type()  == TYPE_MODULE) )
    {
        m_Pos = ( (MODULE*) m_Parent )->GetPosition();
    }

    m_PadShape = PAD_CIRCLE;                        // Shape: PAD_CIRCLE,
                                                    // PAD_RECT PAD_OVAL
                                                    // PAD_TRAPEZOID
    m_Attribut = PAD_STANDARD;                      // Type: NORMAL, PAD_SMD,
                                                    // PAD_CONN
    m_DrillShape     = PAD_CIRCLE;                  // Drill shape = circle
    m_LocalClearance = 0;
    m_LocalSolderMaskMargin  = 0;
    m_LocalSolderPasteMargin = 0;
    m_LocalSolderPasteMarginRatio = 0.0;
    m_Masque_Layer = PAD_STANDARD_DEFAULT_LAYERS;   // set layers mask to
                                                    // default for a standard
                                                    // pad

    SetSubRatsnest( 0 );                            // used in ratsnest
                                                    // calculations
    ComputeRayon();
}


D_PAD::~D_PAD()
{
}


/* Calculate the radius of the pad.
 */
void D_PAD::ComputeRayon()
{
    switch( m_PadShape & 0x7F )
    {
    case PAD_CIRCLE:
        m_Rayon = m_Size.x / 2;
        break;

    case PAD_OVAL:
        m_Rayon = MAX( m_Size.x, m_Size.y ) / 2;
        break;

    case PAD_RECT:
    case PAD_TRAPEZOID:
        m_Rayon = (int) ( sqrt( (double) m_Size.y * m_Size.y
                               + (double) m_Size.x * m_Size.x ) / 2 );
        break;
    }
}


/**
 * Function GetBoundingBox
 * returns the bounding box of this pad
 * Mainly used to redraw the screen area occupied by the pad
 */
EDA_Rect D_PAD::GetBoundingBox()
{
    // Calculate area:
    ComputeRayon();     // calculate the radius of the area, considered as a
                        // circle
    EDA_Rect area;
    area.SetOrigin( m_Pos );
    area.Inflate( m_Rayon, m_Rayon );

    return area;
}


// Returns the position of the pad.
const wxPoint D_PAD::ReturnShapePos()
{
    if( m_Offset.x == 0 && m_Offset.y == 0 )
        return m_Pos;

    wxPoint shape_pos;
    int     dX, dY;

    dX = m_Offset.x;
    dY = m_Offset.y;

    RotatePoint( &dX, &dY, m_Orient );

    shape_pos.x = m_Pos.x + dX;
    shape_pos.y = m_Pos.y + dY;

    return shape_pos;
}


/* Return pad name as string in a wxString
 */
wxString D_PAD::ReturnStringPadName()
{
    wxString name;

    ReturnStringPadName( name );
    return name;
}


/* Return pad name as string in a buffer
 */
void D_PAD::ReturnStringPadName( wxString& text )
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
 * @param const wxString : the new netname
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
    m_Masque_Layer = source->m_Masque_Layer;

    memcpy( m_Padname, source->m_Padname, sizeof(m_Padname) );
    SetNet( source->GetNet() );
    m_Drill = source->m_Drill;
    m_DrillShape = source->m_DrillShape;
    m_Offset     = source->m_Offset;
    m_Size = source->m_Size;
    m_DeltaSize = source->m_DeltaSize;
    m_Pos0     = source->m_Pos0;
    m_Rayon    = source->m_Rayon;
    m_PadShape = source->m_PadShape;
    m_Attribut = source->m_Attribut;
    m_Orient   = source->m_Orient;
    m_LocalClearance = source->m_LocalClearance;
    m_LocalSolderMaskMargin  = source->m_LocalSolderMaskMargin;
    m_LocalSolderPasteMargin = source->m_LocalSolderPasteMargin;
    m_LocalSolderPasteMarginRatio = source->m_LocalSolderPasteMarginRatio;

    SetSubRatsnest( 0 );
    SetSubNet( 0 );
    m_Netname = source->m_Netname;
    m_ShortNetname = source->m_ShortNetname;
}


/** Virtual function GetClearance
 * returns the clearance in 1/10000 inches.  If \a aItem is not NULL then the
 * returned clearance is the greater of this object's NETCLASS clearance and
 * aItem's NETCLASS clearance.  If \a aItem is NULL, then this objects
 * clearance
 * is returned.
 * @param aItem is another BOARD_CONNECTED_ITEM or NULL
 * @return int - the clearance in 1/10000 inches.
 */
int D_PAD::GetClearance( BOARD_CONNECTED_ITEM* aItem ) const
{
    int clearance = m_LocalClearance;

    if( clearance == 0 )
    {
        if( GetParent() && ( (MODULE*) GetParent() )->m_LocalClearance )
            clearance = ( (MODULE*) GetParent() )->m_LocalClearance;
    }

    if( clearance == 0 )
        return BOARD_CONNECTED_ITEM::GetClearance( aItem );

    if( aItem )
    {
        NETCLASS* hisclass = aItem->GetNetClass();
        if( hisclass )
        {
            int hisClearance = hisclass->GetClearance();
            return max( hisClearance, clearance );
        }
    }

    return clearance;
}


// Mask margins handling:

/** Function GetSolderMaskMargin
 * @return the margin for the solder mask layer
 * usually > 0 (mask shape bigger than pad
 * value is
 * 1 - the local value
 * 2 - if null, the parent footprint value
 * 1 - if null, the global value
 */
int D_PAD::GetSolderMaskMargin()
{
    int margin = m_LocalSolderMaskMargin;

    if( margin == 0 )
    {
        if( GetParent() && ( (MODULE*) GetParent() )->m_LocalSolderMaskMargin )
            margin = ( (MODULE*) GetParent() )->m_LocalSolderMaskMargin;
    }
    if( margin == 0 )
        margin = g_DesignSettings.m_SolderMaskMargin;

    // ensure mask have a size always >= 0
    if( margin < 0 )
    {
        int minsize = -MIN( m_Size.x, m_Size.y ) / 2;
        if( margin < minsize )
            minsize = minsize;
    }
    return margin;
}


/** Function GetSolderPasteMargin
 * @return the margin for the solder mask layer
 * usually < 0 (mask shape smaller than pad
 * value is
 * 1 - the local value
 * 2 - if null, the parent footprint value
 * 1 - if null, the global value
 */
wxSize D_PAD::GetSolderPasteMargin()
{
    int margin = m_LocalSolderPasteMargin;

    if( margin == 0  && GetParent() )
        margin = ( (MODULE*) GetParent() )->m_LocalSolderPasteMargin;
    if( margin == 0  && GetParent() )
        margin = g_DesignSettings.m_SolderPasteMargin;

    double mratio = m_LocalSolderPasteMarginRatio;
    if( mratio == 0.0 && GetParent() )
        mratio = ( (MODULE*) GetParent() )->m_LocalSolderPasteMarginRatio;
    if( mratio == 0.0 )
        mratio = g_DesignSettings.m_SolderPasteMarginRatio;

    wxSize pad_margin;
    pad_margin.x = margin + wxRound( m_Size.x * mratio );
    pad_margin.y = margin + wxRound( m_Size.y * mratio );

    // ensure mask have a size always >= 0
    if( pad_margin.x < -m_Size.x / 2 )
        pad_margin.x = -m_Size.x / 2;

    if( pad_margin.y < -m_Size.y / 2 )
        pad_margin.y = -m_Size.y / 2;

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
int D_PAD::ReadDescr( FILE* File, int* LineNum )
{
    char  Line[1024], BufLine[1024], BufCar[256];
    char* PtLine;
    int   nn, ll, dx, dy;

    while( GetLine( File, Line, LineNum ) != NULL )
    {
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

            nn = sscanf( PtLine, " %s %d %d %d %d %d",
                         BufCar, &m_Size.x, &m_Size.y,
                         &m_DeltaSize.x, &m_DeltaSize.y,
                         &m_Orient );

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

            ComputeRayon();
            break;

        case 'D':
            BufCar[0] = 0;
            nn = sscanf( PtLine, "%d %d %d %s %d %d", &m_Drill.x,
                         &m_Offset.x, &m_Offset.y, BufCar, &dx, &dy );
            m_Drill.y    = m_Drill.x;
            m_DrillShape = PAD_CIRCLE;

            if( nn >= 6 )       // Drill shape = OVAL ?
            {
                if( BufCar[0] == 'O' )
                {
                    m_Drill.x    = dx; m_Drill.y = dy;
                    m_DrillShape = PAD_OVAL;
                }
            }
            break;

        case 'A':
            nn = sscanf( PtLine, "%s %s %X", BufLine, BufCar,
                         &m_Masque_Layer );

            /* BufCar is not used now */
            /* update attributes */
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
            SetNetname( CONV_FROM_UTF8( StrPurge( BufLine ) ) );
            break;

        case 'P':
            nn    = sscanf( PtLine, "%d %d", &m_Pos0.x, &m_Pos0.y );
            m_Pos = m_Pos0;
            break;

        case '.':    /* Read specific data */
            if( strnicmp( Line, ".SolderMask ", 12 ) == 0 )
                m_LocalSolderMaskMargin = atoi( Line + 12 );
            else if( strnicmp( Line, ".SolderPaste ", 13 )  == 0 )
                m_LocalSolderPasteMargin = atoi( Line + 13 );
            else if( strnicmp( Line, ".SolderPasteRatio ", 18 ) == 0 )
                m_LocalSolderPasteMarginRatio = atoi( Line + 18 );
            else if( strnicmp( Line, ".LocalClearance ", 16 ) == 0 )
                m_LocalClearance = atoi( Line + 16 );
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

    fprintf( aFile, "Sh \"%.4s\" %c %d %d %d %d %d\n",
             m_Padname, cshape, m_Size.x, m_Size.y,
             m_DeltaSize.x, m_DeltaSize.y, m_Orient );

    fprintf( aFile, "Dr %d %d %d", m_Drill.x, m_Offset.x, m_Offset.y );
    if( m_DrillShape == PAD_OVAL )
    {
        fprintf( aFile, " %c %d %d", 'O', m_Drill.x, m_Drill.y );
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

    fprintf( aFile, "At %s N %8.8X\n", texttype, m_Masque_Layer );

    fprintf( aFile, "Ne %d \"%s\"\n", GetNet(), CONV_TO_UTF8( m_Netname ) );

    fprintf( aFile, "Po %d %d\n", m_Pos0.x, m_Pos0.y );

    if( m_LocalSolderMaskMargin != 0 )
        fprintf( aFile, ".SolderMask %d\n", m_LocalSolderMaskMargin );
    if( m_LocalSolderPasteMargin != 0 )
        fprintf( aFile, ".SolderPaste %d\n", m_LocalSolderPasteMargin );
    if( m_LocalSolderPasteMarginRatio != 0 )
        fprintf( aFile,
                 ".SolderPasteRatio %g\n",
                 m_LocalSolderPasteMarginRatio );
    if( m_LocalClearance != 0 )
        fprintf( aFile, ".LocalClearance %d\n", m_LocalClearance );

    if( fprintf( aFile, "$EndPAD\n" ) != sizeof("$EndPAD\n") - 1 )
        return false;

    return true;
}


void D_PAD::DisplayInfo( WinEDA_DrawFrame* frame )
{
    int      ii;
    MODULE*  module;
    wxString Line;

    /* Pad messages */
    static const wxString Msg_Pad_Shape[6] =
    { wxT( "??? " ), wxT( "Circ" ), wxT( "Rect" ), wxT( "Oval" ), wxT( "trap" ),
      wxT( "spec" ) };

    static const wxString Msg_Pad_Layer[9] =
    {
        wxT( "??? " ), wxT( "cmp   " ), wxT( "cu    " ), wxT( "cmp+cu " ),
        wxT( "int    " ), wxT( "cmp+int " ), wxT( "cu+int " ),
        wxT( "all    " ), wxT( "No copp" )
    };

    static const wxString Msg_Pad_Attribut[5] =
    { wxT( "norm" ), wxT( "smd " ), wxT( "conn" ), wxT( "????" ) };


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
    Line.Printf( wxT( "%d-%d-%d " ), GetSubRatsnest(),
                 GetSubNet(), m_ZoneSubnet );
    frame->AppendMsgPanel( wxT( "L-P-Z" ), Line, DARKGREEN );
#endif

    wxString LayerInfo;

    ii = 0;
    if( m_Masque_Layer & CUIVRE_LAYER )
        ii = 2;
    if( m_Masque_Layer & CMP_LAYER )
        ii += 1;
    if( (m_Masque_Layer & ALL_CU_LAYERS) == ALL_CU_LAYERS )
        ii = 7;

    LayerInfo = Msg_Pad_Layer[ii];
    if( (m_Masque_Layer & ALL_CU_LAYERS) == 0 )
    {
        if( m_Masque_Layer )
            LayerInfo = Msg_Pad_Layer[8];

        switch( m_Masque_Layer & ~ALL_CU_LAYERS )
        {
        case ADHESIVE_LAYER_CU:
            LayerInfo = ReturnPcbLayerName( ADHESIVE_N_CU );
            break;

        case ADHESIVE_LAYER_CMP:
            LayerInfo = ReturnPcbLayerName( ADHESIVE_N_CMP );
            break;

        case SOLDERPASTE_LAYER_CU:
            LayerInfo = ReturnPcbLayerName( SOLDERPASTE_N_CU );
            break;

        case SOLDERPASTE_LAYER_CMP:
            LayerInfo = ReturnPcbLayerName( SOLDERPASTE_N_CMP );
            break;

        case SILKSCREEN_LAYER_CU:
            LayerInfo = ReturnPcbLayerName( SILKSCREEN_N_CU );
            break;

        case SILKSCREEN_LAYER_CMP:
            LayerInfo = ReturnPcbLayerName( SILKSCREEN_N_CMP );
            break;

        case SOLDERMASK_LAYER_CU:
            LayerInfo = ReturnPcbLayerName( SOLDERMASK_N_CU );
            break;

        case SOLDERMASK_LAYER_CMP:
            LayerInfo = ReturnPcbLayerName( SOLDERMASK_N_CMP );
            break;

        case DRAW_LAYER:
            LayerInfo = ReturnPcbLayerName( DRAW_N );
            break;

        case COMMENT_LAYER:
            LayerInfo = ReturnPcbLayerName( COMMENT_N );
            break;

        case ECO1_LAYER:
            LayerInfo = ReturnPcbLayerName( ECO1_N );
            break;

        case ECO2_LAYER:
            LayerInfo = ReturnPcbLayerName( ECO2_N );
            break;

        case EDGE_LAYER:
            LayerInfo = ReturnPcbLayerName( EDGE_N );
            break;

        default:
            break;
        }
    }
    frame->AppendMsgPanel( _( "Layer" ), LayerInfo, DARKGREEN );

    int attribut = m_Attribut & 15;
    if( attribut > 3 )
        attribut = 3;
    frame->AppendMsgPanel( Msg_Pad_Shape[m_PadShape],
                           Msg_Pad_Attribut[attribut], DARKGREEN );

    valeur_param( m_Size.x, Line );
    frame->AppendMsgPanel( _( "H Size" ), Line, RED );

    valeur_param( m_Size.y, Line );
    frame->AppendMsgPanel( _( "V Size" ), Line, RED );

    valeur_param( (unsigned) m_Drill.x, Line );
    if( m_DrillShape == PAD_CIRCLE )
    {
        frame->AppendMsgPanel( _( "Drill" ), Line, RED );
    }
    else
    {
        valeur_param( (unsigned) m_Drill.x, Line );
        wxString msg;
        valeur_param( (unsigned) m_Drill.y, msg );
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
    frame->AppendMsgPanel( _( "Orient" ), Line, BLUE );

    valeur_param( m_Pos.x, Line );
    frame->AppendMsgPanel( _( "X Pos" ), Line, BLUE );

    valeur_param( m_Pos.y, Line );
    frame->AppendMsgPanel( _( "Y pos" ), Line, BLUE );
}


// see class_pad.h
bool D_PAD::IsOnLayer( int aLayer ) const
{
    return (1 << aLayer) & m_Masque_Layer;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param ref_pos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool D_PAD::HitTest( const wxPoint& ref_pos )
{
    int     deltaX, deltaY;
    int     dx, dy;
    double  dist;

    wxPoint shape_pos = ReturnShapePos();

    deltaX = ref_pos.x - shape_pos.x;
    deltaY = ref_pos.y - shape_pos.y;

    /* Quick test: a test point must be inside the circle. */
    if( ( abs( deltaX ) > m_Rayon ) || ( abs( deltaY ) > m_Rayon ) )
        return false;

    dx = m_Size.x >> 1; // dx also is the radius for rounded pads
    dy = m_Size.y >> 1;

    switch( m_PadShape & 0x7F )
    {
    case PAD_CIRCLE:
        dist = hypot( deltaX, deltaY );
        if( wxRound( dist ) <= dx )
            return true;
        break;

    default:
        RotatePoint( &deltaX, &deltaY, -m_Orient );
        if( (abs( deltaX ) <= dx ) && (abs( deltaY ) <= dy) )
            return true;
        break;
    }

    return false;
}


int D_PAD::Compare( const D_PAD* padref, const D_PAD* padcmp )
{
    int diff;

    if( (diff = padref->m_PadShape - padcmp->m_PadShape) )
        return diff;
    if( (diff = padref->m_Size.x - padcmp->m_Size.x) )
        return diff;
    if( (diff = padref->m_Size.y - padcmp->m_Size.y) )
        return diff;
    if( (diff = padref->m_Offset.x - padcmp->m_Offset.x) )
        return diff;
    if( (diff = padref->m_Offset.y - padcmp->m_Offset.y) )
        return diff;
    if( (diff = padref->m_DeltaSize.x - padcmp->m_DeltaSize.x) )
        return diff;
    if( (diff = padref->m_DeltaSize.y - padcmp->m_DeltaSize.y) )
        return diff;

    // @todo check if export_gencad still works:
    // specctra_export needs this, but maybe export_gencad does not.  added on
    // Jan 24 2008 by Dick.
    if( ( diff = padref->m_Masque_Layer - padcmp->m_Masque_Layer ) )
        return diff;

    return 0;
}


#if defined(DEBUG)

// @todo: could this be useable elsewhere also?
static const char* ShowPadType( int aPadType )
{
    switch( aPadType )
    {
    case PAD_CIRCLE:
        return "circle";

    case PAD_OVAL:
        return "oval";

    case PAD_RECT:
        return "rect";

    case PAD_TRAPEZOID:
        return "trap";

    default:
        return "??unknown??";
    }
}


static const char* ShowPadAttr( int aPadAttr )
{
    switch( aPadAttr )
    {
    case PAD_STANDARD:
        return "STD";

    case PAD_SMD:
        return "SMD";

    case PAD_CONN:
        return "CONN";

    case PAD_HOLE_NOT_PLATED:
        return "HOLE";

    default:
        return "??unkown??";
    }
}


/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void D_PAD::Show( int nestLevel, std::ostream& os )
{
    char padname[5] =
    { m_Padname[0], m_Padname[1], m_Padname[2], m_Padname[3], 0 };

    char layerMask[16];

    sprintf( layerMask, "0x%08X", m_Masque_Layer );

    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " shape=\"" << ShowPadType( m_PadShape ) << '"' <<
    " attr=\"" << ShowPadAttr( m_Attribut ) << '"' <<
    " num=\"" << padname << '"' <<
    " net=\"" << m_Netname.mb_str() << '"' <<
    " netcode=\"" << GetNet() << '"' <<
    " layerMask=\"" << layerMask << '"' << m_Pos << "/>\n";

//    NestedSpace( nestLevel+1, os ) << m_Text.mb_str() << '\n';

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str()
//    << ">\n";
}


#endif
