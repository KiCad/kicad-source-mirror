/////////////////////////////////////////////////////////////////////////////
// Name:        sch_sheet_pin.cpp
// Purpose:     member functions SCH_SHEET_PIN
//              header = sch_sheet.h
// Author:      jean-pierre Charras
// Modified by:
// Created:     08/02/2006 18:37:02
// RCS-ID:
// Copyright:   License GNU
// License:
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "plot_common.h"
#include "trigo.h"
#include "richio.h"
#include "class_sch_screen.h"

#include "general.h"
#include "protos.h"
#include "sch_sheet.h"


/* m_Edge define on which edge the pin is positionned:
 *
 *        0: pin on left side
 *        1: pin on right side
 *        2: pin on top side
 *        3: pin on bottom side
 *  for compatibility reasons, this does not follow same values as text
 *  ortientation.
 */

SCH_SHEET_PIN::SCH_SHEET_PIN( SCH_SHEET* parent, const wxPoint& pos, const wxString& text ) :
    SCH_HIERLABEL( pos, text, SCH_SHEET_LABEL_T )
{
    SetParent( parent );
    wxASSERT( parent );
    m_Layer = LAYER_SHEETLABEL;
    m_Pos   = pos;
    if( parent->IsVerticalOrientation() )
        SetEdge( 2 );
    else
        SetEdge( 0 );
    m_Shape = NET_INPUT;
    m_IsDangling = TRUE;
    m_Number     = 2;
}


SCH_SHEET_PIN::SCH_SHEET_PIN( const SCH_SHEET_PIN& aSheetLabel ) :
    SCH_HIERLABEL( aSheetLabel )
{
    m_Number = aSheetLabel.m_Number;
    m_Edge = aSheetLabel.m_Edge;
}


EDA_ITEM* SCH_SHEET_PIN::doClone() const
{
    return new SCH_SHEET_PIN( *this );
}


void SCH_SHEET_PIN::Draw( EDA_DRAW_PANEL* aPanel,
                          wxDC*           aDC,
                          const wxPoint&  aOffset,
                          int             aDraw_mode,
                          int             aColor )
{
    // The icon selection is handle by the virtual method CreateGraphicShape
    // called by ::Draw
    SCH_HIERLABEL::Draw( aPanel, aDC, aOffset, aDraw_mode, aColor );
}


void SCH_SHEET_PIN::SwapData( SCH_SHEET_PIN* copyitem )
{
    SCH_TEXT::SwapData( (SCH_TEXT*) copyitem );
    int tmp;
    tmp = copyitem->GetNumber();
    copyitem->SetNumber( GetNumber() );
    SetNumber( tmp );
    tmp = copyitem->GetEdge();
    copyitem->SetEdge( GetEdge() );
    SetEdge( tmp );
}


bool SCH_SHEET_PIN::operator==( const SCH_SHEET_PIN* aPin ) const
{
    return aPin == this;
}


int SCH_SHEET_PIN::GetPenSize() const
{
    return g_DrawDefaultLineThickness;
}


void SCH_SHEET_PIN::SetNumber( int aNumber )
{
    wxASSERT( aNumber >= 2 );

    m_Number = aNumber;
}


void SCH_SHEET_PIN::SetEdge( int aEdge )
{
    SCH_SHEET* Sheet = (SCH_SHEET*) GetParent();

    /* use -1 to adjust text orientation without changing edge*/
    if( aEdge > -1 )
        m_Edge = aEdge;

    switch( m_Edge )
    {
    case 0:        /* pin on left side*/
        m_Pos.x = Sheet->m_Pos.x;
        SetSchematicTextOrientation( 2 ); /* Orientation horiz inverse */
        break;

    case 1:        /* pin on right side*/
        m_Pos.x = Sheet->m_Pos.x + Sheet->m_Size.x;
        SetSchematicTextOrientation( 0 ); /* Orientation horiz normal */
        break;

    case 2:        /* pin on top side*/
        m_Pos.y = Sheet->m_Pos.y;
        SetSchematicTextOrientation( 3 ); /* Orientation vert BOTTOM  */
        break;

    case 3:        /* pin on bottom side*/
        m_Pos.y = Sheet->m_Pos.y + Sheet->m_Size.y;
        SetSchematicTextOrientation( 1 ); /* Orientation vert UP  */
        break;
    }
}


