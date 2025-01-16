/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Author: Dick Hollenbeck
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef TITLE_BLOCK_H
#define TITLE_BLOCK_H

#include <wx/string.h>
#include <wx/arrstr.h>
#include <ki_exception.h>

class OUTPUTFORMATTER;
class PROJECT;

/**
 * Hold the information shown in the lower right corner of a plot, printout, or
 * editing view.
 */
class KICOMMON_API TITLE_BLOCK
{
    // Texts are stored in wxArraystring.
    // TEXTS_IDX gives the index of known texts in this array
    enum TEXTS_IDX
    {
        TITLE_IDX = 0,
        DATE_IDX,
        REVISION_IDX,
        COMPANY_IDX,
        COMMENT1_IDX    // idx of the first comment: one can have more than 1 comment
    };

public:
    TITLE_BLOCK() {};
    virtual ~TITLE_BLOCK() {};      // a virtual dtor seems needed to build
                                    // python lib without warning

    void SetTitle( const wxString& aTitle )
    {
        setTbText( TITLE_IDX, aTitle );
    }

    const wxString& GetTitle() const
    {
        return getTbText( TITLE_IDX );
    }

    /**
     * Set the date field, and defaults to the current time and date.
     */
    void SetDate( const wxString& aDate )
    {
        setTbText( DATE_IDX, aDate );
    }

    const wxString& GetDate() const
    {
        return getTbText( DATE_IDX );
    }

    void SetRevision( const wxString& aRevision )
    {
        setTbText( REVISION_IDX, aRevision );
    }

    const wxString& GetRevision() const
    {
        return getTbText( REVISION_IDX );
    }

    void SetCompany( const wxString& aCompany )
    {
        setTbText( COMPANY_IDX, aCompany );
    }

    const wxString& GetCompany() const
    {
        return getTbText( COMPANY_IDX );
    }

    void SetComment( int aIdx, const wxString& aComment )
    {
        aIdx += COMMENT1_IDX;
        return setTbText( aIdx, aComment );
    }

    const wxString& GetComment( int aIdx ) const
    {
        aIdx += COMMENT1_IDX;
        return getTbText( aIdx );
    }

    void Clear()
    {
        m_tbTexts.Clear();
    }

    static void GetContextualTextVars( wxArrayString* aVars );
    bool TextVarResolver( wxString* aToken, const PROJECT* aProject, int aFlags = 0 ) const;

    /**
     * Output the object to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @throw IO_ERROR on write error.
     */
    virtual void Format( OUTPUTFORMATTER* aFormatter ) const;

    static wxString GetCurrentDate();

private:
    wxArrayString m_tbTexts;

    void setTbText( int aIdx, const wxString& aText )
    {
        if( (int)m_tbTexts.GetCount() <= aIdx )
            m_tbTexts.Add( wxEmptyString, aIdx + 1 - m_tbTexts.GetCount() );

        m_tbTexts[aIdx] = aText;
    }

    const wxString& getTbText( int aIdx ) const
    {
        static const wxString m_emptytext;

        if( (int)m_tbTexts.GetCount() > aIdx )
            return m_tbTexts[aIdx];
        else
            return m_emptytext;
    }
};

#endif // TITLE_BLOCK_H
