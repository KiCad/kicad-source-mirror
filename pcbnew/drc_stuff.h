/**
 * @file drc_stuff.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

#include <vector>
#include <boost/shared_ptr.hpp>

#define OK_DRC  0
#define BAD_DRC 1


/// DRC error codes:
#define DRCE_                                  1    // not used yet
#define DRCE_UNCONNECTED_PADS                  2    ///< pads are unconnected
#define DRCE_TRACK_NEAR_THROUGH_HOLE           3    ///< thru hole is too close to track
#define DRCE_TRACK_NEAR_PAD                    4    ///< pad too close to track
#define DRCE_TRACK_NEAR_VIA                    5    ///< track too close to via
#define DRCE_VIA_NEAR_VIA                      6    ///< via too close to via
#define DRCE_VIA_NEAR_TRACK                    7    ///< via too close to track
#define DRCE_TRACK_ENDS1                       8    ///< 2 parallel track segments too close: fine start point test
#define DRCE_TRACK_ENDS2                       9    ///< 2 parallel track segments too close: fine start point test
#define DRCE_TRACK_ENDS3                       10   ///< 2 parallel track segments too close: fine end point test
#define DRCE_TRACK_ENDS4                       11   ///< 2 parallel track segments too close: fine end point test
#define DRCE_TRACK_SEGMENTS_TOO_CLOSE          12   ///< 2 parallel track segments too close: segm ends between segref ends
#define DRCE_TRACKS_CROSSING                   13   ///< tracks are crossing
#define DRCE_ENDS_PROBLEM1                     14   ///< track ends are too close
#define DRCE_ENDS_PROBLEM2                     15   ///< track ends are too close
#define DRCE_ENDS_PROBLEM3                     16   ///< track ends are too close
#define DRCE_ENDS_PROBLEM4                     17   ///< track ends are too close
#define DRCE_ENDS_PROBLEM5                     18   ///< track ends are too close
#define DRCE_PAD_NEAR_PAD1                     19   ///< pad too close to pad
#define DRCE_VIA_HOLE_BIGGER                   20   ///< via's hole is bigger than its diameter
#define DRCE_MICRO_VIA_INCORRECT_LAYER_PAIR    21   ///< micro via's layer pair incorrect (layers must be adjacent)
#define COPPERAREA_INSIDE_COPPERAREA           22   ///< copper area outlines intersect
#define COPPERAREA_CLOSE_TO_COPPERAREA         23   ///< copper area outlines are too close
#define DRCE_SUSPICIOUS_NET_FOR_ZONE_OUTLINE   24   ///< copper area has a net but no pads in nets, which is suspicious
#define DRCE_HOLE_NEAR_PAD                     25   ///< hole too close to pad
#define DRCE_HOLE_NEAR_TRACK                   26   ///< hole too close to track
#define DRCE_TOO_SMALL_TRACK_WIDTH             27   ///< Too small track width
#define DRCE_TOO_SMALL_VIA                     28   ///< Too small via size
#define DRCE_TOO_SMALL_MICROVIA                29   ///< Too small micro via size
#define DRCE_NETCLASS_TRACKWIDTH               30   ///< netclass has TrackWidth < board.m_designSettings->m_TrackMinWidth
#define DRCE_NETCLASS_CLEARANCE                31   ///< netclass has Clearance < board.m_designSettings->m_TrackClearance
#define DRCE_NETCLASS_VIASIZE                  32   ///< netclass has ViaSize < board.m_designSettings->m_ViasMinSize
#define DRCE_NETCLASS_VIADRILLSIZE             33   ///< netclass has ViaDrillSize < board.m_designSettings->m_ViasMinDrill
#define DRCE_NETCLASS_uVIASIZE                 34   ///< netclass has ViaSize < board.m_designSettings->m_MicroViasMinSize
#define DRCE_NETCLASS_uVIADRILLSIZE            35   ///< netclass has ViaSize < board.m_designSettings->m_MicroViasMinDrill
#define DRCE_VIA_INSIDE_KEEPOUT                36   ///< Via in inside a keepout area
#define DRCE_TRACK_INSIDE_KEEPOUT              37   ///< Track in inside a keepout area
#define DRCE_PAD_INSIDE_KEEPOUT                38   ///< Pad in inside a keepout area
#define DRCE_VIA_INSIDE_TEXT                   39   ///< Via in inside a text area
#define DRCE_TRACK_INSIDE_TEXT                 40   ///< Track in inside a text area
#define DRCE_PAD_INSIDE_TEXT                   41   ///< Pad in inside a text area


class EDA_DRAW_PANEL;
class PCB_EDIT_FRAME;
class DIALOG_DRC_CONTROL;
class BOARD_ITEM;
class BOARD;
class D_PAD;
class ZONE_CONTAINER;
class TRACK;
class MARKER_PCB;
class DRC_ITEM;
class NETCLASS;


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
    virtual void DeleteAllItems() = 0;

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
    virtual void DeleteItem( int aIndex ) = 0;

    /**
     * Function GetCount
     * returns the number of items in the list.
     */
    virtual int GetCount() = 0;

    virtual ~DRC_ITEM_LIST() { }
};


