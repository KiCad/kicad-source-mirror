/************************/
/*	sch_item_struct.cpp */
/************************/

#include "fctsys.h"
#include "common.h"
#include "gr_basic.h"
#include "base_struct.h"
#include "sch_item_struct.h"
#include "class_sch_screen.h"
#include "class_drawpanel.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"

#include "../eeschema/dialogs/dialog_schematic_find.h"


/* Constructor and destructor for SCH_ITEM */
/* They are not inline because this creates problems with gcc at linking time
 * in debug mode
*/

SCH_ITEM::SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType ) :
    EDA_ITEM( aParent, aType )
{
    m_Layer = 0;
}


SCH_ITEM::~SCH_ITEM()
{
    // Do not let the connections container go out of scope with any ojbects or they
    // will be deleted by the container will cause the EESchema to crash.  These objects
    // are owned by the sheet object container.
    if( !m_connections.empty() )
        m_connections.release();
}


/**
 * place the struct in m_drawList.
 * if it is a new item, it it also put in undo list
 * for an "old" item, saving it in undo list must be done before editiing,
 * and not here!
 */
void SCH_ITEM::Place( SCH_EDIT_FRAME* frame, wxDC* DC )
{
    if( m_Flags & IS_NEW )
    {
        SCH_SCREEN* screen = frame->GetScreen();

        if( !screen->CheckIfOnDrawList( this ) )  //don't want a loop!
            screen->AddToDrawList( this );

        g_ItemToRepeat = this;
        frame->SaveCopyInUndoList( this, UR_NEW );
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


bool SCH_ITEM::Matches( const wxString& aText, wxFindReplaceData& aSearchData )
{
    wxString text = aText;
    wxString searchText = aSearchData.GetFindString();

    if( aSearchData.GetFlags() & wxFR_WHOLEWORD )
        return aText.IsSameAs( searchText, aSearchData.GetFlags() & wxFR_MATCHCASE );

    if( aSearchData.GetFlags() & FR_MATCH_WILDCARD )
    {
        if( aSearchData.GetFlags() & wxFR_MATCHCASE )
            return text.Matches( searchText );

        return text.MakeUpper().Matches( searchText.MakeUpper() );
    }

    if( aSearchData.GetFlags() & wxFR_MATCHCASE )
        return aText.Find( searchText ) != wxNOT_FOUND;

    return text.MakeUpper().Find( searchText.MakeUpper() ) != wxNOT_FOUND;
}
