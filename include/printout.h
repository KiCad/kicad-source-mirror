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

#ifndef PRINTOUT_H
#define PRINTOUT_H

#include <page_info.h>

class APP_SETTINGS_BASE;
class COLOR_SETTINGS;

/**
 * Handle the parameters used to print a board drawing.
 */
struct PRINTOUT_SETTINGS
{
    PRINTOUT_SETTINGS( const PAGE_INFO& aPageInfo )
        : m_pageInfo( aPageInfo )
    {
        m_scale         = 1.0;
        m_titleBlock    = false;
        m_blackWhite    = true;
        m_pageCount     = 0;
        m_background    = false;
        m_colorSettings = nullptr;
    }

    virtual ~PRINTOUT_SETTINGS()
    {
    }

    virtual void Save( APP_SETTINGS_BASE* aConfig );

    virtual void Load( APP_SETTINGS_BASE* aConfig );

    /**
     * Returns true if the drawing border and title block should be printed.
     */
    bool PrintBorderAndTitleBlock() const { return m_titleBlock; }


    double m_scale;         ///< Printing scale
    bool   m_titleBlock;    ///< Print frame and title block
    bool   m_blackWhite;    ///< Print in B&W or Color
    int    m_pageCount;     ///< Number of pages to print
    bool   m_background;    ///< Print background color
    const PAGE_INFO& m_pageInfo;

    /// The color settings to be used for printing
    COLOR_SETTINGS* m_colorSettings;
};

#endif /* PRINTOUT_H */
