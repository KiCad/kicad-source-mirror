/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file printout_controler.h
 * @brief Board print handler definition file.
 */

#ifndef PRINTOUT_CONTROLLER_H
#define PRINTOUT_CONTROLLER_H


#include <wx/dcps.h>
#include <layers_id_colors_and_visibility.h>
#include <wx/print.h>

#define DEFAULT_ORIENTATION_PAPER wxLANDSCAPE   // other option is wxPORTRAIT


/**
 * Class PRINT_PARAMETERS
 * handles the parameters used to print a board drawing.
 */

class PRINT_PARAMETERS
{
public:
    int    m_PenDefaultSize;                 // The default value pen size to plot/print items
                                             // that have no defined pen size
    double m_PrintScale;                     // general scale when printing
    double m_XScaleAdjust;                   // fine scale adjust for X axis
    double m_YScaleAdjust;                   // fine scale adjust for Y axis
    bool   m_Print_Sheet_Ref;                // Option: print page references
    LSET m_PrintMaskLayer;                 // Layers to print
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

public:
    PRINT_PARAMETERS();

    /**
     * Function PrintBorderAndTitleBlock
     * returns true if the drawing border and title block should be printed.
     *
     * For scale factors greater than one, the border is not printed because it will end up
     * scaling off of the page.
     */
    bool PrintBorderAndTitleBlock() const { return m_PrintScale <= 1.0 && m_Print_Sheet_Ref; }

    /**
     * Function CenterOnBoardOutline
     * returns true if the print should be centered by the board outline instead of the
     * paper size.
     */
    bool CenterOnBoardOutline() const
    {
        return !PrintBorderAndTitleBlock() && ( m_ForceCentered || (m_PrintScale > 1.0) ||
                                                (m_PrintScale == 0) );
    }
};


/**
 * Class BOARD_PRINTOUT_CONTROLLER
 * is a class derived from wxPrintout to handle the necessary information to control a printer
 * when printing a board
 */
class BOARD_PRINTOUT_CONTROLLER : public wxPrintout
{
private:
    EDA_DRAW_FRAME*     m_Parent;
    PRINT_PARAMETERS    m_PrintParams;

public:
    BOARD_PRINTOUT_CONTROLLER( const PRINT_PARAMETERS& aParams,
                               EDA_DRAW_FRAME*         aParent,
                               const wxString&         aTitle );

    bool OnPrintPage( int aPage );

    bool HasPage( int aPage )       // do not test page num
    {
        if( aPage <= m_PrintParams.m_PageCount )
            return true;
        else
            return false;
    }

    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo );

    void DrawPage();
};

#endif      // PRINTOUT_CONTROLLER_H
