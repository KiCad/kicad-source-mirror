/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2014-2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <sch_edit_frame.h>
#include <dialog_sch_sheet_props.h>
#include <validators.h>
#include <wildcards_and_files_ext.h>
#include <confirm.h>

DIALOG_SCH_SHEET_PROPS::DIALOG_SCH_SHEET_PROPS( SCH_EDIT_FRAME* parent, SCH_SHEET* aSheet ) :
    DIALOG_SCH_SHEET_PROPS_BASE( parent ),
    m_sheet( aSheet ),
    m_filenameTextSize( parent, m_filenameSizeLabel, m_filenameSizeCtrl, m_filenameSizeUnits, true ),
    m_sheetnameTextSize( parent, m_sheetnameSizeLabel, m_sheetnameSizeCtrl, m_sheetnameSizeUnits, true )
{
    m_textFileName->SetValidator( FILE_NAME_WITH_PATH_CHAR_VALIDATOR() );
    m_textFileName->SetFocus();
    m_sdbSizer1OK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();

    /* This ugly hack fixes a bug in wxWidgets 2.8.7 and likely earlier
     * versions for the flex grid sizer in wxGTK that prevents the last
     * column from being sized correctly.  It doesn't cause any problems
     * on win32 so it doesn't need to wrapped in ugly #ifdef __WXGTK__
     * #endif.
     * Still present in wxWidgets 3.0.2
     */
    Layout();
    Fit();
    SetMinSize( GetSize() );
    GetSizer()->Fit( this );
}


bool DIALOG_SCH_SHEET_PROPS::TransferDataToWindow()
{
    // Filenames are stored using unix notation
    wxString fname = m_sheet->GetFileName();

#ifdef __WINDOWS__
    fname.Replace( wxT("/"), wxT("\\") );
#endif
    m_textFileName->SetValue( fname );

    m_textSheetName->SetValue( m_sheet->GetName() );

    m_filenameTextSize.SetValue( m_sheet->GetFileNameSize() );
    m_sheetnameTextSize.SetValue( m_sheet->GetSheetNameSize() );

    auto tstamp = wxString::Format( wxT( "%8.8lX" ), (unsigned long) m_sheet->GetTimeStamp() );
    m_textCtrlTimeStamp->SetValue( tstamp );

    return true;
}


bool DIALOG_SCH_SHEET_PROPS::TransferDataFromWindow()
{
    wxFileName fileName = GetFileName();
    fileName.SetExt( SchematicFileExtension );

    if( !fileName.IsOk() )
    {
        DisplayError( this, _( "File name is not valid!" ) );
        return false;
    }

    // Duplicate sheet names are not valid.
    SCH_SHEET_LIST   hierarchy( g_RootSheet );
    const SCH_SHEET* sheet = hierarchy.FindSheetByName( GetSheetName() );

    if( sheet && (sheet != m_sheet) )
    {
        DisplayError( this, wxString::Format( _( "A sheet named \"%s\" already exists." ),
                                              GetSheetName() ) );
        return false;
    }

    return true;
}


const wxString DIALOG_SCH_SHEET_PROPS::GetFileName()
{
    // Filenames are stored using unix notation
    wxString fname = m_textFileName->GetValue();
    fname.Replace( wxT("\\"), wxT("/") );
    return fname;
}
