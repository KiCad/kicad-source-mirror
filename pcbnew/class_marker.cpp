/*****************************************************************************/
/* Functions to handle markers used to show somthing (usually a drc problem) */
/*****************************************************************************/

/* file class_marker.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "class_marker.h"



/*******************/
/* Classe MARKER */
/*******************/

MARKER::MARKER( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, TYPE_MARKER ),
    MARKER_BASE( )
{
}


MARKER::MARKER( int aErrorCode, const wxPoint& aMarkerPos,
               const wxString& aText, const wxPoint& aPos,
               const wxString& bText, const wxPoint& bPos ) :
    BOARD_ITEM( NULL, TYPE_MARKER ),  // parent set during BOARD::Add()
    MARKER_BASE(  aErrorCode, aMarkerPos, aText, aPos, bText, bPos )

{
}

MARKER::MARKER( int aErrorCode, const wxPoint& aMarkerPos,
           const wxString& aText, const wxPoint& aPos ) :
    BOARD_ITEM( NULL, TYPE_MARKER ),  // parent set during BOARD::Add()
    MARKER_BASE( aErrorCode, aMarkerPos, aText,  aPos )
{
}


/* Effacement memoire de la structure */
MARKER::~MARKER()
{
}

/* supprime du chainage la structure Struct
 *  les structures arrieres et avant sont chainees directement
 */
void MARKER::UnLink()
{
    wxFAIL_MSG( wxT("MARKER::UnLink is deprecated") );
}


void MARKER::DisplayInfo( WinEDA_DrawFrame* frame )
{
    int      text_pos;

    frame->MsgPanel->EraseMsgBox();

    const DRC_ITEM& rpt = m_drc;

    text_pos = 1;
    Affiche_1_Parametre( frame, text_pos, _( "Type" ), _("Marker"), DARKCYAN );

    wxString errorTxt;

    errorTxt << _("ErrType") << wxT("(") << rpt.GetErrorCode() << wxT(")-  ") << rpt.GetErrorText() << wxT(":");

    text_pos = 5;
    Affiche_1_Parametre( frame, text_pos, errorTxt, wxEmptyString, RED );

    wxString txtA;
    txtA << DRC_ITEM::ShowCoord( rpt.GetPointA() ) << wxT(": ") << rpt.GetTextA();

    wxString txtB;
    if ( rpt.HasSecondItem() )
        txtB << DRC_ITEM::ShowCoord( rpt.GetPointB() ) << wxT(": ") << rpt.GetTextB();

    text_pos = 25;
    Affiche_1_Parametre( frame, text_pos, txtA, txtB, DARKBROWN );
}
