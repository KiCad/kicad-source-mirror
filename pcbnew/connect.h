/**
 * @file connect.h
 * @brief helper classes to find track to track and track to pad connections.
 */
#ifndef CONNECT_H
#define CONNECT_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <class_track.h>
#include <class_board.h>


// Helper classes to handle connection points (i.e. candidates) for tracks

/* class CONNECTED_POINT describes a coordinate having a track or pad parent.
 * when a pad is the parent, the coordinate is (obviously) the connection point
 * when a track is the parent, the coordinate is the staring point
 * or the ending point.
 * therefore when building a list of CONNECTED_POINT, a pad or via generates one item,
 * and a track generates 2 items.
 */
class CONNECTED_POINT
{
private:
    BOARD_CONNECTED_ITEM * m_item;  // a link to the parent item (track, via or pad)
    wxPoint m_point;                // coordinates of this connected point

public:
    // ctor to build a CONNECTED_POINT instance, when the parent is a track or via
    CONNECTED_POINT( TRACK * aTrack, const wxPoint & aPoint)
    {
        m_item = aTrack;
        m_point = aPoint;
    }

    // ctor to build a CONNECTED_POINT instance, when the parent is a pad
    CONNECTED_POINT( D_PAD * aPad, const wxPoint & aPoint)
    {
        m_item = aPad;
        m_point = aPoint;
    }

    /**
     * Function GetTrack
     * @return the parent track or via of this connected point,
     * or null if the parent is a pad
     */
    TRACK * GetTrack() const
    {
        return m_item->Type() != PCB_PAD_T ? (TRACK*) m_item : NULL ;
    }

    /**
     * Function GetPad
     * @return the parent pad of this connected point,
     * or null if the parent is a track or via
     */
    D_PAD * GetPad() const
    {
        return m_item->Type() == PCB_PAD_T ? (D_PAD*) m_item : NULL;
    }

    const wxPoint & GetPoint() const { return m_point; }
};

// A helper class to handle connections calculations:
class CONNECTIONS
{
private:
    std::vector <TRACK*> m_connected;           // List of connected tracks/vias
                                                // to a given track or via
    std::vector <CONNECTED_POINT> m_candidates; // List of points to test
                                                // (end points of tracks or vias location )
    BOARD * m_brd;                              // the master board.
    const TRACK * m_firstTrack;                 // The first track used to build m_Candidates
    const TRACK * m_lastTrack;                  // The last track used to build m_Candidates
    std::vector<D_PAD*> m_sortedPads;           // list of sorted pads by X (then Y) coordinate

public:
    CONNECTIONS( BOARD * aBrd );
    ~CONNECTIONS() {};

    /**
     * Function BuildPadsList
     * Fills m_sortedPads with all pads that be connected to tracks
     * pads are sorted by X then Y coordinates to allow fast binary search in list
     * @param aNetcode = net code to use to filter pads
     * if  aNetcode < 0, all pads will be put in list (default)
     */
    void BuildPadsList( int aNetcode = -1 );

    /**
     * Function GetPadsList
     * @return the pads list used in connections calculations
     */
    std::vector<D_PAD*>& GetPadsList() { return m_sortedPads; }

    /**
     * Function Build_CurrNet_SubNets_Connections
     * should be called after a track change (delete or add a track):
     * Connections to pads and to tracks are recalculated
     *   If a track is deleted, the other pointers to pads do not change.
     *   When a new track is added in track list, its pointers to pads are already initialized
     * Builds the subnets inside a net (tracks from aFirstTrack to aFirstTrack).
     * subnets are clusters of pads and tracks that are connected together.
     * When all tracks are created relative to the net, there is only a cluster
     * when not tracks there are a cluster per pad
     * @param aFirstTrack = first track of the given net
     * @param aLastTrack = last track of the given net
     * @param aNetcode = the netcode of the given net
     */
    void Build_CurrNet_SubNets_Connections( TRACK* aFirstTrack, TRACK* aLastTrack, int aNetcode );

    /**
     * Function BuildTracksCandidatesList
     * Fills m_Candidates with all connecting points (track ends or via location)
     * with tracks from aBegin to aEnd.
     * @param aBegin = first track to store in list (should not be NULL)
     * @param aEnd = last track to store in list
     * if aEnd == NULL, uses all tracks from aBegin
     */
    void BuildTracksCandidatesList( TRACK * aBegin, TRACK * aEnd = NULL);

    /**
     * Function BuildPadsCandidatesList
     * Populates m_candidates with all pads connecting points (pads position)
     * m_sortedPads is expected to be populated by the pad candidates list
     */
    void BuildPadsCandidatesList();

