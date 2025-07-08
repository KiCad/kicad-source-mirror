/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef SCH_PREVIEW_PANEL_H
#define SCH_PREVIEW_PANEL_H

#include <class_draw_panel_gal.h>


class SCH_RENDER_SETTINGS;

namespace KIGFX {
    class SCH_VIEW;
    namespace PREVIEW {
        class SELECTION_AREA;
    };
};


class SCH_PREVIEW_PANEL : public EDA_DRAW_PANEL_GAL
{
public:
    SCH_PREVIEW_PANEL( wxWindow* aParentWindow, wxWindowID aWindowId, const wxPoint& aPosition,
                       const wxSize& aSize, KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                       GAL_TYPE aGalType = GAL_TYPE_OPENGL );

    ~SCH_PREVIEW_PANEL() override;

    ///< @copydoc EDA_DRAW_PANEL_GAL::OnShow()
    void OnShow() override;

    /// @copydoc wxWindow::Refresh()
    void Refresh( bool aEraseBackground, const wxRect* aRect ) override;

    SCH_RENDER_SETTINGS* GetRenderSettings() const;

protected:

    void onPaint( wxPaintEvent& WXUNUSED( aEvent ) ) override;

    KIGFX::SCH_VIEW* view() const;

    ///< Reassign layer order to the initial settings.
    void setDefaultLayerOrder();

    ///< Set rendering targets & dependencies for layers.
    void setDefaultLayerDeps();
};

#endif
