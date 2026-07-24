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

#include <tools/match_properties.h>

#include <board.h>
#include <eda_item.h>
#include <footprint.h>
#include <i18n_utility.h>
#include <pad.h>
#include <pcb_barcode.h>
#include <pcb_dimension.h>
#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <properties/wx_any_utils.h>
#include <zone.h>

#include <map>


struct FAMILY
{
    /// One item type carrying every property the family advertises.
    TYPE_ID  m_Type;
    wxString m_Label;
};


/// Keyed by the names Family() returns.  Every family there must appear here.
static const std::map<wxString, FAMILY>& families()
{
    // clang-format off
    static const std::map<wxString, FAMILY> families = {
        { wxS( "route" ),           { TYPE_HASH( PCB_TRACK ),           _HKI( "Tracks and Arcs" ) } },
        { wxS( "via" ),             { TYPE_HASH( PCB_VIA ),             _HKI( "Vias" ) } },
        { wxS( "pad" ),             { TYPE_HASH( PAD ),                 _HKI( "Pads" ) } },
        { wxS( "shape" ),           { TYPE_HASH( PCB_SHAPE ),           _HKI( "Graphics" ) } },
        { wxS( "text" ),            { TYPE_HASH( PCB_TEXT ),            _HKI( "Text" ) } },
        { wxS( "textbox" ),         { TYPE_HASH( PCB_TEXTBOX ),         _HKI( "Text Boxes" ) } },
        { wxS( "dimension" ),       { TYPE_HASH( PCB_DIM_ALIGNED ),     _HKI( "Dimensions" ) } },
        { wxS( "zone" ),            { TYPE_HASH( ZONE ),                _HKI( "Zones" ) } },
        { wxS( "rule_area" ),       { TYPE_HASH( ZONE ),                _HKI( "Rule Areas" ) } },
        { wxS( "footprint" ),       { TYPE_HASH( FOOTPRINT ),           _HKI( "Footprints" ) } },
        { wxS( "table" ),           { TYPE_HASH( PCB_TABLE ),           _HKI( "Tables" ) } },
        { wxS( "table_cell" ),      { TYPE_HASH( PCB_TABLECELL ),       _HKI( "Table Cells" ) } },
        { wxS( "target" ),          { TYPE_HASH( PCB_TARGET ),          _HKI( "Targets" ) } },
        { wxS( "reference_image" ), { TYPE_HASH( PCB_REFERENCE_IMAGE ), _HKI( "Reference Images" ) } },
        { wxS( "barcode" ),         { TYPE_HASH( PCB_BARCODE ),         _HKI( "Barcodes" ) } }
    };
    // clang-format on

    return families;
}


/// A property that means the same thing in more than one family.  Names cannot be matched
/// because a track's "Width" is a stroke weight and a text's is a glyph width.
struct COMMON_PROPERTY
{
    wxString                     m_Label;
    std::map<wxString, wxString> m_Names;   ///< family -> the name it is registered under there
};


/// The prefix common keys carry in the enabled set, in place of a family.
static const wxString COMMON_FAMILY = wxS( "common" );


