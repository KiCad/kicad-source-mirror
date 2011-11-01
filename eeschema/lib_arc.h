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
 * @file lib_arc.h
 */

#ifndef _LIB_ARC_H_
#define _LIB_ARC_H_

#include "lib_draw_item.h"


class TRANSFORM;


class LIB_ARC : public LIB_ITEM
{
    enum SELECT_T
    {
        START,
        END,
        OUTLINE,
    };

    int      m_Radius;
    int      m_t1;              /* First radius angle of the arc in 0.1 degrees. */
    int      m_t2;              /* Second radius angle of the arc in 0.1 degrees. */
    wxPoint  m_ArcStart;
    wxPoint  m_ArcEnd;          /* Arc end position. */
    wxPoint  m_Pos;             /* Radius center point. */
    int      m_Width;           /* Line width */
    double   m_editCenterDistance;
    SELECT_T m_editSelectPoint;
    int      m_editState;
    int      m_editDirection;
    int      m_lastEditState;

    /**
     * Draws the arc.
     */
    void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform );

    /**
     * Draw the graphics when the arc is being edited.
     */
    void drawEditGraphics( EDA_RECT* aClipBox, wxDC* aDC, int aColor );

    /**
     * Calculates the center, radius, and angles at \a aPosition when the arc is being edited.
     *
     * Note: The center may not necessarily be on the grid.
     *
     * @param aPosition - The current mouse position in drawing coordinates.
     */
    void calcEdit( const wxPoint& aPosition );

    /**
     * Calculate the radius and angle of an arc using the start, end, and center points.
     */
    void calcRadiusAngles();

public:
    LIB_ARC( LIB_COMPONENT * aParent );
    LIB_ARC( const LIB_ARC& aArc );
    ~LIB_ARC() { }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_ARC" );
    }


    /**
     * Save arc object to a FILE in "*.lib" format.
     *
     * @param aFormatter A reference to an OUTPUTFORMATTER to write the component library
     *                   arc to.
     * @return True if success writing else false.
     */
    virtual bool Save( OUTPUTFORMATTER& aFormatter );

    virtual bool Load( LINE_READER& aLineReader, wxString& aErrorMsg );

    /**
     * Tests if the given wxPoint is within the bounds of this object.
     *
     * @param aPosition - Coordinates to test
     * @return - True if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosition );

     /**
     * @param aPosition - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransform - the transform matrix
     * @return - True if the point \a aPosition is near this object
     */
    virtual bool HitTest( wxPoint aPosition, int aThreshold, const TRANSFORM& aTransform );

    virtual EDA_RECT GetBoundingBox() const;
    virtual void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( ) const;

    /**
     * See LIB_ITEM::BeginEdit().
     */
    void BeginEdit( int aEditMode, const wxPoint aStartPoint = wxPoint( 0, 0 ) );

    /**
     * See LIB_ITEM::ContinueEdit().
     */
    bool ContinueEdit( const wxPoint aNextPoint );

    /**
     * See LIB_ITEM::AbortEdit().
     */
    void EndEdit( const wxPoint& aPosition, bool aAbort = false );

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_arc_xpm; }

protected:
    virtual EDA_ITEM* doClone() const;

    /**
     * Provide the arc draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Arc horizontal (X) position.
     *      - Arc vertical (Y) position.
     *      - Arc start angle.
     *      - Arc end angle.
     */
    virtual int DoCompare( const LIB_ITEM& aOther ) const;
    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_RECT& aRect ) const;
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() const { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoMirrorVertical( const wxPoint& aCenter );
    virtual void DoRotate( const wxPoint& aCenter, bool aRotateCCW = true );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform );
    virtual int DoGetWidth() const { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


#endif    // _LIB_ARC_H_
