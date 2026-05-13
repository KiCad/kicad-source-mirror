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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include <widgets/wx_data_view_hyperlink_renderer.h>

#include <wx/dcclient.h>
#include <wx/settings.h>
#include <wx/utils.h>


namespace
{

// Parse [label](url) starting at aStart
bool tryParseMarkdown( const wxString& aText, size_t aStart, size_t& aEnd, wxString& aLabel, wxString& aHref )
{
    if( aStart >= aText.length() || aText[aStart] != '[' )
        return false;

    size_t labelEnd = aText.find( ']', aStart + 1 );

    if( labelEnd == wxString::npos || labelEnd + 1 >= aText.length() || aText[labelEnd + 1] != '(' )
        return false;

    // Match balanced parens so URLs like "file:///foo (1).pdf" survive.
    size_t hrefStart = labelEnd + 2;
    size_t hrefEnd = wxString::npos;
    int    depth = 1;

    for( size_t i = hrefStart; i < aText.length(); ++i )
    {
        wxChar c = aText[i];

        if( c == '(' )
            ++depth;
        else if( c == ')' && --depth == 0 )
        {
            hrefEnd = i;
            break;
        }
    }

    if( hrefEnd == wxString::npos )
        return false;

    aLabel = aText.SubString( aStart + 1, labelEnd - 1 );
    aHref = aText.SubString( hrefStart, hrefEnd - 1 );
    aEnd = hrefEnd + 1;
    return true;
}


void layoutRuns( wxDC& aDC, std::vector<HYPERLINK_DV_RENDERER::RUN>& aRuns, int aX, int aY, int aHeight )
{
    int x = aX;

    for( HYPERLINK_DV_RENDERER::RUN& run : aRuns )
    {
        wxSize ext = aDC.GetTextExtent( run.text );
        run.bounds = wxRect( x, aY, ext.x, aHeight > 0 ? aHeight : ext.y );
        x += ext.x;
    }
}

} // namespace


bool HYPERLINK_DV_RENDERER::IsSafeUrl( const wxString& aHref )
{
    wxString lower = aHref.Lower();

    if( lower.StartsWith( wxT( "http://" ) ) || lower.StartsWith( wxT( "https://" ) ) )
        return true;

    bool isLocalFile = lower.StartsWith( wxT( "file://" ) ) || lower.StartsWith( wxT( "\\\\" ) );

    if( isLocalFile )
    {
        static const wxString blockedExtensions[] = { wxT( ".exe" ), wxT( ".com" ), wxT( ".bat" ), wxT( ".cmd" ),
                                                      wxT( ".ps1" ), wxT( ".vbs" ), wxT( ".js" ),  wxT( ".jar" ),
                                                      wxT( ".msi" ), wxT( ".scr" ), wxT( ".pif" ), wxT( ".lnk" ),
                                                      wxT( ".hta" ) };

        for( const wxString& ext : blockedExtensions )
        {
            if( lower.EndsWith( ext ) )
                return false;
        }

        return true;
    }

    return false;
}


wxString HYPERLINK_DV_RENDERER::StripMarkup( const wxString& aValue )
{
    std::vector<RUN> runs;
    ParseRuns( aValue, runs );

    wxString result;

    for( const RUN& run : runs )
        result += run.text;

    return result;
}


void HYPERLINK_DV_RENDERER::ParseRuns( const wxString& aValue, std::vector<RUN>& aRuns )
{
    aRuns.clear();

    size_t   pos = 0;
    wxString plain;

    while( pos < aValue.length() )
    {
        size_t   end = pos;
        wxString label;
        wxString href;

        if( aValue[pos] == '[' && tryParseMarkdown( aValue, pos, end, label, href ) )
        {
            if( !plain.IsEmpty() )
            {
                aRuns.push_back( { plain, wxEmptyString, wxRect() } );
                plain.clear();
            }

            if( IsSafeUrl( href ) )
                aRuns.push_back( { label, href, wxRect() } );
            else
                aRuns.push_back( { label, wxEmptyString, wxRect() } );

            pos = end;
        }
        else
        {
            plain += aValue[pos];
            ++pos;
        }
    }

    if( !plain.IsEmpty() )
        aRuns.push_back( { plain, wxEmptyString, wxRect() } );
}


