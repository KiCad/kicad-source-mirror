/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * DRC_ITEM
 * is a holder for a DRC (in Pcbnew) or ERC (in Eeschema) error item.
 * There are holders for information on two EDA_ITEMs.  Some errors involve only one item
 * (item with an incorrect param) so m_hasSecondItem is set to false in this case.
 */
class DRC_ITEM
{
protected:
    int          m_ErrorCode;         // the error code's numeric value
    wxString     m_MainText;          // text for the first EDA_ITEM
    wxString     m_AuxText;           // text for the second EDA_ITEM
    wxPoint      m_MainPosition;      // the location of the first EDA_ITEM
    wxPoint      m_AuxPosition;       // the location of the second EDA_ITEM
    bool         m_hasPositions;
    bool         m_hasSecondItem;     // true when 2 items create a DRC/ERC error
    MARKER_BASE* m_parent;            // The marker this item belongs to, if any
    KIID         m_mainItemUuid;
    KIID         m_auxItemUuid;

public:

    DRC_ITEM()
    {
        m_ErrorCode     = 0;
        m_hasPositions  = false;
        m_hasSecondItem = false;
        m_parent        = nullptr;
        m_mainItemUuid  = niluuid;
        m_auxItemUuid   = niluuid;
    }

    DRC_ITEM( EDA_UNITS aUnits, int aErrorCode,
              EDA_ITEM* aMainItem,
              EDA_ITEM* bAuxItem = nullptr )
    {
        SetData( aUnits, aErrorCode, aMainItem, bAuxItem );
    }

    DRC_ITEM( EDA_UNITS aUnits, int aErrorCode,
              EDA_ITEM* aMainItem, const wxPoint& aMainPos,
              EDA_ITEM* bAuxItem = nullptr, const wxPoint& bAuxPos = wxPoint() )
    {
        SetData( aUnits, aErrorCode, aMainItem, aMainPos, bAuxItem, bAuxPos );
    }

