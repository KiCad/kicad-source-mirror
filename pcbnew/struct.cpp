/***********************************************/
/* Routines d'effacement et copie de structures*/
/***********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

/* Routines Locales */
/* Variables locales */


/**************************************************/
void DeleteStructure( BOARD_ITEM* PtStruct )
/**************************************************/

/* Supprime de la liste chainee la stucture pointee par GenericStructure
 *  et libere la memoire correspondante
 */
{
    BOARD_ITEM*     PtNext;
    BOARD_ITEM*     PtBack;
    int             IsDeleted;
    wxString        Line;

    if( PtStruct == NULL )
        return;

    IsDeleted  = PtStruct->GetState( DELETED );

    PtNext = PtStruct->Next();
    PtBack = PtStruct->Back();

    KICAD_T typestruct = PtStruct->Type();

    switch( typestruct )
    {
    case TYPE_NOT_INIT:
        DisplayError( NULL, wxT( "DeleteStruct: Type Not Init" ) );
        break;

    case PCB_EQUIPOT_STRUCT_TYPE:
            #undef Struct
            #define Struct ( (EQUIPOT*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPEMODULE:
            #undef Struct
            #define Struct ( (MODULE*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;


    case TYPEPAD:
            #undef Struct
            #define Struct ( (D_PAD*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPECOTATION:
            #undef Struct
            #define Struct ( (COTATION*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPEMIRE:
            #undef Struct
            #define Struct ( (MIREPCB*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPEDRAWSEGMENT:
            #undef Struct
            #define Struct ( (DRAWSEGMENT*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPETEXTE:
            #undef Struct
            #define Struct ( (TEXTE_PCB*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;


    case TYPETEXTEMODULE:
            #undef Struct
            #define Struct ( (TEXTE_MODULE*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPEEDGEMODULE:
            #undef Struct
            #define Struct ( (EDGE_MODULE*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPETRACK:
            #undef Struct
            #define Struct ( (TRACK*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPEVIA:
            #undef Struct
            #define Struct ( (SEGVIA*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPEZONE:
            #undef Struct
            #define Struct ( (SEGZONE*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPEMARQUEUR:
            #undef Struct
            #define Struct ( (MARQUEUR*) PtStruct )
        Struct->UnLink();
        delete Struct;
        break;

    case TYPEPCB:

    default:
        Line.Printf( wxT( " DeleteStructure: unexpected Type %d" ),
                    PtStruct->Type() );
        DisplayError( NULL, Line );
        break;
    }
}


/**************************************************/
void DeleteStructList( BOARD_ITEM* PtStruct )
/**************************************************/

/* Supprime la liste chainee pointee par PtStruct
 *  et libere la memoire correspondante
 */
{
    BOARD_ITEM* PtNext;

    while( PtStruct )
    {
        PtNext = PtStruct->Next();
        delete PtStruct;
        PtStruct = PtNext;
    }
}
