/*
 * Copyright (C) 2018 CERN
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

class wxConfigBase;

/**
 * Class PRINT_PARAMETERS
 * handles the parameters used to print a board drawing.
 */
struct PRINTOUT_SETTINGS
{
    PRINTOUT_SETTINGS( const PAGE_INFO& aPageInfo )
        : m_pageInfo( aPageInfo )
    {
        // TODO Millimeter2iu is depends on PCBNEW/GERBVIEW #ifdefs, so it cannot be compiled to libcommon
        //m_lineWidth        = Millimeter2iu( 0.2 ); // A reasonable default value to draw items
                                        // which do not have a specified line width
        m_lineWidth  = 0.0;
        m_scale      = 1.0;
        m_titleBlock = false;
        m_blackWhite = true;
        m_pageCount  = 0;
    }

    virtual void Save( wxConfigBase* aConfig );
    virtual void Load( wxConfigBase* aConfig );

    int    m_lineWidth;     ///< The default value pen size to plot/print items
                            ///< that have no defined pen size
    double m_scale;         ///< Printing scale
    bool   m_titleBlock;    ///< Print frame and title block
    bool   m_blackWhite;    ///< Print in B&W or Color
    int    m_pageCount;     ///< Number of pages to print
    const PAGE_INFO& m_pageInfo;

    /**
     * Returns true if the drawing border and title block should be printed.
     *
     * For scale factors greater than one, the border is not printed because it will end up
     * scaling off of the page.
     */
    bool PrintBorderAndTitleBlock() const
    {
        return m_scale <= 1.0 && m_titleBlock;
    }
};

#endif /* PRINTOUT_H */
