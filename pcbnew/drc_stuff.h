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


#define OK_DRC      0
#define BAD_DRC     1


/// DRC error codes:
#define DRCE_                               1   // not used yet
#define DRCE_UNCONNECTED_PADS               2   ///< pads are unconnected
#define DRCE_TRACK_NEAR_THROUGH_HOLE        3   ///< thru hole is too close to track
#define DRCE_TRACK_NEAR_PAD                 4   ///< pad too close to track
#define DRCE_TRACK_NEAR_VIA                 5   ///< track too close to via
#define DRCE_VIA_NEAR_VIA                   6   ///< via too close to via
#define DRCE_VIA_NEAR_TRACK                 7   ///< via too close to track
#define DRCE_TRACK_ENDS1                    8   ///< @todo say what this problem is
#define DRCE_TRACK_ENDS2                    9   ///< @todo say what this problem is
#define DRCE_TRACK_ENDS3                    10  ///< @todo say what this problem is
#define DRCE_TRACK_ENDS4                    11  ///< @todo say what this problem is
#define DRCE_TRACK_UNKNOWN1                 12  ///< @todo check source code and change this comment
#define DRCE_TRACKS_CROSSING                13  ///< tracks are crossing
#define DRCE_ENDS_PROBLEM1                  14  ///< track ends are too close
#define DRCE_ENDS_PROBLEM2                  15  ///< track ends are too close
#define DRCE_ENDS_PROBLEM3                  16  ///< track ends are too close
#define DRCE_ENDS_PROBLEM4                  17  ///< track ends are too close
#define DRCE_ENDS_PROBLEM5                  18  ///< track ends are too close
#define DRCE_PAD_NEAR_PAD1                  19  ///< pad too close to pad
#define DRCE_VIA_HOLE_BIGGER                20  ///< via's hole is bigger than its diameter
#define DRCE_MICRO_VIA_INCORRECT_LAYER_PAIR 21  ///< micro via's layer pair incorrect (layers must be adjacent)
#define COPPERAREA_INSIDE_COPPERAREA        22  ///< copper area outlines intersect
#define COPPERAREA_CLOSE_TO_COPPERAREA      23  ///< copper area outlines are too close
#define DRCE_NON_EXISTANT_NET_FOR_ZONE_OUTLINE  24  ///< copper area outline has an incorrect netcode due to a netname not found
#define DRCE_HOLE_NEAR_PAD                  25  ///< hole too close to pad
#define DRCE_HOLE_NEAR_TRACK                26  ///< hole too close to track
#define DRCE_TOO_SMALL_TRACK_WIDTH          27  ///< Too small track width
#define DRCE_TOO_SMALL_VIA                  28  ///< Too small via size
#define DRCE_TOO_SMALL_MICROVIA             29  ///< Too small micro via size

/**
 * Class DRC_ITEM
 * is a holder for a DRC error item.  It is generated when two objects are
 * too close.  There are holders for information on two items.  The
 * information held is the board coordinate and the MenuText for each item.
 * Also held is the type of error by number and the location of the MARKER.
 * A function is provided to translate that number into text.
 * Some errors involve only one item (item with an incorrect param) so
 * m_hasSecondItem is set to false in this case.
 */
class DRC_ITEM
{
protected:
    int      m_ErrorCode;       ///< the error code's numeric value
    wxPoint  m_Pos;             ///< position of the issue
    wxString m_AText;           ///< text for the first BOARD_ITEM
    wxString m_BText;           ///< text for the second BOARD_ITEM
    wxPoint  m_APos;            ///< the location of the first (or main ) BOARD_ITEM
    wxPoint  m_BPos;            ///< the location of the second BOARD_ITEM
    bool     m_hasSecondItem;   ///< true when 2 items create a DRC error, false if only one item


public:

    DRC_ITEM() :
        m_ErrorCode( 0 )
    {
    }


    DRC_ITEM( int aErrorCode, const wxPoint& aIssuePos,
              const wxString& aText, const wxString& bText,
              const wxPoint& aPos, const wxPoint& bPos )
    {
        SetData( aErrorCode, aIssuePos,
                 aText, bText,
                 aPos, bPos );
    }

    DRC_ITEM( int aErrorCode, const wxPoint& aIssuePos,
              const wxString& aText, const wxPoint& aPos )
    {
        SetData( aErrorCode, aIssuePos, aText, aPos );
    }


    void SetData( int aErrorCode, const wxPoint& aIssuePos,
                  const wxString& aText, const wxPoint& aPos )
    {
        SetData( aErrorCode, aIssuePos,
                 aText, aText,
                 aPos, aPos );
        m_hasSecondItem = false;
    }

