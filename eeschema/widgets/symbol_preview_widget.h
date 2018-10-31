/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SYMBOL_PREVIEW_WIDGET_H
#define SYMBOL_PREVIEW_WIDGET_H

#include <wx/panel.h>
#include <kiway.h>
#include <gal/gal_display_options.h>
#include <class_draw_panel_gal.h>


class EDA_ITEM;
class LIB_ID;
class LIB_ALIAS;
class LIB_PART;
class wxStaticText;
class wxSizer;


class SYMBOL_PREVIEW_WIDGET: public wxPanel
{
public:

    /**
     * Construct a symbol preview widget.
     *
     * @param aParent - parent window
     * @param aKiway - an active Kiway instance
     * @param aCanvasType = the type of canvas (GAL_TYPE_OPENGL or GAL_TYPE_CAIRO only)
     */
    SYMBOL_PREVIEW_WIDGET( wxWindow* aParent, KIWAY& aKiway,
                           EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    ~SYMBOL_PREVIEW_WIDGET() override;

    /**
     * Set the contents of the status label and display it.
     */
    void SetStatusText( wxString const& aText );

    /**
     * Set the currently displayed symbol.
     */
    void DisplaySymbol( const LIB_ID& aSymbolID, int aUnit );

    void DisplayPart( LIB_PART* aPart, int aUnit );

private:
    void onSize( wxSizeEvent& aEvent );

    void fitOnDrawArea();    // set the view scale to fit the item on screen and center

    KIWAY&                     m_kiway;

    KIGFX::GAL_DISPLAY_OPTIONS m_galDisplayOptions;
    EDA_DRAW_PANEL_GAL*        m_preview;

    wxStaticText*              m_status;
    wxSizer*                   m_statusSizer;

    /** a local copy of the LIB_ALIAS or the LIB_PART to display on the canvas
     */
    EDA_ITEM*                  m_previewItem;

    /// The bounding box of the current item
    BOX2I                      m_itemBBox;
};


#endif // SYMBOL_PREVIEW_WIDGET_H
