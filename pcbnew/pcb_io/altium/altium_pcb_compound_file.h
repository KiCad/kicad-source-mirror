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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef ALTIUM_PCB_COMPOUND_FILE_H
#define ALTIUM_PCB_COMPOUND_FILE_H

#include <memory>
#include <altium_parser_pcb.h>
#include <io/altium/altium_binary_parser.h>
#include <case_insensitive_map.h>


class ALTIUM_PCB_COMPOUND_FILE : public ALTIUM_COMPOUND_FILE
{
public:
    ALTIUM_PCB_COMPOUND_FILE() : ALTIUM_COMPOUND_FILE() {}

    ALTIUM_PCB_COMPOUND_FILE( const wxString& aFilePath );
    ALTIUM_PCB_COMPOUND_FILE( const void* aBuffer, size_t aLen );
    ~ALTIUM_PCB_COMPOUND_FILE();

    CASE_INSENSITIVE_MAP<wxString> ListLibFootprints();

    std::tuple<wxString, const CFB::COMPOUND_FILE_ENTRY*> FindLibFootprintDirName( const wxString& aFpUnicodeName );

    const std::pair<AMODEL, std::vector<char>>* GetLibModel( const wxString& aModelID ) const;

    bool CacheLibModels();
private:

    void cacheLibFootprintNames();

    CASE_INSENSITIVE_MAP<const CFB::COMPOUND_FILE_ENTRY*>      m_libFootprintNameCache;
    CASE_INSENSITIVE_MAP<wxString>                             m_libFootprintDirNameCache;
    CASE_INSENSITIVE_MAP<std::pair<AMODEL, std::vector<char>>> m_libModelsCache;
};

#endif // ALTIUM_PCB_COMPOUND_FILE_H