    void SetData( int aErrorCode, const wxPoint& aIssuePos,
                  const wxString& aText, const wxString& bText,
                  const wxPoint& aPos, const wxPoint& bPos )
    {
        m_ErrorCode = aErrorCode;
        m_Pos   = aIssuePos;
        m_AText = aText;
        m_BText = bText;
        m_APos  = aPos;
        m_BPos  = bPos;
        m_hasSecondItem = true;
    }

    bool HasSecondItem() const { return m_hasSecondItem; }


    /**
     * Function ShowHtml
     * translates this object into a fragment of HTML suitable for the
     * wxWidget's wxHtmlListBox class.
     * @return wxString - the html text.
     */
    wxString ShowHtml() const
    {
        wxString ret;

        if( m_hasSecondItem )
        {
            // an html fragment for the entire message in the listbox.  feel free
            // to add color if you want:
            ret.Printf( _( "ErrType(%d): <b>%s</b><ul><li> %s: %s </li><li> %s: %s </li></ul>" ),
                       m_ErrorCode,
                       GetErrorText().GetData(),
                       ShowCoord( m_APos ).GetData(), m_AText.GetData(),
                       ShowCoord( m_BPos ).GetData(), m_BText.GetData() );
        }
        else
        {
            ret.Printf( _( "ErrType(%d): <b>%s</b><ul><li> %s: %s </li></ul>" ),
                       m_ErrorCode,
                       GetErrorText().GetData(),
                       ShowCoord( m_APos ).GetData(), m_AText.GetData() );
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
                       GetErrorText().GetData(),
                       ShowCoord( m_APos ).GetData(), m_AText.GetData(),
                       ShowCoord( m_BPos ).GetData(), m_BText.GetData() );
        }
        else
        {
            ret.Printf( wxT( "ErrType(%d): %s\n    %s: %s\n" ),
                       m_ErrorCode,
                       GetErrorText().GetData(),
                       ShowCoord( m_APos ).GetData(), m_AText.GetData() );
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
        return m_AText;
    }


    const wxString& GetTextB() const
    {
        return m_BText;
    }


    const wxPoint&  GetPointA() const
    {
        return m_APos;
    }


    const wxPoint&  GetPointB() const
    {
        return m_BPos;
    }


    /**
     * Function GetPosition
     * @return wxPoint& - the position of this report item within
     *  the drawing.
     */
    const wxPoint&  GetPosition() const
    {
        return m_Pos;
    }


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
class DIALOG_DRC_CONTROL;


/**
 * Class DRC_ITEM_LIST
 * provides an abstract interface of a DRC_ITEM* list manager.  The details
 * of the actual list architecture are hidden from the caller.  Any class
 * that implements this interface can then be used by a DRCLISTBOX class without
 * it knowing the actual architecture of the list.
 */
class DRC_ITEM_LIST
{
public:

    /**
     * Function DeleteAllItems
     * removes and deletes all the items in the list.
     */
    virtual void            DeleteAllItems() = 0;

    /**
     * Function GetItem
     * retrieves a DRC_ITEM by pointer.  The actual item remains owned by the
     * list container.
     * @param aIndex The 0 based index into the list of the desired item.
     * @return const DRC_ITEM* - the desired item or NULL if aIndex is out of range.
     */
    virtual const DRC_ITEM* GetItem( int aIndex ) = 0;

    /**
     * Function DeleteAllItems
     * removes and deletes desired item from the list.
     * @param aIndex The 0 based index into the list of the desired item which
     *         is to be deleted.
     */
    virtual void            DeleteItem( int aIndex ) = 0;

    /**
     * Function GetCount
     * returns the number of items in the list.
     */
    virtual int             GetCount() = 0;

    virtual ~DRC_ITEM_LIST() { }
};


typedef std::vector<DRC_ITEM*>  DRC_LIST;


/**
 * Class DRC
 * is the Design Rule Checker, and performs all the DRC tests.  The output of
 * the checking goes to the BOARD file in the form of two MARKER lists.  Those
 * two lists are displayable in the drc dialog box.  And they can optionally
 * be sent to a text file on disk.
 * This class is given access to the windows and the BOARD
 * that it needs via its constructor or public access functions.
 */
class DRC
{
    friend class DIALOG_DRC_CONTROL;

private:

    //  protected or private functions() are lowercase first character.

    bool     m_doPad2PadTest;
    bool     m_doUnconnectedTest;
    bool     m_doZonesTest;
    bool     m_doCreateRptFile;

    wxString m_rptFilename;

    // int              m_errorCount;

    MARKER*           m_currentMarker;

