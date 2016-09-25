/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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


#include <sch_item_struct.h>

class NETLIST_OBJECT_LIST;

/**
 * Class SCH_LINE
 * is a segment description base class to describe items which have 2 end
 * points (track, wire, draw line ...)
 */
class SCH_LINE : public SCH_ITEM
{
    bool    m_startIsDangling;  ///< True if start point is not connected.
    bool    m_endIsDangling;    ///< True if end point is not connected.
    wxPoint m_start;            ///< Line start point
    wxPoint m_end;              ///< Line end point

public:
    SCH_LINE( const wxPoint& pos = wxPoint( 0, 0 ), int layer = LAYER_NOTES );

    SCH_LINE( const SCH_LINE& aLine );

    ~SCH_LINE() { }

    SCH_LINE* Next() const { return (SCH_LINE*) Pnext; }
    SCH_LINE* Back() const { return (SCH_LINE*) Pback; }

    wxString GetClass() const override
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

    const EDA_RECT GetBoundingBox() const override;

    /**
     * Function GetLength
     * @return The length of the line segment.
     */
    double GetLength() const;

    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
               GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor = UNSPECIFIED_COLOR ) override;

    bool Save( FILE* aFile ) const override;

    bool Load( LINE_READER& aLine, wxString& aErrorMsg ) override;

    int GetPenSize() const override;

    void Move( const wxPoint& aMoveVector ) override;

    void MirrorX( int aXaxis_position ) override;

    void MirrorY( int aYaxis_position ) override;

    void Rotate( wxPoint aPosition ) override;

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

    void GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList ) override;

    bool IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList ) override;

    bool IsDangling() const override { return m_startIsDangling || m_endIsDangling; }

    bool IsSelectStateChanged( const wxRect& aRect ) override;

    bool IsConnectable() const override;

    void GetConnectionPoints(std::vector< wxPoint >& aPoints ) const override;

    wxString GetSelectMenuText() const override;

    BITMAP_DEF GetMenuImage() const override;

    void GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems, SCH_SHEET_PATH* aSheetPath ) override;

    bool operator <( const SCH_ITEM& aItem ) const override;

    wxPoint GetPosition() const override { return m_start; }

    void SetPosition( const wxPoint& aPosition ) override;

    bool HitTest( const wxPoint& aPosition, int aAccuracy ) const override;

    bool HitTest( const EDA_RECT& aRect, bool aContained = false, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter ) override;

    EDA_ITEM* Clone() const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

private:
    bool doIsConnected( const wxPoint& aPosition ) const override;
};


#endif    // _SCH_LINE_H_