/// Keyed by the name the settings use after the prefix.  A family that does not offer the
/// property is dropped when the catalog is built, so listing one here is only a claim.
static const std::map<wxString, COMMON_PROPERTY>& commonProperties()
{
    // clang-format off
    static const std::map<wxString, COMMON_PROPERTY> common = {
        { wxS( "Layer" ), { _HKI( "Layer" ), {
                { wxS( "barcode" ),         wxS( "Layer" ) },
                { wxS( "dimension" ),       wxS( "Layer" ) },
                { wxS( "shape" ),           wxS( "Layer" ) },
                { wxS( "table" ),           wxS( "Layer" ) },
                { wxS( "target" ),          wxS( "Layer" ) },
                { wxS( "text" ),            wxS( "Layer" ) },
                { wxS( "textbox" ),         wxS( "Layer" ) },
                { wxS( "reference_image" ), wxS( "Associated Layer" ) } } } },

        { wxS( "Line Width" ), { _HKI( "Line Width" ), {
                { wxS( "route" ),  wxS( "Width" ) },
                { wxS( "shape" ),  wxS( "Line Width" ) },
                { wxS( "table" ),  wxS( "Border Width" ) },
                { wxS( "target" ), wxS( "Width" ) } } } },

        { wxS( "Soldermask" ), { _HKI( "Soldermask" ), {
                { wxS( "route" ), wxS( "Soldermask" ) },
                { wxS( "shape" ), wxS( "Soldermask" ) } } } },

        { wxS( "Soldermask Margin Override" ), { _HKI( "Soldermask Margin Override" ), {
                { wxS( "pad" ),   wxS( "Soldermask Margin Override" ) },
                { wxS( "route" ), wxS( "Soldermask Margin Override" ) },
                { wxS( "shape" ), wxS( "Soldermask Margin Override" ) } } } },

        // Fill is registered on text boxes and table cells but its availability func refuses
        // both, so claiming it here would pair kinds over something that can never apply.

        { wxS( "Font" ), { _HKI( "Font" ), {
                { wxS( "dimension" ),  wxS( "Font" ) },
                { wxS( "table_cell" ), wxS( "Font" ) },
                { wxS( "text" ),       wxS( "Font" ) },
                { wxS( "textbox" ),    wxS( "Font" ) } } } },

        { wxS( "Text Width" ), { _HKI( "Text Width" ), {
                { wxS( "dimension" ),  wxS( "Width" ) },
                { wxS( "table_cell" ), wxS( "Width" ) },
                { wxS( "text" ),       wxS( "Width" ) },
                { wxS( "textbox" ),    wxS( "Width" ) } } } },

        { wxS( "Text Height" ), { _HKI( "Text Height" ), {
                { wxS( "dimension" ),  wxS( "Height" ) },
                { wxS( "table_cell" ), wxS( "Height" ) },
                { wxS( "text" ),       wxS( "Height" ) },
                { wxS( "textbox" ),    wxS( "Height" ) } } } },

        { wxS( "Text Thickness" ), { _HKI( "Text Thickness" ), {
                { wxS( "dimension" ),  wxS( "Thickness" ) },
                { wxS( "table_cell" ), wxS( "Thickness" ) },
                { wxS( "text" ),       wxS( "Thickness" ) },
                { wxS( "textbox" ),    wxS( "Thickness" ) } } } },

        { wxS( "Bold" ), { _HKI( "Bold" ), {
                { wxS( "dimension" ),  wxS( "Bold" ) },
                { wxS( "table_cell" ), wxS( "Bold" ) },
                { wxS( "text" ),       wxS( "Bold" ) },
                { wxS( "textbox" ),    wxS( "Bold" ) } } } },

        { wxS( "Italic" ), { _HKI( "Italic" ), {
                { wxS( "dimension" ),  wxS( "Italic" ) },
                { wxS( "table_cell" ), wxS( "Italic" ) },
                { wxS( "text" ),       wxS( "Italic" ) },
                { wxS( "textbox" ),    wxS( "Italic" ) } } } },

        { wxS( "Horizontal Justification" ), { _HKI( "Horizontal Justification" ), {
                { wxS( "dimension" ),  wxS( "Horizontal Justification" ) },
                { wxS( "table_cell" ), wxS( "Horizontal Justification" ) },
                { wxS( "text" ),       wxS( "Horizontal Justification" ) },
                { wxS( "textbox" ),    wxS( "Horizontal Justification" ) } } } },

        { wxS( "Vertical Justification" ), { _HKI( "Vertical Justification" ), {
                { wxS( "dimension" ),  wxS( "Vertical Justification" ) },
                { wxS( "table_cell" ), wxS( "Vertical Justification" ) },
                { wxS( "text" ),       wxS( "Vertical Justification" ) },
                { wxS( "textbox" ),    wxS( "Vertical Justification" ) } } } },

        { wxS( "Knockout" ), { _HKI( "Knockout" ), {
                { wxS( "barcode" ),    wxS( "Knockout" ) },
                { wxS( "dimension" ),  wxS( "Knockout" ) },
                { wxS( "table_cell" ), wxS( "Knockout" ) },
                { wxS( "text" ),       wxS( "Knockout" ) },
                { wxS( "textbox" ),    wxS( "Knockout" ) } } } }
    };
    // clang-format on

    return common;
}


