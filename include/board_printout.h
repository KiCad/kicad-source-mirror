/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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


#ifndef BOARD_PRINTOUT_H
#define BOARD_PRINTOUT_H

#include <wx/print.h>
#include <layer_ids.h>
#include <lset.h>
#include <printout.h>
#include <math/box2.h>

#include <memory>

namespace KIGFX
{
class GAL;
class VIEW;
class PAINTER;
};


struct BOARD_PRINTOUT_SETTINGS : public PRINTOUT_SETTINGS
{
    BOARD_PRINTOUT_SETTINGS( const PAGE_INFO& aPageInfo );

    virtual ~BOARD_PRINTOUT_SETTINGS()
    {
    }

    LSET m_LayerSet;                   ///< Layers to print
    bool m_Mirror;                     ///< Print mirrored

    void Load( APP_SETTINGS_BASE* aConfig ) override;
    void Save( APP_SETTINGS_BASE* aConfig ) override;
};


/**
 * An object derived from wxPrintout to handle the necessary information to control a printer
 * when printing a board.
 */
class BOARD_PRINTOUT : public wxPrintout
{
public:
    BOARD_PRINTOUT( const BOARD_PRINTOUT_SETTINGS& aParams, const KIGFX::VIEW* aView,
                    const wxString& aTitle );

    virtual ~BOARD_PRINTOUT() {}

    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo ) override;

    bool HasPage( int aPage ) override
    {
        return aPage <= m_settings.m_pageCount;
    }

    /**
     * Print a page (or a set of pages).
     *
     * @note This function prepares the print parameters for the function which actually prints
     *       the draw layers.
     *
     * @param aLayerName a text which can be printed as layer name.
     * @param aPageNum the number of the current page (only used to print this value).
     * @param aPageCount the number of pages to print (only used to print this value).
     */
    virtual void DrawPage( const wxString& aLayerName = wxEmptyString,
                           int aPageNum = 1, int aPageCount = 1 );

protected:
    /// Convert mils to internal units.
    virtual int milsToIU( double aMils ) const = 0;

    /// Enable layers visibility for a printout.
    virtual void setupViewLayers( KIGFX::VIEW& aView, const LSET& aLayerSet );

    /// Configure #PAINTER object for a printout.
    virtual void setupPainter( KIGFX::PAINTER& aPainter );

    /// Configure #GAL object for a printout.
    virtual void setupGal( KIGFX::GAL* aGal );

    /// Return bounding box of the printed objects (excluding drawing-sheet frame).
    virtual BOX2I getBoundingBox() = 0;

    /// Return the #PAINTER instance used to draw the items.
    virtual std::unique_ptr<KIGFX::PAINTER> getPainter( KIGFX::GAL* aGal ) = 0;

    /// Source VIEW object (note that actual printing only refers to this object).
    const KIGFX::VIEW* m_view;

    /// Printout parameters.
    BOARD_PRINTOUT_SETTINGS m_settings;

    /// True if the caller is Gerbview, false for Pcbnew.
    bool  m_gerbviewPrint;
};

#endif      // BOARD_PRINTOUT_H
