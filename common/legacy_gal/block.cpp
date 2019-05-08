/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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
    m_color      = BROWN;
    m_appendUndo = false;          // Indicates at least one undo record has been saved and
                                   // any additional undo records should be appended.
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

