#ifndef TITLE_BLOCK_H_
#define TITLE_BLOCK_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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


extern wxString GenDate();


/**
 * Class TITLE_BLOCK
 * holds the information shown in the lower right corner of a plot, printout, or
 * editing view.
 *
 * @author Dick Hollenbeck
 */
class TITLE_BLOCK
{
public:
    // TITLE_BLOCK();

    void SetTitle( const wxString& aTitle )             { m_title = aTitle; }
    const wxString& GetTitle() const                    { return m_title; }

    /**
     * Function SetDate
     * sets the date field, and defaults to the current time and date.
     */
    void SetDate( const wxString& aDate = GenDate() )   { m_date = aDate; }
    const wxString& GetDate() const                     { return m_date; }

    void SetRevision( const wxString& aRevision )       { m_revision = aRevision; }
    const wxString& GetRevision() const                 { return m_revision; }

    void SetCompany( const wxString& aCompany )         { m_company = aCompany; }
    const wxString& GetCompany() const                  { return m_company; }

    void SetComment1( const wxString& aComment )        { m_comment1 = aComment; }
    const wxString& GetComment1() const                 { return m_comment1; }

    void SetComment2( const wxString& aComment )        { m_comment2 = aComment; }
    const wxString& GetComment2() const                 { return m_comment2; }

    void SetComment3( const wxString& aComment )        { m_comment3 = aComment; }
    const wxString& GetComment3() const                 { return m_comment3; }

    void SetComment4( const wxString& aComment )        { m_comment4 = aComment; }
    const wxString& GetComment4() const                 { return m_comment4; }

    void Clear()
    {
        m_title.clear();
        m_date.clear();
        m_revision.clear();
        m_company.clear();
        m_comment1.clear();
        m_comment2.clear();
        m_comment3.clear();
        m_comment4.clear();
    }

private:
    wxString    m_title;
    wxString    m_date;
    wxString    m_revision;
    wxString    m_company;
    wxString    m_comment1;
    wxString    m_comment2;
    wxString    m_comment3;
    wxString    m_comment4;
};

#endif // TITLE_BLOCK_H_