/// The families a common property actually reaches, once the unregistered ones are dropped.
static const std::map<wxString, std::map<wxString, wxString>>& commonReach();


/// One value staged for the target.  Optional entries are dropped if they will not go.
struct PENDING_PROPERTY
{
    PROPERTY_BASE* m_Property;
    wxAny          m_Value;
    bool           m_Optional;
};


/// Empty if the item has no family.  Never matches a key.
static wxString familyPrefix( const EDA_ITEM& aItem )
{
    const wxString family = MATCH_PROPERTIES_CATALOG::Family( aItem );

    return family.IsEmpty() ? wxString() : family + wxS( "/" );
}


wxString MATCH_PROPERTIES_CATALOG::Family( const EDA_ITEM& aItem )
{
    switch( aItem.Type() )
    {
    case PCB_TRACE_T:
    case PCB_ARC_T: return wxS( "route" );
    case PCB_VIA_T: return wxS( "via" );
    case PCB_PAD_T: return wxS( "pad" );
    case PCB_SHAPE_T: return wxS( "shape" );
    case PCB_TEXT_T:
    case PCB_FIELD_T: return wxS( "text" );
    case PCB_TEXTBOX_T: return wxS( "textbox" );
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_LEADER_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T: return wxS( "dimension" );
    case PCB_FOOTPRINT_T: return wxS( "footprint" );
    case PCB_TABLE_T: return wxS( "table" );
    case PCB_TABLECELL_T: return wxS( "table_cell" );
    case PCB_TARGET_T: return wxS( "target" );
    case PCB_REFERENCE_IMAGE_T: return wxS( "reference_image" );
    case PCB_BARCODE_T: return wxS( "barcode" );
    case PCB_ZONE_T: return static_cast<const ZONE&>( aItem ).GetIsRuleArea() ? wxS( "rule_area" ) : wxS( "zone" );
    default: return wxEmptyString;
    }
}


wxString MATCH_PROPERTIES_CATALOG::FamilyLabel( const wxString& aFamily )
{
    if( aFamily == COMMON_FAMILY )
        return _( "Common to Several Kinds" );

    auto entry = families().find( aFamily );

    return entry == families().end() ? aFamily : wxGetTranslation( entry->second.m_Label );
}


std::set<wxString> MATCH_PROPERTIES_CATALOG::FamiliesFor( const wxString& aKey )
{
    const wxString family = aKey.BeforeFirst( '/' );

    if( family != COMMON_FAMILY )
        return { family };

    auto entry = commonReach().find( aKey.AfterFirst( '/' ) );

    if( entry == commonReach().end() )
        return {};

    std::set<wxString> families;

    for( const auto& [name, property] : entry->second )
        families.insert( name );

    return families;
}


bool MATCH_PROPERTIES_CATALOG::PropertyIsRegistered( const wxString& aFamily, const wxString& aName )
{
    auto entry = families().find( aFamily );

    if( entry == families().end() )
        return false;

    return PROPERTY_MANAGER::Instance().GetProperty( entry->second.m_Type, aName ) != nullptr;
}


wxString MATCH_PROPERTIES_CATALOG::PropertyLabel( const wxString& aKey )
{
    if( aKey.BeforeFirst( '/' ) == COMMON_FAMILY )
    {
        auto entry = commonProperties().find( aKey.AfterFirst( '/' ) );

        if( entry != commonProperties().end() )
            return wxGetTranslation( entry->second.m_Label );
    }

    // wxGetTranslation() returns its own argument for a missing msgid, so the name it is given
    // has to outlive the call.
    const wxString name = aKey.AfterFirst( '/' );

    return wxGetTranslation( name );
}


