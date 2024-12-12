/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RE_CLASSES_H_
#define DRC_RE_CLASSES_H_

#include <wx/arrstr.h>
#include <wx/bitmap.h>
#include <wx/chartype.h>
#include <wx/statbmp.h>
#include <wx/tokenzr.h>

#include <bitmaps.h>
#include <confirm.h>
#include <eda_text.h>
#include <footprint_editor_settings.h>
#include <grid_layer_box_helpers.h>
#include <grid_tricks.h>
#include <kidialog.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <template_fieldnames.h>
#include <string_utils.h>

#include <algorithm>
#include <vector>

#include "drc_rule_editor_enums.h"

class DRC_RULE_EDITOR_CONTENT_PANEL_BASE
{
public:
    DRC_RULE_EDITOR_CONTENT_PANEL_BASE() = default;

    virtual ~DRC_RULE_EDITOR_CONTENT_PANEL_BASE() = default;

    virtual bool ValidateInputs( int* aErrorCount, std::string* aValidationMessage ) = 0;

    virtual wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) = 0;

    wxStaticBitmap* GetConstraintImage(wxPanel* aParent, BITMAPS aBitMap)
    {
        return new wxStaticBitmap( aParent, wxID_ANY, KiBitmapBundle( aBitMap ),
                                   wxDefaultPosition, wxSize( -1, 250 ), 0 );
    }

protected:
    static wxString sanitizeRuleName( const wxString& aRuleName )
    {
        if( aRuleName.IsEmpty() )
            return wxString( wxS( "rule" ) );

        wxString result;
        result.reserve( aRuleName.length() );

        for( wxUniChar c : aRuleName )
        {
            if( wxIsspace( c ) )
            {
                result.append( '_' );
            }
            else if( wxIsalnum( c ) || c == '_' || c == '-' || c == '.' )
            {
                result.append( c );
            }
            else
            {
                result.append( '_' );
            }
        }

        // Avoid names starting with a digit which S-expression parsers treat specially.
        if( !result.empty() && wxIsdigit( *result.begin() ) )
            result.Prepend( wxS( "r_" ) );

        return result;
    }

    static wxString escapeCondition( const wxString& aCondition )
    {
        return EscapeString( aCondition, CTX_QUOTED_STR );
    }

    static wxString trimTrailingZeros( const wxString& aValue )
    {
        wxString result( aValue );

        if( !result.Contains( '.' ) )
            return result;

        while( result.Length() > 1 && result.Last() == '0' )
            result.Truncate( result.Length() - 1 );

        if( result.Last() == '.' )
            result.Truncate( result.Length() - 1 );

        return result;
    }

    static wxString formatDouble( double aValue, int aPrecision = 6 )
    {
        wxString formatted = wxString::Format( wxS( "%.*f" ), aPrecision, aValue );
        return trimTrailingZeros( formatted );
    }

    wxString buildRule( const RULE_GENERATION_CONTEXT& aContext,
                        const std::vector<wxString>& aConstraintClauses ) const
    {
        wxString rule;
        rule << wxS( "(rule " ) << sanitizeRuleName( aContext.ruleName ) << wxS( "\n" );

        if( !aContext.comment.IsEmpty() )
        {
            wxArrayString lines = wxSplit( aContext.comment, '\n', '\0' );

            for( const wxString& line : lines )
            {
                if( line.IsEmpty() )
                    continue;

                rule << wxS( "\t# " ) << line << wxS( "\n" );
            }
        }

        if( !aContext.layerClause.IsEmpty() )
            rule << wxS( "\t" ) << aContext.layerClause << wxS( "\n" );

        for( const wxString& clause : aConstraintClauses )
        {
            if( clause.IsEmpty() )
                continue;

            rule << wxS( "\t" ) << clause << wxS( "\n" );
        }

        if( !aContext.conditionExpression.IsEmpty() )
        {
            rule << wxS( "\t(condition \"" ) << escapeCondition( aContext.conditionExpression )
                 << wxS( "\")\n" );
        }

        rule << wxS( ")\n" );

        return rule;
    }
};

#endif // DRC_RE_CLASSES_H_
