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
 * Class REPORT_ISSUE
 * is an abstract interface used by DRCLISTBOX.  It has functions to return
 * either html text or disk file report text on this item.  It also can
 * return the drawing coordinate of the report item.
 */
class REPORT_ISSUE
{
public:

    /**
     * Function ShowHtml
     * translates this object into a fragment of HTML suitable for the
     * wxWidget's wxHtmlListBox class.
     * @return wxString - the html text.
     */
    virtual wxString ShowHtml() const = 0;

    
    /**
     * Function ShowText
     * translates this object into a text string suitable for saving
     * to disk in a report.
     * @return wxString - the simple non-html text.
     */
    virtual wxString ShowText() const = 0;


    /**
     * Function GetPosition
     * @return const wxPoint& - the position of this report item within 
     *  the drawing.
     */
    virtual const wxPoint& GetPosition() const = 0;    
};



/**
 * Class DRC_ITEM
 * is a holder for a DRC error item.  It is generated when two objects are
 * too close.  There are holders for information on two items.  The
 * information held is the board coordinate and the MenuText for each item.
 * Also held is the type of error by number and the location of the MARKER.
 * A function is provided to translate that number into text.
 */
class DRC_ITEM : public REPORT_ISSUE 
{

protected:
    int         m_ErrorCode;    ///< the error code's numeric value
    wxPoint     m_Pos;          ///< position of the issue
    wxString    m_AText;        ///< text for the first BOARD_ITEM
    wxString    m_BText;        ///< text for the second BOARD_ITEM
    wxPoint     m_APos;         ///< the location of the first BOARD_ITEM
    wxPoint     m_BPos;         ///< the location of the first BOARD_ITEM


public:

    DRC_ITEM( int aErrorCode, const wxPoint& aIssuePos, 
             const wxString& aText, const wxString& bText,
             const wxPoint& aPos, const wxPoint& bPos )
    {
        m_ErrorCode = aErrorCode;
        m_Pos       = aIssuePos;
        m_AText     = aText;
        m_BText     = bText;
        m_APos      = aPos;
        m_BPos      = bPos;
    }

    //-----<Interface REPORT_ISSUE>---------------------------------------

    /**
     * Function ShowHtml
     * translates this object into a fragment of HTML suitable for the
     * wxWidget's wxHtmlListBox class.
     * @return wxString - the html text.
     */
    wxString ShowHtml() const
    {
        wxString ret;

        ret.Printf( wxT("<b>%s</b><ul><li> %s: %s </li><li> %s: %s </li></ul>"),
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
     * Function GetPosition
     * @return const wxPoint& - the position of this report item within 
     *  the drawing.
     */
    const wxPoint& GetPosition() const
    {
        return m_Pos;
    }

    //-----</Interface REPORT_ISSUE>---------------------------------------

    
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
class MARKER;

/// A smart pointer to a DRC_ITEM
//typedef OWNER<DRC_ITEM>                 DRC_ITEM_OWNER;

/// A list of DRC_ITEM_PTRs
typedef std::vector<DRC_ITEM*>          DRC_LIST;


/**
 * Class DRC_TESTER
 * performs all the DRC tests, and can optionally generate a DRC test report
 * to a disk file.  This class is given access to the windows and the BOARD
 * that it needs via its constructor or access functions.
 */  
class DRC_TESTER
{
protected:
    bool                m_doPad2PadTest;
    bool                m_doUnconnectedTest;
    bool                m_doZonesTest;
    bool                m_doCreateRptFile;
                        
    FILE*               m_rptFile;
                        
    wxString            m_rptFilename;
                        
    int                 m_errorCount;
                        
    MARKER*           m_currentMarker;
                        
    bool                m_aboartDRC;
    bool                m_drcInProgress;
    int                 m_spotcx;
    int                 m_spotcy;
    int                 m_finx;
    int                 m_finy;           // coord relatives de l'extremite du segm de reference
                        
    int                 m_segmAngle;      // angle d'inclinaison du segment de reference en 0,1 degre
    int                 m_segmLong;       // longueur du segment de reference
                        
    int                 m_xcliplo;
    int                 m_ycliplo;
    int                 m_xcliphi;
    int                 m_ycliphi;        // coord de la surface de securite du segment a comparer
                        
    int                 m_unconnectedCount;
                        
    DRC_LIST*           m_drcList;

    WinEDA_DrawPanel*   m_drawPanel;
    
public:
    DRC_TESTER()
    {
        m_doPad2PadTest     = true;
        m_doUnconnectedTest = true;
        m_doZonesTest       = false;
        m_doCreateRptFile   = false;
 
        m_rptFile = 0;
        
        m_errorCount = 0;
        
        m_currentMarker = 0;
        
        m_aboartDRC = false;
        m_drcInProgress = false;
        m_spotcx = 0;
        m_spotcy = 0;
        m_finx = 0;
        m_finy = 0;           // coord relatives de l'extremite du segm de reference
        
        m_segmAngle = 0;      // angle d'inclinaison du segment de reference en 0,1 degre
        m_segmLong = 0;       // longueur du segment de reference
        
        m_xcliplo = 0;
        m_ycliplo = 0;
        m_xcliphi = 0;
        m_ycliphi = 0;        // coord de la surface de securite du segment a comparer

        m_unconnectedCount = 0;        

        m_drcList = new DRC_LIST();
        
        m_drawPanel = 0;
        
        for( int i=0; i<12; ++i )
        {
            DRC_ITEM* ditem = new DRC_ITEM( 2, wxPoint(12000,3000), 
                       wxString( wxT("A item") ), wxString( wxT("B item") ), 
                       wxPoint(12000,3000), wxPoint(13000,3000));
        
            m_drcList->push_back( ditem );
        }
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
        m_doPad2PadTest     = aPad2PadTest;
        m_doUnconnectedTest = aUnconnectedTest;
        m_doZonesTest       = aZonesTest;

        m_rptFilename       = aRptFilename;
        if( m_rptFilename.IsEmpty() )
            m_doCreateRptFile = false;
        else
            m_doCreateRptFile = true;
    }
};



#endif  // _DRC_STUFF_H

//EOF