    bool              m_aboartDRC;
    bool              m_drcInProgress;
    int               m_spotcx;
    int               m_spotcy;
    int               m_finx;
    int               m_finy;               // coord relatives de l'extremite du segm de reference

    int               m_segmAngle;          // angle d'inclinaison du segment de reference en 0,1 degre
    int               m_segmLength;         // length of the reference segment

    int               m_xcliplo;
    int               m_ycliplo;
    int               m_xcliphi;
    int               m_ycliphi;          // coord de la surface de securite du segment a comparer

    WinEDA_PcbFrame*  m_mainWindow;
    WinEDA_DrawPanel* m_drawPanel;
    BOARD*            m_pcb;
    DIALOG_DRC_CONTROL*        m_ui;

    DRC_LIST          m_unconnected;    ///< list of unconnected pads, as DRC_ITEMs


    /**
     * Function updatePointers
     * is a private helper function used to update needed pointers from the
     * one pointer which is known not to change, m_mainWindow.
     */
    void    updatePointers();


    /**
     * Function fillMarker
     * optionally creates a marker and fills it in with information,
     * but does not add it to the BOARD.  Use this to report any kind of
     * DRC problem, or unconnected pad problem.
     *
     * @param aTrack The reference track
     * @param aItem  Another item on the BOARD, such as a SEGVIA, SEGZONE,
     *         or TRACK.
     * @param aErrorCode A categorizing identifier for the particular type
     *         of error that is being reported.
     * @param fillMe A MARKER* which is to be filled in, or NULL if one is to
     *         first be allocated, then filled.
     */
    MARKER* fillMarker( TRACK* aTrack, BOARD_ITEM* aItem, int aErrorCode, MARKER* fillMe );

    MARKER* fillMarker( D_PAD* aPad, D_PAD* bPad, int aErrorCode, MARKER* fillMe );

    MARKER* fillMarker( ZONE_CONTAINER * aArea, int aErrorCode, MARKER* fillMe );

    /**
     * Function fillMarker
     * optionally creates a marker and fills it in with information,
     * but does not add it to the BOARD.  Use this to report any kind of
     * DRC problem, or unconnected pad problem.
     *
     * @param aEdge edge zone to test
     * @param aPos position of error
     * @param aErrorCode  Type of error
     * @param fillMe A MARKER* which is to be filled in, or NULL if one is to
     *         first be allocated, then filled.
     */
    MARKER* fillMarker( const ZONE_CONTAINER * aArea, const wxPoint & aPos, int aErrorCode, MARKER* fillMe );

    //-----<categorical group tests>-----------------------------------------

    void    testTracks();

    void    testPad2Pad();

    void    testUnconnected();

    void    testZones(bool adoTestFillSegments);


    //-----<single "item" tests>-----------------------------------------

    /**
     * Function doPadToPadsDrc
     * tests the clearance between aRefPad and other pads.
     * The pad list must be sorted by x coordinate.
     * @param aRefPad The pad to test
     * @param aStart The start of the pad list to test against
     * @param aEnd Marks the end of the list and is not included
     * @param max_size The size of the biggest pad (used to stop the test when the X distance is > max_size)
     */
    bool    doPadToPadsDrc( D_PAD* aRefPad, LISTE_PAD* aStart,
                            LISTE_PAD* aEnd, int max_size );

    /**
     * Function DoTrackDrc
     * tests the current segment.
     * @param aRefSeg The segment to test
     * @param aStart The head of a list of tracks to test against (usually BOARD::m_Track)
     * @param doPads true if should do pads test
     * @return bool - true if no poblems, else false and m_currentMarker is
     *          filled in with the problem information.
     */
    bool    doTrackDrc( TRACK* aRefSeg, TRACK* aStart, bool doPads = true );


    /**
     * Function doEdgeZoneDrc
     * tests a segment in ZONE_CONTAINER * aArea:
     *      Test Edge inside other areas
     *      Test Edge too close other areas
     * @param aArea The current area.
     * @param aCornerIndex The first corner of the segment to test.
     * @return bool - false if DRC error  or true if OK
     */
    bool doEdgeZoneDrc( ZONE_CONTAINER * aArea, int aCornerIndex );

    //-----<single tests>----------------------------------------------

    /**
     * Function checkClearancePadToPad
     * @param aRefPad The reference pad to check
     * @param aPad Another pad to check against
     * @return bool - true if clearance between aRefPad and pad is >= dist_min, else false
     */
    bool        checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad, const int dist_min );


