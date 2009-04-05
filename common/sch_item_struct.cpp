/****************************************************/
/*	class_drawpickedstruct.cpp */
/****************************************************/

#include "fctsys.h"
#include "common.h"
#include "gr_basic.h"
#include "base_struct.h"
#include "sch_item_struct.h"
#include "class_sch_screen.h"
#include "class_drawpanel.h"
#include "wxEeschemaStruct.h"

#include "program.h"
#include "general.h"
#include "libcmp.h"
#include "protos.h"

/* Constructor and destructor for SCH_ITEM */
/* They are not inline because this creates problems with gcc at linking time
 * in debug mode
*/

SCH_ITEM::SCH_ITEM( EDA_BaseStruct* aParent, KICAD_T aType ) :
    EDA_BaseStruct( aParent, aType )
{
    m_Layer = 0;
}

SCH_ITEM::~SCH_ITEM()
{
}

/**
 * place the struct in EEDrawList.
 * if it is a new item, it it also put in undo list
 * for an "old" item, saving it in undo list must be done before editiing,
 * and not here!
 */
void SCH_ITEM::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
{
    if( m_Flags & IS_NEW )
    {
        SCH_SCREEN* screen = frame->GetScreen();
        if( !screen->CheckIfOnDrawList( this ) )  //don't want a loop!
            screen->AddToDrawList( this );
        g_ItemToRepeat = this;
        frame->SaveCopyInUndoList( this, IS_NEW );
    }

    m_Flags = 0;
    frame->GetScreen()->SetModify();
    frame->GetScreen()->SetCurItem( NULL );
    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;

    if( DC )
    {
        frame->DrawPanel->CursorOff( DC );      // Erase schematic cursor
        RedrawOneStruct( frame->DrawPanel, DC, this, GR_DEFAULT_DRAWMODE );
        frame->DrawPanel->CursorOn( DC );       // Display schematic cursor
    }
}
