/****************************/
/*  EESchema - eesavlib.cpp */
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


/*
 * Routine de copie d'une partlib
 *  Parametres d'entree: pointeur sur la structure de depart
 *  Parametres de sortie: pointeur sur la structure creee
 *  Do not copy new items ( i.e. with m_Flag  & IS_NEW)
 */
LIB_COMPONENT* CopyLibEntryStruct( LIB_COMPONENT* OldEntry )
{
    wxString           msg;
    LIB_COMPONENT*     NewStruct;
    LibEDA_BaseStruct* NewDrawings, * OldDrawings;
    LibEDA_BaseStruct* LastItem;
    LibDrawField*      OldField, * NewField;

    if( OldEntry->Type != ROOT )
    {
        msg.Printf( wxT( "Component <%s> must be root type to make copy." ),
                    (const wxChar*) OldEntry->GetName() );
        wxLogError( msg );
        return NULL;
    }

    NewStruct = new LIB_COMPONENT( wxEmptyString );

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
        NewField = (LibDrawField*) OldField->GenCopy();
        NewStruct->m_Fields.PushBack( NewField );
    }

    /* Copie des elements type Drawing */
    LastItem = NULL;
    for( OldDrawings = OldEntry->m_Drawings; OldDrawings != NULL;
         OldDrawings = OldDrawings->Next() )
    {
        if( ( OldDrawings->m_Flags & IS_NEW ) != 0 )
            continue;

        NewDrawings = OldDrawings->GenCopy();

        if( NewDrawings )
        {
            if( LastItem == NULL )
                NewStruct->m_Drawings = NewDrawings;
            else
                LastItem->SetNext( NewDrawings );

            LastItem = NewDrawings;
            NewDrawings->SetNext( NULL );
        }
        else    // Should never occur, just in case...
        {       // CopyDrawEntryStruct() was not able to duplicate the type
                // of OldDrawings
                // occurs when an unexpected type is encountered
            msg.Printf( wxT( "Error attempting to copy draw item <%s> from \
component <%s>." ),
                        (const wxChar*) OldDrawings->GetClass(),
                        (const wxChar*) OldEntry->GetName() );
            wxLogError( msg );
            break;
        }
    }

    /* Copy the footprint filter list */
    for( unsigned ii = 0; ii < OldEntry->m_FootprintList.GetCount(); ii++ )
        NewStruct->m_FootprintList.Add( OldEntry->m_FootprintList[ii] );

    return NewStruct;
}
