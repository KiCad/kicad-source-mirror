/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
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

/**
 * @file sch_bus_entry.h
 *
 */

#ifndef _SCH_BUS_ENTRY_H_
#define _SCH_BUS_ENTRY_H_

#include "sch_item_struct.h"


/* Flags for BUS ENTRY (bus to bus or wire to bus */
#define WIRE_TO_BUS 0
#define BUS_TO_BUS  1


/**
 * Class SCH_BUS_ENTRY
 *
 * Defines a bus or wire entry.
 */
class SCH_BUS_ENTRY : public SCH_ITEM
{
    wxPoint m_Pos;

public:
    int     m_Width;
    wxSize  m_Size;

public:
    SCH_BUS_ENTRY( const wxPoint& pos = wxPoint( 0, 0 ), int shape = '\\', int id = WIRE_TO_BUS );

    SCH_BUS_ENTRY( const SCH_BUS_ENTRY& aBusEntry );

    ~SCH_BUS_ENTRY() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_BUS_ENTRY" );
    }

    wxPoint m_End() const;

    virtual void SwapData( SCH_ITEM* aItem );

    virtual void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic bus entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic bus entry from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic bus entry.
     * @return True if the schematic bus entry loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_RECT GetBoundingBox() const;

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    /**
     * Function Move
     * moves and item to a new position by \a aMoveVector.
     * @param aMoveVector The displacement vector.
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }

    /**
     * Function Mirror_Y
     * mirrors the item relative to \a aYaxis_position.
     * @param aYaxis_position The Y axis coordinate to mirror around.
     */
    virtual void Mirror_Y( int aYaxis_position );

    virtual void Mirror_X( int aXaxis_position );

    virtual void Rotate( wxPoint rotationPoint );

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual bool IsConnectable() const { return true; }

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_entry_xpm; }

private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const;
    virtual bool doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const;
    virtual EDA_ITEM* doClone() const;
    virtual void doPlot( PLOTTER* aPlotter );
    virtual wxPoint doGetPosition() const { return m_Pos; }
    virtual void doSetPosition( const wxPoint& aPosition ) { m_Pos = aPosition; }
};


#endif    // _SCH_BUS_ENTRY_H_
