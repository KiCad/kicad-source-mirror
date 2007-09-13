	/********************************************/
	/*  library editor: undo and redo functions */
	/********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "id.h"

#include "protos.h"


/*************************************************************************/
void WinEDA_LibeditFrame::SaveCopyInUndoList(EDA_BaseStruct * ItemToCopy,
	int unused_flag)
/*************************************************************************/
{
EDA_BaseStruct * item;
EDA_LibComponentStruct * CopyItem;

	CopyItem = CopyLibEntryStruct ( this, (EDA_LibComponentStruct *) ItemToCopy);
	GetScreen()->AddItemToUndoList((EDA_BaseStruct *)CopyItem);
	/* Clear current flags (which can be temporary set by a current edit command) */
	for ( item = CopyItem->m_Drawings; item != NULL; item = item->Pnext )
		item->m_Flags = 0;
	

	/* Clear redo list, because after new save there is no redo to do */
	while ( GetScreen()->m_RedoList )
	{
		item = GetScreen()->m_RedoList->Pnext;
		delete GetScreen()->m_RedoList;
		GetScreen()->m_RedoList = item;
	}
}

/******************************************************/
void WinEDA_LibeditFrame::GetComponentFromRedoList()
/******************************************************/
/* Redo the last edition:
	- Place the current edited library component in undo list
	- Get old version of the current edited library component
*/
{
	if ( GetScreen()->m_RedoList == NULL ) return;
		
	GetScreen()->AddItemToUndoList((EDA_BaseStruct *)CurrentLibEntry);
	CurrentLibEntry =
		(EDA_LibComponentStruct *) GetScreen()->GetItemFromRedoList();
	if ( CurrentLibEntry ) CurrentLibEntry->Pnext = NULL;
	CurrentDrawItem = NULL;
	GetScreen()->SetModify();
	ReCreateHToolbar();
	SetToolbars();
}

/******************************************************/
void WinEDA_LibeditFrame::GetComponentFromUndoList()
/******************************************************/
/* Undo the last edition:
	- Place the current edited library component in Redo list
	- Get old version of the current edited library component
*/
{
	if ( GetScreen()->m_UndoList == NULL ) return;
		
	GetScreen()->AddItemToRedoList((EDA_BaseStruct *)CurrentLibEntry);
	CurrentLibEntry =
		(EDA_LibComponentStruct *) GetScreen()->GetItemFromUndoList();

	if ( CurrentLibEntry ) CurrentLibEntry->Pnext = NULL;
	CurrentDrawItem = NULL;
	GetScreen()->SetModify();
	ReCreateHToolbar();
	SetToolbars();
}