wxString MATCH_PROPERTIES_CATALOG::DisplayLabel( const wxString& aKey )
{
    return wxString::Format( wxS( "%s — %s" ), FamilyLabel( aKey.BeforeFirst( '/' ) ), PropertyLabel( aKey ) );
}


bool MATCH_PROPERTIES_CATALOG::Compatible( const EDA_ITEM& aSource, const EDA_ITEM& aTarget )
{
    const wxString source = Family( aSource );
    const wxString target = Family( aTarget );

    if( source.IsEmpty() || target.IsEmpty() )
        return false;

    if( source == target )
        return true;

    // Different kinds still meet over anything they both understand.
    for( const auto& [key, families] : commonReach() )
    {
        if( families.contains( source ) && families.contains( target ) )
            return true;
    }

    return false;
}


std::vector<EDA_ITEM*> MATCH_PROPERTIES_CATALOG::CompatibleTargets( const EDA_ITEM&               aSource,
                                                                    const std::vector<EDA_ITEM*>& aCandidates )
{
    std::vector<EDA_ITEM*> targets;

    for( EDA_ITEM* candidate : aCandidates )
    {
        if( candidate && candidate != &aSource && Compatible( aSource, *candidate ) )
            targets.push_back( candidate );
    }

    return targets;
}


bool MATCH_PROPERTIES_CATALOG::AnyEnabledFor( const EDA_ITEM& aItem, const std::set<wxString>& aEnabledKeys )
{
    const wxString prefix = familyPrefix( aItem );

    if( prefix.IsEmpty() )
        return false;

    auto it = aEnabledKeys.lower_bound( prefix );

    if( it != aEnabledKeys.end() && it->StartsWith( prefix ) )
        return true;

    const wxString family = Family( aItem );

    for( const auto& [key, families] : commonReach() )
    {
        if( families.contains( family ) && aEnabledKeys.contains( COMMON_FAMILY + wxS( "/" ) + key ) )
            return true;
    }

    return false;
}


/// Target properties to write, paired with values read off the source.
static std::vector<PENDING_PROPERTY> plan( const EDA_ITEM& aSource, const EDA_ITEM& aTarget,
                                           const std::set<wxString>& aEnabledKeys )
{
    std::vector<PENDING_PROPERTY> pending;
    const wxString                prefix = familyPrefix( aSource );
    const wxString                sourceFamily = MATCH_PROPERTIES_CATALOG::Family( aSource );
    const wxString                targetFamily = MATCH_PROPERTIES_CATALOG::Family( aTarget );
    const bool                    acrossKinds = sourceFamily != targetFamily;
    PROPERTY_MANAGER&             mgr = PROPERTY_MANAGER::Instance();
    EDA_ITEM*                     source = const_cast<EDA_ITEM*>( &aSource );
    const TYPE_ID                 sourceType = TYPE_HASH( aSource );
    const TYPE_ID                 targetType = TYPE_HASH( aTarget );

    auto stage =
            [&]( const wxString& aSourceName, const wxString& aTargetName, bool aOptional )
            {
                PROPERTY_BASE* sourceProperty = mgr.GetProperty( sourceType, aSourceName );
                PROPERTY_BASE* targetProperty = mgr.GetProperty( targetType, aTargetName );

                if( !sourceProperty || !targetProperty )
                    return;

                // The enabled set is persisted and hand-editable.  It can name a stale property.
                if( !sourceProperty->IsCopyable() || sourceProperty->IsHiddenFromPropertiesManager()
                    || !mgr.IsAvailableFor( sourceType, sourceProperty, source )
                    || !mgr.IsWriteableFor( sourceType, sourceProperty, source ) )
                {
                    return;
                }

                pending.push_back( { targetProperty, aSource.Get( sourceProperty ), aOptional } );
            };

    // Enabled keys are the short list.  Walk their prefix range, not every property of the type.
    // GetProperty() is a linear scan, so one lookup per property is quadratic.
    if( !acrossKinds )
    {
        for( auto it = aEnabledKeys.lower_bound( prefix ); it != aEnabledKeys.end() && it->StartsWith( prefix );
             ++it )
        {
            const wxString name = it->Mid( prefix.length() );

            stage( name, name, false );
        }
    }

    // A common property is registered under a different name in each family, and crossing kinds
    // it may simply not fit.  Those are dropped rather than reported.
    for( const auto& [key, families] : commonReach() )
    {
        auto sourceName = families.find( sourceFamily );
        auto targetName = families.find( targetFamily );

        if( sourceName == families.end() || targetName == families.end() )
            continue;

        if( !aEnabledKeys.contains( COMMON_FAMILY + wxS( "/" ) + key ) )
            continue;

        stage( sourceName->second, targetName->second, acrossKinds );
    }

    return pending;
}


