/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#ifndef MARKER_BASE_H
#define MARKER_BASE_H

#include <drc_item.h>
#include <gr_basic.h>
#include <eda_rect.h>
class SHAPE_LINE_CHAIN;

/* Marker are mainly used to show a DRC or ERC error or warning
 */


class MARKER_BASE
{
public:
    enum TYPEMARKER {   // Marker type: can be used to identify the purpose of the marker
        MARKER_UNSPEC,
        MARKER_ERC,
        MARKER_PCB,
        MARKER_SIMUL
    };
    enum MARKER_SEVERITY {  // Severity of the marker: this is the level of error
        MARKER_SEVERITY_UNSPEC,
        MARKER_SEVERITY_INFO,
        MARKER_SEVERITY_WARNING,
        MARKER_SEVERITY_ERROR
    };

    wxPoint               m_Pos;                 ///< position of the marker

protected:
    int                   m_ScalingFactor;       ///< Scaling factor to convert corners coordinates
                                                 ///< to internat units coordinates
    TYPEMARKER            m_MarkerType;          ///< The type of marker (useful to filter markers)
    MARKER_SEVERITY       m_ErrorLevel;          ///< Specify the severity of the error
    COLOR4D               m_Color;               ///< color
    EDA_RECT              m_ShapeBoundingBox;    ///< Bounding box of the graphic symbol, relative
                                                 ///< to the position of the shape, in marker shape units
    DRC_ITEM              m_drc;

    void init();

public:

    MARKER_BASE( int aScalingFactor );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER on the BOARD
     * @param aItem The first of two objects
     * @param aPos The position of the first of two objects
     * @param bItem The second of the two conflicting objects
     * @param bPos The position of the second of two objects
     * @param aScalingFactor the scaling factor to convert the shape coordinates to IU coordinates
     */
    MARKER_BASE( EDA_UNITS_T aUnits, int aErrorCode, const wxPoint& aMarkerPos,
                 EDA_ITEM* aItem, const wxPoint& aPos,
                 EDA_ITEM* bItem, const wxPoint& bPos, int aScalingFactor );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER on the BOARD
     * @param aText Text describing the first of two objects
     * @param aPos The position of the first of two objects
     * @param bText Text describing the second of the two conflicting objects
     * @param bPos The position of the second of two objects
     * @param aScalingFactor the scaling factor to convert the shape coordinates to IU coordinates
     */
    MARKER_BASE( int aErrorCode, const wxPoint& aMarkerPos,
                 const wxString& aText, const wxPoint& aPos,
                 const wxString& bText, const wxPoint& bPos, int aScalingFactor );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER on the BOARD
     * @param aText Text describing the object
     * @param aPos The position of the object
     * @param aScalingFactor the scaling factor to convert the shape coordinates to IU coordinates
     */
    MARKER_BASE( int aErrorCode, const wxPoint& aMarkerPos,
                 const wxString& aText, const wxPoint& aPos, int aScalingFactor );

    /**
     * Contructor
     * makes a copy of \a aMarker but does not copy the DRC_ITEM.
     *
     * @param aMarker The marker to copy.
     */
    MARKER_BASE( const MARKER_BASE& aMarker );

    ~MARKER_BASE();

    /** The scaling factor to convert polygonal shape coordinates to internal units
     */
    int MarkerScale() const { return m_ScalingFactor; }

    /** Returns the shape polygon in internal units in a SHAPE_LINE_CHAIN
     * the coordinates are relatives to the marker position (are not absolute)
     * @param aPolygon is the SHAPE_LINE_CHAIN to fill with the shape
     */
    void ShapeToPolygon( SHAPE_LINE_CHAIN& aPolygon) const;

    /** @return the shape corner list
     */
    const VECTOR2I* GetShapePolygon() const;

    /** @return the shape polygon corner aIdx
     */
    const VECTOR2I& GetShapePolygonCorner( int aIdx ) const;

    /** @return the default shape polygon corner count
     */
    int GetShapePolygonCornerCount() const;

    /**
     * Function PrintMarker
     * Prints the shape is the polygon defined in m_Corners (array of wxPoints).
     */
    void PrintMarker( wxDC* aDC, const wxPoint& aOffset );

    /**
     * Function GetPos
     * @return the position of this MARKER in internal units.
     */
    const wxPoint& GetPos() const
    {
        return m_Pos;
    }

    /**
     * Function SetColor
     * Set the color of this marker
     */
    void SetColor( COLOR4D aColor )
    {
        m_Color = aColor;
    }

    /**
     * accessors to set/get error levels (warning, error, fatal error..)
     */
    void SetErrorLevel( MARKER_SEVERITY aErrorLevel )
    {
        m_ErrorLevel = aErrorLevel;
    }

    MARKER_SEVERITY GetErrorLevel() const
    {
        return m_ErrorLevel;
    }

    /** accessors to set/get marker type (DRC, ERC, or other)
     */
    void SetMarkerType( enum TYPEMARKER aMarkerType )
    {
        m_MarkerType = aMarkerType;
    }

    enum TYPEMARKER GetMarkerType() const
    {
        return m_MarkerType;
    }

    /**
     * Function SetData
     * fills in all the reportable data associated with a MARKER.
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER on the BOARD
     * @param aItem The first of two objects
     * @param aPos The position of the first of two objects
     * @param bItem The second of the two conflicting objects
     * @param bPos The position of the second of two objects
     */
    void SetData( EDA_UNITS_T aUnits, int aErrorCode, const wxPoint& aMarkerPos,
                  EDA_ITEM* aItem, const wxPoint& aPos,
                  EDA_ITEM* bItem = nullptr, const wxPoint& bPos = wxPoint() );

    /**
     * Function SetData
     * fills in all the reportable data associated with a MARKER.
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER on the BOARD
     * @param aText Text describing the first of two objects
     * @param aPos The position of the first of two objects
     * @param bText Text describing the second of the two conflicting objects
     * @param bPos The position of the second of two objects
     */
    void SetData( int aErrorCode, const wxPoint& aMarkerPos,
                  const wxString& aText, const wxPoint& aPos,
                  const wxString& bText = wxEmptyString, const wxPoint& bPos = wxPoint() );

    /**
     * Function SetAuxiliaryData
     * initialize data for the second (auxiliary) item
     * @param aAuxiliaryText = the second text (main text) concerning the second schematic or
     *                         board item
     * @param aAuxiliaryPos = position the second item
     */
    void SetAuxiliaryData( const wxString& aAuxiliaryText, const wxPoint& aAuxiliaryPos )
    {
        m_drc.SetAuxiliaryData( aAuxiliaryText, aAuxiliaryPos );
    }

    void SetShowNoCoordinate()
    {
        m_drc.SetShowNoCoordinate();
    }

    /**
     * Function GetReporter
     * returns the DRC_ITEM held within this MARKER so that its
     * interface may be used.
     * @return const& DRC_ITEM
     */
    const DRC_ITEM& GetReporter() const
    {
        return m_drc;
    }

    /**
     * Function DisplayMarkerInfo
     * displays the full info of this marker, in a HTML window.
     */
    void DisplayMarkerInfo( EDA_DRAW_FRAME* aFrame );

    /**
     * Tests if the given wxPoint is within the bounds of this object.
     * @param aHitPosition is the wxPoint to test (in internal units)
     * @return bool - true if a hit, else false
     */
    bool HitTestMarker( const wxPoint& aHitPosition, int aAccuracy ) const;

    /**
     * Function GetBoundingBoxMarker
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_RECT GetBoundingBoxMarker() const;
};


#endif      //  MARKER_BASE_H
