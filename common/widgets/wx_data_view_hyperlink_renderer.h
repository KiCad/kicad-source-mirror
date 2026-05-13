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

#ifndef WX_DATA_VIEW_HYPERLINK_RENDERER_H
#define WX_DATA_VIEW_HYPERLINK_RENDERER_H

#include <vector>
#include <wx/dataview.h>
#include <wx/dc.h>
#include <wx/string.h>
#include <kicommon.h>


/**
 * wxDataViewCtrl renderer that draws [label](url) markup as clickable link
 * runs inline with plain text.
 */
class KICOMMON_API HYPERLINK_DV_RENDERER : public wxDataViewCustomRenderer
{
public:
    HYPERLINK_DV_RENDERER();

    bool   SetValue( const wxVariant& aValue ) override;
    bool   GetValue( wxVariant& aValue ) const override;
    wxSize GetSize() const override;
    bool   Render( wxRect aCell, wxDC* aDC, int aState ) override;

    /// Hit-test aPoint against the link runs of aValue laid out in aCell.
    bool HitTestRunsForCell( const wxString& aValue, const wxRect& aCell, const wxPoint& aPoint,
                             wxString* aHref ) const;

    struct RUN
    {
        wxString text;
        wxString href; // empty for plain text
        wxRect   bounds;
    };

    /// Split aValue into plain text and link runs.
    static void ParseRuns( const wxString& aValue, std::vector<RUN>& aRuns );

    /// Accept only schemes that wxLaunchDefaultBrowser cannot turn into program execution.
    static bool IsSafeUrl( const wxString& aHref );

    /// Flatten [label](url) markup, keeping just the label.
    static wxString StripMarkup( const wxString& aValue );

private:
    wxString         m_value;
    std::vector<RUN> m_runs;
    wxFont           m_renderFont;
};

#endif // WX_DATA_VIEW_HYPERLINK_RENDERER_H
