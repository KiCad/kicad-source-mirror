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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/// @file easypc_file.h The OLE compound file container, over thirdparty/compoundfilereader

#ifndef EASYPC_FILE_H
#define EASYPC_FILE_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <wx/string.h>

namespace CFB
{
class CompoundFileReader;
}


namespace EASYPC
{

/// The FileType strings the importer reads
enum class FILE_KIND
{
    PCB_DESIGN,
    SCHEMATIC_DESIGN,
    PCB_SYMBOL_LIBRARY,
    SCHEMATIC_SYMBOL_LIBRARY,
    COMPONENT_LIBRARY
};


const char* FileKindString( FILE_KIND aKind );


/// Undo the item name escaping, which stores # ! \ / : * ? " < > | as 0x06..0x10
wxString UnescapeItemName( const std::u16string& aName );


/// The whole content of a file; throws IO_ERROR when it cannot be read
std::vector<uint8_t> ReadWholeFile( const wxString& aPath );


struct SOURCE_FB_ENTRY
{
    uint32_t       Id = 0;
    std::u16string Name;     ///< without the terminator
    uint8_t        Type = 0; ///< 1 storage, 2 stream, 5 root

    bool IsStorage() const { return Type == 1 || Type == 5; }
    bool IsStream() const { return Type == 2; }

    /// FileType, LibFormat and SummaryInformation start with 0x05 and are never items
    bool IsAppStream() const { return !Name.empty() && Name[0] == 5; }
};


/// A compound file held in memory; any malformed structure throws IO_ERROR
class COMPOUND_FILE
{
public:
    explicit COMPOUND_FILE( const wxString& aPath );
    ~COMPOUND_FILE();

    const SOURCE_FB_ENTRY& Root() const { return m_entries.at( 0 ); }

    /// The members of a storage in directory-tree order
    std::vector<const SOURCE_FB_ENTRY*> Children( const SOURCE_FB_ENTRY& aStorage ) const;

    const SOURCE_FB_ENTRY* FindChild( const SOURCE_FB_ENTRY& aStorage, const std::u16string& aName ) const;
    std::vector<uint8_t>   ReadStream( const SOURCE_FB_ENTRY& aStream ) const;

    FILE_KIND FileKind() const;

    /// The LibFormat product, 1 when missing, short or 0
    int32_t LibraryProduct() const;

private:
    std::vector<uint8_t>                      m_image;
    std::unique_ptr<CFB::CompoundFileReader>  m_reader;
    std::map<uint32_t, SOURCE_FB_ENTRY>       m_entries;
    std::map<uint32_t, std::vector<uint32_t>> m_children; ///< storage id to member ids in tree order
};

} // namespace EASYPC

#endif // EASYPC_FILE_H
