/*
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef _CAIRO_PRINT_H_
#define _CAIRO_PRINT_H_

#include <gal/cairo/cairo_gal.h>
#include <gal/gal_print.h>

class wxDC;
class wxGCDC;

namespace KIGFX
{
/**
 * Provide a Cairo context created from wxPrintDC.
 *
 * It allows one to prepare printouts using the Cairo library and let wxWidgets handle the rest.
 */
class CAIRO_PRINT_CTX : public PRINT_CONTEXT
{
public:
    CAIRO_PRINT_CTX( wxDC* aDC );
    ~CAIRO_PRINT_CTX();

    cairo_t* GetContext() const
    {
        return m_ctx;
    }

    cairo_surface_t* GetSurface() const
    {
        return m_surface;
    }

    double GetNativeDPI() const override
    {
        return m_dpi;
    }

    bool HasNativeLandscapeRotation() const override
    {
#if defined(__WXGTK__) && !defined(__WXGTK3__)
        return false;
#else
        return true;
#endif
    }

private:
    wxGCDC* m_gcdc;
    cairo_t* m_ctx;
    cairo_surface_t* m_surface;

#ifdef __WXMSW__
    ///< DC handle on Windows
    void* m_hdc;        // the real type is HDC, but do not pull in extra headers
#endif /* __WXMSW__ */

    double m_dpi;
};


class CAIRO_PRINT_GAL : public CAIRO_GAL_BASE, public GAL_PRINT
{
public:
    CAIRO_PRINT_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions,
                     std::unique_ptr<CAIRO_PRINT_CTX> aContext );

    void ComputeWorldScreenMatrix() override;

    GAL* GetGAL() override
    {
        return this;
    }

    PRINT_CONTEXT* GetPrintCtx() const override
    {
        return m_printCtx.get();
    }

    /**
     * @param aSize is the printing sheet size expressed in inches.
     * @param aRotateIfLandscape true if the platform requires 90 degrees rotation in order
     *                           to print in landscape format.
     */
    void SetNativePaperSize( const VECTOR2D& aSize, bool aRotateIfLandscape ) override;

    /**
     * @param aSize is the schematics sheet size expressed in inches.
     */
    void SetSheetSize( const VECTOR2D& aSize ) override;

private:
    ///< Returns true if page orientation is landscape
    bool isLandscape() const
    {
        return m_nativePaperSize.x > m_nativePaperSize.y;
    }

    ///< Printout size
    VECTOR2D m_nativePaperSize;

    ///< Flag indicating whether the platform rotates page automatically or
    ///< GAL needs to handle it in the transformation matrix
    bool m_hasNativeLandscapeRotation;

    std::unique_ptr<CAIRO_PRINT_CTX> m_printCtx;
};
} // namespace KIGFX

#endif /* _CAIRO_PRINT_H_ */
