/**
 * This file is part of the common libary.
 * @file  block_commande.h
 * @see   common.h
 */

#ifndef __INCLUDE__BLOCK_COMMANDE_H__
#define __INCLUDE__BLOCK_COMMANDE_H__ 1


void    AbortBlockCurrentCommand( WinEDA_DrawPanel* Panel, wxDC* DC );

/* Cancel Current block operation. */
void    InitBlockLocateDatas( WinEDA_DrawPanel* Panel, const wxPoint& startpos );

/* Init the initial values of a BlockLocate, before starting a block command */
void    DrawAndSizingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* Redraw the outlines of the block which shows the search area for block commands
 *  The first point of the rectangle showing the area is initialised
 *  by InitBlockLocateDatas().
 *  The other point of the rectangle is the mouse cursor */


#endif /* __INCLUDE__BLOCK_COMMANDE_H__ */

