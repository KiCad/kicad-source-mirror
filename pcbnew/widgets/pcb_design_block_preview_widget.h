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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PCB_DESIGN_BLOCK_PREVIEW_WIDGET_H
#define PCB_DESIGN_BLOCK_PREVIEW_WIDGET_H

#include <wx/panel.h>
#include <widgets/design_block_preview_widget.h>
#include <kiway.h>
#include <gal_display_options_common.h>
#include <class_draw_panel_gal.h>


class LIB_ID;
class DESIGN_BLOCK;
class SCHEMATIC;
class PCB_SHEET;
class wxStaticText;
class wxSizer;


class PCB_DESIGN_BLOCK_PREVIEW_WIDGET : public DESIGN_BLOCK_PREVIEW_WIDGET
{
public:
    /**
     * Construct a schematic design block preview widget.
     *
     * @param aParent - parent window
     */
    PCB_DESIGN_BLOCK_PREVIEW_WIDGET( wxWindow* aParent, PCB_EDIT_FRAME* aFrame );

    ~PCB_DESIGN_BLOCK_PREVIEW_WIDGET() override;

    /**
     * Set the contents of the status label and display it.
     */
    void SetStatusText( const wxString& aText ) override;

    /**
     * Set the currently displayed symbol.
     */
    void DisplayDesignBlock( DESIGN_BLOCK* aDesignBlock ) override;

protected:
    void onSize( wxSizeEvent& aEvent );

    void fitOnDrawArea();    // set the view scale to fit the item on screen and center

    GAL_DISPLAY_OPTIONS_IMPL   m_galDisplayOptions;
    EDA_DRAW_PANEL_GAL*        m_preview;

    wxStaticText*              m_status;
    wxPanel*                   m_statusPanel;
    wxSizer*                   m_statusSizer;
    wxSizer*                   m_outerSizer;

    BOARD*                     m_previewItem;

    /// The bounding box of the current item
    BOX2I                      m_itemBBox;
};


#endif
