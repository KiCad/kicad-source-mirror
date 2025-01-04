/*
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAL_PRINT_H
#define GAL_PRINT_H

#include <gal/gal.h>

class wxDC;

namespace KIGFX {
class GAL;
class GAL_DISPLAY_OPTIONS;


class GAL_API PRINT_CONTEXT
{
public:
    virtual ~PRINT_CONTEXT() {}
    virtual double GetNativeDPI() const = 0;
    virtual bool HasNativeLandscapeRotation() const = 0;
};


/**
 * Wrapper around GAL to provide information needed for printing.
 */
class GAL_API GAL_PRINT
{
public:
    static std::unique_ptr<GAL_PRINT> Create( GAL_DISPLAY_OPTIONS& aOptions, wxDC* aDC );

    virtual ~GAL_PRINT() {}

    virtual GAL* GetGAL() = 0;

    virtual PRINT_CONTEXT* GetPrintCtx() const = 0;

    /**
     * @param aSize is the printing sheet size expressed in inches.
     * @param aRotateIfLandscape true if the platform requires 90 degrees
     * rotation in order to print in landscape format.
     */
    virtual void SetNativePaperSize( const VECTOR2D& aSize, bool aRotateIfLandscape ) = 0;

    /**
     * @param aSize is the schematics sheet size expressed in inches.
     */
    virtual void SetSheetSize( const VECTOR2D& aSize ) = 0;
};

}; // end namespace KIGFX

#endif /* GAL_PRINT_H */
