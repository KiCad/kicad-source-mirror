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
    m_NbNodes = m_NbLink = m_NbNoconn = 0;
    m_ForceWidth    = 0;
    m_RatsnestStart = NULL; // pointeur sur debut de liste ratsnests du net
    m_RatsnestEnd   = NULL; // pointeur sur fin de liste ratsnests du net
}


/* destructor */

NETINFO_ITEM::~NETINFO_ITEM()
{
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

        if( strncmp( Line, "Lw", 2 ) == 0 ) /* Texte */
        {
            sscanf( Line + 2, " %d", &tmp );
            m_ForceWidth = tmp;
            continue;
        }
    }

    return 1;
}


/**************************************/
bool NETINFO_ITEM::Save( FILE* aFile ) const
/**************************************/
/** Note: the old name of class NETINFO_ITEM was EQUIPOT
 * so in Save (and read) functions, for compatibility, we use EQUIPOT as keyword
 */
{
    bool success = false;

    fprintf( aFile, "$EQUIPOT\n" );
    fprintf( aFile, "Na %d \"%s\"\n", GetNet(), CONV_TO_UTF8( m_Netname ) );
    fprintf( aFile, "St %s\n", "~" );

    if( m_ForceWidth )
        fprintf( aFile, "Lw %d\n", m_ForceWidth );

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
void NETINFO_ITEM::SetNetname( const wxString & aNetname )
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
    double	    lengthnet = 0;

    frame->MsgPanel->EraseMsgBox();

    Affiche_1_Parametre( frame, 1, _( "Net Name" ), GetNetname(), RED );

    txt.Printf( wxT( "%d" ), GetNet() );
    Affiche_1_Parametre( frame, 30, _( "Net Code" ), txt, RED );

    count = 0;
    module = ((WinEDA_BasePcbFrame*)frame)->GetBoard()->m_Modules;
    for( ; module != 0; module = module->Next() )
    {
        for( pad = module->m_Pads; pad != 0; pad = pad->Next() )
        {
            if( pad->GetNet() == GetNet() )
                count++;
        }
    }

    txt.Printf( wxT( "%d" ), count );
    Affiche_1_Parametre( frame, 40, _( "Pads" ), txt, DARKGREEN );

    count = 0;
    Struct = ((WinEDA_BasePcbFrame*)frame)->GetBoard()->m_Track;
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
    Affiche_1_Parametre( frame, 50, _( "Vias" ), txt, BLUE );

    valeur_param( (int) lengthnet, txt );
    Affiche_1_Parametre( frame, 60, _( "Net Length" ), txt, RED );
}


/***********************/
/* class RATSNEST_ITEM */
/***********************/

/** function Draw 
 * Draws a line from the starting pad to the ending pad
 */
void RATSNEST_ITEM::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& aOffset )
{
    EDA_Colors color = (EDA_Colors) g_DesignSettings.m_RatsnestColor;
    if ( (m_Status & LOCAL_RATSNEST_ITEM) )
        color = YELLOW;
    GRLine( &panel->m_ClipBox, DC, m_PadStart->m_Pos.x + aOffset.x,
            m_PadStart->m_Pos.y + aOffset.y,
            m_PadEnd->m_Pos.x + aOffset.x,
            m_PadEnd->m_Pos.y + aOffset.y,
            0, color );
}

