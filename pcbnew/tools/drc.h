/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2016 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2017-2019 KiCad Developers, see change_log.txt for contributors.
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

#ifndef DRC_H
#define DRC_H

#include <class_board.h>
#include <class_track.h>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <memory>
#include <vector>
#include <tools/pcb_tool_base.h>
#include <drc/drc_marker_factory.h>

#define OK_DRC  0
#define BAD_DRC 1


// DRC error codes could be defined by an enum.
// however a #define is used because error code value is displayed in DRC messages,
// and using #define that shows each numerical value helps for debug.

/// DRC error codes:
#define DRCE_                                  1    // not used yet
#define DRCE_UNCONNECTED_ITEMS                 2    ///< items are unconnected
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
#define DRCE_ZONES_INTERSECT                   22   ///< copper area outlines intersect
#define DRCE_ZONES_TOO_CLOSE                   23   ///< copper area outlines are too close
#define DRCE_SUSPICIOUS_NET_FOR_ZONE_OUTLINE   24   ///< copper area has a net but no pads in nets, which is suspicious
#define DRCE_HOLE_NEAR_PAD                     25   ///< hole too close to pad
#define DRCE_HOLE_NEAR_TRACK                   26   ///< hole too close to track
#define DRCE_TOO_SMALL_TRACK_WIDTH             27   ///< Too small track width
#define DRCE_TOO_SMALL_VIA                     28   ///< Too small via size
#define DRCE_TOO_SMALL_MICROVIA                29   ///< Too small micro via size
#define DRCE_TOO_SMALL_VIA_DRILL               30   ///< Too small via drill
#define DRCE_TOO_SMALL_MICROVIA_DRILL          31   ///< Too small micro via drill
#define DRCE_NETCLASS_TRACKWIDTH               32   ///< netclass has TrackWidth < board.m_designSettings->m_TrackMinWidth
#define DRCE_NETCLASS_CLEARANCE                33   ///< netclass has Clearance < board.m_designSettings->m_TrackClearance
#define DRCE_NETCLASS_VIASIZE                  34   ///< netclass has ViaSize < board.m_designSettings->m_ViasMinSize
#define DRCE_NETCLASS_VIADRILLSIZE             35   ///< netclass has ViaDrillSize < board.m_designSettings->m_ViasMinDrill
#define DRCE_NETCLASS_uVIASIZE                 36   ///< netclass has ViaSize < board.m_designSettings->m_MicroViasMinSize
#define DRCE_NETCLASS_uVIADRILLSIZE            37   ///< netclass has ViaSize < board.m_designSettings->m_MicroViasMinDrill
#define DRCE_VIA_INSIDE_KEEPOUT                38   ///< Via in inside a keepout area
#define DRCE_TRACK_INSIDE_KEEPOUT              39   ///< Track in inside a keepout area
#define DRCE_PAD_INSIDE_KEEPOUT                40   ///< Pad in inside a keepout area
#define DRCE_TRACK_NEAR_COPPER                 41   ///< track & copper graphic collide or are too close
#define DRCE_VIA_NEAR_COPPER                   42   ///< via and copper graphic collide or are too close
#define DRCE_PAD_NEAR_COPPER                   43   ///< pad and copper graphic collide or are too close
#define DRCE_TRACK_NEAR_ZONE                   44   ///< track & zone collide or are too close together
#define DRCE_OVERLAPPING_FOOTPRINTS            45   ///< footprint courtyards overlap
#define DRCE_MISSING_COURTYARD_IN_FOOTPRINT    46   ///< footprint has no courtyard defined
#define DRCE_MALFORMED_COURTYARD_IN_FOOTPRINT  47   ///< footprint has a courtyard but malformed
                                                    ///< (not convertible to a closed polygon with holes)
#define DRCE_MICRO_VIA_NOT_ALLOWED             48   ///< micro vias are not allowed
#define DRCE_BURIED_VIA_NOT_ALLOWED            49   ///< buried vias are not allowed
#define DRCE_DISABLED_LAYER_ITEM               50   ///< item on a disabled layer
#define DRCE_DRILLED_HOLES_TOO_CLOSE           51   ///< overlapping drilled holes break drill bits
#define DRCE_TRACK_NEAR_EDGE                   53   ///< track too close to board edge
#define DRCE_INVALID_OUTLINE                   54   ///< invalid board outline
#define DRCE_MISSING_FOOTPRINT                 55   ///< footprint not found for netlist item
#define DRCE_DUPLICATE_FOOTPRINT               56   ///< more than one footprints found for netlist item
#define DRCE_EXTRA_FOOTPRINT                   57   ///< netlist item not found for footprint

