/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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

#ifndef _CLASS_DRC_ITEM_H
#define _CLASS_DRC_ITEM_H

#include <macros.h>


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
    int      m_ErrorCode;                       ///< the error code's numeric value
    wxString m_MainText;                        ///< text for the first BOARD_ITEM or SCH_ITEM
    wxString m_AuxiliaryText;                   ///< text for the second BOARD_ITEM or SCH_ITEM
    wxPoint  m_MainPosition;                    ///< the location of the first (or main ) BOARD_ITEM or SCH_ITEM. This is also the position of the marker
    wxPoint  m_AuxiliaryPosition;               ///< the location of the second BOARD_ITEM or SCH_ITEM
    bool     m_hasSecondItem;                   ///< true when 2 items create a DRC/ERC error, false if only one item
    bool     m_noCoordinate;

public:

    DRC_ITEM()
    {
        m_ErrorCode     = 0;
        m_hasSecondItem = false;
        m_noCoordinate = false;
    }

    DRC_ITEM( int aErrorCode,
              const wxString& aMainText, const wxString& bAuxiliaryText,
              const wxPoint& aMainPos, const wxPoint& bAuxiliaryPos )
    {
        SetData( aErrorCode,
                 aMainText, bAuxiliaryText,
                 aMainPos, bAuxiliaryPos );
    }

    DRC_ITEM( int aErrorCode,
              const wxString& aText, const wxPoint& aPos )
    {
        SetData( aErrorCode, aText, aPos );
    }


    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainText = the text concerning the schematic or board item
     * @param aMainPos = position the item and therefore of this issue
     */
    void SetData( int aErrorCode,
                  const wxString& aMainText, const wxPoint& aMainPos )
    {
        SetData( aErrorCode,
                 aMainText, aMainText,
                 aMainPos, aMainPos );
        m_hasSecondItem = false;
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainText = the first text (main text) concerning the main schematic or board item
     * @param bAuxiliaryText = the second text (main text) concerning the second schematic or board item
     * @param aMainPos = position the first item and therefore of this issue
     * @param bAuxiliaryPos = position the second item
     */
    void SetData( int aErrorCode,
                  const wxString& aMainText, const wxString& bAuxiliaryText,
                  const wxPoint& aMainPos, const wxPoint& bAuxiliaryPos )
    {
        m_ErrorCode         = aErrorCode;
        m_MainText          = aMainText;
        m_AuxiliaryText     = bAuxiliaryText;
        m_MainPosition      = aMainPos;
        m_AuxiliaryPosition = bAuxiliaryPos;
        m_hasSecondItem     = true;
        m_noCoordinate      = false;
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
    }


    bool HasSecondItem() const { return m_hasSecondItem; }

    void SetShowNoCoordinate() { m_noCoordinate = true; }

    /** acces to A and B texts
     */
    wxString GetMainText() const { return m_MainText; }
    wxString GetAuxiliaryText() const { return m_AuxiliaryText; }


    /**
     * Function ShowHtml
     * translates this object into a fragment of HTML suitable for the
     * wxWidget's wxHtmlListBox class.
     * @return wxString - the html text.
     */
    wxString ShowHtml() const
    {
        wxString ret;
        wxString mainText = m_MainText;
        // a wxHtmlWindows does not like < and > in the text to display
        // because these chars have a special meaning in html
        mainText.Replace( wxT("<"), wxT("&lt;") );
        mainText.Replace( wxT(">"), wxT("&gt;") );

        wxString errText = GetErrorText();
        errText.Replace( wxT("<"), wxT("&lt;") );
        errText.Replace( wxT(">"), wxT("&gt;") );


        if( m_noCoordinate )
        {
            // omit the coordinate, a NETCLASS has no location
            ret.Printf( _( "ErrType(%d): <b>%s</b><ul><li> %s </li></ul>" ),
                        m_ErrorCode,
                        GetChars( errText ),
                        GetChars( mainText ) );
        }
        else if( m_hasSecondItem )
        {
            wxString auxText = m_AuxiliaryText;
            auxText.Replace( wxT("<"), wxT("&lt;") );
            auxText.Replace( wxT(">"), wxT("&gt;") );

            // an html fragment for the entire message in the listbox.  feel free
            // to add color if you want:
            ret.Printf( _( "ErrType(%d): <b>%s</b><ul><li> %s: %s </li><li> %s: %s </li></ul>" ),
                        m_ErrorCode,
                        GetChars( errText ),
                        GetChars( ShowCoord( m_MainPosition )), GetChars( mainText ),
                        GetChars( ShowCoord( m_AuxiliaryPosition )), GetChars( auxText ) );
        }
        else
        {
            ret.Printf( _( "ErrType(%d): <b>%s</b><ul><li> %s: %s </li></ul>" ),
                        m_ErrorCode,
                        GetChars( errText ),
                        GetChars( ShowCoord( m_MainPosition ) ), GetChars( mainText ) );
        }

        return ret;
    }


    /**
     * Function ShowReport
     * translates this object into a text string suitable for saving
     * to disk in a report.
     * @return wxString - the simple multi-line report text.
     */
    wxString ShowReport() const
    {
        wxString ret;

        if( m_hasSecondItem )
        {
            ret.Printf( wxT( "ErrType(%d): %s\n    %s: %s\n    %s: %s\n" ),
                        m_ErrorCode,
                        GetChars( GetErrorText() ),
                        GetChars( ShowCoord( m_MainPosition ) ), GetChars( m_MainText ),
                        GetChars( ShowCoord( m_AuxiliaryPosition ) ), GetChars( m_AuxiliaryText ) );
        }
        else
        {
            ret.Printf( wxT( "ErrType(%d): %s\n    %s: %s\n" ),
                        m_ErrorCode,
                        GetChars( GetErrorText() ),
                        GetChars( ShowCoord( m_MainPosition ) ), GetChars( m_MainText ) );
        }

        return ret;
    }


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
    static wxString ShowCoord( const wxPoint& aPos );
};


#endif      // _CLASS_DRC_ITEM_H