MATCH_PROPERTIES_RESULT MATCH_PROPERTIES_CATALOG::Copy( const EDA_ITEM& aSource, EDA_ITEM& aTarget,
                                                        const std::set<wxString>& aEnabledKeys )
{
    MATCH_PROPERTIES_RESULT result;

    if( !Compatible( aSource, aTarget ) )
    {
        result.m_Error = _( "The source and target have no properties in common." );
        return result;
    }

    PROPERTY_MANAGER&             mgr = PROPERTY_MANAGER::Instance();
    const TYPE_ID                 targetType = TYPE_HASH( aTarget );
    std::unique_ptr<EDA_ITEM>     staged( aTarget.Clone() );
    std::vector<PENDING_PROPERTY> pending = plan( aSource, aTarget, aEnabledKeys );
    std::vector<std::pair<PROPERTY_BASE*, bool>> stagedProperties;

    // Setting one property can make another writeable.  Sweep until nothing moves.  What is
    // still pending does not apply to this target.  Drop it.
    for( bool madeProgress = true; madeProgress; )
    {
        madeProgress = false;

        for( auto it = pending.begin(); it != pending.end(); )
        {
            PROPERTY_BASE* targetProperty = it->m_Property;

            if( !mgr.IsAvailableFor( targetType, targetProperty, staged.get() )
                || !mgr.IsWriteableFor( targetType, targetProperty, staged.get() ) )
            {
                ++it;
                continue;
            }

            if( !staged->Set( targetProperty, it->m_Value, false ) )
            {
                // Between two kinds of item the same idea can still be held differently.  That
                // is a property this target does not take, not a failure of the copy.
                if( it->m_Optional )
                {
                    it = pending.erase( it );
                    madeProgress = true;
                    continue;
                }

                result.m_Error = wxString::Format( _( "Could not apply property '%s' to the target item." ),
                                                   wxGetTranslation( targetProperty->Name() ) );
                return result;
            }

            stagedProperties.push_back( { targetProperty, it->m_Optional } );
            it = pending.erase( it );
            madeProgress = true;
        }
    }

    // A later property can turn a staged one read-only again.  A validator means nothing until
    // the item is finished.  Re-check both before touching the real item.
    std::vector<std::pair<PROPERTY_BASE*, wxAny>> apply;

    for( const auto& [targetProperty, optional] : stagedProperties )
    {
        if( !mgr.IsAvailableFor( targetType, targetProperty, staged.get() )
            || !mgr.IsWriteableFor( targetType, targetProperty, staged.get() ) )
        {
            continue;
        }

        wxAny stagedValue = staged->Get( targetProperty );

        if( targetProperty->Validate( wxAny( stagedValue ), staged.get() ) )
        {
            // Across kinds a value that does not suit the target is nothing to report, the
            // same as one that would not stage.
            if( optional )
                continue;

            result.m_Error = wxString::Format( _( "The value for property '%s' is not valid for the target item." ),
                                               wxGetTranslation( targetProperty->Name() ) );
            return result;
        }

        apply.emplace_back( targetProperty, std::move( stagedValue ) );
    }

    for( auto& [targetProperty, value] : apply )
    {
        if( !KiWxAnyEquals( aTarget.Get( targetProperty ), value, targetProperty )
            && aTarget.Set( targetProperty, value ) )
        {
            ++result.m_Changed;
        }
    }

    return result;
}


