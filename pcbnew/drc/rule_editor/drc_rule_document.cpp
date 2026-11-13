/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "drc_rule_document.h"

#include <board.h>
#include <drc/drc_rule_parser.h>
#include <kiplatform/io.h>
#include <string_utils.h>
#include <algorithm>
#include <wx/convauto.h>
#include <wx/ffile.h>
#include <wx/filename.h>

#include "drc_re_rule_loader.h"
#include "drc_re_panel_matcher.h"
#include "drc_re_rule_saver.h"
#include "drc_re_custom_rule_constraint_data.h"
#include "drc_rule_editor_utils.h"


DRC_RULE_DOCUMENT::DRC_RULE_DOCUMENT( BOARD* aBoard ) :
        m_board( aBoard ),
        m_hasRawText( false ),
        m_textCurrent( false ),
        m_modified( false ),
        m_nextId( 1 ),
        m_nextGroupId( 1 )
{
}


void DRC_RULE_DOCUMENT::Load()
{
    load( m_board->GetDesignRulesPath() );
    m_modified = false;
}


void DRC_RULE_DOCUMENT::LoadFrom( const wxString& aPath )
{
    if( !wxFileName::FileExists( aPath ) )
        return;

    if( load( aPath ) )
        m_modified = true;
}


bool DRC_RULE_DOCUMENT::load( const wxString& aPath )
{
    bool     exists = wxFileName::FileExists( aPath );
    wxString text;

    if( exists )
    {
        wxFFile file( aPath, wxS( "rb" ) );

        // wxConvAuto falls back to Latin-1, so a file that is not UTF-8 still reads
        if( !file.IsOpened() || !file.ReadAll( &text, wxConvAuto() ) || ( file.Length() > 0 && text.IsEmpty() ) )
            return false;

        text.Replace( wxS( "\r\n" ), wxS( "\n" ) );
    }

    m_entries.clear();
    m_trivia = DRC_RE_FILE_TRIVIA();
    m_rawText.clear();
    m_hasRawText = false;
    m_textCurrent = false;

    if( !exists )
        return true;

    // A rules file written outside KiCad can carry typographic quotes that the parser rejects.
    ConvertSmartQuotesAndDashes( &text );

    m_rawText = text;
    m_textCurrent = true;
    m_hasRawText = !parseInto( text );
    return true;
}


bool DRC_RULE_DOCUMENT::parseInto( const wxString& aText )
{
    DRC_RULE_LOADER loader;
    bool            parsed = false;

    std::vector<DRC_RE_LOADED_PANEL_ENTRY> entries = loader.LoadFromString( aText, &parsed );

    if( !parsed )
        return false;

    m_entries = std::move( entries );
    m_trivia = loader.GetFileTrivia();
    prepareEntries();

    return true;
}


void DRC_RULE_DOCUMENT::prepareEntries()
{
    for( DRC_RE_LOADED_PANEL_ENTRY& entry : m_entries )
    {
        m_nextGroupId = std::max( m_nextGroupId, entry.ruleGroupId + 1 );

        if( !entry.constraintData )
            continue;

        entry.constraintData->SetId( m_nextId++ );

        wxString layerSource = entry.layerSource;

        // An edited panel writes the canonical name, and siblings of a split rule must match it
        if( m_board && entry.layerCondition.count() == 1 )
        {
            PCB_LAYER_ID layer = entry.layerCondition.Seq().front();

            if( layerSource == m_board->GetLayerName( layer ) )
                layerSource = LSET::Name( layer );
        }

        entry.constraintData->SetLayerSource( layerSource );

        if( !entry.layerSource.IsEmpty() )
            entry.constraintData->SetLayers( entry.layerCondition.Seq() );

        entry.constraintData->SetSeverity( entry.severity );
    }
}


bool DRC_RULE_DOCUMENT::Save( wxString* aError )
{
    wxString    text = GetText();
    std::string utf8 = std::string( text.mb_str( wxConvUTF8 ) );

    if( !KIPLATFORM::IO::AtomicWriteFile( m_board->GetDesignRulesPath(), utf8.data(), utf8.size(), aError ) )
        return false;

    m_modified = false;
    return true;
}


wxString DRC_RULE_DOCUMENT::GetText() const
{
    if( m_textCurrent )
        return m_rawText;

    DRC_RULE_SAVER saver;
    return saver.GenerateRulesText( m_entries, m_board, m_trivia );
}


void DRC_RULE_DOCUMENT::SetText( const wxString& aText )
{
    if( aText == GetText() )
        return;

    m_rawText = aText;
    m_textCurrent = true;
    m_hasRawText = !parseInto( aText );
    m_modified = true;
}


