/*****************************************************************************/
/* Functions to handle markers used to show something (usually a drc problem) */
/*****************************************************************************/

/* file class_marker.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "wxstruct.h"
#include "trigo.h"

#include "pcbnew.h"
#include "class_marker_pcb.h"


#define SCALING_FACTOR 30       // Adjust the actual size of markers, when using default shape


MARKER_PCB::MARKER_PCB( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, PCB_MARKER_T ),
    MARKER_BASE( )
{
    m_Color = WHITE;
    m_ScalingFactor = SCALING_FACTOR;
}


MARKER_PCB::MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                        const wxString& aText, const wxPoint& aPos,
                        const wxString& bText, const wxPoint& bPos ) :
    BOARD_ITEM( NULL, PCB_MARKER_T ),  // parent set during BOARD::Add()
    MARKER_BASE( aErrorCode, aMarkerPos, aText, aPos, bText, bPos )

{
    m_Color = WHITE;
    m_ScalingFactor = SCALING_FACTOR;
}

MARKER_PCB::MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                        const wxString& aText, const wxPoint& aPos ) :
    BOARD_ITEM( NULL, PCB_MARKER_T ),  // parent set during BOARD::Add()
    MARKER_BASE( aErrorCode, aMarkerPos, aText,  aPos )
{
    m_Color = WHITE;
    m_ScalingFactor = SCALING_FACTOR;
}


/* destructor */
MARKER_PCB::~MARKER_PCB()
{
}


void MARKER_PCB::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    frame->ClearMsgPanel();

    const DRC_ITEM& rpt = m_drc;

    frame->AppendMsgPanel( _( "Type" ), _( "Marker" ), DARKCYAN );

    wxString errorTxt;

    errorTxt << _( "ErrType" ) << wxT( "(" ) << rpt.GetErrorCode() << wxT( ")-  " )
             << rpt.GetErrorText() << wxT( ":" );

    frame->AppendMsgPanel( errorTxt, wxEmptyString, RED );

    wxString txtA;
    txtA << DRC_ITEM::ShowCoord( rpt.GetPointA() ) << wxT( ": " ) << rpt.GetTextA();

    wxString txtB;

    if ( rpt.HasSecondItem() )
        txtB << DRC_ITEM::ShowCoord( rpt.GetPointB() ) << wxT( ": " ) << rpt.GetTextB();

    frame->AppendMsgPanel( txtA, txtB, DARKBROWN );
}


void MARKER_PCB::Rotate(const wxPoint& aRotCentre, double aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
}


void MARKER_PCB::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - (m_Pos.y - aCentre.y);
}


wxString MARKER_PCB::GetSelectMenuText() const
{
    wxString text;

    text << _( "Marker" ) << wxT( " @(" ) << GetPos().x << wxT( "," ) << GetPos().y << wxT( ")" );

    return text;
}