    /**
     * Function checkClearanceSegmToPad
     * check the distance from a pad to segment.  This function uses several
     * instance variable not passed in:
     *      segmLength = length of the segment being tested
     *      segmAngle  = angle d'inclinaison du segment;
     *      finx, finy = end coordinate of the segment
     *      spot_cX, spot_cY = position of pad / origin of segment
     * @param pad_to_test Is the pad involved in the check
     * @param w_segm Hhalf width of the segment to test
     * @param dist_min Is the minimum clearance needed
     *
     * @return false distance >= dist_min,
     *         true if distance < dist_min
     */
    bool        checkClearanceSegmToPad( const D_PAD* pad_to_test, int w_segm, int dist_min );


    /**
     * Function checkMarginToCircle
     * @todo this translation is no good, fix this:
     * calculates the distance from a circle (via or round end of track) to the
     * segment of reference on the right hand side.
     *
     * @param cx The x coordinate of the circle's center
     * @param cy The y coordinate of the circle's center
     * @param radius A "keep out" radius centered over the circle
     * @param length The length of the segment (i.e. coordinate of end)
     * @return bool - true if distance >= radius, else
     *                false when distance < radius
     */
    static bool checkMarginToCircle( int cx, int cy, int radius, int length );


    /**
     * Function checkLine
     * tests to see if one track is in contact with another track.
     *
     * Cette routine controle si la ligne (x1,y1 x2,y2) a une partie s'inscrivant
     * dans le cadre (xcliplo,ycliplo xcliphi,ycliphi) (variables globales,
     * locales a ce fichier)
     */
    bool        checkLine( int x1, int y1, int x2, int y2 );

    //-----</single tests>---------------------------------------------

public:
    DRC( WinEDA_PcbFrame* aPcbWindow );

    ~DRC();

    /**
     * Function Drc
     * tests the current segment and returns the result and displays the error
     * in the status panel only if one exists.
     * @param aRefSeg The current segment to test.
     * @param aList The track list to test (usually m_Pcb->m_Track)
     * @return int - BAD_DRC (1) if DRC error  or OK_DRC (0) if OK
     */
    int Drc( TRACK* aRefSeg, TRACK* aList );

    /**
     * Function Drc
     * tests the outline segment starting at CornerIndex and returns the result and displays the error
     * in the status panel only if one exists.
     *      Test Edge inside other areas
     *      Test Edge too close other areas
     * @param aEdge The areaparent which contains the corner.
     * @param CornerIndex The starting point of the segment to test.
     * @return int - BAD_DRC (1) if DRC error  or OK_DRC (0) if OK
     */
    int Drc( ZONE_CONTAINER * aArea, int CornerIndex  );

    /**
     * Function DrcBlind
     * tests the current segment and returns the result.  Any error is not
     * displayed in the status panel.
     * @param aRefSeg The current segment to test.
     * @param aList The track list to test (usually m_Pcb->m_Track)
     * @return int - BAD_DRC (1) if DRC error  or OK_DRC (0) if OK
     */
    int DrcBlind( TRACK* aRefSeg, TRACK* aList )
    {
        updatePointers();

        return doTrackDrc( aRefSeg, aList ) ? OK_DRC : BAD_DRC;
    }


    /**
     * Function ShowDialog
     * opens a dialog and prompts the user, then if a test run button is
     * clicked, runs the test(s) and creates the MARKERS.  The dialog is only
     * created if it is not already in existence.
     */
    void    ShowDialog();


    /**
     * Function DestroyDialog
     * deletes this ui dialog box and zeros out its pointer to remember
     * the state of the dialog's existence.
     * @param aReason Indication of which button was clicked to cause the destruction.
     */
    void    DestroyDialog( int aReason );


    /**
     * Function SetSettings
     * saves all the UI or test settings and may be called before running the tests.
     * @param aPad2PadTest Tells whether to test pad to pad distances.
     * @param aUnconnectedTest Tells whether to list unconnected pads.
     * @param aZonesTest Tells whether to test zones.
     * @param aReportName A string telling the disk file report name entered.
     * @param aSaveReport A boolean telling whether to generate disk file report.
     */
    void SetSettings( bool aPad2PadTest, bool aUnconnectedTest,
                      bool aZonesTest, const wxString& aReportName, bool aSaveReport )
    {
        m_doPad2PadTest     = aPad2PadTest;
        m_doUnconnectedTest = aUnconnectedTest;
        m_doZonesTest     = aZonesTest;
        m_rptFilename     = aReportName;
        m_doCreateRptFile = aSaveReport;
    }


    /**
     * Function RunTests
     * will actually run all the tests specified with a previous call to
     * SetSettings()
     */
    void    RunTests();


    /**
     * Function ListUnconnectedPad
     * gathers a list of all the unconnected pads and shows them in the
     * dialog, and optionally prints a report of such.
     */
    void    ListUnconnectedPads();
};


#endif  // _DRC_STUFF_H

//EOF
