/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <io/io_base.h>
#include <ki_exception.h>
#include <wildcards_and_files_ext.h>
#include <wx/translation.h>

#define FMT_UNIMPLEMENTED wxT( "IO interface \"%s\" does not implement the \"%s\" function." )
#define NOT_IMPLEMENTED( aCaller )                                       \
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED,                 \
                                      GetName(),                         \
                                      wxString::FromUTF8( aCaller ) ) );


wxString IO_BASE::IO_FILE_DESC::FileFilter() const
{
    return wxGetTranslation( m_Description ) + AddFileExtListToFilter( m_FileExtensions );
}


void IO_BASE::CreateLibrary( const wxString& aLibraryPath, const STRING_UTF8_MAP* aProperties )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


bool IO_BASE::DeleteLibrary( const wxString& aLibraryPath, const STRING_UTF8_MAP* aProperties )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


bool IO_BASE::IsLibraryWritable( const wxString& aLibraryPath )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}

void IO_BASE::GetLibraryOptions( STRING_UTF8_MAP* aListToAppendTo ) const
{
    // No global options to append
}


bool IO_BASE::CanReadLibrary( const wxString& aFileName ) const
{
    // TODO: Push file extension based checks from PCB_IO and SCH_IO into this function
    return false;
}
