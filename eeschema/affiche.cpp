/**********************************************************/
/* Routines d'affichage de parametres et caracteristiques */
/**********************************************************/

/* Fichier AFFICHE.CPP */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "schframe.h"


/***********************************************************/
void DrawSheetStruct::Display_Infos( WinEDA_DrawFrame* frame )
/************************************************************/
{
    frame->MsgPanel->EraseMsgBox();
    Affiche_1_Parametre( frame, 1, _( "Name" ), m_SheetName, CYAN );
    Affiche_1_Parametre( frame, 30, _( "FileName" ), m_FileName, BROWN );
}


/***************************************************************/
void EDA_SchComponentStruct::Display_Infos( WinEDA_DrawFrame* frame )
/***************************************************************/
{
    EDA_LibComponentStruct* Entry = FindLibPart( m_ChipName.GetData(), wxEmptyString, FIND_ROOT );;

    wxString msg;

    frame->MsgPanel->EraseMsgBox();

    Affiche_1_Parametre( frame, 1, _( "Ref" ),
    	GetRef(((WinEDA_SchematicFrame*)frame)->GetSheet()), DARKCYAN );

    if( Entry && Entry->m_Options == ENTRY_POWER )
        msg = _( "Pwr Symb" );
    else
        msg = _( "Val" );
    Affiche_1_Parametre( frame, 10, msg, m_Field[VALUE].m_Text, DARKCYAN );

    Affiche_1_Parametre( frame, 28, _( "RefLib" ), m_ChipName.GetData(), BROWN );

    msg = FindLibName;
    Affiche_1_Parametre( frame, 40, _( "Lib" ), msg, DARKRED );

    if( Entry )
    {
        Affiche_1_Parametre( frame, 52, Entry->m_Doc, wxEmptyString, DARKCYAN );
        Affiche_1_Parametre( frame, 52, wxEmptyString, Entry->m_KeyWord, DARKGREEN );
    }
}


/*******************************************************/
void LibDrawPin::Display_Infos( WinEDA_DrawFrame* frame )
/*******************************************************/

/* Affiche en bas d'ecran les caracteristiques de la pin
 */
{
    wxString Text;
    int      ii;

    frame->MsgPanel->EraseMsgBox();

    /* Affichage du nom */
    Affiche_1_Parametre( frame, 24, _( "PinName" ), m_PinName, DARKCYAN );

    /* Affichage du numero */
    if( m_PinNum == 0 )
        Text = wxT( "?" );
    else
        ReturnPinStringNum( Text );

    Affiche_1_Parametre( frame, 40, _( "PinNum" ), Text, DARKCYAN );

    /* Affichage du type */
    ii = m_PinType;
    Affiche_1_Parametre( frame, 48, _( "PinType" ), MsgPinElectricType[ii], RED );

    /* Affichage de la visiblite */
    ii = m_Attributs;
    if( ii & 1 )
        Text = _( "no" );
    else
        Text = _( "yes" );
    Affiche_1_Parametre( frame, 58, _( "Display" ), Text, DARKGREEN );

    /* Affichage de la longueur */
    Text.Printf( wxT( "%d" ), m_PinLen );
    Affiche_1_Parametre( frame, 66, _( "Lengh" ), Text, MAGENTA );

    /* Affichage de l'orientation */
    switch( m_Orient )
    {
    case PIN_UP:
        Text = _( "Up" ); break;

    case PIN_DOWN:
        Text = _( "Down" ); break;

    case PIN_LEFT:
        Text = _( "Left" ); break;

    case PIN_RIGHT:
        Text = _( "Right" ); break;

    default:
        Text = wxT( "??" ); break;
    }

    Affiche_1_Parametre( frame, 72, _( "Orient" ), Text, MAGENTA );
}


/***********************************************************************/
void LibEDA_BaseStruct::Display_Infos_DrawEntry( WinEDA_DrawFrame* frame )
/***********************************************************************/

/* Affiche en bas d'ecran les caracteristiques de l'element
 */
{
    wxString msg;

    frame->MsgPanel->EraseMsgBox();

    /* affichage du type */
    msg = wxT( "??" );

    switch( Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        msg = wxT( "Arc" ); break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        msg = wxT( "Circle" ); break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        msg = wxT( "Text" ); break;

    case COMPONENT_RECT_DRAW_TYPE:
        msg = wxT( "Rect" ); break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
        msg = wxT( "PolyLine" ); break;

    case COMPONENT_LINE_DRAW_TYPE:
        msg = wxT( "Segment" ); break;

    case COMPONENT_PIN_DRAW_TYPE:
        ( (LibDrawPin*) this )->Display_Infos( frame );
        msg = wxT( "Pin" );
        break;

    default:
        ;
    }

    Affiche_1_Parametre( frame, 1, wxT( "Type" ), msg, CYAN );


    /* Affichage de l'appartenance */
    if( m_Unit == 0 )
        msg = _( "All" );
    else
        msg.Printf( wxT( "%d" ), m_Unit );
    Affiche_1_Parametre( frame, 10, _( "Unit" ), msg, BROWN );

    if( m_Convert == 0 )
        msg = _( "All" );
    else if( m_Convert == 1 )
        msg = _( "no" );
    else if( m_Convert == 2 )
        msg = _( "yes" );
    else
        msg = wxT( "?" );
    Affiche_1_Parametre( frame, 16, _( "Convert" ), msg, BROWN );

    if( m_Width )
        valeur_param( m_Width, msg );
    else
        msg = _( "default" );
    Affiche_1_Parametre( frame, 24, _( "Width" ), msg, BLUE );
}
