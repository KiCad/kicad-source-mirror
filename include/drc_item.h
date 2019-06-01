/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_ITEM_H
#define DRC_ITEM_H

#include <macros.h>
#include <base_struct.h>

class MARKER_BASE;
class BOARD;
class BOARD_ITEM;


/**
 * Class DRC_ITEM
 * is a holder for a DRC (in Pcbnew) or ERC (in Eeschema) error item.
 * It is generated when two objects are too close (DRC)
 * or two connected objects (pins) have incompatible electrical types (ERC).
 * There are holders for information on two items.  The
 * information held is the board coordinate and the MenuText for each item.
 * Also held is the type of error by number and the location of the MARKER.
 * A function is provided to translate that number into text.
 * Some errors involve only one item (item with an incorrect param) so
 * m_hasSecondItem is set to false in this case.
 */
class DRC_ITEM
{
protected:
    int          m_ErrorCode;         // the error code's numeric value
    wxString     m_MainText;          // text for the first BOARD_ITEM or SCH_ITEM
    wxString     m_AuxiliaryText;     // text for the second BOARD_ITEM or SCH_ITEM
    wxPoint      m_MainPosition;      // the location of the first (or main ) BOARD_ITEM or SCH_ITEM.
    wxPoint      m_AuxiliaryPosition; // the location of the second BOARD_ITEM or SCH_ITEM
    bool         m_hasSecondItem;     // true when 2 items create a DRC/ERC error, false if only one item
    bool         m_noCoordinate;

    MARKER_BASE* m_parent;            // The marker this item belongs to, if any
    void*        m_mainItemWeakRef;   // search the current BOARD_ITEMs or SCH_ITEMs for a match
    void*        m_auxItemWeakRef;    // search the current BOARD_ITEMs or SCH_ITEMs for a match

public:

    DRC_ITEM()
    {
        m_ErrorCode       = 0;
        m_hasSecondItem   = false;
        m_noCoordinate    = false;
        m_parent          = nullptr;
        m_mainItemWeakRef = nullptr;
        m_auxItemWeakRef  = nullptr;
    }

    DRC_ITEM( EDA_UNITS_T aUnits, int aErrorCode, EDA_ITEM* aMainItem, const wxPoint& aMainPos,
              EDA_ITEM* bAuxiliaryItem, const wxPoint& bAuxiliaryPos )
    {
        SetData( aUnits, aErrorCode, aMainItem, aMainPos, bAuxiliaryItem, bAuxiliaryPos );
    }

    DRC_ITEM( EDA_UNITS_T aUnits, int aErrorCode, EDA_ITEM* aMainItem, const wxPoint& aMainPos )
    {
        SetData( aUnits, aErrorCode, aMainItem, aMainPos );
    }