const std::set<wxString>& MATCH_PROPERTIES_CATALOG::AllSafeKeys()
{
    // PROPERTY_MANAGER registrations are static.  The answer cannot change after the first ask.
    static const std::set<wxString> keys = []
    {
        std::set<wxString> result;
        PROPERTY_MANAGER&  mgr = PROPERTY_MANAGER::Instance();

        // Zones and rule areas are one item type, told apart by a flag.  Ask a rule area which
        // properties it offers.
        BOARD board;
        ZONE  ruleArea( &board );
        ruleArea.SetIsRuleArea( true );

        for( const auto& [family, entry] : families() )
        {
            const TYPE_ID type = entry.m_Type;
            const bool    ruleAreaFamily = family == wxS( "rule_area" );

            for( PROPERTY_BASE* property : mgr.GetProperties( type ) )
            {
                if( !property->IsCopyable() || property->IsHiddenFromPropertiesManager() )
                    continue;

                if( type == TYPE_HASH( ZONE )
                    && mgr.IsAvailableFor( type, property, &ruleArea ) != ruleAreaFamily )
                {
                    continue;
                }

                result.insert( family + wxS( "/" ) + property->Name() );
            }
        }

        // A common property stands in for the per-family entries it covers, so the same idea is
        // not offered twice under different names.
        for( const auto& [key, common] : commonProperties() )
        {
            std::vector<wxString> covered;

            for( const auto& [family, name] : common.m_Names )
            {
                const wxString familyKey = family + wxS( "/" ) + name;

                if( result.contains( familyKey ) )
                    covered.push_back( familyKey );
            }

            // One family is not something in common; leave it where it was.
            if( covered.size() < 2 )
                continue;

            for( const wxString& familyKey : covered )
                result.erase( familyKey );

            result.insert( COMMON_FAMILY + wxS( "/" ) + key );
        }

        return result;
    }();

    return keys;
}


const std::map<wxString, std::map<wxString, wxString>>& commonReach()
{
    static const std::map<wxString, std::map<wxString, wxString>> reach = []
    {
        std::map<wxString, std::map<wxString, wxString>> result;

        // AllSafeKeys() decided which families a common key really covers.  Rebuild that same
        // decision here so the two cannot drift.
        for( const auto& [key, common] : commonProperties() )
        {
            if( !MATCH_PROPERTIES_CATALOG::AllSafeKeys().contains( COMMON_FAMILY + wxS( "/" ) + key ) )
                continue;

            for( const auto& [family, name] : common.m_Names )
            {
                if( MATCH_PROPERTIES_CATALOG::PropertyIsRegistered( family, name ) )
                    result[key][family] = name;
            }
        }

        return result;
    }();

    return reach;
}


const std::set<wxString>& MATCH_PROPERTIES_CATALOG::DefaultKeys()
{
    static const std::set<wxString> keys = {
        wxS( "common/Layer" ),      wxS( "common/Line Width" ),    wxS( "shape/Fill" ),
        wxS( "common/Soldermask" ), wxS( "common/Soldermask Margin Override" ),
        wxS( "common/Font" ),       wxS( "common/Text Width" ),    wxS( "common/Text Height" ),
        wxS( "common/Text Thickness" ), wxS( "common/Bold" ),      wxS( "common/Italic" ),
        wxS( "shape/Line Style" ),  wxS( "via/Diameter" ),         wxS( "via/Hole" ),
        wxS( "zone/Fill Mode" ),    wxS( "table/Border Style" )
    };

    return keys;
}