std::set<DRC_CONSTRAINT_T> DRC_RULE_DOCUMENT::ConstraintsOf( const DRC_RE_LOADED_PANEL_ENTRY& aEntry )
{
    std::set<DRC_CONSTRAINT_T> result;

    if( !aEntry.constraintData )
        return result;

    // The condition cannot change which constraints are set, and leaving it out keeps the parse from failing on it
    RULE_GENERATION_CONTEXT ctx;
    ctx.ruleName = wxS( "rule" );
    ctx.constraintCode = aEntry.constraintData->GetConstraintCode();

    wxString                               ruleText = aEntry.constraintData->GenerateRule( ctx );
    std::vector<std::shared_ptr<DRC_RULE>> rules;

    try
    {
        DRC_RULES_PARSER parser( wxS( "(version 2)\n" ) + ruleText, wxS( "Rule Document" ) );
        parser.Parse( rules, nullptr );
    }
    catch( const IO_ERROR& )
    {
        return result;
    }

    for( const std::shared_ptr<DRC_RULE>& rule : rules )
    {
        for( const DRC_CONSTRAINT& constraint : rule->m_Constraints )
            result.insert( constraint.m_Type );
    }

    return result;
}


std::vector<int> DRC_RULE_DOCUMENT::IndicesFor( DRC_RULE_EDITOR_CONSTRAINT_NAME aType ) const
{
    // A rule no graphical panel could take apart is kept as text. It still belongs on the
    // page of every constraint it sets.
    static DRC_PANEL_MATCHER matcher;
    std::vector<int>         result;

    // The entries are a previous parse, so they no longer describe the text
    if( m_hasRawText )
        return result;

    for( size_t i = 0; i < m_entries.size(); ++i )
    {
        const DRC_RE_LOADED_PANEL_ENTRY& entry = m_entries[i];

        if( !entry.constraintData )
            continue;

        if( entry.panelType == aType )
        {
            result.push_back( (int) i );
        }
        else if( entry.panelType == CUSTOM_RULE && aType != CUSTOM_RULE )
        {
            for( DRC_CONSTRAINT_T constraint : ConstraintsOf( entry ) )
            {
                if( matcher.GetHomePanel( constraint ) == aType )
                {
                    result.push_back( (int) i );
                    break;
                }
            }
        }
    }

    return result;
}


std::vector<int> DRC_RULE_DOCUMENT::IndicesSetting( DRC_CONSTRAINT_T aConstraint ) const
{
    std::vector<int> result;

    if( m_hasRawText )
        return result;

    for( size_t i = 0; i < m_entries.size(); ++i )
    {
        if( ConstraintsOf( m_entries[i] ).count( aConstraint ) )
            result.push_back( (int) i );
    }

    return result;
}


int DRC_RULE_DOCUMENT::Add( const DRC_RE_LOADED_PANEL_ENTRY& aEntry )
{
    // Regenerating from stale entries would replace the text the user is still fixing
    if( m_hasRawText )
        return -1;

    m_entries.push_back( aEntry );

    if( m_entries.back().ruleGroupId <= 0 )
        m_entries.back().ruleGroupId = m_nextGroupId++;

    touch();

    return (int) m_entries.size() - 1;
}


void DRC_RULE_DOCUMENT::UpdateEntry( int aIndex, const DRC_RE_LOADED_PANEL_ENTRY& aEdited )
{
    if( m_hasRawText || aIndex < 0 || aIndex >= (int) m_entries.size() )
        return;

    m_entries[aIndex] = aEdited;
    m_entries[aIndex].wasEdited = true;

    const DRC_RE_LOADED_PANEL_ENTRY& edited = m_entries[aIndex];

    for( size_t i = 0; i < m_entries.size(); ++i )
    {
        DRC_RE_LOADED_PANEL_ENTRY& sibling = m_entries[i];

        if( (int) i == aIndex || sibling.ruleGroupId <= 0 || sibling.ruleGroupId != edited.ruleGroupId )
            continue;

        sibling.ruleName = edited.ruleName;
        sibling.condition = edited.condition;
        sibling.layerCondition = edited.layerCondition;
        sibling.layerSource = edited.layerSource;
        sibling.severity = edited.severity;
        sibling.wasEdited = true;

        if( sibling.constraintData )
        {
            sibling.constraintData->SetRuleName( edited.ruleName );
            sibling.constraintData->SetRuleCondition( edited.condition );
            sibling.constraintData->SetLayers( edited.layerCondition.Seq() );
            sibling.constraintData->SetLayerSource( edited.layerSource );
            sibling.constraintData->SetSeverity( edited.severity );

            if( edited.constraintData )
                sibling.constraintData->SetComment( edited.constraintData->GetComment() );

            sibling.constraintData->SetGeneratedRule( wxEmptyString );
        }
    }

    touch();
}


void DRC_RULE_DOCUMENT::Remove( int aIndex )
{
    if( m_hasRawText || aIndex < 0 || aIndex >= (int) m_entries.size() )
        return;

    m_entries.erase( m_entries.begin() + aIndex );
    touch();
}


