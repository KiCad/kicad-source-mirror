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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMMON_WIDGETS_WX_BUSY_INDICATOR__H
#define COMMON_WIDGETS_WX_BUSY_INDICATOR__H

#include <widgets/busy_indicator.h>


class wxBusyCursor;


/**
 * Simple wrapper around wxBusyCursor for used with the generic BUSY_INDICATOR interface.
 *
 * Can be used to provide a WX busy cursor (spinner) to generic code that otherwise has no
 * concept of wx cursors.
 */
class WX_BUSY_INDICATOR : public BUSY_INDICATOR
{
public:
    /**
     * Construct a busy indicator represented by a wxBusyCursor, which will be
     * active as long as this object exists (just like wxBustCursor itself).
     */
    WX_BUSY_INDICATOR();

private:
    /// This is the actual WX cursor that is the indicator.
    std::unique_ptr<wxBusyCursor> m_cursor;
};

#endif // COMMON_WIDGETS_WX_BUSY_INDICATOR__H