HYPERLINK_DV_RENDERER::HYPERLINK_DV_RENDERER() :
        wxDataViewCustomRenderer( wxT( "string" ), wxDATAVIEW_CELL_INERT, wxDVR_DEFAULT_ALIGNMENT )
{
}


bool HYPERLINK_DV_RENDERER::SetValue( const wxVariant& aValue )
{
    m_value = aValue.GetString();
    ParseRuns( m_value, m_runs );
    return true;
}


bool HYPERLINK_DV_RENDERER::GetValue( wxVariant& aValue ) const
{
    aValue = m_value;
    return true;
}


wxSize HYPERLINK_DV_RENDERER::GetSize() const
{
    if( m_value.IsEmpty() )
        return wxSize( wxDVC_DEFAULT_RENDERER_SIZE, wxDVC_DEFAULT_RENDERER_SIZE );

    // Sum the visible-text width across runs (markup is hidden, so the raw
    // m_value width over-reports).
    wxSize size( 0, 0 );

    for( const RUN& run : m_runs )
    {
        wxSize ext = GetTextExtent( run.text );
        size.x += ext.x;
        size.y = std::max( size.y, ext.y );
    }

    if( size.y == 0 )
        size.y = wxDVC_DEFAULT_RENDERER_SIZE;

    return size;
}


bool HYPERLINK_DV_RENDERER::Render( wxRect aCell, wxDC* aDC, int aState )
{
    RenderBackground( aDC, aCell );

    // Capture aDC's font so HitTestRunsForCell can measure with the same font.
    m_renderFont = aDC->GetFont();

    layoutRuns( *aDC, m_runs, aCell.x, aCell.y, aCell.height );

    const bool selected = ( aState & wxDATAVIEW_CELL_SELECTED ) != 0;
    int        xoffset = 0;

#ifdef __WXOSX__
    const int textState = 0;
#else
    const int textState = aState;
#endif

    for( const RUN& run : m_runs )
    {
        wxRect runRect = aCell;
        runRect.x += xoffset;
        runRect.width = run.bounds.width;

        if( !run.href.IsEmpty() )
        {
            wxDCTextColourChanger col( *aDC, selected ? wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT )
                                                      : wxSystemSettings::GetColour( wxSYS_COLOUR_HOTLIGHT ) );

            wxFont linkFont = aDC->GetFont();
            linkFont.SetUnderlined( true );
            wxDCFontChanger font( *aDC, linkFont );

            RenderText( run.text, 0, runRect, aDC, textState );
        }
        else
        {
#ifdef __WXOSX__
            if( selected )
                aDC->SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT ) );
#endif
            RenderText( run.text, 0, runRect, aDC, textState );
        }

        xoffset += run.bounds.width;
    }

    return true;
}


bool HYPERLINK_DV_RENDERER::HitTestRunsForCell( const wxString& aValue, const wxRect& aCell, const wxPoint& aPoint,
                                                wxString* aHref ) const
{
    std::vector<RUN> runs;
    ParseRuns( aValue, runs );

    wxClientDC dc( const_cast<wxDataViewCtrl*>( GetView() ) );
    dc.SetFont( m_renderFont.IsOk() ? m_renderFont : GetView()->GetFont() );
    layoutRuns( dc, runs, aCell.x, aCell.y, aCell.height );

    for( const RUN& run : runs )
    {
        if( !run.href.IsEmpty() && run.bounds.Contains( aPoint ) )
        {
            if( aHref )
                *aHref = run.href;

            return true;
        }
    }

    return false;
}