    DRC_ITEM( int aErrorCode, const wxString& aMainText )
    {
        SetData( aErrorCode, aMainText, wxPoint() );
        m_hasPositions = false;
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainItem = the first (main) schematic or board item
     * @param bAuxItem = the second schematic or board item
     */
    void SetData( EDA_UNITS aUnits, int aErrorCode,
                  EDA_ITEM* aMainItem,
                  EDA_ITEM* bAuxItem = nullptr )
    {
        m_ErrorCode       = aErrorCode;
        m_MainText        = aMainItem->GetSelectMenuText( aUnits );
        m_AuxText         = wxEmptyString;
        m_hasPositions    = false;
        m_hasSecondItem   = bAuxItem != nullptr;
        m_parent          = nullptr;
        m_mainItemUuid    = aMainItem->m_Uuid;

        if( m_hasSecondItem )
        {
            m_AuxText     = bAuxItem->GetSelectMenuText( aUnits );
            m_auxItemUuid = bAuxItem->m_Uuid;
        }
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainItem = the first (main) schematic or board item
     * @param bAuxItem = the second schematic or board item
     * @param aMainPos = position the first item and therefore of this issue
     * @param bAuxPos = position the second item
     */
    void SetData( EDA_UNITS aUnits, int aErrorCode,
                  EDA_ITEM* aMainItem, const wxPoint& aMainPos,
                  EDA_ITEM* bAuxItem = nullptr, const wxPoint& bAuxPos = wxPoint() )
    {
        m_ErrorCode       = aErrorCode;
        m_MainText        = aMainItem->GetSelectMenuText( aUnits );
        m_AuxText         = wxEmptyString;
        m_MainPosition    = aMainPos;
        m_AuxPosition     = bAuxPos;
        m_hasPositions    = true;
        m_hasSecondItem   = bAuxItem != nullptr;
        m_parent          = nullptr;
        m_mainItemUuid    = aMainItem->m_Uuid;

        if( m_hasSecondItem )
        {
            m_AuxText     = bAuxItem->GetSelectMenuText( aUnits );
            m_auxItemUuid = bAuxItem->m_Uuid;
        }
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainText = a description of the first (main) item
     * @param bAuxText = a description of the second item
     */
    void SetData( int aErrorCode,
                  const wxString& aMainText,
                  const wxString& bAuxText = wxEmptyString )
    {
        m_ErrorCode     = aErrorCode;
        m_MainText      = aMainText;
        m_AuxText       = bAuxText;
        m_hasPositions  = false;
        m_hasSecondItem = !bAuxText.IsEmpty();
        m_parent        = nullptr;
        m_mainItemUuid  = niluuid;
        m_auxItemUuid   = niluuid;
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainText = a description of the first (main) item
     * @param aMainPos = position the first item and therefore of this issue
     * @param bAuxText = a description of the second item
     * @param bAuxPos = position the second item
     */
    void SetData( int aErrorCode,
                  const wxString& aMainText, const wxPoint& aMainPos,
                  const wxString& bAuxText = wxEmptyString, const wxPoint& bAuxPos = wxPoint() )
    {
        m_ErrorCode     = aErrorCode;
        m_MainText      = aMainText;
        m_AuxText       = bAuxText;
        m_MainPosition  = aMainPos;
        m_AuxPosition   = bAuxPos;
        m_hasPositions  = true;
        m_hasSecondItem = !bAuxText.IsEmpty();
        m_parent        = nullptr;
        m_mainItemUuid  = niluuid;
        m_auxItemUuid   = niluuid;
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainText = a description of the first (main) item
     * @param aMainID = UUID of the main item
     * @param bAuxText = a description of the second item
     * @param bAuxID = UUID of the second item
     */
    void SetData( int aErrorCode,
                  const wxString& aMainText, const KIID& aMainID,
                  const wxString& bAuxText, const KIID& bAuxID )
    {
        m_ErrorCode     = aErrorCode;
        m_MainText      = aMainText;
        m_AuxText       = bAuxText;
        m_hasPositions  = false;
        m_hasSecondItem = !bAuxText.IsEmpty() || bAuxID != niluuid;
        m_parent        = nullptr;
        m_mainItemUuid  = aMainID;
        m_auxItemUuid   = bAuxID;
    }

    /**
     * Function SetAuxiliaryData
     * initialize data for the second (auxiliary) item
     * @param aAuxiliaryText = the second text (main text) concerning the second schematic or board item
     * @param aAuxiliaryPos = position the second item
     */
    void SetAuxiliaryData( const wxString& aAuxiliaryText, const wxPoint& aAuxiliaryPos )
    {
        m_AuxText       = aAuxiliaryText;
        m_AuxPosition   = aAuxiliaryPos;
        m_hasSecondItem = true;
        m_auxItemUuid   = niluuid;
    }

    void SetParent( MARKER_BASE* aMarker ) { m_parent = aMarker; }
    MARKER_BASE* GetParent() const { return m_parent; }

    bool HasSecondItem() const { return m_hasSecondItem; }

    bool HasPositions() { return m_hasPositions; }

    /**
     * Access to A and B texts
     */
    wxString GetMainText() const { return m_MainText; }
    wxString GetAuxiliaryText() const { return m_AuxText; }

    /**
     * Access to A and B items for BOARDs
     */
    BOARD_ITEM* GetMainItem( BOARD* aBoard ) const;
    BOARD_ITEM* GetAuxiliaryItem( BOARD* aBoard ) const;

    KIID GetMainItemID() const { return m_mainItemUuid; }
    KIID GetAuxItemID() const { return m_auxItemUuid; }

    /**
     * Function ShowHtml
     * translates this object into a fragment of HTML suitable for the wxHtmlListBox class.
     * @return wxString - the html text.
     */
    wxString ShowHtml( EDA_UNITS aUnits ) const;

    /**
     * Function ShowReport
     * translates this object into a text string suitable for saving to disk in a report.
     * @return wxString - the simple multi-line report text.
     */
    wxString ShowReport( EDA_UNITS aUnits ) const;

    int GetErrorCode() const { return m_ErrorCode; }

    /**
     * Function GetErrorText
     * returns the string form of a drc error code.
     */
    wxString GetErrorText() const;

    const wxString& GetTextA() const { return m_MainText; }
    const wxString& GetTextB() const { return m_AuxText; }

    const wxPoint& GetPointA() const { return m_MainPosition; }
    const wxPoint& GetPointB() const { return m_AuxPosition; }

    /**
     * Function ShowCoord
     * formats a coordinate or position to text.
     */
    static wxString ShowCoord( EDA_UNITS aUnits, const wxPoint& aPos );
};


#endif      // DRC_ITEM_H
