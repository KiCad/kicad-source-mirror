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

    m_Size.x = m_Size.y = 500;          // give it a reasonable size
    m_Orient = 0;                       // Pad rotation in 1/10 degrees
    m_LengthDie = 0;

    if( m_Parent && (m_Parent->Type()  == PCB_MODULE_T) )
    {
        m_Pos = ( (MODULE*) m_Parent )->GetPosition();
    }

    m_PadShape = PAD_CIRCLE;                        // Shape: PAD_CIRCLE, PAD_RECT PAD_OVAL
                                                    // PAD_TRAPEZOID
    m_Attribut = PAD_STANDARD;                      // Type: NORMAL, PAD_SMD, PAD_CONN
    m_DrillShape     = PAD_CIRCLE;                  // Drill shape = circle
    m_LocalClearance = 0;
    m_LocalSolderMaskMargin  = 0;
    m_LocalSolderPasteMargin = 0;
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
        radius = m_Size.x / 2;
        break;

    case PAD_OVAL:
        radius = MAX( m_Size.x, m_Size.y ) / 2;
        break;

    case PAD_RECT:
        radius = 1 + (int) ( sqrt( (double) m_Size.y * m_Size.y
                                   + (double) m_Size.x * m_Size.x ) / 2 );
        break;

    case PAD_TRAPEZOID:
        x = m_Size.x + ABS( m_DeltaSize.y );   // Remember: m_DeltaSize.y is the m_Size.x change
        y = m_Size.y + ABS( m_DeltaSize.x );   // Remember: m_DeltaSize.x is the m_Size.y change
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

    area.SetOrigin( m_Pos );
    area.Inflate( radius );

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


const wxString D_PAD::GetPadName() const
{
#if 0   // m_Padname is not ASCII and not UTF8, it is LATIN1 basically, whatever
        // 8 bit font is supported in KiCad plotting and drawing.

    // Return pad name as wxString, assume it starts as a non-terminated
    // utf8 character sequence

    char    temp[sizeof(m_Padname)+1];      // a place to terminate with '\0'

    strncpy( temp, m_Padname, sizeof(m_Padname) );

    temp[sizeof(m_Padname)] = 0;

    return FROM_UTF8( temp );
#else

    wxString name;

    ReturnStringPadName( name );
    return name;
#endif
}


void D_PAD::ReturnStringPadName( wxString& text ) const
{
#if 0   // m_Padname is not ASCII and not UTF8, it is LATIN1 basically, whatever
        // 8 bit font is supported in KiCad plotting and drawing.

    // Return pad name as wxString, assume it starts as a non-terminated
    // utf8 character sequence

    char    temp[sizeof(m_Padname)+1];      // a place to terminate with '\0'

    strncpy( temp, m_Padname, sizeof(m_Padname) );

    temp[sizeof(m_Padname)] = 0;

    text = FROM_UTF8( temp );

#else

    text.Empty();

    for( int ii = 0;  ii < PADNAMEZ && m_Padname[ii];  ii++ )
    {
        // m_Padname is 8 bit KiCad font junk, do not sign extend
        text.Append( (unsigned char) m_Padname[ii] );
    }
#endif
}


// Change pad name
void D_PAD::SetPadName( const wxString& name )
{
    int ii, len;

    len = name.Length();

    if( len > PADNAMEZ )
        len = PADNAMEZ;

    // m_Padname[] is not UTF8, it is an 8 bit character that matches the KiCad font,
    // so only copy the lower 8 bits of each character.

    for( ii = 0; ii < len; ii++ )
        m_Padname[ii] = (char) name.GetChar( ii );

    for( ii = len; ii < PADNAMEZ; ii++ )
        m_Padname[ii] = '\0';
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
    int clearance = m_LocalClearance;

    if( clearance == 0 )
    {   // If local clearance is 0, use the parent footprint clearance value
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
    int margin = m_LocalSolderMaskMargin;
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
            margin = brd->GetDesignSettings().m_SolderMaskMargin;
        }
    }

    // ensure mask have a size always >= 0
    if( margin < 0 )
    {
        int minsize = -MIN( m_Size.x, m_Size.y ) / 2;

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
    int margin = m_LocalSolderPasteMargin;
    double mratio = m_LocalSolderPasteMarginRatio;
    MODULE * module = (MODULE*) GetParent();

    if( module )
    {
        if( margin == 0  )
            margin = module->m_LocalSolderPasteMargin;

        BOARD * brd = GetBoard();

        if( margin == 0  )
            margin = brd->GetDesignSettings().m_SolderPasteMargin;

        if( mratio == 0.0 )
            mratio = module->m_LocalSolderPasteMarginRatio;

        if( mratio == 0.0 )
        {
           mratio = brd->GetDesignSettings().m_SolderPasteMarginRatio;
        }
    }

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
    Line.Printf( wxT( "%d-%d-%d " ), GetSubRatsnest(), GetSubNet(), GetZoneSubNet() );
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

    int module_orient = module ? module->GetOrientation() : 0;

    if( module_orient )
        Line.Printf( wxT( "%3.1f(+%3.1f)" ),
                     (float) ( m_Orient - module_orient ) / 10,
                     (float) module_orient / 10 );
    else
        Line.Printf( wxT( "%3.1f" ), (float) m_Orient / 10 );

    frame->AppendMsgPanel( _( "Orient" ), Line, LIGHTBLUE );

    valeur_param( m_Pos.x, Line );
    frame->AppendMsgPanel( _( "X Pos" ), Line, LIGHTBLUE );

    valeur_param( m_Pos.y, Line );
    frame->AppendMsgPanel( _( "Y pos" ), Line, LIGHTBLUE );

    if( m_LengthDie )
    {
        valeur_param( m_LengthDie, Line );
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

    dx = m_Size.x >> 1; // dx also is the radius for rounded pads
    dy = m_Size.y >> 1;

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

    if( ( diff = padref->m_PadShape - padcmp->m_PadShape ) != 0 )
        return diff;

    if( ( diff = padref->m_DrillShape - padcmp->m_DrillShape ) != 0)
        return diff;

    if( ( diff = padref->m_Drill.x - padcmp->m_Drill.x ) != 0 )
        return diff;

    if( ( diff = padref->m_Drill.y - padcmp->m_Drill.y ) != 0 )
        return diff;

    if( ( diff = padref->m_Size.x - padcmp->m_Size.x ) != 0 )
        return diff;

    if( ( diff = padref->m_Size.y - padcmp->m_Size.y ) != 0 )
        return diff;

    if( ( diff = padref->m_Offset.x - padcmp->m_Offset.x ) != 0 )
        return diff;

    if( ( diff = padref->m_Offset.y - padcmp->m_Offset.y ) != 0 )
        return diff;

    if( ( diff = padref->m_DeltaSize.x - padcmp->m_DeltaSize.x ) != 0 )
        return diff;

    if( ( diff = padref->m_DeltaSize.y - padcmp->m_DeltaSize.y ) != 0 )
        return diff;

    // Dick: specctra_export needs this
    // Lorenzo: gencad also needs it to implement padstacks!
    if( ( diff = padref->m_layerMask - padcmp->m_layerMask ) != 0 )
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
                 GetChars(GetPadName() ), GetChars( padlayers ),
                 GetChars(( (MODULE*) GetParent() )->GetReference() ) );

    return text;
}

#if defined(DEBUG)

void D_PAD::Show( int nestLevel, std::ostream& os ) const
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
