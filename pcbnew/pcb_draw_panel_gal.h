/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef PCB_DRAW_PANEL_GAL_H_
#define PCB_DRAW_PANEL_GAL_H_

#include <class_draw_panel_gal.h>

namespace KIGFX
{
    class WORKSHEET_VIEWITEM;
    class RATSNEST_VIEWITEM;
}

class PCB_DRAW_PANEL_GAL : public EDA_DRAW_PANEL_GAL
{
public:
    PCB_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId, const wxPoint& aPosition,
                        const wxSize& aSize, GalType aGalType = GAL_TYPE_OPENGL );

    virtual ~PCB_DRAW_PANEL_GAL();

    /**
     * Function DisplayBoard
     * adds all items from the current board to the VIEW, so they can be displayed by GAL.
     * @param aBoard is the PCB to be loaded.
     */
    void DisplayBoard( const BOARD* aBoard );

    /**
     * Function SetWorksheet
     * Sets (or updates) worksheet used by the draw panel.
     * @param aWorksheet is the worksheet to be used.
     *        The object is then owned by PCB_DRAW_PANEL_GAL.
     */
    void SetWorksheet( KIGFX::WORKSHEET_VIEWITEM* aWorksheet );

    ///> @copydoc EDA_DRAW_PANEL_GAL::SetHighContrastLayer()
    virtual void SetHighContrastLayer( LAYER_ID aLayer );

    ///> @copydoc EDA_DRAW_PANEL_GAL::SetTopLayer()
    virtual void SetTopLayer( LAYER_ID aLayer );

protected:
    ///> Currently used worksheet
    KIGFX::WORKSHEET_VIEWITEM* m_worksheet;

    ///> Ratsnest view item
    KIGFX::RATSNEST_VIEWITEM* m_ratsnest;
};

#endif /* PCB_DRAW_PANEL_GAL_H_ */
