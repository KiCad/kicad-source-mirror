/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_marker_pcb.h
 * @brief Markers used to show a drc problem on boards.
 */

#ifndef CLASS_MARKER_PCB_H
#define CLASS_MARKER_PCB_H


#include <class_board_item.h>
#include <class_marker_base.h>


class MSG_PANEL_ITEM;


class MARKER_PCB : public BOARD_ITEM, public MARKER_BASE
{

public:

    MARKER_PCB( BOARD_ITEM* aParent );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER_PCB on the BOARD
     * @param aText Text describing the first of two objects
     * @param aPos The position of the first of two objects
     * @param bText Text describing the second of the two conflicting objects
     * @param bPos The position of the second of two objects
     */
    MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                const wxString& aText, const wxPoint& aPos,
                const wxString& bText, const wxPoint& bPos );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER_PCB on the BOARD
     * @param aText Text describing the object
     * @param aPos The position of the object
     */
    MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                const wxString& aText, const wxPoint& aPos );

    ~MARKER_PCB();

    void Move(const wxPoint& aMoveVector)
    {
        m_Pos += aMoveVector;
    }

    void Rotate( const wxPoint& aRotCentre, double aAngle );

    void Flip( const wxPoint& aCentre );

    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
               GR_DRAWMODE aDrawMode, const wxPoint& aOffset = ZeroOffset )
    {
        DrawMarker( aPanel, aDC, aDrawMode, aOffset );
    }

    const wxPoint& GetPosition() const          { return m_Pos; }
    void SetPosition( const wxPoint& aPos )     { m_Pos = aPos; }

    void SetItem( const BOARD_ITEM* aItem )
    {
        m_item = aItem;
    }

    const BOARD_ITEM* GetItem() const
    {
        return m_item;
    }

    bool HitTest( const wxPoint& aPosition ) const
    {
        return HitTestMarker( aPosition );
    }

    bool IsOnLayer( LAYER_ID aLayer ) const;

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList );

    wxString GetSelectMenuText() const;

    BITMAP_DEF GetMenuImage() const { return  drc_xpm; }

    ///> @copydoc VIEW_ITEM::ViewBBox()
    virtual const BOX2I ViewBBox() const
    {
        return GetParent()->ViewBBox();
    }

    ///> @copydoc VIEW_ITEM::ViewGetLayers()
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif

protected:
    ///> Pointer to BOARD_ITEM that causes DRC error.
    const BOARD_ITEM* m_item;
};

#endif      //  CLASS_MARKER_PCB_H