int DRC_RULE_DOCUMENT::Duplicate( int aIndex )
{
    if( m_hasRawText || aIndex < 0 || aIndex >= (int) m_entries.size() || !m_entries[aIndex].constraintData )
        return -1;

    const DRC_RE_LOADED_PANEL_ENTRY& source = m_entries[aIndex];

    // Drop a trailing number so copying a copy counts on instead of stacking suffixes.
    wxString baseName = source.ruleName;
    int      lastSpace = baseName.Find( ' ', true );

    if( lastSpace != wxNOT_FOUND )
    {
        long number;

        if( baseName.Mid( lastSpace + 1 ).ToLong( &number ) )
            baseName = baseName.Left( lastSpace );
    }

    wxString                                     name = UniqueRuleName( baseName );
    DRC_RE_BASE_CONSTRAINT_DATA                  base( NextId(), name );
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data =
            DRC_RULE_EDITOR_UTILS::CreateConstraintData( source.panelType, base );

    data->CopyFrom( *source.constraintData );
    data->SetRuleName( name );
    data->SetIsNew( false );

    // The copy is written from its own fields, so drop the text the original was read from.
    data->SetGeneratedRule( wxEmptyString );

    DRC_RE_LOADED_PANEL_ENTRY copy( source.panelType, data, name, source.condition, source.severity,
                                    source.layerCondition );

    copy.layerSource = source.layerSource;
    copy.wasEdited = true;
    copy.ruleGroupId = m_nextGroupId++;

    m_entries.insert( m_entries.begin() + aIndex + 1, copy );
    touch();

    return aIndex + 1;
}


bool DRC_RULE_DOCUMENT::Move( int aIndex, int aDelta, DRC_RULE_EDITOR_CONSTRAINT_NAME aType )
{
    if( m_hasRawText || aDelta == 0 )
        return false;

    std::vector<int> order = IndicesFor( aType );
    auto             it = std::find( order.begin(), order.end(), aIndex );

    if( it == order.end() )
        return false;

    int position = (int) ( it - order.begin() ) + ( aDelta > 0 ? 1 : -1 );

    if( position < 0 || position >= (int) order.size() )
        return false;

    int movedGroup = m_entries[aIndex].ruleGroupId;
    int otherGroup = m_entries[order[position]].ruleGroupId;

    if( movedGroup <= 0 || otherGroup <= 0 || movedGroup == otherGroup )
    {
        std::swap( m_entries[aIndex], m_entries[order[position]] );
        touch();
        return true;
    }

    // The saver writes a rule where its first entry sits, so whole rules swap to keep the file in step
    std::vector<int> groups;

    for( const DRC_RE_LOADED_PANEL_ENTRY& entry : m_entries )
    {
        if( std::find( groups.begin(), groups.end(), entry.ruleGroupId ) == groups.end() )
            groups.push_back( entry.ruleGroupId );
    }

    std::iter_swap( std::find( groups.begin(), groups.end(), movedGroup ),
                    std::find( groups.begin(), groups.end(), otherGroup ) );

    std::vector<DRC_RE_LOADED_PANEL_ENTRY> reordered;
    reordered.reserve( m_entries.size() );

    for( int group : groups )
    {
        for( DRC_RE_LOADED_PANEL_ENTRY& entry : m_entries )
        {
            if( entry.ruleGroupId == group )
                reordered.push_back( std::move( entry ) );
        }
    }

    m_entries = std::move( reordered );
    touch();
    return true;
}


bool DRC_RULE_DOCUMENT::IsRuleNameUnique( const wxString& aName, DRC_RULE_EDITOR_CONSTRAINT_NAME aType,
                                          int aExceptIndex ) const
{
    if( aExceptIndex >= 0 && aExceptIndex < (int) m_entries.size() )
        aType = m_entries[aExceptIndex].panelType;

    for( size_t i = 0; i < m_entries.size(); ++i )
    {
        if( (int) i == aExceptIndex )
            continue;

        if( m_entries[i].panelType == aType && m_entries[i].ruleName == aName )
            return false;
    }

    return true;
}


wxString DRC_RULE_DOCUMENT::UniqueRuleName( const wxString& aBase ) const
{
    auto exists = [this]( const wxString& aName )
    {
        for( const DRC_RE_LOADED_PANEL_ENTRY& entry : m_entries )
        {
            if( entry.ruleName == aName )
                return true;
        }

        return false;
    };

    int      n = 1;
    wxString name = wxString::Format( wxS( "%s %d" ), aBase, n );

    while( exists( name ) )
        name = wxString::Format( wxS( "%s %d" ), aBase, ++n );

    return name;
}


void DRC_RULE_DOCUMENT::touch()
{
    m_modified = true;
    m_textCurrent = false;
}


size_t DRC_RULE_DOCUMENT::FindConstraintClause( const wxString& aText, const wxString& aCode )
{
    const wxString keyword = wxS( "(constraint" );
    size_t         pos = aText.find( keyword );

    auto isSpace = []( wxUniChar aChar )
    {
        return aChar == ' ' || aChar == '\t' || aChar == '\n' || aChar == '\r';
    };

    while( pos != wxString::npos )
    {
        size_t code = pos + keyword.length();

        while( code < aText.length() && isSpace( aText[code] ) )
            code++;

        size_t end = code + aCode.length();

        if( code > pos + keyword.length() && aText.compare( code, aCode.length(), aCode ) == 0
            && ( end >= aText.length() || isSpace( aText[end] ) || aText[end] == ')' ) )
        {
            return pos;
        }

        pos = aText.find( keyword, pos + keyword.length() );
    }

    return wxString::npos;
}
