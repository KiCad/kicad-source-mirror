/*************************************************************************/
/* NETINFO_ITEM class, to handle info on nets (netnames, net constraints */
/*************************************************************************/

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <wxBasePcbFrame.h>
#include <common.h>
#include <kicad_string.h>
#include <pcbnew.h>
#include <colors_selection.h>
#include <richio.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>


/*********************************************************/
/* class NETINFO_ITEM: handle data relative to a given net */
/*********************************************************/

NETINFO_ITEM::NETINFO_ITEM( BOARD_ITEM* aParent, const wxString& aNetName, int aNetCode )
{
    SetNet( aNetCode );
    m_NbNodes  = 0;
    m_NbLink   = 0;
    m_NbNoconn = 0;
    m_Flag     = 0;
    m_RatsnestStartIdx = 0;     // Starting point of ratsnests of this net in a
                                // general buffer of ratsnest
    m_RatsnestEndIdx   = 0;     // Ending point of ratsnests of this net

    m_NetClassName = aNetName;

    m_NetClass = 0;
}


NETINFO_ITEM::~NETINFO_ITEM()
{
    // m_NetClass is not owned by me.
}


/**
 * Function SetNetname
 * @param aNetname : the new netname
 */
void NETINFO_ITEM::SetNetname( const wxString& aNetname )
{
    m_Netname = aNetname;
    m_ShortNetname = m_Netname.AfterLast( '/' );
}


/**
 * Function Draw (TODO)
 */
void NETINFO_ITEM::Draw( EDA_DRAW_PANEL* panel,
                         wxDC*           DC,
                         int             aDrawMode,
                         const wxPoint&  aOffset )
{
}


/**
 * Function DisplayInfo
 * has knowledge about the frame and how and where to put status information
 * about this object into the frame's message panel.
 * Is virtual from EDA_ITEM.
 * @param frame A EDA_DRAW_FRAME in which to print status information.
 */
void NETINFO_ITEM::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    int       count;
    EDA_ITEM* Struct;
    wxString  txt;
    MODULE*   module;
    D_PAD*    pad;
    double    lengthnet = 0;        // This  is the lenght of tracks on pcb
    double    lengthdie = 0;        // this is the lenght of internal ICs connections

    frame->ClearMsgPanel();

    frame->AppendMsgPanel( _( "Net Name" ), GetNetname(), RED );

    txt.Printf( wxT( "%d" ), GetNet() );
    frame->AppendMsgPanel( _( "Net Code" ), txt, RED );

    count  = 0;
    module = ( (PCB_BASE_FRAME*) frame )->GetBoard()->m_Modules;
    for( ; module != 0; module = module->Next() )
    {
        for( pad = module->m_Pads; pad != 0; pad = pad->Next() )
        {
            if( pad->GetNet() == GetNet() )
            {
                count++;
                lengthdie += pad->GetDieLength();
            }
        }
    }

    txt.Printf( wxT( "%d" ), count );
    frame->AppendMsgPanel( _( "Pads" ), txt, DARKGREEN );

    count  = 0;
    Struct = ( (PCB_BASE_FRAME*) frame )->GetBoard()->m_Track;

    for( ; Struct != NULL; Struct = Struct->Next() )
    {
        if( Struct->Type() == PCB_VIA_T )
        {
            if( ( (SEGVIA*) Struct )->GetNet() == GetNet() )
                count++;
        }

        if( Struct->Type() == PCB_TRACE_T )
        {
            if( ( (TRACK*) Struct )->GetNet() == GetNet() )
                lengthnet += ( (TRACK*) Struct )->GetLength();
        }
    }

    txt.Printf( wxT( "%d" ), count );
    frame->AppendMsgPanel( _( "Vias" ), txt, BLUE );

    // Displays the full net lenght (tracks on pcb + internal ICs connections ):
    txt = frame->CoordinateToString( lengthnet + lengthdie );
    frame->AppendMsgPanel( _( "Net Length:" ), txt, RED );

    // Displays the net lenght of tracks only:
    txt = frame->CoordinateToString( lengthnet );
    frame->AppendMsgPanel( _( "On Board" ), txt, RED );

    // Displays the net lenght of internal ICs connections (wires inside ICs):
    txt = frame->CoordinateToString( lengthdie );
    frame->AppendMsgPanel( _( "On Die" ), txt, RED );
}


/***********************/
/* class RATSNEST_ITEM */
/***********************/

RATSNEST_ITEM::RATSNEST_ITEM()
{
    m_NetCode  = 0;         // netcode ( = 1.. n ,  0 is the value used for not
                            // connected items)
    m_Status   = 0;         // state
    m_PadStart = NULL;      // pointer to the starting pad
    m_PadEnd   = NULL;      // pointer to ending pad
    m_Lenght   = 0;         // length of the line (temporary used in some
                            // calculations)
}


/**
 * Function Draw
 * Draws a line (a ratsnest) from the starting pad to the ending pad
 */
void RATSNEST_ITEM::Draw( EDA_DRAW_PANEL* panel,
                          wxDC*           DC,
                          int             aDrawMode,
                          const wxPoint&  aOffset )
{
    GRSetDrawMode( DC, aDrawMode );

    int color = g_ColorsSettings.GetItemColor(RATSNEST_VISIBLE);

    GRLine( panel->GetClipBox(), DC,
            m_PadStart->GetPosition() - aOffset,
            m_PadEnd->GetPosition() - aOffset, 0, color );
}