#define DRCE_SHORT                             58
#define DRCE_REDUNDANT_VIA                     59
#define DRCE_DUPLICATE_TRACK                   60
#define DRCE_MERGE_TRACKS                      61
#define DRCE_DANGLING_TRACK                    62
#define DRCE_DANGLING_VIA                      63
#define DRCE_ZERO_LENGTH_TRACK                 64
#define DRCE_TRACK_IN_PAD                      65


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
class EDA_TEXT;
class DRAWSEGMENT;
class NETLIST;
class wxWindow;
class wxString;
class wxTextCtrl;


/**
 * Provide an abstract interface of a DRC_ITEM* list manager.  The details
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
 * Design Rule Checker object that performs all the DRC tests.  The output of
 * the checking goes to the BOARD file in the form of two MARKER lists.  Those
 * two lists are displayable in the drc dialog box.  And they can optionally
 * be sent to a text file on disk.
 * This class is given access to the windows and the BOARD
 * that it needs via its constructor or public access functions.
 */
class DRC : public PCB_TOOL_BASE
{
    friend class DIALOG_DRC_CONTROL;

public:
    DRC();
    ~DRC();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

private:

    //  protected or private functions() are lowercase first character.
    bool     m_doPad2PadTest;           // enable pad to pad clearance tests
    bool     m_doUnconnectedTest;       // enable unconnected tests
    bool     m_doZonesTest;             // enable zone to items clearance tests
    bool     m_doKeepoutTest;           // enable keepout areas to items clearance tests
    bool     m_doCreateRptFile;         // enable creating a report file
    bool     m_refillZones;             // refill zones if requested (by user).
    bool     m_reportAllTrackErrors;    // Report all tracks errors (or only 4 first errors)
    bool     m_testFootprints;          // Test footprints against schematic

    wxString m_rptFilename;

    MARKER_PCB* m_currentMarker;

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

    PCB_EDIT_FRAME*     m_pcbEditorFrame;   ///< The pcb frame editor which owns the board
    BOARD*              m_pcb;
    SHAPE_POLY_SET      m_board_outlines;   ///< The board outline including cutouts
    DIALOG_DRC_CONTROL* m_drcDialog;
    DRC_MARKER_FACTORY  m_markerFactory;    ///< Class that generates markers

    DRC_LIST            m_unconnected;      ///< list of unconnected pads, as DRC_ITEMs
    DRC_LIST            m_footprints;       ///< list of footprint warnings, as DRC_ITEMs
    bool                m_drcRun;
    bool                m_footprintsTested;


    ///> Sets up handlers for various events.
    void setTransitions() override;

    /**
     * Update needed pointers from the one pointer which is known not to change.
     */
    void updatePointers();

    /**
     * Adds a DRC marker to the PCB through the COMMIT mechanism.
     */
    void addMarkerToPcb( MARKER_PCB* aMarker );

    //-----<categorical group tests>-----------------------------------------

    /**
     * Go through each NETCLASS and verifies that its clearance, via size, track width, and
     * track clearance are larger than those in board.m_designSettings.
     * This is necessary because the actual DRC checks are run against the NETCLASS
     * limits, so in order enforce global limits, we first check the NETCLASSes against
     * the global limits.
     * @return bool - true if succes, else false but only after
     *  reporting _all_ NETCLASS violations.
     */
    bool testNetClasses();

    /**
     * Perform the DRC on all tracks.
     *
     * This test can take a while, a progress bar can be displayed
     * @param aActiveWindow = the active window ued as parent for the progress bar
     * @param aShowProgressBar = true to show a progress bar
     * (Note: it is shown only if there are many tracks)
     */
    void testTracks( wxWindow * aActiveWindow, bool aShowProgressBar );

    void testPad2Pad();

    void testDrilledHoles();

    void testUnconnected();

    void testZones();

    void testKeepoutAreas();

    // aTextItem is type BOARD_ITEM* to accept either TEXTE_PCB or TEXTE_MODULE
    void testCopperTextItem( BOARD_ITEM* aTextItem );

    void testCopperDrawItem( DRAWSEGMENT* aDrawing );

    void testCopperTextAndGraphics();

