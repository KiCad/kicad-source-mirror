/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_line.h
 */

#ifndef _SCH_LINE_H_
#define _SCH_LINE_H_


#include "sch_item_struct.h"


/**
 * Class SCH_LINE
 * is a segment description base class to describe items which have 2 end
 * points (track, wire, draw line ...)
 */
class SCH_LINE : public SCH_ITEM
{
    bool    m_startIsDangling;  ///< True if start point is not connected.
    bool    m_endIsDangling;    ///< True if end point is not connected.
    int     m_width;            ///< Set to 0 for wires and greater than 0 for busses.
    wxPoint m_start;            ///< Line start point
    wxPoint m_end;              ///< Line end point

public:
    SCH_LINE( const wxPoint& pos = wxPoint( 0, 0 ), int layer = LAYER_NOTES );

    SCH_LINE( const SCH_LINE& aLine );

    ~SCH_LINE() { }

    SCH_LINE* Next() const { return (SCH_LINE*) Pnext; }
    SCH_LINE* Back() const { return (SCH_LINE*) Pback; }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_LINE" );
    }

    bool IsEndPoint( const wxPoint& aPoint ) const
    {
        return aPoint == m_start || aPoint == m_end;
    }

    bool IsNull() const { return m_start == m_end; }

    wxPoint GetStartPoint() const { return m_start; }

    void SetStartPoint( const wxPoint& aPosition ) { m_start = aPosition; }

    wxPoint GetEndPoint() const { return m_end; }

    void SetEndPoint( const wxPoint& aPosition ) { m_end = aPosition; }

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_RECT GetBoundingBox() const;

    /**
     * Function GetLength
     * @return The length of the line segment.
     */
    double GetLength() const;

    virtual void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic line from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic line from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic line.
     * @return True if the schematic line loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    /**
     * Function Move
     * moves the item by \a aMoveVector.
     * @param aMoveVector The displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector );

    virtual void Mirror_X( int aXaxis_position );

    /**
     * Function Mirror_Y
     * mirrors the item relative to \a aYaxis_position.
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );

    virtual void Rotate( wxPoint rotationPoint );

    /**
     * Check line against \a aLine to see if it overlaps and merge if it does.
     *
     * This method will change the line to be equivalent of the line and \a aLine if the
     * two lines overlap.  This method is used to merge multiple line segments into a single
     * line.
     *
     * @param aLine - Line to compare.
     * @return True if lines overlap and the line was merged with \a aLine.
     */
    bool MergeOverlap( SCH_LINE* aLine );

    virtual void GetEndPoints( vector <DANGLING_END_ITEM>& aItemList );

    virtual bool IsDanglingStateChanged( vector< DANGLING_END_ITEM >& aItemList );

    virtual bool IsDangling() const { return m_startIsDangling || m_endIsDangling; }

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    /**
     * Function IsConnectable
     * returns true if the schematic item can connect to another schematic item.
     */
    virtual bool IsConnectable() const;

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const;

    virtual void GetNetListItem( vector<NETLIST_OBJECT*>& aNetListItems,
                                 SCH_SHEET_PATH*          aSheetPath );

    virtual bool operator <( const SCH_ITEM& aItem ) const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const; // override
#endif

private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const;
    virtual bool doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const;
    virtual bool doIsConnected( const wxPoint& aPosition ) const;
    virtual EDA_ITEM* doClone() const;
    virtual void doPlot( PLOTTER* aPlotter );
    virtual wxPoint doGetPosition() const { return m_start; }
    virtual void doSetPosition( const wxPoint& aPosition );
};


#endif    // _SCH_LINE_H_
