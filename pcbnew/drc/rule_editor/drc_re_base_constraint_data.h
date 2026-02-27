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

#ifndef DRC_RE_BASE_CONSTRAINT_DATA_H_
#define DRC_RE_BASE_CONSTRAINT_DATA_H_

#include <bitmaps.h>
#include <dialogs/rule_editor_data_base.h>
#include <widgets/report_severity.h>
#include "drc_re_overlay_types.h"
#include "drc_rule_editor_enums.h"
#include <string_utils.h>
#include <wx/arrstr.h>

#include <vector>


class DRC_RE_BASE_CONSTRAINT_DATA : public RULE_EDITOR_DATA_BASE
{
public:
    DRC_RE_BASE_CONSTRAINT_DATA() = default;

    explicit DRC_RE_BASE_CONSTRAINT_DATA( int aId, int aParentId, const wxString& aRuleName ) :
            RULE_EDITOR_DATA_BASE( aId, aParentId, aRuleName )
    {
    }

    virtual ~DRC_RE_BASE_CONSTRAINT_DATA() = default;

    /**
     * Validates the constraint data.
     * Override in derived classes to add constraint-specific validation.
     *
     * @return Validation result containing any errors.
     */
    virtual VALIDATION_RESULT Validate() const { return VALIDATION_RESULT(); }

    virtual wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) { return wxEmptyString; }

    /**
     * Returns the bitmap to use for the overlay panel background.
     * Override in derived classes to provide constraint-specific artwork.
     *
     * @return BITMAPS enum value for the overlay bitmap, or BITMAPS::INVALID_BITMAP if none.
     */
    virtual BITMAPS GetOverlayBitmap() const { return BITMAPS::INVALID_BITMAP; }

    /**
     * Returns the field positions for controls overlaid on the constraint bitmap.
     * Override in derived classes to specify where input fields should be placed.
     * Each position includes X range, Y coordinate, and tab order for navigation.
     *
     * @return Vector of field position specifications, empty if no overlay fields.
     */
    virtual std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const { return {}; }

    /**
     * Returns just the constraint clauses without the rule wrapper.
     * Used for merging multiple constraint panels into a single rule.
     */
    virtual std::vector<wxString> GetConstraintClauses( const RULE_GENERATION_CONTEXT& aContext ) const
    {
        return {};
    }

    std::vector<PCB_LAYER_ID> GetLayers() { return m_layers; }

    void SetLayers( std::vector<PCB_LAYER_ID> aLayers ) { m_layers = aLayers; }

    wxString GetLayerSource() { return m_layerSource; }

    void SetLayerSource( const wxString& aSource ) { m_layerSource = aSource; }

    wxString GetRuleCondition() { return m_ruleCondition; }

    void SetRuleCondition( const wxString &aRuleCondition ) { m_ruleCondition = aRuleCondition; }

    wxString GetConstraintCode() const { return m_constraintCode; }

    void SetConstraintCode( const wxString& aCode ) { m_constraintCode = aCode; }

    wxString GetGeneratedRule() const { return m_generatedRule; }

    void SetGeneratedRule( const wxString& aRule ) { m_generatedRule = aRule; }

    wxString GetOriginalRuleText() const { return m_originalRuleText; }

    void SetOriginalRuleText( const wxString& aText ) { m_originalRuleText = aText; }

    bool WasEdited() const { return m_wasEdited; }

    void SetWasEdited( bool aEdited ) { m_wasEdited = aEdited; }

    SEVERITY GetSeverity() const { return m_severity; }

    void SetSeverity( SEVERITY aSeverity ) { m_severity = aSeverity; }

    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& source = dynamic_cast<const DRC_RE_BASE_CONSTRAINT_DATA&>( aSource );

        RULE_EDITOR_DATA_BASE::CopyFrom( source );

        m_layers = source.m_layers;
        m_layerSource = source.m_layerSource;
        m_constraintCode = source.m_constraintCode;
        m_generatedRule = source.m_generatedRule;
        m_originalRuleText = source.m_originalRuleText;
        m_wasEdited = source.m_wasEdited;
        m_severity = source.m_severity;
        m_ruleCondition = source.m_ruleCondition;
    }

    /**
     * Sanitize a rule name for use in S-expression output.
     * Replaces spaces and invalid characters with underscores.
     */
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
            result.insert( 0, "_" );

        return result;
    }

protected:
    static wxString quoteString( const wxString& aCondition )
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
        wxString formatted = wxString::FromCDouble( aValue, aPrecision );
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

        wxString condExpr = aContext.conditionExpression;

        if( !aContext.layerClause.IsEmpty() )
        {
            if( aContext.layerClause.StartsWith( wxS( "(condition " ) ) )
            {
                wxString expr = aContext.layerClause.AfterFirst( '"' ).BeforeLast( '"' );

                if( !condExpr.IsEmpty() )
                    condExpr = expr + wxS( " && " ) + condExpr;
                else
                    condExpr = expr;
            }
            else
            {
                rule << wxS( "\t" ) << aContext.layerClause << wxS( "\n" );
            }
        }

        for( const wxString& clause : aConstraintClauses )
        {
            if( clause.IsEmpty() )
                continue;

            rule << wxS( "\t" ) << clause << wxS( "\n" );
        }

        if( !condExpr.IsEmpty() )
            rule << wxS( "\t(condition \"" ) << quoteString( condExpr ) << wxS( "\")\n" );

        rule << wxS( ")" );

        return rule;
    }

private:
    std::vector<PCB_LAYER_ID> m_layers;
    wxString m_layerSource;     ///< Original layer text: "inner", "outer", or layer name
    wxString m_ruleCondition;
    wxString m_constraintCode;
    wxString m_generatedRule;
    wxString m_originalRuleText;
    bool     m_wasEdited = false;
    SEVERITY m_severity = RPT_SEVERITY_UNDEFINED;
};

#endif // DRC_RE_BASE_CONSTRAINT_DATA_H_