    ///> Tests for items placed on disabled layers (causing false connections).
    void testDisabledLayers();

    /**
     * Test that the board outline is contiguous and composed of valid elements
     */
    void testOutline();

    //-----<single "item" tests>-----------------------------------------

    bool doNetClass( const std::shared_ptr<NETCLASS>& aNetClass, wxString& msg );

    /**
     * Test the clearance between aRefPad and other pads.
     *
     * The pad list must be sorted by x coordinate.
     *
     * @param aRefPad is the pad to test
     * @param aStart is the first pad of the list to test against aRefPad
     * @param aEnd is the end of the list and is not included
     * @param x_limit is used to stop the test
     * (i.e. when the current pad pos X in list exceeds this limit, because the list
     * is sorted by X coordinate)
     */
    bool doPadToPadsDrc( D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd, int x_limit );

    /**
     * Test the current segment.
     *
     * @param aRefSeg The segment to test
     * @param aStartIt the iterator to the first track to test
     * @param aEndIt the marker for the iterator end
     * @param aTestZones true if should do copper zones test. This can be very time consumming
     * @return bool - true if no problems, else false and m_currentMarker is
     *          filled in with the problem information.
     */
    bool doTrackDrc( TRACK* aRefSeg, TRACKS::iterator aStartIt, TRACKS::iterator aEndIt,
                     bool aTestZones );

    /**
     * Test for footprint courtyard overlaps.
     */
    void doFootprintOverlappingDrc();

    //-----<single tests>----------------------------------------------

    /**
     * @param aRefPad The reference pad to check
     * @param aPad Another pad to check against
     * @return bool - true if clearance between aRefPad and aPad is >= dist_min, else false
     */
    bool checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad );


    /**
     * Check the distance from a pad to segment.  This function uses several
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
     * Check the distance from a point to a segment.
     *
     * The segment is expected starting at 0,0, and on the X axis
     * (used to test DRC between a segment and a round pad, via or round end of a track
     * @param aCentre The coordinate of the circle's center
     * @param aRadius A "keep out" radius centered over the circle
     * @param aLength The length of the segment (i.e. coordinate of end, because it is on
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
    bool checkLine( wxPoint aSegStart, wxPoint aSegEnd );

    //-----</single tests>---------------------------------------------

public:
    /**
     * Tests whether distance between zones complies with the DRC rules.
     *
     * @param aZone: zone to compare with other zones, or if NULL then
     *          all zones are compared to all others.
     * @param aCreateMarkers: if true create DRC markers.
     * False: do not create markers. only fing drc errors
     * @return Errors count
     */
    int TestZoneToZoneOutline( ZONE_CONTAINER* aZone, bool aCreateMarkers );

    /**
     * Test the board footprints against a netlist.  Will report DRCE_MISSING_FOOTPRINT,
     * DRCE_DUPLICATE_FOOTPRINT and DRCE_EXTRA_FOOTPRINT errors in aDRCList.
     */
    static void TestFootprints( NETLIST& aNetlist, BOARD* aPCB, EDA_UNITS_T aUnits,
                                DRC_LIST& aDRCList );

    /**
     * Open a dialog and prompts the user, then if a test run button is
     * clicked, runs the test(s) and creates the MARKERS.  The dialog is only
     * created if it is not already in existence.
     *
     * @param aParent is the parent window for wxWidgets. Usually the PCB editor frame
     * but can be another dialog
     * if aParent == NULL (default), the parent will be the PCB editor frame
     * and the dialog will be not modal (just float on parent
     * if aParent is specified, the dialog will be modal.
     * The modal mode is mandatory if the dialog is created from another dialog, not
     * from the PCB editor frame
     */
    void ShowDRCDialog( wxWindow* aParent );

    int ShowDRCDialog( const TOOL_EVENT& aEvent );

    /**
     * Deletes this ui dialog box and zeros out its pointer to remember
     * the state of the dialog's existence.
     *
     * @param aReason Indication of which button was clicked to cause the destruction.
     * if aReason == wxID_OK, design parameters values which can be entered from the dialog
     * will bbe saved in design parameters list
     */
    void DestroyDRCDialog( int aReason );

    /**
     * Run all the tests specified with a previous call to
     * SetSettings()
     * @param aMessages = a wxTextControl where to display some activity messages. Can be NULL
     */
    void RunTests( wxTextCtrl* aMessages = NULL );
};


#endif  // DRC_H
