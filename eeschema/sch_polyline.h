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
 * @file sch_polyline.h
 *
 */

#ifndef _SCH_POLYLINE_H_
#define _SCH_POLYLINE_H_


#include "sch_item_struct.h"


class SCH_POLYLINE : public SCH_ITEM
{
public:
    int m_Width;                            /* Thickness */
    std::vector<wxPoint> m_PolyPoints;      // list of points (>= 2)

public:
    SCH_POLYLINE( int layer = LAYER_NOTES );

    SCH_POLYLINE( const SCH_POLYLINE& aPolyLine );

    ~SCH_POLYLINE();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_POLYLINE" );
    }

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
     * Load schematic poly line entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic poly line from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic poly line.
     * @return True if the schematic poly line loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function AddPoint
     * add a corner to m_PolyPoints
     */
    void AddPoint( const wxPoint& point )
    {
        m_PolyPoints.push_back( point );
    }

    /**
     * Function GetCornerCount
     * @return the number of corners
     */

    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    /**
     * Function Move
     * moves an item to a new position by \a aMoveVector.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
            m_PolyPoints[ii] += aMoveVector;
    }

    /**
     * Function Mirror_Y
     * mirrors an item relative to \a aYaxis_position.
     * @param aYaxis_position The y axis position to mirror around.
     */
    virtual void Mirror_Y( int aYaxis_position );

    virtual void Mirror_X( int aXaxis_position );

    virtual void Rotate( wxPoint rotationPoint );

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const;

private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const;
    virtual bool doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const;
    virtual EDA_ITEM* doClone() const;
    virtual wxPoint doGetPosition() const { return m_PolyPoints[0]; }
    virtual void doSetPosition( const wxPoint& aPosition );
};


#endif    // _SCH_POLYLINE_H_
