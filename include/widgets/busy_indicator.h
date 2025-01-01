/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef COMMON_WIDGETS_BUSY_INDICATOR__H
#define COMMON_WIDGETS_BUSY_INDICATOR__H

#include <functional>
#include <memory>

/**
 * A class that can be used to implement a "busy" indicator. The exact form
 * of the busy indicator is unspecified. It could be a "spinner" cursor in a GUI
 * context, for example.
 *
 * This base class provides a "null" implementation, and can be overridden for
 * specific behaviours.
 *
 * THe busy-ness semantics are defined by this object's lifetime.
 */
class BUSY_INDICATOR
{
public:
    /**
     * A factory function that returns a new busy indicator.
     *
     * Because BUSY_INDICATORs are RAII objects (i.e. the busy-ness is defined
     * by the object's lieftime), it's convenient to pass a factory function for
     * a client to be able to make a busy indicator when needed.
     */
    using FACTORY = std::function<std::unique_ptr<BUSY_INDICATOR>()>;

    /**
     * This class is intended to be handled by pointer-to-base class
     */
    virtual ~BUSY_INDICATOR() = default;
};

#endif // COMMON_WIDGETS_BUSY_INDICATOR__H