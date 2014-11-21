/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
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

#include <wx/string.h>
#include <dialog_sch_sheet_props.h>
#include <validators.h>


DIALOG_SCH_SHEET_PROPS::DIALOG_SCH_SHEET_PROPS( wxWindow* parent ) :
    DIALOG_SCH_SHEET_PROPS_BASE( parent )
{
    m_textFileName->SetValidator( FILE_NAME_WITH_PATH_CHAR_VALIDATOR() );
    m_textFileName->SetFocus();
    m_sdbSizer1OK->SetDefault();

    GetSizer()->Fit( this );
}


void DIALOG_SCH_SHEET_PROPS::SetFileName( const wxString& aFileName )
{
    // Filenames are stored using unix notation
    wxString fname = aFileName;
#ifdef __WINDOWS__
    fname.Replace( wxT("/"), wxT("\\") );
#endif
    m_textFileName->SetValue( fname );
}


const wxString DIALOG_SCH_SHEET_PROPS::GetFileName()
{
    // Filenames are stored using unix notation
    wxString fname = m_textFileName->GetValue();
    fname.Replace( wxT("\\"), wxT("/") );
    return fname;
}