int SCH_SHEET_PIN::GetEdge()
{
    return m_Edge;
}


void SCH_SHEET_PIN::ConstraintOnEdge( wxPoint Pos )
{
    SCH_SHEET* Sheet = (SCH_SHEET*) GetParent();

    if( Sheet == NULL )
        return;

    if( m_Edge<2 ) /*horizontal sheetpin*/
    {
        if( Pos.x > ( Sheet->m_Pos.x + ( Sheet->m_Size.x / 2 ) ) )
        {
            SetEdge( 1 );
        }
        else
        {
            SetEdge( 0 );
        }

        m_Pos.y = Pos.y;
        if( m_Pos.y < Sheet->m_Pos.y )
            m_Pos.y = Sheet->m_Pos.y;
        if( m_Pos.y > (Sheet->m_Pos.y + Sheet->m_Size.y) )
            m_Pos.y = Sheet->m_Pos.y + Sheet->m_Size.y;
    }
    else /* vertical sheetpin*/
    {
        if( Pos.y > ( Sheet->m_Pos.y + ( Sheet->m_Size.y / 2 ) ) )
        {
            SetEdge( 3 ); //bottom
        }
        else
        {
            SetEdge( 2 ); //top
        }

        m_Pos.x = Pos.x;

        if( m_Pos.x < Sheet->m_Pos.x )
            m_Pos.x = Sheet->m_Pos.x;

        if( m_Pos.x > (Sheet->m_Pos.x + Sheet->m_Size.x) )
            m_Pos.x = Sheet->m_Pos.x + Sheet->m_Size.x;
    }
}


bool SCH_SHEET_PIN::Save( FILE* aFile ) const
{
    int type = 'U', side = 'L';

    if( m_Text.IsEmpty() )
        return true;

    switch( m_Edge )
    {
    case 0:     //pin on left side
        side = 'L';
        break;

    case 1:     //pin on right side
        side = 'R';
        break;

    case 2:      //pin on top side
        side = 'T';
        break;

    case 3:     //pin on bottom side
        side = 'B';
        break;
    }

    switch( m_Shape )
    {
    case NET_INPUT:
        type = 'I'; break;

    case NET_OUTPUT:
        type = 'O'; break;

    case NET_BIDI:
        type = 'B'; break;

    case NET_TRISTATE:
        type = 'T'; break;

    case NET_UNSPECIFIED:
        type = 'U'; break;
    }

    if( fprintf( aFile, "F%d \"%s\" %c %c %-3d %-3d %-3d\n", m_Number,
                 CONV_TO_UTF8( m_Text ), type, side, m_Pos.x, m_Pos.y,
                 m_Size.x ) == EOF )
    {
        return false;
    }

    return true;
}


