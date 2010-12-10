/*******************************************/
/* Schematic marker object implementation. */
/*******************************************/

#include "fctsys.h"
#include "wxstruct.h"
#include "class_drawpanel.h"
#include "common.h"
#include "trigo.h"

#include "general.h"
#include "sch_marker.h"
#include "erc.h"


/* Marker are mainly used to show an ERC error
 * but they could be used to give a specific info
 */


const wxChar* NameMarqueurType[] =
{
    wxT( "" ),
    wxT( "ERC" ),
    wxT( "PCB" ),
    wxT( "SIMUL" ),
    wxT( "?????" )
};


/**************************/
/* class SCH_MARKER */
/**************************/

SCH_MARKER::SCH_MARKER() : SCH_ITEM( NULL, SCH_MARKER_T ), MARKER_BASE()
{
}


SCH_MARKER::SCH_MARKER( const wxPoint& pos, const wxString& text ) :
    SCH_ITEM( NULL, SCH_MARKER_T ),
    MARKER_BASE( 0, pos, text, pos )
{
}


SCH_MARKER::~SCH_MARKER()
{
}


SCH_MARKER* SCH_MARKER::GenCopy()
{
    SCH_MARKER* newitem = new SCH_MARKER( GetPos(), GetReporter().GetMainText() );

    newitem->SetMarkerType( GetMarkerType() );
    newitem->SetErrorLevel( GetErrorLevel() );

    return newitem;
}


#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void SCH_MARKER::Show( int nestLevel, std::ostream& os )
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << GetPos() << "/>\n";
}


#endif

/**
 * Function Save (do nothing : markers are no more saved in files )
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_MARKER::Save( FILE* aFile ) const
{
    return true;
}


void SCH_MARKER::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                       const wxPoint& aOffset, int aDrawMode, int aColor )
{
    EDA_Colors color = (EDA_Colors) m_Color;
    EDA_Colors tmp   = color;

    if( GetMarkerType() == MARK_ERC )
    {
        color = ( GetErrorLevel() == WAR ) ?
                (EDA_Colors) g_LayerDescr.LayerColor[LAYER_ERC_WARN] :
                (EDA_Colors) g_LayerDescr.LayerColor[LAYER_ERC_ERR];
    }

    if( aColor < 0 )
        m_Color = color;
    else
        m_Color = (EDA_Colors) aColor;

    DrawMarker( aPanel, aDC, aDrawMode, aOffset );
    m_Color = tmp;
}


bool SCH_MARKER::Matches( wxFindReplaceData& aSearchData, wxPoint * aFindLocation )
{
    if( !SCH_ITEM::Matches( m_drc.GetMainText(), aSearchData ) )
    {
        if( SCH_ITEM::Matches( m_drc.GetAuxiliaryText(), aSearchData ) )
        {
            if( aFindLocation )
                *aFindLocation = m_Pos;
            return true;
        }
        return false;
    }

    if( aFindLocation )
        *aFindLocation = m_Pos;
    return true;
}


/**
 * Function GetBoundingBox
 * returns the orthogonal, bounding box of this object for display purposes.
 * This box should be an enclosing perimeter for visible components of this
 * object, and the units should be in the pcb or schematic coordinate system.
 * It is OK to overestimate the size by a few counts.
 */
EDA_Rect SCH_MARKER::GetBoundingBox() const
{
    return GetBoundingBoxMarker();
}


void SCH_MARKER::DisplayInfo( WinEDA_DrawFrame* aFrame )
{
    if( aFrame == NULL )
        return;

    wxString msg;

    aFrame->ClearMsgPanel();
    aFrame->AppendMsgPanel( _( "Electronics rule check error" ),
                            GetReporter().GetErrorText(), DARKRED );
}


void SCH_MARKER::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
}


void SCH_MARKER::Mirror_X( int aXaxis_position )
{
    m_Pos.y -= aXaxis_position;
    m_Pos.y  = -m_Pos.y;
    m_Pos.y += aXaxis_position;
}


void SCH_MARKER::Mirror_Y( int aYaxis_position )
{
    m_Pos.x -= aYaxis_position;
    m_Pos.x  = -m_Pos.x;
    m_Pos.x += aYaxis_position;
}


bool SCH_MARKER::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    if( aRect.Contains( m_Pos ) )
        m_Flags |= SELECTED;
    else
        m_Flags &= ~SELECTED;

    return previousState != IsSelected();
}
