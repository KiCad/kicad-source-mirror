/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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


/**
 * @file board_printout.h
 * @brief Board print handler definition file.
 */

#ifndef BOARD_PRINTOUT_H
#define BOARD_PRINTOUT_H

#include <wx/print.h>
#include <layers_id_colors_and_visibility.h>
#include <eda_rect.h>

namespace KIGFX {
class GAL;
class VIEW;
class PAINTER;
};

/**
 * Class PRINT_PARAMETERS
 * handles the parameters used to print a board drawing.
 */

class PRINT_PARAMETERS
{
public:
    PRINT_PARAMETERS();

    int    m_PenDefaultSize;                 // The default value pen size to plot/print items
                                             // that have no defined pen size
    double m_PrintScale;                     // general scale when printing
    double m_XScaleAdjust;                   // fine scale adjust for X axis
    double m_YScaleAdjust;                   // fine scale adjust for Y axis
    bool   m_Print_Sheet_Ref;                // Option: print page references
    LSET   m_PrintMaskLayer;                 // Layers to print
    bool   m_PrintMirror;                    // Option: Print mirrored
    bool   m_Print_Black_and_White;          // Option: Print in B&W or Color
    int    m_OptionPrintPage;                // Option: 0 = a layer per page, 1 = all layers at once
    int    m_PageCount;                      // Number of pages to print
    bool   m_ForceCentered;                  // Force plot origin to page centre (used in modedit)
    int    m_Flags;                          // Can be used to pass some other info
    wxPageSetupDialogData* m_PageSetupData;  // A wxPageSetupDialogData for page options (margins)

    enum DrillShapeOptT {
        NO_DRILL_SHAPE    = 0,
        SMALL_DRILL_SHAPE = 1,
        FULL_DRILL_SHAPE  = 2
    };

    DrillShapeOptT m_DrillShapeOpt;          // Options to print pads and via holes

    /**
     * Returns true if the drawing border and title block should be printed.
     *
     * For scale factors greater than one, the border is not printed because it will end up
     * scaling off of the page.
     */
    bool PrintBorderAndTitleBlock() const { return m_PrintScale <= 1.0 && m_Print_Sheet_Ref; }

    /**
     * Returns true if the print should be centered by the board outline instead of the
     * paper size.
     */
    bool CenterOnBoardOutline() const
    {
        return !PrintBorderAndTitleBlock()
               && ( m_ForceCentered || ( m_PrintScale > 1.0 ) || ( m_PrintScale == 0 ) );
    }
};


/**
 * Class BOARD_PRINTOUT
 * is a class derived from wxPrintout to handle the necessary information to control a printer
 * when printing a board
 */
class BOARD_PRINTOUT : public wxPrintout
{
public:
    BOARD_PRINTOUT( const PRINT_PARAMETERS& aParams, const KIGFX::VIEW* aView,
            const wxSize& aSheetSize, const wxString& aTitle );

    virtual ~BOARD_PRINTOUT() {}

    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo ) override;

    bool HasPage( int aPage ) override;

    /**
     * Print a page (or a set of pages).
     * Note: this function prepare print parameters for the function
     * which actually print the draw layers.
     * @param aLayerName = a text which can be printed as layer name
     * @param aPageNum = the number of the current page (only used to print this value)
     * @param aPageCount = the number of pages to ptint (only used to print this value)
     */
    virtual void DrawPage( const wxString& aLayerName = wxEmptyString,
            int aPageNum = 1, int aPageCount = 1 );

protected:
    ///> Enables layers visibility for a printout
    virtual void setupViewLayers( const std::unique_ptr<KIGFX::VIEW>& aView, const LSET& aLayerSet );

    ///> Returns bounding box of the printed objects (excluding worksheet frame)
    virtual EDA_RECT getBoundingBox() = 0;

    ///> Returns a PAINTER instance used to draw the items.
    virtual std::unique_ptr<KIGFX::PAINTER> getPainter( KIGFX::GAL* aGal ) = 0;

    ///> Source VIEW object (note that actual printing only refers to this object)
    const KIGFX::VIEW* m_view;

    ///> Printout parameters
    PRINT_PARAMETERS m_PrintParams;

    ///> Sheet size expressed in internal units
    wxSize m_sheetSize;
};

#endif      // BOARD_PRINTOUT_H
