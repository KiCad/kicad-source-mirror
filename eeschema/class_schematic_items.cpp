/***********************************************************************/
/* Methodes de base de gestion des classes des elements de schematique */
/***********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "id.h"

#include "protos.h"

/****************************/
/* class DrawBusEntryStruct */
/***************************/

const wxChar*  NameMarqueurType[] =
{
    wxT( "" ),
    wxT( "ERC" ),
    wxT( "PCB" ),
    wxT( "SIMUL" ),
    wxT( "?????" )
};


DrawBusEntryStruct::DrawBusEntryStruct( const wxPoint& pos, int shape, int id ) :
    SCH_ITEM( NULL, DRAW_BUSENTRY_STRUCT_TYPE )
{
    m_Pos    = pos;
    m_Size.x = 100;
    m_Size.y = 100;
    m_Layer  = LAYER_WIRE;
    m_Width  = 0;

    if( id == BUS_TO_BUS )
    {
        m_Layer = LAYER_BUS;
    }

    if( shape == '/' )
        m_Size.y = -100;
}


wxPoint DrawBusEntryStruct::m_End() const
{
    return wxPoint( m_Pos.x + m_Size.x, m_Pos.y + m_Size.y );
}


DrawBusEntryStruct* DrawBusEntryStruct::GenCopy()
{
    DrawBusEntryStruct* newitem = new DrawBusEntryStruct( m_Pos, 0, 0 );

    newitem->m_Layer = m_Layer;
    newitem->m_Width = m_Width;
    newitem->m_Size  = m_Size;
    newitem->m_Flags = m_Flags;

    return newitem;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool DrawBusEntryStruct::Save( FILE* aFile ) const
{
    bool        success = true;

    const char* layer = "Wire";
    const char* width = "Line";

    if( GetLayer() == LAYER_BUS )
    {
        layer = "Bus"; width = "Bus";
    }

    if( fprintf( aFile, "Entry %s %s\n", layer, width ) == EOF )
    {
        success = false;
    }
    if( fprintf( aFile, "\t%-4d %-4d %-4d %-4d\n",
                 m_Pos.x, m_Pos.y, m_End().x, m_End().y ) == EOF )
    {
        success = false;
    }

    return success;
}


EDA_Rect DrawBusEntryStruct::GetBoundingBox()
{
    int      dx = m_Pos.x - m_End().x;
    int      dy = m_Pos.y - m_End().y;
    EDA_Rect box( wxPoint( m_Pos.x, m_Pos.y ), wxSize( dx, dy ) );

    box.Normalize();
    int width = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;
    box.Inflate( width / 2, width / 2 );

    return box;
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int DrawBusEntryStruct::GetPenSize( )
{
    int pensize = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;
    if( m_Layer == LAYER_BUS )      // TODO: find a better way to handle bus thickness
    {
        pensize = wxRound(pensize * 1.3);
        pensize = MAX(pensize, 3);
    }

    return pensize;
}


void DrawBusEntryStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                               const wxPoint& offset, int DrawMode, int Color )
{
    int color;

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    GRLine( &panel->m_ClipBox, DC, m_Pos.x + offset.x, m_Pos.y + offset.y,
            m_End().x + offset.x, m_End().y + offset.y, GetPenSize( ), color );
}


/****************************/
/* class DrawJunctionStruct */
/***************************/

DrawJunctionStruct::DrawJunctionStruct( const wxPoint& pos ) :
    SCH_ITEM( NULL, DRAW_JUNCTION_STRUCT_TYPE )
{
    m_Pos   = pos;
    m_Layer = LAYER_JUNCTION;
}


DrawJunctionStruct* DrawJunctionStruct::GenCopy()
{
    DrawJunctionStruct* newitem = new DrawJunctionStruct( m_Pos );

    newitem->m_Layer = m_Layer;
    newitem->m_Flags = m_Flags;

    return newitem;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool DrawJunctionStruct::Save( FILE* aFile ) const
{
    bool success = true;

    if( fprintf( aFile, "Connection ~ %-4d %-4d\n", m_Pos.x, m_Pos.y ) == EOF )
    {
        success = false;
    }

    return success;
}


EDA_Rect DrawJunctionStruct::GetBoundingBox()
// return a bounding box
{
    int      width = DRAWJUNCTION_SIZE * 2;
    int      xmin  = m_Pos.x - DRAWJUNCTION_SIZE;
    int      ymin  = m_Pos.y - DRAWJUNCTION_SIZE;

    EDA_Rect ret( wxPoint( xmin, ymin ), wxSize( width, width ) );

    return ret;
};


/** Function HitTest
 * @return true if the point aPosRef is within item area
 * @param aPosRef = a wxPoint to test
 */
bool DrawJunctionStruct::HitTest( const wxPoint& aPosRef )
{
    wxPoint dist = aPosRef - m_Pos;

    return sqrt( ( (double) ( dist.x * dist.x ) ) +
                 ( (double) ( dist.y * dist.y ) ) ) < DRAWJUNCTION_SIZE;
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 * has no meaning for DrawJunctionStruct
 */
int DrawJunctionStruct::GetPenSize( )
{
    return 0;
}

/*****************************************************************************
* Routine to redraw connection struct.										 *
*****************************************************************************/
void DrawJunctionStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                               const wxPoint& offset, int DrawMode, int Color )
{
    int color;

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    GRFilledCircle( &panel->m_ClipBox, DC, m_Pos.x + offset.x,
                    m_Pos.y + offset.y, DRAWJUNCTION_SIZE, 0, color, color );
}


#if defined(DEBUG)
void DrawJunctionStruct::Show( int nestLevel, std::ostream& os )
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str()
        << m_Pos << "/>\n";
}

#endif


/*****************************/
/* class DrawNoConnectStruct */
/*****************************/

DrawNoConnectStruct::DrawNoConnectStruct( const wxPoint& pos ) :
    SCH_ITEM( NULL, DRAW_NOCONNECT_STRUCT_TYPE )
{
    m_Pos = pos;
}


DrawNoConnectStruct* DrawNoConnectStruct::GenCopy()
{
    DrawNoConnectStruct* newitem = new DrawNoConnectStruct( m_Pos );

    newitem->m_Flags = m_Flags;

    return newitem;
}


EDA_Rect DrawNoConnectStruct::GetBoundingBox()
{
    const int DELTA = DRAWNOCONNECT_SIZE / 2;
    EDA_Rect  box( wxPoint( m_Pos.x - DELTA, m_Pos.y - DELTA ),
                   wxSize( 2 * DELTA, 2 * DELTA ) );

    box.Normalize();
    return box;
}


/**
 * Function HitTest
 * @return true if the point aPosRef is within item area
 * @param aPosRef = a wxPoint to test
 */
bool DrawNoConnectStruct::HitTest( const wxPoint& aPosRef )
{
    int width = g_DrawDefaultLineThickness;
    int delta = ( DRAWNOCONNECT_SIZE + width) / 2;

    wxPoint dist = aPosRef - m_Pos;
    if( (ABS(dist.x) <= delta) && (ABS(dist.y) <= delta) )
        return true;
    return false;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool DrawNoConnectStruct::Save( FILE* aFile ) const
{
    bool success = true;

    if( fprintf( aFile, "NoConn ~ %-4d %-4d\n", m_Pos.x, m_Pos.y ) == EOF )
    {
        success = false;
    }

    return success;
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int DrawNoConnectStruct::GetPenSize( )
{
    return g_DrawDefaultLineThickness;
}

void DrawNoConnectStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                                const wxPoint& offset, int DrawMode, int Color )
{
    const int DELTA = (DRAWNOCONNECT_SIZE / 2);
    int       pX, pY, color;
    int       width = g_DrawDefaultLineThickness;

    pX = m_Pos.x + offset.x; pY = m_Pos.y + offset.y;

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( LAYER_NOCONNECT );
    GRSetDrawMode( DC, DrawMode );

    GRLine( &panel->m_ClipBox, DC, pX - DELTA, pY - DELTA, pX + DELTA,
            pY + DELTA, width, color );
    GRLine( &panel->m_ClipBox, DC, pX + DELTA, pY - DELTA, pX - DELTA,
            pY + DELTA, width, color );
}


/**************************/
/* class DrawMarkerStruct */
/**************************/

char marq_bitmap[] =
{
    12, 12, 0,  0, /* Dimensions x et y, offsets x et y du bitmap de marqueurs*/
    YELLOW,        /* Couleur */
    1,  1,  1,  1, 1, 1, 1, 1, 0, 0, 0, 0,  /* bitmap: >= 1 : color, */
    1,  1,  1,  0, 1, 0, 1, 1, 0, 0, 0, 0,  /*  0 = notrace */
    1,  1,  1,  1, 0, 0, 0, 1, 0, 0, 0, 0,
    1,  0,  1,  1, 1, 0, 0, 0, 0, 0, 0, 0,
    1,  1,  0,  1, 1, 1, 0, 0, 0, 0, 0, 0,
    1,  1,  0,  0, 1, 1, 1, 0, 0, 0, 0, 0,
    1,  1,  1,  0, 0, 1, 1, 1, 0, 0, 0, 0,
    0,  0,  0,  0, 0, 0, 1, 1, 1, 0, 0, 0,
    0,  0,  0,  0, 0, 0, 0, 1, 1, 1, 0, 0,
    0,  0,  0,  0, 0, 0, 0, 0, 1, 1, 1, 0,
    0,  0,  0,  0, 0, 0, 0, 0, 0, 1, 1, 1,
    0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 1, 0
};

char marqERC_bitmap[] =
{
    8, 8, 0, 0, /* Dimensions x et y , offsets x et y du bitmap de marqueurs*/
    -1,         /* Color: -1 = couleur non pr�cis�e */
    1, 1, 1, 1, 1, 0, 0, 0,
    1, 1, 1, 0, 1, 0, 0, 0,
    1, 1, 1, 1, 0, 0, 0, 0,
    1, 0, 1, 1, 1, 0, 0, 0,
    1, 1, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 0,
};


DrawMarkerStruct::DrawMarkerStruct( const wxPoint& pos, const wxString& text ) :
    SCH_ITEM( NULL, DRAW_MARKER_STRUCT_TYPE )
{
    m_Pos       = pos;              /* XY coordinates of marker. */
    m_Type      = MARQ_UNSPEC;
    m_MarkFlags = 0;                // complements d'information
    m_Comment   = text;
}


DrawMarkerStruct::~DrawMarkerStruct()
{
}


DrawMarkerStruct* DrawMarkerStruct::GenCopy()
{
    DrawMarkerStruct* newitem = new DrawMarkerStruct( m_Pos, m_Comment );

    newitem->m_Type      = m_Type;
    newitem->m_MarkFlags = m_MarkFlags;

    return newitem;
}


wxString DrawMarkerStruct::GetComment()
{
    return m_Comment;
}


#if defined (DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void DrawMarkerStruct::Show( int nestLevel, std::ostream& os )
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << m_Pos
                                 << "/>\n";
}
#endif

/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool DrawMarkerStruct::Save( FILE* aFile ) const
{
    bool success = true;

    if( fprintf( aFile, "Kmarq %c %-4d %-4d \"%s\" F=%X\n",
                 int( m_Type ) + 'A', m_Pos.x, m_Pos.y,
                 CONV_TO_UTF8( m_Comment ), m_MarkFlags ) == EOF )
    {
        success = false;
    }

    return success;
}


void DrawMarkerStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                             const wxPoint& offset, int DrawMode, int Color )
{
#define WAR 1   // utilis� aussi dans erc.cpp

    if( m_Type == MARQ_ERC )
    {
        int color = Color;
        if( Color <= 0 )
        {
            color = (m_MarkFlags == WAR ) ?
                g_LayerDescr.LayerColor[LAYER_ERC_WARN] :
                g_LayerDescr.LayerColor[LAYER_ERC_ERR];
        }

        Draw_Marqueur( panel, DC, m_Pos + offset, marqERC_bitmap, DrawMode,
                       color );
    }
    else
        Draw_Marqueur( panel, DC, m_Pos + offset, marq_bitmap, DrawMode, Color );
}


