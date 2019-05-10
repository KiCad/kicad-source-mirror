/**
 * OBSOLETE
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <draw_frame.h>
#include <common.h>
#include <macros.h>
#include <block_commande.h>


BLOCK_SELECTOR::BLOCK_SELECTOR() :
    EDA_RECT()
{
    m_state      = STATE_NO_BLOCK; // State (enum BLOCK_STATE_T) of block.
    m_command    = BLOCK_IDLE;     // Type (enum BLOCK_COMMAND_T) of operation.
}


BLOCK_SELECTOR::~BLOCK_SELECTOR()
{
}


void BLOCK_SELECTOR::SetMessageBlock( EDA_DRAW_FRAME* frame )
{
}


void BLOCK_SELECTOR::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                           GR_DRAWMODE aDrawMode, COLOR4D aColor )
{
}


void BLOCK_SELECTOR::InitData( EDA_DRAW_PANEL* aPanel, const wxPoint& startpos )
{
}


void BLOCK_SELECTOR::ClearItemsList()
{
}


void BLOCK_SELECTOR::ClearListAndDeleteItems()
{
}


void BLOCK_SELECTOR::PushItem( ITEM_PICKER& aItem )
{
}


void BLOCK_SELECTOR::Clear()
{
}