    /**
     * function SearchConnectedTracks
     * Populates .m_connected with tracks/vias connected to aTrack
     * m_candidates is expected to be populated by the track candidates ends list
     * @param aTrack = track or via to use as reference
     */
    int SearchConnectedTracks( const TRACK * aTrack );

    /**
     * Function GetConnectedTracks
     * Copy m_Connected that contains the list of tracks connected
     * calculated by SearchConnectedTracks
     * in aTrack->m_TracksConnected
     * @param aTrack = track or via to fill with connected tracks
     */
    void GetConnectedTracks(TRACK * aTrack)
    {
        aTrack->m_TracksConnected = m_connected;
    }

    /**
     * function SearchConnectionsPadsToIntersectingPads
     * Explores the list of pads and adds to m_PadsConnected member
     * of each pad pads connected to
     * Here, connections are due to intersecting pads, not tracks
     * m_sortedPads must be initialized
     */
    void SearchConnectionsPadsToIntersectingPads();

    /**
     * function SearchTracksConnectedToPads
     * Explores the list of pads.
     * if( add_to_padlist )
     *  adds to m_PadsConnected member of each track the pad(s) connected to
     * if add_to_tracklist
     *  adds to m_TracksConnected member of each pad the track(s) connected to
     * D_PAD::m_TracksConnected is cleared before adding items
     * TRACK::m_PadsConnected is not cleared
     * @param add_to_padlist = true to fill m_PadsConnected member of each track
     * @param add_to_tracklist = true to fill m_TracksConnected member of each pad
     */
    void SearchTracksConnectedToPads( bool add_to_padlist = true, bool add_to_tracklist = true);

    /**
     * function CollectItemsNearTo
     * Used by SearchTracksConnectedToPads
     * Fills aList with pads near to aPosition
     * near means aPosition to pad position <= aDistMax
     * @param aList = list to fill
     * @param aPosition = aPosition to use as reference
     * @param aDistMax = dist max from aPosition to a candidate to select it
     */
    void CollectItemsNearTo( std::vector<CONNECTED_POINT*>& aList,
                            const wxPoint& aPosition, int aDistMax );

    /**
     * Function Propagate_SubNets
     * Test a list of tracks, to create or propagate a sub netcode to pads and
     * segments connected together.
     * The track list must be sorted by nets, and all segments
     * from m_firstTrack to m_lastTrack have the same net.
     * When 2 items are connected (a track to a pad, or a track to an other track),
     * they are grouped in a cluster.
     * For pads, this is the .m_physical_connexion member which is a cluster identifier
     * For tracks, this is the .m_Subnet member which is a cluster identifier
     * For a given net, if all tracks are created, there is only one cluster.
     * but if not all tracks are created, there are more than one cluster,
     * and some ratsnests will be left active.
     */
    void Propagate_SubNets();

private:
    /**
     * function searchEntryPointInCandidatesList
     * Search an item in m_Connected connected to aPoint
     * note m_Connected containts usually more than one candidate
     * and searchEntryPointInCandidatesList returns an index to one of these candidates
     * Others are neightbor of the indexed item.
     * @param aPoint is the reference coordinates
     * @return the index of item found or -1 if no candidate
     */
    int searchEntryPointInCandidatesList( const wxPoint & aPoint);

    /**
     * Function Merge_SubNets
     * Change a subnet old value to a new value, for tracks and pads which are connected to
     * tracks from m_firstTrack to m_lastTrack and their connected pads.
     * and modify the subnet parameter (change the old value to the new value).
     * After that, 2 cluster (or subnets) are merged into only one.
     * Note: the resulting sub net value is the smallest between aOldSubNet and aNewSubNet
     * @return modification count
     * @param aOldSubNet = subnet value to modify
     * @param aNewSubNet = new subnet value for each item which have old_val as subnet value
     */
    int Merge_SubNets( int aOldSubNet, int aNewSubNet );

    /**
     * Function Merge_PadsSubNets
     * Change a subnet value to a new value, in m_sortedPads pad list
     * After that, 2 cluster (or subnets) are merged into only one.
     * Note: the resulting subnet value is the smallest between aOldSubNet et aNewSubNet
     * @return modification count
     * @param aOldSubNet = subnet value to modify
     * @param aNewSubNet = new subnet value for each item which have old_val as subnet value
     */
    int Merge_PadsSubNets( int aOldSubNet, int aNewSubNet );
};

#endif      //  ifndef CONNECT_H
