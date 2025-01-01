/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#ifndef PREVIEW_SIMPLE_OUTLINE_ITEM__H_
#define PREVIEW_SIMPLE_OUTLINE_ITEM__H_

#include <eda_item.h>

#include <layer_ids.h>
#include <gal/color4d.h>

namespace KIGFX
{

class VIEW;
class GAL;

namespace PREVIEW
{

/**
 * SIMPLE_OVERLAY_ITEM is class that represents a visual area drawn on
 * a canvas, used to temporarily demarcate an area or show something
 * on an overlay. An example could be the drag select lasso box.
 *
 * This class is pretty generic in terms of what the area looks like.
 * It provides a fill, stroke and width, which is probably sufficient
 * for most simple previews, but the inheritor has freedom to override
 * this.
 *
 * If this doesn't suit a particular preview, it may mean you should
 * implement your own EDA_ITEM derivative rather than inheriting this.
 */
class SIMPLE_OVERLAY_ITEM : public EDA_ITEM
{
public:

    SIMPLE_OVERLAY_ITEM();

    /**
     * Set the overlay layer only. You can override this if
     * you have more layers to draw on.
     */
    std::vector<int> ViewGetLayers() const override;

    /**
     * Draw the preview - this is done by calling the two functions:
     * setupGal() and drawPreviewShape(). If you need more than this,
     * or direct access to the VIEW, you probably should make a new
     *.
     */
    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override;


#if defined(DEBUG)
    void Show( int x, std::ostream& st ) const override
    {
    }
#endif

    /**
     * Get class name
     * @return  string "SIMPLE_OVERLAY_ITEM"
     */
    virtual wxString GetClass() const override
    {
        return "SIMPLE_OVERLAY_ITEM";
    }

    ///< Set the stroke color to set before drawing preview
    void SetStrokeColor( const COLOR4D& aNewColor )
    {
        m_strokeColor = aNewColor;
    }

    ///< Set the fill color to set before drawing preview
    void SetFillColor( const COLOR4D& aNewColor )
    {
        m_fillColor = aNewColor;
    }

    ///< Set the line width to set before drawing preview
    void SetLineWidth( double aNewWidth )
    {
        m_lineWidth = aNewWidth;
    }

private:

    /**
     * Set up the GAL canvas - this provides a default implementation,
     * that sets fill, stroke and width.
     *
     * If that's not suitable, you can set more options in
     * updatePreviewShape(), but you might find that defining a new
     * EDA_ITEM derivative is easier for heavily customized cases.
     */
    void setupGal( KIGFX::GAL& aGal ) const;

    /**
     * Draw the preview onto the given GAL. setupGal() will be called before this function.
     *
     * Subclasses should implement this in terms of their own graphical
     * data.
     */
    virtual void drawPreviewShape( KIGFX::VIEW* aView ) const { };


    COLOR4D m_fillColor;
    COLOR4D m_strokeColor;
    double  m_lineWidth;
};

} // PREVIEW
} // KIGFX

#endif  // PREVIEW_SIMPLE_OUTLINE_ITEM__H_
