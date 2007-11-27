/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2007 Kicad Developers, see change_log.txt for contributors.
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

#ifndef _DRC_STUFF_H
#define _DRC_STUFF_H


#include "fctsys.h"


/// DRC error codes:
#define DRCE_ 1



/**
 * Class DRC_ITEM
 * is a holder for a DRC error item.  It is generated when two objects are
 * too close.  There are holders for information on two items.  The
 * information held is the board coordinate and the MenuText for each item.
 * Also held is the type of error by number and the location of the MARQUEUR.
 * A function is provided to translate that number into text.
 */
class DRC_ITEM
{

protected:
    int         m_ErrorCode;    ///< the error code's numeric value
    wxPoint     m_MarkerPos;    ///< position of the MARKER
    wxString    m_AText;        ///< text for the first BOARD_ITEM
    wxString    m_BText;        ///< text for the second BOARD_ITEM
    wxPoint     m_APos;         ///< the location of the first BOARD_ITEM
    wxPoint     m_BPos;         ///< the location of the first BOARD_ITEM


public:

    DRC_ITEM( int aErrorCode, const wxString& aText, const wxString& bText,
            const wxPoint& aPos, const wxPoint& bPos )
    {
        m_ErrorCode = aErrorCode;
        m_AText     = aText;
        m_BText     = bText;
        m_APos      = aPos;
        m_BPos      = bPos;
    }


    /**
     * Function ShowHtml
     * translates this object into a fragment of HTML suitable for the
     * wxWidget's wxHtmlListBox class.
     * @return wxString - the html text.
     */
    wxString ShowHtml() const
    {
        wxString ret;

        ret.Printf( wxT("<b>%s</b> <ul><li> %s: %s </li> <li> %s: %s </li> </ul>"),
            GetErrorText().GetData(),
                ShowCoord( m_APos ).GetData(), m_AText.GetData(),
                ShowCoord( m_BPos ).GetData(), m_BText.GetData() );

        return ret;
    }


    /**
     * Function ShowText
     * translates this object into a text string suitable for saving
     * to disk in a report.  Change this as needed to format the report.
     * @return wxString - the simple non-html text.
     */
    wxString ShowText() const
    {
        wxString ret;

        ret.Printf( wxT("%s\n    %s: %s\n    %s: %s\n"),
            GetErrorText().GetData(),
                ShowCoord( m_APos ).GetData(), m_AText.GetData(),
                ShowCoord( m_BPos ).GetData(), m_BText.GetData() );
        
        return ret;
    }


    /**
     * Function GetErrorText
     * returns the string form of a drc error code.
     */
    const wxString& GetErrorText() const;


    /**
     * Function ShowCoord
     * formats a coordinate or position to text.
     * @param aPos The position to format
     * @return wxString - The formated string
     */
    static wxString ShowCoord( const wxPoint& aPos );

};


class WinEDA_DrawPanel;
class MARQUEUR;

typedef std::vector<DRC_ITEM>   DRC_LIST;



/**
 * Class DRC_TESTER
 * performs all the DRC tests, and can optionally generate a DRC test report
 * to a disk file.  This class is given access to the windows and the BOARD
 * that it needs via its constructor or access functions.
 */  
class DRC_TESTER
{
protected:
    bool        doPad2PadTest;
    bool        doUnconnectedTest;
    bool        doZonesTest;
    bool        doCreateRptFile;
    
    FILE*       rptFile;
    
    wxString    rptFilename;

    int         errorCount;
    
    MARQUEUR*   currentMarker;

    bool        abortDrc;
    bool        drcInProgress;
    int         spot_cX;
    int         spot_cY;
    int         finx;
    int         finy;           // coord relatives de l'extremite du segm de reference
    
    int         segmAngle;      // angle d'inclinaison du segment de reference en 0,1 degre
    int         segmLong;       // longueur du segment de reference
    
    int         xcliplo;
    int         ycliplo;
    int         xcliphi;
    int         ycliphi;        // coord de la surface de securite du segment a comparer
    
    DRC_LIST    drcList;

    WinEDA_DrawPanel*   drawPanel;
    
public:
    DRC_TESTER()
    {
        doPad2PadTest     = true;
        doUnconnectedTest = true;
        doZonesTest       = false;
        doCreateRptFile   = false;
    }

    
    /**
     * Function SetTests
     * sets all the test flags and may be called before running the tests.
     * @param aPad2PadTest Tells whether to test pad to pad distances.
     * @param aUnconnectedTest Tells whether to list unconnected pads.
     * @param aZonesTest Tells whether to test zones.
     * @param aRptFilename If non-Empty, is the name of the file to 
     *         save the report to.  If Empty, means save no report.
     */
    void SetTests( bool aPad2PadTest, bool aUnconnectedTest, bool aZonesTest, const wxString& aRptFilename )
    {
        doPad2PadTest     = aPad2PadTest;
        doUnconnectedTest = aUnconnectedTest;
        doZonesTest       = aZonesTest;

        rptFilename       = aRptFilename;
        if( rptFilename.IsEmpty() )
            doCreateRptFile = false;
        else
            doCreateRptFile = true;
    }


    
};



#endif  // _DRC_STUFF_H

//EOF
