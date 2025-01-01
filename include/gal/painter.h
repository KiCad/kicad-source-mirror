/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __CLASS_PAINTER_H
#define __CLASS_PAINTER_H

#include <map>
#include <set>

#include <wx/dc.h>

#include <gal/gal.h>
#include <gal/color4d.h>
#include <render_settings.h>
#include <layer_ids.h>
#include <memory>

namespace KIGFX
{
class GAL;
class VIEW_ITEM;

/**
 * Contains all the knowledge about how to draw graphical object onto any particular
 * output device.
 *
 * This knowledge is held outside the individual graphical objects so that alternative
 * output devices may be used, and so that the graphical objects themselves to not
 * contain drawing routines.  Drawing routines in the objects cause problems with usages
 * of the objects as simple container objects in DLL/DSOs.  PAINTER is an abstract layer
 * because every module (pcbnew, eeschema, etc.) has to draw different kinds of objects.
 */
class GAL_API PAINTER
{
public:
    /**
     * Initialize this object for painting on any of the polymorphic
     * GRAPHICS_ABSTRACTION_LAYER* derivatives.
     *
     * @param aGal is a pointer to a polymorphic GAL device on which to draw (i.e. Cairo,
     *             OpenGL, wxDC).  No ownership is given to this PAINTER of aGal.
     */
    PAINTER( GAL* aGal );
    virtual ~PAINTER();

    /**
     * Changes Graphics Abstraction Layer used for drawing items for a new one.
     *
     * @param aGal is the new GAL instance.
     */
    void SetGAL( GAL* aGal )
    {
        m_gal = aGal;
    }

    /**
     * Return a pointer to current settings that are going to be used when drawing items.
     *
     * @return Current rendering settings.
     */
    virtual RENDER_SETTINGS* GetSettings() = 0;

    /**
     * Takes an instance of VIEW_ITEM and passes it to a function that knows how to draw
     * the item.
     *
     * @param aItem is an item to be drawn.
     * @param aLayer tells which layer is currently rendered so that draw functions may
     *               know what to draw (eg. for pads there are separate layers for holes,
     *               because they have other dimensions then the pad itself.
     */
    virtual bool Draw( const VIEW_ITEM* aItem, int aLayer ) = 0;

protected:
    /// Instance of graphic abstraction layer that gives an interface to call
    /// commands used to draw (eg. DrawLine, DrawCircle, etc.)
    GAL* m_gal;
};

} // namespace KIGFX

#endif /* __CLASS_PAINTER_H */
