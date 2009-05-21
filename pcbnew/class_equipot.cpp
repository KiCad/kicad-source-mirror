/*****************************************************************/
/* fonctions membres de la classe EQUIPOT et fonctions associ�s */
/*****************************************************************/

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "kicad_string.h"
#include "pcbnew.h"


/*********************************************************/
/* classe EQUIPOT: gestion des listes d'equipotentielles */
/*********************************************************/

/* Constructeur de la classe EQUIPOT */
EQUIPOT::EQUIPOT( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, TYPE_EQUIPOT )
{
    SetNet( 0 );
    m_NbNodes = m_NbLink = m_NbNoconn = 0;
    m_Masque_Layer  = 0;
    m_Masque_Plan   = 0;
    m_ForceWidth    = 0;
    m_PadzoneStart  = NULL; // pointeur sur debut de liste pads du net
    m_PadzoneEnd    = NULL; // pointeur sur fin de liste pads du net
    m_RatsnestStart = NULL; // pointeur sur debut de liste ratsnests du net
    m_RatsnestEnd   = NULL; // pointeur sur fin de liste ratsnests du net
}


/* destructeut */

EQUIPOT::~EQUIPOT()
{
}


wxPoint& EQUIPOT::GetPosition()
{
    static wxPoint dummy;

    return dummy;
}


/*********************************************************/
int EQUIPOT:: ReadDescr( FILE* File, int* LineNum )
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
bool EQUIPOT::Save( FILE* aFile ) const
/**************************************/
{
    if( GetState( DELETED ) )
        return true;

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
void EQUIPOT::SetNetname( const wxString & aNetname )
{
    m_Netname = aNetname;
    m_ShortNetname = m_Netname.AfterLast( '/' );
}


/** function Draw
 * we actually could show a NET, simply show all the tracks and pads or net name on pad and vias
 */
void EQUIPOT::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& offset )
{
}


/**
 * Function DisplayInfo
 * has knowledge about the frame and how and where to put status information
 * about this object into the frame's message panel.
 * Is virtual from EDA_BaseStruct.
 * @param frame A WinEDA_DrawFrame in which to print status information.
 */
 void EQUIPOT::DisplayInfo( WinEDA_DrawFrame* frame )
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

#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void EQUIPOT::Show( int nestLevel, std::ostream& os )
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " name=\"" << m_Netname.mb_str() << '"' <<
    " netcode=\"" << GetNet() << "\"/>\n";
}


#endif
