/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef PCB_SELECTION_CONDITIONS_H_
#define PCB_SELECTION_CONDITIONS_H_

#include <tool/selection.h>
#include <tool/selection_conditions.h>
#include <eda_shape.h>


class PCB_SELECTION_CONDITIONS : public SELECTION_CONDITIONS
{
public:
    /**
     * Create a functor that tests if selection contains items belonging to the same net or are
     * unconnected if aAllowUnconnected == true.
     *
     * @param aAllowUnconnected determines if unconnected items (with no net code assigned) should
     *                          be treated as connected to the same net.
     * @return Functor testing if selected items are belonging to the same net.
     */
    static SELECTION_CONDITION SameNet( bool aAllowUnconnected = false );

    /**
     * Creates a functor that tests if selection contains items that belong exclusively to the same
     * layer.
     *
     * In case of items belonging to multiple layers, it is enough to have a single common layer
     * with other items.
     *
     * @return Functor testing if selected items share at least one common layer.
     */
    static SELECTION_CONDITION SameLayer();

    /**
     * Test if any selected items are locked.
     */
    static bool HasLockedItems( const SELECTION& aSelection );

    /**
     * Test if any selected items are unlocked.
     */
    static bool HasUnlockedItems( const SELECTION& aSelection );

private:
    ///< Helper function used by SameNet()
    static bool sameNetFunc( const SELECTION& aSelection, bool aAllowUnconnected );

    ///< Helper function used by SameLayer()
    static bool sameLayerFunc( const SELECTION& aSelection );

};

#endif /* PCB_SELECTION_CONDITIONS_H_ */
