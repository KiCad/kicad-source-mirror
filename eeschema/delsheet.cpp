/****************/
/* delsheet.cpp */
/****************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_sheet.h"


/*  Free (delete) all schematic data (include the sub hierarchy sheets )
 * for the hierarchical sheet FirstSheet
 * FirstSheet is not deleted.
 */
void DeleteSubHierarchy( SCH_SHEET* FirstSheet, bool confirm_deletion )
{
    EDA_ITEM*       DrawStruct;
    EDA_ITEM*       EEDrawList;
    wxString        msg;
    SCH_EDIT_FRAME* frame;

    frame = (SCH_EDIT_FRAME*)wxGetApp().GetTopWindow();

    if( FirstSheet == NULL )
        return;

    if( FirstSheet->Type() != SCH_SHEET_T )
    {
        DisplayError( NULL, wxT( "DeleteSubHierarchy error(): NOT a Sheet" ) );
        return;
    }

    if( FirstSheet->m_AssociatedScreen->IsModify() && confirm_deletion )
    {
        msg.Printf( _( "Sheet %s (file %s) modified. Save it?" ),
                    FirstSheet->m_SheetName.GetData(),
                    FirstSheet->GetFileName().GetData() );

        if( IsOK( NULL, msg ) )
        {
            frame->SaveEEFile( FirstSheet->m_AssociatedScreen, FILE_SAVE_AS );
        }
    }

    /* free the sub hierarchy */
    if( FirstSheet->m_AssociatedScreen )
    {
        EEDrawList = FirstSheet->m_AssociatedScreen->GetDrawItems();

        while( EEDrawList != NULL )
        {
            DrawStruct = EEDrawList;
            EEDrawList = EEDrawList->Next();

            if( DrawStruct->Type() == SCH_SHEET_T )
            {
                DeleteSubHierarchy( (SCH_SHEET*) DrawStruct, confirm_deletion );
            }
        }

        FirstSheet->m_AssociatedScreen->FreeDrawList();
    }
}


/* free the draw list screen->EEDrawList and the subhierarchies
 * clear the screen datas (filenames ..)
 */
bool ClearProjectDrawList( SCH_SCREEN* screen, bool confirm_deletion )
{
    if( screen == NULL )
        return TRUE;

    screen->FreeDrawList();

    /* Clear the  screen datas */
    screen->m_ScreenNumber = screen->m_NumberOfScreen = 1;
    screen->m_Title.Empty();
    screen->m_Revision.Empty();
    screen->m_Company.Empty();
    screen->m_Commentaire1.Empty();
    screen->m_Commentaire2.Empty();
    screen->m_Commentaire3.Empty();
    screen->m_Commentaire4.Empty();
    screen->m_Date = GenDate();

    return TRUE;
}
