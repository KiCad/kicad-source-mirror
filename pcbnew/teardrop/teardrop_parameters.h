/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

// IDs for targets when creating teardrops
enum TARGET_TD
{
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
    friend class TEARDROP_MANAGER;
    friend class TEARDROP_PARAMETERS_LIST;

public:
    TEARDROP_PARAMETERS():
        m_tdMaxLen( Millimeter2iu( 1.0 ) ),
        m_tdMaxHeight( Millimeter2iu( 2.0 ) ),
        m_lengthRatio( 0.5),
        m_heightRatio( 1.0 ),
        m_curveSegCount( 0 )
    {
    }

    /**
     * Set max allowed length and height for teardrops in IU.
     * a value <= 0 disable the constraint
     */
    void SetTeardropMaxSize( int aMaxLen, int aMaxHeight )
    {
        m_tdMaxLen = aMaxLen;
        m_tdMaxHeight = aMaxHeight;
    }

    /**
     * Set prefered length and height ratio for teardrops
     * the prefered length and height are VIAPAD width * aLenghtRatio and
     * VIAPAD width * aHeightRatio
     */
    void SetTeardropSizeRatio( double aLenghtRatio = 0.5, double aHeightRatio = 1.0 )
    {
        m_lengthRatio = aLenghtRatio;
        m_heightRatio = aHeightRatio;
    }

    /**
     * Set the params for teardrop using curved shape
     * note: if aCurveSegCount is < 3, the shape uses a straight line
     */
    void SetTeardropCurvedPrm( int aCurveSegCount = 0 )
    {
        m_curveSegCount = aCurveSegCount;
    }

    bool IsCurved() const { return m_curveSegCount > 2; }

protected:
    /// max allowed length for teardrops in IU. <= 0 to disable
    int     m_tdMaxLen;
    /// max allowed height for teardrops in IU. <= 0 to disable
    int     m_tdMaxHeight;
    /// The length of a teardrop as ratio between length and size of pad/via
    double  m_lengthRatio;
    /// The height of a teardrop as ratio between height and size of pad/via
    double  m_heightRatio;
    /// number of segments to build the curved sides of a teardrop area
    /// must be > 2. for values <= 2 a straight line is used
    int     m_curveSegCount;
};



/**
 * TEARDROP_PARAMETERS_LIST is a helper class to handle the list of TEARDROP_PARAMETERS
 * needed  to build teardrops of different shapes (round, rect, tracks)
 */
class TEARDROP_PARAMETERS_LIST
{
    std::vector<TEARDROP_PARAMETERS> m_params_list;

public:
    TEARDROP_PARAMETERS_LIST()
    {
        m_params_list.emplace_back( );       // parameters for TARGET_ROUND
        m_params_list.emplace_back( );       // parameters for TARGET_RECT
        m_params_list.emplace_back( );       // parameters for TARGET_TRACK
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
    int GetParametersCount()
    {
        return m_params_list.size();
    }
};


#endif  // ifndef TEARDROP_PARAMS_H
