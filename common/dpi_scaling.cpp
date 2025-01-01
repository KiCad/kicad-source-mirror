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

#include <dpi_scaling.h>


double DPI_SCALING::GetMaxScaleFactor()
{
    // displays with higher than 4.0 DPI are not really going to be useful
    // for KiCad (even an 8k display would be effectively only ~1080p at 4x)
    return 6.0;
}


double DPI_SCALING::GetMinScaleFactor()
{
    // scales under 1.0 don't make sense from a HiDPI perspective
    return 1.0;
}


double DPI_SCALING::GetDefaultScaleFactor()
{
    // no scaling => 1.0
    return 1.0;
}
