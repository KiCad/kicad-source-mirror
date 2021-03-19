/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCBNEW_H
#define PCBNEW_H

#include <convert_to_biu.h> // to define Mils2iu() conversion function

// These are only here for algorithmic safety, not to tell the user what to do
#define TEXTS_MIN_SIZE  Mils2iu( 1 )        ///< Minimum text size in internal units (1 mil)
#define TEXTS_MAX_SIZE  Mils2iu( 10000 )    ///< Maximum text size in internal units (10 inches)
#define TEXTS_MAX_WIDTH Mils2iu( 10000 )    ///< Maximum text width in internal units (10 inches)


#endif // PCBNEW_H
