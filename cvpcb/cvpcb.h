/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef __CVPCB_H__
#define __CVPCB_H__

// config for footprints doc file access
#define DEFAULT_FOOTPRINTS_LIST_FILENAME wxT( "footprints_doc/footprints.pdf" )

// Define print format to display a schematic component line
#define CMP_FORMAT wxT( "%3d %8s - %16s : %s" )

#define FILTERFOOTPRINTKEY "FilterFootprint"

#define LISTB_STYLE     ( wxSUNKEN_BORDER | wxLC_NO_HEADER | wxLC_REPORT | wxLC_VIRTUAL | \
                          wxLC_SINGLE_SEL | wxVSCROLL | wxHSCROLL )

extern const wxString FootprintAliasFileExtension;
extern const wxString FootprintAliasFileWildcard;


#endif /* __CVPCB_H__ */
