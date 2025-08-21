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

#ifndef SYMBOL_PREVIEW_WIDGET_H
#define SYMBOL_PREVIEW_WIDGET_H

#include <wx/panel.h>
#include <kiway.h>
#include <gal_display_options_common.h>
#include <class_draw_panel_gal.h>


class LIB_ID;
class LIB_SYMBOL;
class wxStaticText;
class wxSizer;
class SCH_RENDER_SETTINGS;


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
    SYMBOL_PREVIEW_WIDGET( wxWindow* aParent, KIWAY* aKiway, bool aIncludeStatus,
                           EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    ~SYMBOL_PREVIEW_WIDGET() override;

    /**
     * Set the contents of the status label and display it.
     */
    void SetStatusText( const wxString& aText );

    /**
     * Set the currently displayed symbol.
     */
    void DisplaySymbol( const LIB_ID& aSymbolID, int aUnit, int aBodyStyle = 0 );

    void DisplayPart( LIB_SYMBOL* aSymbol, int aUnit, int aBodyStyle = 0 );

    EDA_DRAW_PANEL_GAL* GetCanvas() const { return m_preview; }

    SCH_RENDER_SETTINGS* GetRenderSettings() { return m_renderSettings; }

protected:
    void onSize( wxSizeEvent& aEvent );

    void fitOnDrawArea();    // set the view scale to fit the item on screen and center

    KIWAY*                     m_kiway;

    GAL_DISPLAY_OPTIONS_IMPL   m_galDisplayOptions;
    EDA_DRAW_PANEL_GAL*        m_preview;

    wxStaticText*              m_status;
    wxPanel*                   m_statusPanel;
    wxSizer*                   m_statusSizer;
    wxSizer*                   m_outerSizer;

    /**
     * A local copy of the #LIB_SYMBOL to display on the canvas.
     */
    LIB_SYMBOL*                m_previewItem;
    SCH_RENDER_SETTINGS*       m_renderSettings;

    /// The bounding box of the current item
    BOX2I                      m_itemBBox;
};


#endif // SYMBOL_PREVIEW_WIDGET_H
