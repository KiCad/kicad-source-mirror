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

/**
 * @file easypc_document.h
 * @brief Loading designs, library items and projects.
 *
 * A design (.pcb .sch) is one archive, the Contents stream, read to its exact end.  A symbol library (.psl .ssl)
 * holds one archive per item stream and a component library (.cml) one storage per component with a
 * "[ScmComponent]" stream and one stream per package.  Each library stream is one ReadObject, after which the
 * program ignores the rest, since items rewritten in place leave stale bytes.  A project (.prj) is a bare archive.
 */

#ifndef EASYPC_DOCUMENT_H
#define EASYPC_DOCUMENT_H

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <wx/string.h>

#include <io/easypc/easypc_archive.h>
#include <io/easypc/easypc_classes_project.h>
#include <io/easypc/easypc_file.h>


namespace EASYPC
{

struct DESIGN_DOCUMENT
{
    FILE_KIND                            FileKind = FILE_KIND::PCB_DESIGN;
    DOC_KIND                             Kind = DOC_KIND::PCB;
    std::unique_ptr<OBJECT>              Design; ///< design root, not in the load array
    std::vector<std::unique_ptr<OBJECT>> Objects;
    std::vector<uint8_t>                 SummaryInformation; ///< the SummaryInformation stream, if any
};


struct LIBRARY_STREAM
{
    OBJECT*  Item = nullptr; ///< what the stream's single ReadObject returned
    wxString StreamName;
};


/// One stream, or for a component library every stream of the component's storage
struct LIBRARY_ITEM
{
    FILE_KIND                            FileKind = FILE_KIND::PCB_SYMBOL_LIBRARY;
    wxString                             Name;
    std::vector<LIBRARY_STREAM>          Streams; ///< for .cml "[ScmComponent]" first
    std::vector<std::unique_ptr<OBJECT>> Objects;
};


struct PROJECT_DOCUMENT
{
    std::unique_ptr<SOURCE_PROJECT>      Project;
    std::vector<std::unique_ptr<OBJECT>> Objects;
};


struct FILE_INFO
{
    bool                     IsProject = false;
    std::optional<FILE_KIND> FileKind;
};


/// A library parsed once; items are then served by name without reading the file again
class LIBRARY_FILE
{
public:
    /// Throws IO_ERROR when the file is not a library or is malformed
    explicit LIBRARY_FILE( const wxString& aPath );

    /// A library file's modification time, 0 when it does not exist
    static long long Timestamp( const wxString& aPath );

    FILE_KIND Kind() const { return m_kind; }

    /// Item names in directory order, unescaped
    const std::vector<wxString>& ItemNames() const { return m_names; }
    bool                         HasItem( const wxString& aItemName ) const { return m_byName.count( aItemName ) > 0; }

    /// Warnings for item names that collide after unescaping
    const std::vector<wxString>& Warnings() const { return m_warnings; }

    /// Throws IO_ERROR if no item has that name
    std::unique_ptr<LIBRARY_ITEM> LoadItem( const wxString& aItemName ) const;

private:
    wxString                            m_path;
    COMPOUND_FILE                       m_file;
    FILE_KIND                           m_kind;
    int32_t                             m_product = PRODUCT_EASYPC;
    std::vector<const SOURCE_FB_ENTRY*> m_entries;
    std::vector<wxString>               m_names;
    std::map<wxString, size_t>          m_byName;
    std::vector<wxString>               m_warnings;
};


std::unique_ptr<DESIGN_DOCUMENT> LoadDesign( const wxString& aPath );

std::unique_ptr<PROJECT_DOCUMENT> LoadProject( const wxString& aPath );

/**
 * The file a project item names, as Easy-PC opens it: the Windows path relative to the project's directory,
 * the file name matched without regard to case.  Returned even when no such file exists.
 */
wxString ResolveProjectItem( const wxString& aProjectPath, const wxString& aItemName );

/// Identify a file; projects are parsed fully, while designs and libraries are only inspected. Never throws.
FILE_INFO Probe( const wxString& aPath );

} // namespace EASYPC

#endif // EASYPC_DOCUMENT_H
