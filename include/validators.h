/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2013 KiCad Developers, see change_log.txt for contributors.
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
 * @file validators.h
 * @brief Custom text control validator definitions.
 */

#include <wx/valtext.h>

/**
 * Class FILE_NAME_CHAR_VALIDATOR
 *
 * This class provides a custom wxValidator object for limiting the allowable characters when
 * defining footprint names.  Since the introduction of the PRETTY footprint library format,
 * footprint names cannot have any characters that would prevent file creation on any platform.
 */
class FILE_NAME_CHAR_VALIDATOR : public wxTextValidator
{
public:
    FILE_NAME_CHAR_VALIDATOR( wxString* aValue = NULL ) :
        wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST, aValue )
    {
        // The Windows (DOS) file system forbidden characters already include the forbidden
        // file name characters for both Posix and OSX systems.  The characters \/*?|"<> are
        // illegal and filtered by the validator.
        wxString illegalChars = wxFileName::GetForbiddenChars( wxPATH_DOS );
        wxTextValidator nameValidator( wxFILTER_EXCLUDE_CHAR_LIST );
        wxArrayString illegalCharList;

        for( unsigned i = 0;  i < illegalChars.size();  i++ )
            illegalCharList.Add( wxString( illegalChars[i] ) );

        SetExcludes( illegalCharList );
    }
};
