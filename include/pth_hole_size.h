/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>

#include <wx/string.h>


// Sizes used in this file are in nanometers (nm) unless otherwise specified.

enum class PTH_LEAD_SHAPE
{
    ROUND,
    SQUARE,
    RECTANGULAR
};

// How a recommended hole size is chosen from the allowable range: rounded to the nearest
// multiple of the rounding step, or rounded up to the next multiple.
enum class PTH_HOLE_ROUNDING
{
    NEAREST,
    UP
};

struct PTH_LEAD_DEF
{
    PTH_LEAD_SHAPE m_shape = PTH_LEAD_SHAPE::ROUND; // Lead cross-section shape
    int            m_minX = 0;                      // Smallest X dimension of the lead
    int            m_maxX = 0;                      // Largest X dimension of the lead
    int            m_minY = 0;                      // Smallest Y dimension of the lead (rectangular leads only)
    int            m_maxY = 0;                      // Largest Y dimension of the lead (rectangular leads only)
};

// The allowable hole size range a standard computes for a given lead.  The standard defines
// the range; choosing a specific (recommended) hole within it is a design decision and is done
// separately (see ComputeRecommendedPthHoleSize).
struct PTH_HOLE_SIZE_RESULT
{
    int m_leadMin = 0; // Effective smallest lead size
    int m_leadMax = 0; // Effective largest lead size
    int m_holeMin = 0; // Smallest allowable finished hole
    int m_holeMax = 0; // Largest allowable finished hole
};

/**
 * A source of hole size rules for a plated through hole.
 *
 * It's likely the exact interface will change as more standards are added,
 * but this is a start.
 */
class PTH_HOLE_SIZE_STANDARD
{
public:
    virtual ~PTH_HOLE_SIZE_STANDARD() = default;

    /**
     * @return a unique ID for the standard, e.g. "IPC-2222B".
     */
    virtual const wxString& GetID() const = 0;

    /**
     * @return the display name of the standard, used for menus and titles.
     */
    virtual const wxString& GetDisplayName() const = 0;

    /**
     * @return the number of density levels the standard defines (e.g. 3 for the IPC-2222B
     *         levels A, B and C).
     */
    virtual int GetLevelCount() const = 0;

    /**
     * @param aLevel is the level index, in the range [0, GetLevelCount()).
     * @return the display name of the density level.
     */
    virtual wxString GetLevelName( int aLevel ) const = 0;

    /**
     * @param aLevel is the level index, in the range [0, GetLevelCount()).
     * @return the allowance added to the largest lead size to give the smallest allowable hole
     *         for that level.
     */
    virtual int GetMinHoleAddend( int aLevel ) const = 0;

    /**
     * @param aLevel is the level index, in the range [0, GetLevelCount()).
     * @return the allowance added to the smallest lead size to give the largest allowable hole
     *         for that level.
     */
    virtual int GetMaxHoleAddend( int aLevel ) const = 0;

    /**
     * Compute the allowable hole size range for a lead.
     *
     * The effective lead size used for hole sizing is the diameter for a round lead and the
     * diagonal for a square or rectangular lead.
     *
     * @param aLevel is the density level index, in the range [0, GetLevelCount()).
     * @param aLead is the lead cross-section shape and dimensions.
     * @return the effective lead sizes and the allowable finished hole range.
     */
    virtual PTH_HOLE_SIZE_RESULT ComputeHoleSize( int aLevel, const PTH_LEAD_DEF& aLead ) const = 0;
};

/**
 * Choose a recommended hole within an allowable range.
 *
 * The midpoint of the range is rounded onto the rounding grid: to the nearest multiple of the
 * step, or always up to the next multiple.  The result is never below the range minimum.
 *
 * @param aRange is the allowable hole size range (m_holeMin and m_holeMax).
 * @param aRoundingStep is the rounding grid.  A value of 0 or less returns the exact midpoint
 *        of the range.
 * @param aRounding selects how the midpoint is rounded onto the grid.
 * @return the recommended hole size.
 */
int ComputeRecommendedPthHoleSize( const PTH_HOLE_SIZE_RESULT& aRange, int aRoundingStep, PTH_HOLE_ROUNDING aRounding );

/**
 * Find a hole size standard in the registry by its ID (e.g. "IPC-2222B").
 *
 * The caller does not own the returned pointer.
 *
 * @param aId is the standard ID.
 * @return the standard, or nullptr if the ID is not known.
 */
const PTH_HOLE_SIZE_STANDARD* FindPthHoleSizeStandard( const wxString& aId );

/**
 * @return the available hole size standards, in the order they should be offered in the UI.
 *         The pointers are non-owning and remain valid for the program lifetime.
 */
const std::vector<const PTH_HOLE_SIZE_STANDARD*>& GetPthHoleSizeStandards();