/***************************/
/* Class EDA_DrawLineStruct */
/***************************/

EDA_DrawLineStruct::EDA_DrawLineStruct( const wxPoint& pos, int layer ) :
    SCH_ITEM( NULL, DRAW_SEGMENT_STRUCT_TYPE )
{
    m_Start = pos;
    m_End   = pos;
    m_Width = 0;        // Default thickness used
    m_StartIsDangling = m_EndIsDangling = FALSE;

    switch( layer )
    {
    default:
        m_Layer = LAYER_NOTES;     /* Mettre ds Notes */
        break;

    case LAYER_WIRE:
        m_Layer = LAYER_WIRE;
        break;

    case LAYER_BUS:
        m_Layer = LAYER_BUS;
        break;
    }
}


EDA_DrawLineStruct* EDA_DrawLineStruct::GenCopy()
{
    EDA_DrawLineStruct* newitem = new EDA_DrawLineStruct( m_Start, m_Layer );

    newitem->m_End = m_End;

    return newitem;
}


bool EDA_DrawLineStruct::IsOneEndPointAt( const wxPoint& pos )
{
    if( (pos.x == m_Start.x) && (pos.y == m_Start.y) )
        return TRUE;
    if( (pos.x == m_End.x) && (pos.y == m_End.y) )
        return TRUE;
    return FALSE;
}


