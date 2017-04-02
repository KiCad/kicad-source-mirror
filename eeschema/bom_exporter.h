/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef EESCHEMA_BOM_EXPORTER_H_
#define EESCHEMA_BOM_EXPORTER_H_

#include <vector>
#include <wx/file.h>

#include "bom_table_column.h"
#include "bom_table_model.h"

enum BOM_EXPORT_TYPE
{
    BOM_EXPORT_CSV = 0,
    BOM_EXPORT_HTML
};

class BOM_FILE_WRITER
{
public:
    BOM_FILE_WRITER();
    virtual ~BOM_FILE_WRITER() {};

    virtual bool WriteToFile( wxFile& aFile ) = 0;

    void SetHeader( const wxArrayString aHeader );
    void AddLine( const wxArrayString aLine );

    void IncludeExtraData( bool aInclude = true ) { m_includeExtraData = aInclude; }
    void ShowRowNumbers( bool aShow = true ) { m_showRowNumbers = aShow; }

    // Project information
    void SetKicadVersion( const wxString aVersion ) { m_kicadVersion = aVersion; }
    void SetSchematicTitle( const wxString aProject ) { m_schematicTitle = aProject; }
    void SetSchematicVersion( const wxString aVersion ) { m_schematicVersion = aVersion; }
    void SetSchematicDate( const wxString aDate ) { m_schematicDate = aDate; }

    void SetGroupCount( unsigned int aCount ) { m_groupCount = wxString::Format( "%u", aCount ); }
    void SetComponentCount( unsigned int aCount ) { m_componentCount = wxString::Format( "%u", aCount ); }

protected:
    wxArrayString ExtraDataPairs();
    virtual wxString DataPair( const wxString& aFirst, const wxString& aSecond ) = 0;

    wxArrayString m_bomHeader;
    std::vector< wxArrayString > m_bomLines;

    bool m_includeExtraData;
    bool m_showRowNumbers;

    // Extra details for BOM file
    wxString m_kicadVersion;
    wxString m_schematicTitle;
    wxString m_schematicVersion;
    wxString m_schematicDate;

    wxString m_componentCount;
    wxString m_groupCount;

};

class BOM_CSV_WRITER : public BOM_FILE_WRITER
{
public:
    BOM_CSV_WRITER( wxChar aDelim=',' );

    virtual bool WriteToFile( wxFile& aFile ) override;

protected:
    wxArrayString EscapeLine( wxArrayString line );
    virtual wxString DataPair( const wxString& aFirst, const wxString& aSecond ) override;

    wxChar m_delim;
};

class BOM_HTML_WRITER : public BOM_FILE_WRITER
{
public:
    BOM_HTML_WRITER();

    virtual bool WriteToFile( wxFile& aFile ) override;

protected:
    virtual wxString DataPair( const wxString& aFirst, const wxString& aSecond ) override;

    wxString HtmlHeader();
    wxString HtmlFooter();
    wxString HtmlMetaTag( const wxString aTitle, const wxString aData );

    wxString ExtraData();

    wxString LinkText( const wxString& aText );

    wxString TableHeader( const wxArrayString& aHeaderData );
    wxString TableRow( const int& aRowNum, const wxArrayString& aRowData );
    wxString TableEntry( wxString aData, wxString aColor = wxEmptyString );

    wxArrayString m_linkChecks;
};

#endif /* EESCHEMA_BOM_EXPORTER_H_ */
