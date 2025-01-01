/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef TEARDROP_PARAMS_H
#define TEARDROP_PARAMS_H

#include <string>
#include <vector>
#include <base_units.h>

// IDs for targets when creating teardrops
enum TARGET_TD
{
    TARGET_UNKNOWN = -1,
    TARGET_ROUND = 0,
    TARGET_RECT =  1,
    TARGET_TRACK = 2,
    TARGET_COUNT = 3
};

/**
 * TEARDROP_PARAMETARS is a helper class to handle parameters needed to build teardrops
 * for a board
 * these parameters are sizes and filters
 */
class TEARDROP_PARAMETERS
{
public:
    TEARDROP_PARAMETERS():
            m_TdMaxLen( pcbIUScale.mmToIU( 1.0 ) ),
            m_TdMaxWidth( pcbIUScale.mmToIU( 2.0 ) ),
            m_BestLengthRatio( 0.5),
            m_BestWidthRatio( 1.0 ),
            m_WidthtoSizeFilterRatio( 0.9 ),
            m_CurvedEdges( false ),
            m_Enabled( false ),
            m_AllowUseTwoTracks( true ),
            m_TdOnPadsInZones( false )
    {
    }

    /**
     * Set max allowed length and height for teardrops in IU.
     * a value <= 0 disable the constraint
     */
    void SetTeardropMaxSize( int aMaxLen, int aMaxHeight )
    {
        m_TdMaxLen = aMaxLen;
        m_TdMaxWidth = aMaxHeight;
    }

    /**
     * Set prefered length and height ratio for teardrops
     * the prefered length and height are VIAPAD width * aLenghtRatio and
     * VIAPAD width * aHeightRatio
     */
    void SetTeardropSizeRatio( double aLenghtRatio = 0.5, double aHeightRatio = 1.0 )
    {
        m_BestLengthRatio = aLenghtRatio;
        m_BestWidthRatio = aHeightRatio;
    }

    bool operator== ( const TEARDROP_PARAMETERS& aOther ) const
    {
        return m_Enabled == aOther.m_Enabled &&
               m_AllowUseTwoTracks == aOther.m_AllowUseTwoTracks &&
               m_TdMaxLen == aOther.m_TdMaxLen &&
               m_TdMaxWidth == aOther.m_TdMaxWidth &&
               m_BestLengthRatio == aOther.m_BestLengthRatio &&
               m_BestWidthRatio == aOther.m_BestWidthRatio &&
               m_CurvedEdges == aOther.m_CurvedEdges &&
               m_WidthtoSizeFilterRatio == aOther.m_WidthtoSizeFilterRatio &&
               m_TdOnPadsInZones == aOther.m_TdOnPadsInZones;
    }

    bool operator!= ( const TEARDROP_PARAMETERS& aOther ) const
    {
        return !( *this == aOther );
    }

public:
    /// max allowed length for teardrops in IU. <= 0 to disable
    int     m_TdMaxLen;
    /// max allowed height for teardrops in IU. <= 0 to disable
    int     m_TdMaxWidth;
    /// The length of a teardrop as ratio between length and size of pad/via
    double  m_BestLengthRatio;
    /// The height of a teardrop as ratio between height and size of pad/via
    double  m_BestWidthRatio;
    /// The ratio (H/D) between the via/pad size and the track width max value to create a teardrop
    /// 1.0 (100 %) always creates a teardrop, 0.0 (0%) never create a teardrop
    double  m_WidthtoSizeFilterRatio;
    /// True if the teardrop should be curved
    bool    m_CurvedEdges;

    /// Flag to enable teardrops
    bool m_Enabled;
    /// True to create teardrops using 2 track segments if the first in too small
    bool m_AllowUseTwoTracks;
    /// A filter to exclude pads inside zone fills
    bool m_TdOnPadsInZones;
};



/**
 * TEARDROP_PARAMETERS_LIST is a helper class to handle the list of TEARDROP_PARAMETERS
 * needed  to build teardrops of different shapes (round, rect, tracks)
 */
class TEARDROP_PARAMETERS_LIST
{
    std::vector<TEARDROP_PARAMETERS> m_params_list;

public:
    /// True to create teardrops for vias
    bool     m_TargetVias;
    /// True to create teardrops for pads with holes
    bool     m_TargetPTHPads;
    /// True to create teardrops for pads SMD, edge connectors,
    bool     m_TargetSMDPads;
    /// True to create teardrops at the end of a track connected to the end of
    /// another track having a different width
    bool     m_TargetTrack2Track;
    /// True to create teardrops for round shapes only
    bool     m_UseRoundShapesOnly;

public:
    TEARDROP_PARAMETERS_LIST() :
            m_TargetVias( true ),
            m_TargetPTHPads( true ),
            m_TargetSMDPads( true ),
            m_TargetTrack2Track( false ),
            m_UseRoundShapesOnly( false )
    {
        m_params_list.emplace_back();     // parameters for TARGET_ROUND
        m_params_list.emplace_back();     // parameters for TARGET_RECT
        m_params_list.emplace_back();     // parameters for TARGET_TRACK
    }

    /**
     * @return the TEARDROP_PARAMETERS for aTdType target item
     */
    TEARDROP_PARAMETERS* GetParameters( TARGET_TD aTdType )
    {
        return &m_params_list.at( aTdType );
    }

    /**
     * @return the number of TEARDROP_PARAMETERS item. Should be 3
     */
    size_t GetParametersCount()
    {
        return m_params_list.size();
    }
};


/**
 * @return the canonical name of a target type of a TEARDROP_PARAMETERS
 * @param aTdType is the target type
 */
std::string GetTeardropTargetCanonicalName( TARGET_TD aTdType );

/**
 * @return the target type from a canonical name of a TEARDROP_PARAMETERS
 * @param aTargetName is the canonical name
 */
TARGET_TD GetTeardropTargetTypeFromCanonicalName( const std::string& aTargetName );

#endif  // ifndef TEARDROP_PARAMS_H
