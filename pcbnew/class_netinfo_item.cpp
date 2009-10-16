/*************************************************************************/
/* NETINFO_ITEM class, to handle info on nets (netnames, net constraints ...) */
/*************************************************************************/

#include "fctsys.h"
#include "class_drawpanel.h"
#include "common.h"
#include "kicad_string.h"
#include "pcbnew.h"


/*********************************************************/
/* class NETINFO_ITEM: handle data relative to a given net */
/*********************************************************/

/* Constructor */
NETINFO_ITEM::NETINFO_ITEM( BOARD_ITEM* aParent )
{
    SetNet( 0 );
    m_NbNodes = 0;
    m_NbLink = 0;
    m_NbNoconn = 0;
    m_Flag = 0;
    m_RatsnestStartIdx = 0;    // Starting point of ratsnests of this net in a general buffer of ratsnest
    m_RatsnestEndIdx   = 0;    // Ending point of ratsnests of this net

    m_NetClassName = NETCLASS::Default;

    m_NetClass = 0;
}


NETINFO_ITEM::~NETINFO_ITEM()
{
    // m_NetClass is not owned by me.
}


/*********************************************************/
int NETINFO_ITEM:: ReadDescr( FILE* File, int* LineNum )
/*********************************************************/

/* Routine de lecture de 1 descr Equipotentielle.
 *  retourne 0 si OK
 *          1 si lecture incomplete
 */
{
    char Line[1024], Ltmp[1024];
    int  tmp;

    while( GetLine( File, Line, LineNum ) )
    {
        if( strnicmp( Line, "$End", 4 ) == 0 )
            return 0;

        if( strncmp( Line, "Na", 2 ) == 0 ) /* Texte */
        {
            sscanf( Line + 2, " %d", &tmp );
            SetNet( tmp );

            ReadDelimitedText( Ltmp, Line + 2, sizeof(Ltmp) );
            m_Netname = CONV_FROM_UTF8( Ltmp );
            continue;
        }
    }

    return 1;
}


/*******************************************/
bool NETINFO_ITEM::Save( FILE* aFile ) const
/*******************************************/

/** Note: the old name of class NETINFO_ITEM was EQUIPOT
 * so in Save (and read) functions, for compatibility, we use EQUIPOT as keyword
 */
{
    bool success = false;

    fprintf( aFile, "$EQUIPOT\n" );
    fprintf( aFile, "Na %d \"%s\"\n", GetNet(), CONV_TO_UTF8( m_Netname ) );
    fprintf( aFile, "St %s\n", "~" );

    // fprintf( aFile, "NetClass \"%s\"\n", CONV_TO_UTF8(m_NetClassName) );

    if( fprintf( aFile, "$EndEQUIPOT\n" ) != sizeof("$EndEQUIPOT\n") - 1 )
        goto out;

    success = true;

out:
    return success;
}


/**
 * Function SetNetname
 * @param const wxString : the new netname
 */
void NETINFO_ITEM::SetNetname( const wxString& aNetname )
{
    m_Netname = aNetname;
    m_ShortNetname = m_Netname.AfterLast( '/' );
}


/** function Draw (TODO)
 */
void NETINFO_ITEM::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& aOffset )
{
}


/**
 * Function DisplayInfo
 * has knowledge about the frame and how and where to put status information
 * about this object into the frame's message panel.
 * Is virtual from EDA_BaseStruct.
 * @param frame A WinEDA_DrawFrame in which to print status information.
 */
void NETINFO_ITEM::DisplayInfo( WinEDA_DrawFrame* frame )
{
    int             count;
    EDA_BaseStruct* Struct;
    wxString        txt;
    MODULE*         module;
    D_PAD*          pad;
    double          lengthnet = 0;

    frame->ClearMsgPanel();

    frame->AppendMsgPanel( _( "Net Name" ), GetNetname(), RED );

    txt.Printf( wxT( "%d" ), GetNet() );
    frame->AppendMsgPanel( _( "Net Code" ), txt, RED );

    count  = 0;
    module = ( (WinEDA_BasePcbFrame*) frame )->GetBoard()->m_Modules;
    for( ; module != 0; module = module->Next() )
    {
        for( pad = module->m_Pads; pad != 0; pad = pad->Next() )
        {
            if( pad->GetNet() == GetNet() )
                count++;
        }
    }

    txt.Printf( wxT( "%d" ), count );
    frame->AppendMsgPanel( _( "Pads" ), txt, DARKGREEN );

    count  = 0;
    Struct = ( (WinEDA_BasePcbFrame*) frame )->GetBoard()->m_Track;
    for( ; Struct != NULL; Struct = Struct->Next() )
    {
        if( Struct->Type() == TYPE_VIA )
            if( ( (SEGVIA*) Struct )->GetNet() == GetNet() )
                count++;
        if( Struct->Type() == TYPE_TRACK )
            if( ( (TRACK*) Struct )->GetNet() == GetNet() )
                lengthnet += ( (TRACK*) Struct )->GetLength();
    }

    txt.Printf( wxT( "%d" ), count );
    frame->AppendMsgPanel( _( "Vias" ), txt, BLUE );

    valeur_param( (int) lengthnet, txt );
    frame->AppendMsgPanel( _( "Net Length" ), txt, RED );
}


/***********************/
/* class RATSNEST_ITEM */
/***********************/

RATSNEST_ITEM::RATSNEST_ITEM()
{
    m_NetCode  = 0;         // netcode ( = 1.. n ,  0 is the value used for not connected items)
    m_Status   = 0;         // state
    m_PadStart = NULL;      // pointer to the starting pad
    m_PadEnd   = NULL;      // pointer to ending pad
    m_Lenght   = 0;         // lenght of the line (temporary used in some calculations)
}


/** function Draw
 * Draws a line (a ratsnest) from the starting pad to the ending pad
 */
void RATSNEST_ITEM::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& aOffset )
{
    GRLine( &panel->m_ClipBox, DC, m_PadStart->m_Pos - aOffset,
            m_PadEnd->m_Pos - aOffset, 0, g_DesignSettings.m_RatsnestColor );
}
