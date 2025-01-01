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

/**
 * @file i18n_utility.h
 * @brief Some functions to handle hotkeys in KiCad
 */

#ifndef  I18N_UTILITY_H
#define  I18N_UTILITY_H

// A define to allow translation of strings which must be stored in English (for instance
// because they are used both as keywords and as messages in dialogs
// We do not want to use the _( x ) usual macro from wxWidgets, which calls wxGetTranslation(),
// because the English string is used in key file configuration
// The translated string is used only when displaying the help window.
// Therefore translation tools have to use the "_" and the "_HKI" prefix to extract
// strings to translate
#define _HKI( x ) wxT( x )

#endif // I18N_UTILITY_H