bool SCH_SHEET_PIN::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    int  size;
    char number[256];
    char name[256];
    char connectType[256];
    char sheetSide[256];

    /* Read coordinates. */
    if( sscanf( ((char*)aLine), "%s %s %s %s %d %d %d",
                number, name, connectType, sheetSide, &m_Pos.x, &m_Pos.y, &size ) != 7 )
    {
        aErrorMsg.Printf( wxT( "EESchema file sheet hierarchical label error at line %d.\n" ),
                          aLine.LineNumber() );
        aErrorMsg << CONV_FROM_UTF8( ((char*)aLine) );
        return false;
    }

    m_Text = CONV_FROM_UTF8( name );

    m_Text = m_Text.AfterFirst( wxChar( '"' ) );
    m_Text = m_Text.BeforeLast( wxChar( '"' ) );

    if( size == 0 )
        size = DEFAULT_SIZE_TEXT;

    m_Size.x = m_Size.y = size;

    switch( connectType[0] )
    {
    case 'I':
        m_Shape = NET_INPUT;
        break;
    case 'O':
        m_Shape = NET_OUTPUT;
        break;
    case 'B':
        m_Shape = NET_BIDI;
        break;
    case 'T':
        m_Shape = NET_TRISTATE;
        break;
    case 'U':
        m_Shape = NET_UNSPECIFIED;
        break;
    }

    switch( sheetSide[0] )
    {
    case 'R' : /* pin on right side */
        SetEdge( 1 );
        break;
    case 'T' : /* pin on top side */
        SetEdge( 2 );
        break;
    case 'B' : /* pin on bottom side */
        SetEdge( 3 );
        break;
    case 'L' : /* pin on left side */
    default  :
        SetEdge( 0 );
        break;
    }

    return true;
}


bool SCH_SHEET_PIN::Matches( wxFindReplaceData& aSearchData,
                             void* aAuxData, wxPoint * aFindLocation )
{
    if( SCH_ITEM::Matches( m_Text, aSearchData ) )
    {
        if( aFindLocation )
            *aFindLocation = m_Pos;

        return true;
    }

    return false;
}


void SCH_SHEET_PIN::Mirror_X( int aXaxis_position )
{
    int p = m_Pos.y - aXaxis_position;

    m_Pos.y = aXaxis_position - p;

    switch( m_Edge )
    {
    case 2:
        SetEdge( 3 );
        break;

    case 3:
        SetEdge( 2 );
        break;
    }
}


void SCH_SHEET_PIN::Mirror_Y( int aYaxis_position )
{
    int p = m_Pos.x - aYaxis_position;

    m_Pos.x = aYaxis_position - p;

    switch( m_Edge )
    {
    case 0:
        SetEdge( 1 );
        break;

    case 1:
        SetEdge( 0 );
        break;
    }
}


void SCH_SHEET_PIN::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );

    switch( m_Edge )
    {
    case 0:     //pin on left side
        SetEdge( 3 );
        break;

    case 1:     //pin on right side
        SetEdge( 2 );
        break;

    case 2:      //pin on top side
        SetEdge( 0 );
        break;

    case 3:     //pin on bottom side
        SetEdge( 1 );
        break;
    }
}


void SCH_SHEET_PIN::CreateGraphicShape( std::vector <wxPoint>& aCorner_list,
                                        const wxPoint&         aPos )
{
     /* This is the same icon shapes as SCH_HIERLABEL
     * but the graphic icon is slightly different in 2 cases:
     * for INPUT type the icon is the OUTPUT shape of SCH_HIERLABEL
     * for OUTPUT type the icon is the INPUT shape of SCH_HIERLABEL
     */
    int tmp = m_Shape;

    switch( m_Shape )
    {
    case NET_INPUT:
        m_Shape = NET_OUTPUT;
        break;

    case NET_OUTPUT:
        m_Shape = NET_INPUT;
        break;

    default:
        break;
    }

    SCH_HIERLABEL::CreateGraphicShape( aCorner_list, aPos );
    m_Shape = tmp;
}


void SCH_SHEET_PIN::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    DANGLING_END_ITEM item( SHEET_LABEL_END, this );
    item.m_Pos  = m_Pos;
    aItemList.push_back( item );
}


#if defined(DEBUG)

void SCH_SHEET_PIN::Show( int nestLevel, std::ostream& os )
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << ">"
                                 << " pin_name=\"" << CONV_TO_UTF8( m_Text )
                                 << '"' << "/>\n" << std::flush;

//    NestedSpace( nestLevel, os ) << "</" << s.Lower().mb_str() << ">\n";
}

#endif