#if defined (DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void EDA_DrawLineStruct::Show( int nestLevel, std::ostream& os )
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " layer=\"" << m_Layer << '"' <<
    " width=\"" << m_Width << '"' <<
    " startIsDangling=\"" << m_StartIsDangling << '"' <<
    " endIsDangling=\"" << m_EndIsDangling << '"' << ">" <<
    " <start" << m_Start << "/>" <<
    " <end" << m_End << "/>" <<
    "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif


EDA_Rect EDA_DrawLineStruct::GetBoundingBox()
{
    int      width = 25;

    int      xmin = MIN( m_Start.x, m_End.x ) - width;
    int      ymin = MIN( m_Start.y, m_End.y ) - width;

    int      xmax = MAX( m_Start.x, m_End.x ) + width;
    int      ymax = MAX( m_Start.y, m_End.y ) + width;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    EDA_Rect ret( wxPoint( xmin, ymin ),
                  wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool EDA_DrawLineStruct::Save( FILE* aFile ) const
{
    bool        success = true;

    const char* layer = "Notes";
    const char* width = "Line";

    if( GetLayer() == LAYER_WIRE )
        layer = "Wire";
    if( GetLayer() == LAYER_BUS )
        layer = "Bus";
    if( fprintf( aFile, "Wire %s %s\n", layer, width ) == EOF )
    {
        success = false;
    }
    if ( fprintf( aFile, "\t%-4d %-4d %-4d %-4d\n", m_Start.x,m_Start.y,
                  m_End.x,m_End.y ) == EOF )
    {
        success = false;
    }

    return success;
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int EDA_DrawLineStruct::GetPenSize( )
{
    int pensize = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;
    return pensize;
}

void EDA_DrawLineStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                               const wxPoint& offset, int DrawMode, int Color )
{
    int color;
    int width = GetPenSize( );

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    // FIXME: Not compatable with new zoom.
    if( m_Layer == LAYER_BUS)
    {
        width = wxRound(width * 1.4);
        width = MAX(width, 3);
    }

    if( m_Layer == LAYER_NOTES )
        GRDashedLine( &panel->m_ClipBox, DC, m_Start.x + offset.x,
                      m_Start.y + offset.y, m_End.x + offset.x,
                      m_End.y + offset.y, width, color );
    else
        GRLine( &panel->m_ClipBox, DC, m_Start.x + offset.x,
                m_Start.y + offset.y, m_End.x + offset.x, m_End.y + offset.y,
                width, color );

    if( m_StartIsDangling )
        DrawDanglingSymbol( panel, DC, m_Start + offset, color );

    if( m_EndIsDangling )
        DrawDanglingSymbol( panel, DC, m_End + offset, color );
}


/****************************/
/* Class DrawPolylineStruct */
/****************************/

DrawPolylineStruct::DrawPolylineStruct( int layer ) :
    SCH_ITEM( NULL, DRAW_POLYLINE_STRUCT_TYPE )
{
    m_Width  = 0;

    switch( layer )
    {
    default:
        m_Layer = LAYER_NOTES;
        break;

    case LAYER_WIRE:
    case LAYER_NOTES:
    case LAYER_BUS:
        m_Layer = layer;
        break;
    }
}


DrawPolylineStruct::~DrawPolylineStruct()
{
}


DrawPolylineStruct* DrawPolylineStruct::GenCopy()
{
    DrawPolylineStruct* newitem = new DrawPolylineStruct( m_Layer );
    newitem->m_PolyPoints = m_PolyPoints;   // std::vector copy
    return newitem;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool DrawPolylineStruct::Save( FILE* aFile ) const
{
    bool        success = true;

    const char* layer = "Notes";
    const char* width = "Line";

    if( GetLayer() == LAYER_WIRE )
        layer = "Wire";
    if( GetLayer() == LAYER_BUS )
        layer = "Bus";
    if( fprintf( aFile, "Poly %s %s %d\n",
                 width, layer, GetCornerCount() ) == EOF )
    {
        return false;
    }
    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        if( fprintf( aFile, "\t%-4d %-4d\n",
                     m_PolyPoints[ii ].x, m_PolyPoints[ii].y ) == EOF )
        {
            success = false;
            break;
        }
    }

    return success;
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int DrawPolylineStruct::GetPenSize( )
{
    int pensize = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;
    return pensize;
}

void DrawPolylineStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                               const wxPoint& offset, int DrawMode, int Color )
{
    int color;
    int width = GetPenSize( );

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    if( m_Layer == LAYER_BUS )
    {
        width *= 3;
    }

    GRMoveTo( m_PolyPoints[0].x, m_PolyPoints[0].y );

    if( m_Layer == LAYER_NOTES )
    {
        for( unsigned i = 1; i < GetCornerCount(); i++ )
            GRDashedLineTo( &panel->m_ClipBox, DC, m_PolyPoints[i].x + offset.x,
                            m_PolyPoints[i].y + offset.y, width, color );
    }
    else
    {
        for( unsigned i = 1; i < GetCornerCount(); i++ )
            GRLineTo( &panel->m_ClipBox, DC, m_PolyPoints[i].x + offset.x,
                      m_PolyPoints[i].y + offset.y, width, color );
    }
}
