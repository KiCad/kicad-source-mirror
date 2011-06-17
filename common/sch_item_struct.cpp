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


bool sort_schematic_items( const SCH_ITEM* aItem1, const SCH_ITEM* aItem2 )
{
    return *aItem1 < *aItem2;
}


/* Constructor and destructor for SCH_ITEM */
/* They are not inline because this creates problems with gcc at linking time
 * in debug mode
 */

SCH_ITEM::SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType ) :
    EDA_ITEM( aParent, aType )
{
    m_Layer = 0;
}


SCH_ITEM::SCH_ITEM( const SCH_ITEM& aItem ) :
    EDA_ITEM( aItem )
{
    m_Layer = aItem.m_Layer;
}


SCH_ITEM::~SCH_ITEM()
{
    // Do not let the connections container go out of scope with any objects or they
    // will be deleted by the container will cause the EESchema to crash.  These objects
    // are owned by the sheet object container.
    if( !m_connections.empty() )
        m_connections.clear();
}


/**
 * place the struct in m_drawList.
 * if it is a new item, it it also put in undo list
 * for an "old" item, saving it in undo list must be done before editing,
 * and not here!
 */
void SCH_ITEM::Place( SCH_EDIT_FRAME* aFrame, wxDC* aDC )
{
    SCH_SCREEN* screen = aFrame->GetScreen();

    if( IsNew() )
    {
        if( !screen->CheckIfOnDrawList( this ) )  // don't want a loop!
            screen->AddToDrawList( this );

        aFrame->SetRepeatItem( this );
        aFrame->SaveCopyInUndoList( this, UR_NEW );
    }

    m_Flags = 0;
    screen->SetModify();
    screen->SetCurItem( NULL );
    aFrame->DrawPanel->SetMouseCapture( NULL, NULL );

    if( aDC )
    {
        EDA_CROSS_HAIR_MANAGER( aFrame->DrawPanel, aDC );  // Erase schematic cursor
        Draw( aFrame->DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
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


bool SCH_ITEM::IsConnected( const wxPoint& aPosition ) const
{
    if( m_Flags & STRUCT_DELETED || m_Flags & SKIP_STRUCT )
        return false;

    return doIsConnected( aPosition );
}


void SCH_ITEM::SwapData( SCH_ITEM* aItem )
{
    wxFAIL_MSG( wxT( "SwapData() method not implemented for class " ) + GetClass() );
}


bool SCH_ITEM::operator < ( const SCH_ITEM& aItem ) const
{
    wxCHECK_MSG( false, this->Type() < aItem.Type(),
                 wxT( "Less than operator not defined for " ) + GetClass() );
}


void SCH_ITEM::doPlot( PLOTTER* aPlotter )
{
    wxFAIL_MSG( wxT( "doPlot() method not implemented for class " ) + GetClass() );
}