    DRC_ITEM( int aErrorCode, const wxString& aMainText )
    {
        SetData( aErrorCode, aMainText, wxPoint() );
        SetShowNoCoordinate();
    }


    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainItem = the first (main) schematic or board item
     * @param bAuxiliaryItem = the second schematic or board item
     * @param aMainPos = position the first item and therefore of this issue
     * @param bAuxiliaryPos = position the second item
     */
    void SetData( EDA_UNITS_T aUnits, int aErrorCode, EDA_ITEM* aMainItem, const wxPoint& aMainPos,
                  EDA_ITEM* bAuxiliaryItem = nullptr, const wxPoint& bAuxiliaryPos = wxPoint() )
    {
        m_ErrorCode         = aErrorCode;
        m_MainText          = aMainItem->GetSelectMenuText( aUnits );
        m_AuxiliaryText     = wxEmptyString;
        m_MainPosition      = aMainPos;
        m_AuxiliaryPosition = bAuxiliaryPos;
        m_hasSecondItem     = bAuxiliaryItem != nullptr;
        m_noCoordinate      = false;
        m_parent            = nullptr;

        if( m_hasSecondItem )
            m_AuxiliaryText = bAuxiliaryItem->GetSelectMenuText( aUnits );

        // Weak references (void*).  One must search the BOARD_ITEMS or SCH_ITEMS for a match.
        m_mainItemWeakRef   = aMainItem;
        m_auxItemWeakRef    = bAuxiliaryItem;
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainItem = the first (main) schematic or board item
     * @param bAuxiliaryItem = the second schematic or board item
     * @param aMainPos = position the first item and therefore of this issue
     * @param bAuxiliaryPos = position the second item
     */
    void SetData( int aErrorCode, const wxString& aMainText, const wxPoint& aMainPos,
                  const wxString& bAuxiliaryText = wxEmptyString, const wxPoint& bAuxiliaryPos = wxPoint() )
    {
        m_ErrorCode         = aErrorCode;
        m_MainText          = aMainText;
        m_AuxiliaryText     = bAuxiliaryText;
        m_MainPosition      = aMainPos;
        m_AuxiliaryPosition = bAuxiliaryPos;
        m_hasSecondItem     = bAuxiliaryText.Length();
        m_noCoordinate      = false;
        m_parent            = nullptr;

        m_mainItemWeakRef   = nullptr;
        m_auxItemWeakRef    = nullptr;
    }

    /**
     * Function SetAuxiliaryData
     * initialize data for the second (auxiliary) item
     * @param aAuxiliaryText = the second text (main text) concerning the second schematic or board item
     * @param aAuxiliaryPos = position the second item
     */
    void SetAuxiliaryData( const wxString& aAuxiliaryText, const wxPoint& aAuxiliaryPos )
    {
        m_AuxiliaryText     = aAuxiliaryText;
        m_AuxiliaryPosition = aAuxiliaryPos;
        m_hasSecondItem     = true;

        m_auxItemWeakRef  = nullptr;
    }

    void SetParent( MARKER_BASE* aMarker ) { m_parent = aMarker; }

    MARKER_BASE* GetParent() const { return m_parent; }

    bool HasSecondItem() const { return m_hasSecondItem; }

    void SetShowNoCoordinate() { m_noCoordinate = true; }

    /**
     * Access to A and B texts
     */
    wxString GetMainText() const { return m_MainText; }
    wxString GetAuxiliaryText() const { return m_AuxiliaryText; }

    /**
     * Access to A and B items for BOARDs
     */
    BOARD_ITEM* GetMainItem( BOARD* aBoard ) const;
    BOARD_ITEM* GetAuxiliaryItem( BOARD* aBoard ) const;

    /**
     * Function ShowHtml
     * translates this object into a fragment of HTML suitable for the
     * wxWidget's wxHtmlListBox class.
     * @return wxString - the html text.
     */
    wxString ShowHtml( EDA_UNITS_T aUnits ) const;

    /**
     * Function ShowReport
     * translates this object into a text string suitable for saving
     * to disk in a report.
     * @return wxString - the simple multi-line report text.
     */
    wxString ShowReport( EDA_UNITS_T aUnits ) const;

    /**
     * Function GetErrorCode
     * returns the error code.
     */
    int GetErrorCode() const
    {
        return m_ErrorCode;
    }

    /**
     * Function GetErrorText
     * returns the string form of a drc error code.
     */
    wxString GetErrorText() const;

    const wxString& GetTextA() const
    {
        return m_MainText;
    }


    const wxString& GetTextB() const
    {
        return m_AuxiliaryText;
    }


    const wxPoint& GetPointA() const
    {
        return m_MainPosition;
    }


    const wxPoint& GetPointB() const
    {
        return m_AuxiliaryPosition;
    }


    /**
     * Function ShowCoord
     * formats a coordinate or position to text.
     * @param aPos The position to format
     * @return wxString - The formated string
     */
    static wxString ShowCoord( EDA_UNITS_T aUnits, const wxPoint& aPos );
};


#endif      // DRC_ITEM_H