typedef std::vector<DRC_ITEM*> DRC_LIST;


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
    bool     m_doKeepoutTest;
    bool     m_doCreateRptFile;

    wxString m_rptFilename;

    MARKER_PCB* m_currentMarker;

    bool        m_abortDRC;
    bool        m_drcInProgress;

    /* In DRC functions, many calculations are using coordinates relative
     * to the position of the segment under test (segm to segm DRC, segm to pad DRC
     * Next variables store coordinates relative to the start point of this segment
     */
    wxPoint m_padToTestPos; // Position of the pad to compare in drc test segm to pad or pad to pad
    wxPoint m_segmEnd;      // End point of the reference segment (start point = (0,0) )

    /* Some functions are comparing the ref segm to pads or others segments using
     * coordinates relative to the ref segment considered as the X axis
     * so we store the ref segment length (the end point relative to these axis)
     * and the segment orientation (used to rotate other coordinates)
     */
    double m_segmAngle;     // Ref segm orientation in 0,1 degre
    int m_segmLength;       // length of the reference segment

    /* variables used in checkLine to test DRC segm to segm:
     * define the area relative to the ref segment that does not contains any other segment
     */
    int                 m_xcliplo;
    int                 m_ycliplo;
    int                 m_xcliphi;
    int                 m_ycliphi;

    PCB_EDIT_FRAME*     m_mainWindow;
    BOARD*              m_pcb;
    DIALOG_DRC_CONTROL* m_ui;

    DRC_LIST            m_unconnected;  ///< list of unconnected pads, as DRC_ITEMs


    /**
     * Function updatePointers
     * is a private helper function used to update needed pointers from the
     * one pointer which is known not to change, m_mainWindow.
     */
    void updatePointers();


    /**
     * Function fillMarker
     * optionally creates a marker and fills it in with information,
     * but does not add it to the BOARD.  Use this to report any kind of
     * DRC problem, or unconnected pad problem.
     *
     * @param aTrack The reference track.
     * @param aItem  Another item on the BOARD, such as a VIA, SEGZONE,
     *               or TRACK.
     * @param aErrorCode A categorizing identifier for the particular type
     *                   of error that is being reported.
     * @param fillMe A MARKER_PCB* which is to be filled in, or NULL if one is to
     *               first be allocated, then filled.
     */
    MARKER_PCB* fillMarker( const TRACK* aTrack, BOARD_ITEM* aItem, int aErrorCode, MARKER_PCB* fillMe );

    MARKER_PCB* fillMarker( D_PAD* aPad, BOARD_ITEM* aItem, int aErrorCode, MARKER_PCB* fillMe );

    MARKER_PCB* fillMarker( ZONE_CONTAINER* aArea, int aErrorCode, MARKER_PCB* fillMe );

    /**
     * Function fillMarker
     * optionally creates a marker and fills it in with information,
     * but does not add it to the BOARD.  Use this to report any kind of
     * DRC problem, or unconnected pad problem.
     *
     * @param aArea The zone to test
     * @param aPos position of error
     * @param aErrorCode  Type of error
     * @param fillMe A MARKER_PCB* which is to be filled in, or NULL if one is to
     *               first be allocated, then filled.
     */
    MARKER_PCB* fillMarker( const ZONE_CONTAINER* aArea,
                            const wxPoint&        aPos,
                            int                   aErrorCode,
                            MARKER_PCB*           fillMe );

    /**
     * Function fillMarker
     * fills a MARKER which will report on a generic problem with the board which is
     * not geographically locatable.
     */
    MARKER_PCB* fillMarker( int aErrorCode, const wxString& aMessage, MARKER_PCB* fillMe );


    //-----<categorical group tests>-----------------------------------------

    /**
     * Function testNetClasses
     * goes through each NETCLASS and verifies that its clearance, via size,
     * track width, and track clearance are larger than those in board.m_designSettings.
     * This is necessary because the actual DRC checks are run against the NETCLASS
     * limits, so in order enforce global limits, we first check the NETCLASSes against
     * the global limits.
     * @return bool - true if succes, else false but only after
     *  reporting _all_ NETCLASS violations.
     */
    bool testNetClasses();

    /**
     * Function testTracks
     * performs the DRC on all tracks.
     * because this test can take a while, a progress bar can be displayed
     * @param aShowProgressBar = true to show a progrsse bar
     * (Note: it is shown only if there are many tracks)
     */
    void testTracks( bool aShowProgressBar );

    void testPad2Pad();

    void testUnconnected();

    void testZones();

    void testKeepoutAreas();

    void testTexts();

    //-----<single "item" tests>-----------------------------------------

    bool doNetClass( boost::shared_ptr<NETCLASS> aNetClass, wxString& msg );

    /**
     * Function doPadToPadsDrc
     * tests the clearance between aRefPad and other pads.
     * The pad list must be sorted by x coordinate.
     * @param aRefPad The pad to test
     * @param aStart The start of the pad list to test against
     * @param aEnd Marks the end of the list and is not included
     * @param x_limit is used to stop the test (when the any pad's X coord exceeds this)
     */
    bool doPadToPadsDrc( D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd, int x_limit );

    /**
     * Function DoTrackDrc
     * tests the current segment.
     * @param aRefSeg The segment to test
     * @param aStart The head of a list of tracks to test against (usually BOARD::m_Track)
     * @param doPads true if should do pads test
     * @return bool - true if no poblems, else false and m_currentMarker is
     *          filled in with the problem information.
     */
    bool doTrackDrc( TRACK* aRefSeg, TRACK* aStart, bool doPads = true );

    /**
     * Function doTrackKeepoutDrc
     * tests the current segment or via.
     * @param aRefSeg The segment to test
     * @return bool - true if no poblems, else false and m_currentMarker is
     *          filled in with the problem information.
     */
    bool doTrackKeepoutDrc( TRACK* aRefSeg );


    /**
     * Function doEdgeZoneDrc
     * tests a segment in ZONE_CONTAINER * aArea:
     *      Test Edge inside other areas
     *      Test Edge too close other areas
     * @param aArea The current area.
     * @param aCornerIndex The first corner of the segment to test.
     * @return bool - false if DRC error  or true if OK
     */
    bool doEdgeZoneDrc( ZONE_CONTAINER* aArea, int aCornerIndex );

    //-----<single tests>----------------------------------------------

    /**
     * Function checkClearancePadToPad
     * @param aRefPad The reference pad to check
     * @param aPad Another pad to check against
     * @return bool - true if clearance between aRefPad and aPad is >= dist_min, else false
     */
    bool checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad );


    /**
     * Function checkClearanceSegmToPad
     * check the distance from a pad to segment.  This function uses several
     * instance variable not passed in:
     *      m_segmLength = length of the segment being tested
     *      m_segmAngle  = angle of the segment with the X axis;
     *      m_segmEnd    = end coordinate of the segment
     *      m_padToTestPos = position of pad relative to the origin of segment
     * @param aPad Is the pad involved in the check
     * @param aSegmentWidth width of the segment to test
     * @param aMinDist Is the minimum clearance needed
     *
     * @return true distance >= dist_min,
     *         false if distance < dist_min
     */
    bool checkClearanceSegmToPad( const D_PAD* aPad, int aSegmentWidth, int aMinDist );


    /**
     * Helper function checkMarginToCircle
     * Check the distance from a point to
     * a segment. the segment is expected starting at 0,0, and on the X axis
     * (used to test DRC between a segment and a round pad, via or round end of a track
     * @param aCentre The coordinate of the circle's center
     * @param aRadius A "keep out" radius centered over the circle
     * @param aLength The length of the segment (i.e. coordinate of end, becuase it is on
     *                the X axis)
     * @return bool - true if distance >= radius, else
     *                false when distance < aRadius
     */
    static bool checkMarginToCircle( wxPoint aCentre, int aRadius, int aLength );


    /**
     * Function checkLine
     * (helper function used in drc calculations to see if one track is in contact with
     *  another track).
     * Test if a line intersects a bounding box (a rectangle)
     * The rectangle is defined by m_xcliplo, m_ycliplo and m_xcliphi, m_ycliphi
     * return true if the line from aSegStart to aSegEnd is outside the bounding box
     */
    bool        checkLine( wxPoint aSegStart, wxPoint aSegEnd );

    //-----</single tests>---------------------------------------------

