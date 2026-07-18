/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <mdw/client/mdw_compare_doc.h>

#include <git/kigit_symbol_split.h>

#include <wx/filename.h>


MDW_COMPARE_DOC MdwCompareDocument( const wxString& aItemId, const wxString& aLocalPath )
{
    const wxString& source = aLocalPath.IsEmpty() ? aItemId : aLocalPath;

    return { KICAD_DIFF::DocKindFromExtension( source ), wxFileName( source ).GetFullName() };
}


std::string MdwSymbolMemberText( const std::string& aLibraryText, const wxString& aMember )
{
    const KIGIT::SYMBOL_FILE file = KIGIT::SplitSymbols( aLibraryText );
    const auto               span = file.spans.find( aMember.ToStdString( wxConvUTF8 ) );

    if( !file.complete || span == file.spans.end() )
        return std::string();

    return file.header + aLibraryText.substr( span->second.start, span->second.length ) + "\n)\n";
}
