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
#include <kiplatform/io.h>
#include <algorithm>
#include <wx/ffile.h>
#include <wx/filename.h>

#include "drc_re_rule_loader.h"
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

    load( aPath );
    m_modified = true;
}


void DRC_RULE_DOCUMENT::load( const wxString& aPath )
{
    m_entries.clear();
    m_trivia = DRC_RE_FILE_TRIVIA();
    m_rawText.clear();
    m_hasRawText = false;
    m_textCurrent = false;

    if( !wxFileName::FileExists( aPath ) )
        return;

    wxFFile  file( aPath, wxS( "rb" ) );
    wxString text;

    if( !file.IsOpened() || !file.ReadAll( &text, wxConvUTF8 ) )
        return;

    m_rawText = text;
    m_textCurrent = true;
    m_hasRawText = !parseInto( text );
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

        // Edited panels write the canonical name, and siblings of a split rule must keep matching it
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


std::vector<int> DRC_RULE_DOCUMENT::IndicesFor( DRC_RULE_EDITOR_CONSTRAINT_NAME aType ) const
{
    // A rule the graphical panels could not take apart is kept as text. It still belongs on
    // the page of every constraint it sets.
    wxString         code = DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( aType );
    std::vector<int> result;

    // Two sections of a page can offer the same constraint. Only one of them lists text rules,
    // otherwise the same rule shows up twice on that page.
    bool listsTextRules = DRC_RULE_EDITOR_UTILS::SectionForConstraintCode( code ) == aType;

    for( size_t i = 0; i < m_entries.size(); ++i )
    {
        const DRC_RE_LOADED_PANEL_ENTRY& entry = m_entries[i];

        if( !entry.constraintData )
            continue;

        if( entry.panelType == aType )
        {
            result.push_back( (int) i );
        }
        else if( entry.panelType == CUSTOM_RULE && aType != CUSTOM_RULE && listsTextRules )
        {
            auto custom = std::dynamic_pointer_cast<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>( entry.constraintData );

            if( ( custom && FindConstraintClause( custom->GetRuleText(), code ) != wxString::npos )
                || FindConstraintClause( entry.originalRuleText, code ) != wxString::npos )
            {
                result.push_back( (int) i );
            }
        }
    }

    return result;
}


int DRC_RULE_DOCUMENT::Add( const DRC_RE_LOADED_PANEL_ENTRY& aEntry )
{
    m_entries.push_back( aEntry );

    if( m_entries.back().ruleGroupId <= 0 )
        m_entries.back().ruleGroupId = m_nextGroupId++;

    // A rule still open in its first editor is not part of the document until it is applied.
    if( !aEntry.constraintData || !aEntry.constraintData->IsNew() )
        touch();

    return (int) m_entries.size() - 1;
}


void DRC_RULE_DOCUMENT::UpdateEntry( int aIndex, const DRC_RE_LOADED_PANEL_ENTRY& aEdited )
{
    if( aIndex < 0 || aIndex >= (int) m_entries.size() )
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
        }
    }

    touch();
}


void DRC_RULE_DOCUMENT::Remove( int aIndex )
{
    if( aIndex < 0 || aIndex >= (int) m_entries.size() )
        return;

    bool applied = !m_entries[aIndex].constraintData || !m_entries[aIndex].constraintData->IsNew();

    m_entries.erase( m_entries.begin() + aIndex );

    if( applied )
        touch();
}


int DRC_RULE_DOCUMENT::Duplicate( int aIndex )
{
    if( aIndex < 0 || aIndex >= (int) m_entries.size() || !m_entries[aIndex].constraintData )
        return -1;

    const DRC_RE_LOADED_PANEL_ENTRY& source = m_entries[aIndex];

    // A trailing number is dropped so copying a copy counts on rather than stacking suffixes.
    wxString baseName = source.ruleName;
    int      lastSpace = baseName.Find( ' ', true );

    if( lastSpace != wxNOT_FOUND )
    {
        long number;

        if( baseName.Mid( lastSpace + 1 ).ToLong( &number ) )
            baseName = baseName.Left( lastSpace );
    }

    wxString                                     name = UniqueRuleName( baseName );
    DRC_RE_BASE_CONSTRAINT_DATA                  base( NextId(), 0, name );
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data =
            DRC_RULE_EDITOR_UTILS::CreateConstraintData( source.panelType, base );

    data->CopyFrom( *source.constraintData );
    data->SetRuleName( name );
    data->SetIsNew( false );

    // The copy is written from its own fields, so it must not carry the text the original was
    // read from or the rule generated under the original's name.
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
    if( aDelta == 0 )
        return false;

    std::vector<int> order = IndicesFor( aType );
    auto             it = std::find( order.begin(), order.end(), aIndex );

    if( it == order.end() )
        return false;

    int position = (int) ( it - order.begin() ) + ( aDelta > 0 ? 1 : -1 );

    if( position < 0 || position >= (int) order.size() )
        return false;

    std::swap( m_entries[aIndex], m_entries[order[position]] );
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
    wxString prefix = wxString::Format( wxS( "(constraint %s" ), aCode );
    size_t   pos = aText.find( prefix );

    while( pos != wxString::npos )
    {
        size_t end = pos + prefix.length();

        if( end >= aText.length() || aText[end] == ' ' || aText[end] == ')' || aText[end] == '\n'
            || aText[end] == '\t' )
        {
            return pos;
        }

        pos = aText.find( prefix, end );
    }

    return wxString::npos;
}
