/*****************************************************************************/
/* Functions to handle markers used to show somthing (usually a drc problem) */
/*****************************************************************************/

/* file class_marker.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "trigo.h"

#include "pcbnew.h"
#include "class_marker_pcb.h"

#define SCALING_FACTOR 30       // Adjust the actual size of markers, when using default shape

/*******************/
/* Classe MARKER_PCB */
/*******************/

MARKER_PCB::MARKER_PCB( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, TYPE_MARKER_PCB ),
    MARKER_BASE( )
{
    m_Color = WHITE;
    m_ScalingFactor = SCALING_FACTOR;
}


MARKER_PCB::MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
               const wxString& aText, const wxPoint& aPos,
               const wxString& bText, const wxPoint& bPos ) :
    BOARD_ITEM( NULL, TYPE_MARKER_PCB ),  // parent set during BOARD::Add()
    MARKER_BASE(  aErrorCode, aMarkerPos, aText, aPos, bText, bPos )

{
    m_Color = WHITE;
    m_ScalingFactor = SCALING_FACTOR;
}

MARKER_PCB::MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
           const wxString& aText, const wxPoint& aPos ) :
    BOARD_ITEM( NULL, TYPE_MARKER_PCB ),  // parent set during BOARD::Add()
    MARKER_BASE( aErrorCode, aMarkerPos, aText,  aPos )
{
    m_Color = WHITE;
    m_ScalingFactor = SCALING_FACTOR;
}


/* destructor */
MARKER_PCB::~MARKER_PCB()
{
}


void MARKER_PCB::DisplayInfo( WinEDA_DrawFrame* frame )
{
    frame->ClearMsgPanel();

    const DRC_ITEM& rpt = m_drc;

    frame->AppendMsgPanel( _( "Type" ), _("Marker"), DARKCYAN );

    wxString errorTxt;

    errorTxt << _("ErrType") << wxT("(") << rpt.GetErrorCode() << wxT(")-  ") << rpt.GetErrorText() << wxT(":");

    frame->AppendMsgPanel( errorTxt, wxEmptyString, RED );

    wxString txtA;
    txtA << DRC_ITEM::ShowCoord( rpt.GetPointA() ) << wxT(": ") << rpt.GetTextA();

    wxString txtB;
    if ( rpt.HasSecondItem() )
        txtB << DRC_ITEM::ShowCoord( rpt.GetPointB() ) << wxT(": ") << rpt.GetTextB();

    frame->AppendMsgPanel( txtA, txtB, DARKBROWN );
}

/**
 * Function Rotate
 * Rotate this object.
 * @param aRotCentre - the rotation point.
 * @param aAngle - the rotation angle in 0.1 degree.
 */
void MARKER_PCB::Rotate(const wxPoint& aRotCentre, int aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
}

/**
 * Function Flip
 * Flip this object, i.e. change the board side for this object
 * this function has not reeally sense for a marker.
 * It moves just the marker to keep its position on board, when the board is flipped
 * @param aCentre - the rotation point.
 */
void MARKER_PCB::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - (m_Pos.y - aCentre.y);
}