public:
    DRC( PCB_EDIT_FRAME* aPcbWindow );

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
     * tests the outline segment starting at CornerIndex and returns the result and displays
     * the error in the status panel only if one exists.
     *      Test Edge inside other areas
     *      Test Edge too close other areas
     * @param aArea The areaparent which contains the corner.
     * @param aCornerIndex The starting point of the segment to test.
     * @return int - BAD_DRC (1) if DRC error  or OK_DRC (0) if OK
     */
    int Drc( ZONE_CONTAINER* aArea, int aCornerIndex );

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

        if( ! doTrackDrc( aRefSeg, aList ) )
            return BAD_DRC;

        return doTrackKeepoutDrc( aRefSeg ) ? OK_DRC : BAD_DRC;
    }


    /**
     * Function ShowDialog
     * opens a dialog and prompts the user, then if a test run button is
     * clicked, runs the test(s) and creates the MARKERS.  The dialog is only
     * created if it is not already in existence.
     */
    void ShowDialog();

    /**
     * Function DestroyDialog
     * deletes this ui dialog box and zeros out its pointer to remember
     * the state of the dialog's existence.
     * @param aReason Indication of which button was clicked to cause the destruction.
     */
    void DestroyDialog( int aReason );


    /**
     * Function SetSettings
     * saves all the UI or test settings and may be called before running the tests.
     * @param aPad2PadTest Tells whether to test pad to pad distances.
     * @param aUnconnectedTest Tells whether to list unconnected pads.
     * @param aZonesTest Tells whether to test zones.
     * @param aKeepoutTest Tells whether to test keepout areas.
     * @param aReportName A string telling the disk file report name entered.
     * @param aSaveReport A boolean telling whether to generate disk file report.
     */
    void SetSettings( bool aPad2PadTest, bool aUnconnectedTest,
                      bool aZonesTest, bool aKeepoutTest,
                      const wxString& aReportName, bool aSaveReport )
    {
        m_doPad2PadTest     = aPad2PadTest;
        m_doUnconnectedTest = aUnconnectedTest;
        m_doZonesTest       = aZonesTest;
        m_doKeepoutTest     = aKeepoutTest;
        m_rptFilename       = aReportName;
        m_doCreateRptFile   = aSaveReport;
    }


    /**
     * Function RunTests
     * will actually run all the tests specified with a previous call to
     * SetSettings()
     * @param aMessages = a wxTextControl where to display some activity messages. Can be NULL
     */
    void RunTests( wxTextCtrl* aMessages = NULL );

    /**
     * Function ListUnconnectedPad
     * gathers a list of all the unconnected pads and shows them in the
     * dialog, and optionally prints a report of such.
     */
    void ListUnconnectedPads();

    /**
     * @return a pointer to the current marker (last created marker
     */
    MARKER_PCB* GetCurrentMarker( )
    {
        return m_currentMarker;
    }

};


#endif  // _DRC_STUFF_H

//EOF
