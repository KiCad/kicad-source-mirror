/****************************/
/*	EESchema - eesavlib.cpp	*/
/****************************/

/* Functions to save schematic libraries and library components (::Save()
 * members)
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/** Function CopyDrawEntryStruct
 * Duplicate a DrawLibItem
 * the new item is only created, it is not put in the current component linked
 *  list
 * @param DrawEntry = DrawLibItem * item to duplicate
 * @return a pointer to the new item
 * A better way to duplicate a DrawLibItem is to have a virtual GenCopy() in
 * LibEDA_BaseStruct class (ToDo).
 */
LibEDA_BaseStruct* CopyDrawEntryStruct( wxWindow*          frame,
                                        LibEDA_BaseStruct* DrawItem )
{
    LibEDA_BaseStruct* NewDrawItem = NULL;
    wxString           msg;

    switch( DrawItem->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        NewDrawItem = ( (LibDrawArc*) DrawItem )->GenCopy();
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        NewDrawItem = ( (LibDrawCircle*) DrawItem )->GenCopy();
        break;

    case COMPONENT_RECT_DRAW_TYPE:
        NewDrawItem = ( (LibDrawSquare*) DrawItem )->GenCopy();
        break;

    case COMPONENT_PIN_DRAW_TYPE:
        NewDrawItem = ( (LibDrawPin*) DrawItem )->GenCopy();
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        NewDrawItem = ( (LibDrawText*) DrawItem )->GenCopy();
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
        NewDrawItem = ( (LibDrawPolyline*) DrawItem )->GenCopy();
        break;

    default:
        msg.Printf( wxT( "CopyDrawLibEntryStruct: unknown Draw Type %d" ),
                    DrawItem->Type() );
        DisplayError( frame, msg );
        break;
    }

    return NewDrawItem;
}


/*
 * Routine de copie d'une partlib
 *  Parametres d'entree: pointeur sur la structure de depart
 *  Parametres de sortie: pointeur sur la structure creee
 *  Do not copy new items ( i.e. with m_Flag  & IS_NEW)
 */
EDA_LibComponentStruct* CopyLibEntryStruct( wxWindow* frame,
                                            EDA_LibComponentStruct* OldEntry )
{
    EDA_LibComponentStruct* NewStruct;
    LibEDA_BaseStruct*      NewDrawings, * OldDrawings;
    LibEDA_BaseStruct*      LastItem;
    LibDrawField*           OldField, * NewField;

    if( OldEntry->Type != ROOT )
    {
        DisplayError( frame, wxT( "CopyLibEntryStruct(): Type != ROOT" ) );
        return NULL;
    }

    NewStruct = new EDA_LibComponentStruct( NULL );

    OldEntry->m_Prefix.Copy( &NewStruct->m_Prefix );
    OldEntry->m_Name.Copy( &NewStruct->m_Name );

    NewStruct->m_UnitCount   = OldEntry->m_UnitCount;
    NewStruct->m_TextInside  = OldEntry->m_TextInside;
    NewStruct->m_DrawPinNum  = OldEntry->m_DrawPinNum;
    NewStruct->m_DrawPinName = OldEntry->m_DrawPinName;
    NewStruct->m_Options = OldEntry->m_Options;
    NewStruct->m_UnitSelectionLocked = OldEntry->m_UnitSelectionLocked;

    /* Copie des sous structures: */
    NewStruct->m_AliasList = OldEntry->m_AliasList;
    NewStruct->m_Doc     = OldEntry->m_Doc;
    NewStruct->m_KeyWord = OldEntry->m_KeyWord;
    NewStruct->m_DocFile = OldEntry->m_DocFile;

    /* Copie des champs */
    for( OldField = OldEntry->m_Fields; OldField != NULL;
         OldField = OldField->Next() )
    {
        NewField = OldField->GenCopy();
        NewStruct->m_Fields.PushBack( NewField );
    }

    /* Copie des elements type Drawing */
    LastItem = NULL;
    for( OldDrawings = OldEntry->m_Drawings; OldDrawings != NULL;
         OldDrawings = OldDrawings->Next() )
    {
        if( ( OldDrawings->m_Flags & IS_NEW) != 0 )
            continue;

        NewDrawings = CopyDrawEntryStruct( frame, OldDrawings );
        if( NewDrawings )
        {
            if( LastItem == NULL )
                NewStruct->m_Drawings = NewDrawings;
            else
                LastItem->SetNext( NewDrawings );

            LastItem = NewDrawings;
            NewDrawings->SetNext( NULL );
        }
        else	// Should nevers occurs, just in case...
        {       // CopyDrawEntryStruct() was not able to duplicate the type
                // of OldDrawings
                // occurs when an unexpected type is encountered
            DisplayError( frame, wxT( "CopyLibEntryStruct(): error: aborted" ) );
            break;
        }
    }

    /* Copy the footprint filter list */
    for( unsigned ii = 0; ii < OldEntry->m_FootprintList.GetCount(); ii++ )
        NewStruct->m_FootprintList.Add( OldEntry->m_FootprintList[ii] );

    return NewStruct;
}
