/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DESIGN_BLOCK_PREVIEW_WIDGET_H
#define DESIGN_BLOCK_PREVIEW_WIDGET_H

#include <wx/panel.h>
#include <kiway.h>
#include <gal_display_options_common.h>
#include <class_draw_panel_gal.h>


class LIB_ID;
class DESIGN_BLOCK;
class SCHEMATIC;
class SCH_SHEET;
class wxStaticText;
class wxSizer;


class DESIGN_BLOCK_PREVIEW_WIDGET : public wxPanel
{
public:
    /**
     * Construct a symbol preview widget.
     *
     * @param aParent - parent window
     * @param aCanvasType = the type of canvas (GAL_TYPE_OPENGL or GAL_TYPE_CAIRO only)
     */
    DESIGN_BLOCK_PREVIEW_WIDGET( wxWindow* aParent, bool aIncludeStatus,
                                 EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    ~DESIGN_BLOCK_PREVIEW_WIDGET() override;

    /**
     * Set the contents of the status label and display it.
     */
    void SetStatusText( const wxString& aText );

    /**
     * Set the currently displayed symbol.
     */
    void DisplayDesignBlock( DESIGN_BLOCK* aDesignBlock );

protected:
    void onSize( wxSizeEvent& aEvent );

    void fitOnDrawArea();    // set the view scale to fit the item on screen and center

    GAL_DISPLAY_OPTIONS_IMPL   m_galDisplayOptions;
    EDA_DRAW_PANEL_GAL*        m_preview;

    wxStaticText*              m_status;
    wxPanel*                   m_statusPanel;
    wxSizer*                   m_statusSizer;
    wxSizer*                   m_outerSizer;

    SCHEMATIC*                 m_previewItem;

    /// The bounding box of the current item
    BOX2I                      m_itemBBox;
};


#endif // DESIGN_BLOCK_PREVIEW_WIDGET_H
