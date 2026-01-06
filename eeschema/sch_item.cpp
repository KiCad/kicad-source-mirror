/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <eeschema_settings.h>
#include <eda_item.h>
#include <sch_connection.h>
#include <sch_group.h>
#include <sch_rule_area.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <connection_graph.h>
#include <netclass.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <font/kicad_font_name.h>


// Rendering fonts is expensive (particularly when using outline fonts).  At small effective
// sizes (ie: zoomed out) the visual differences between outline and/or stroke fonts and the
// bitmap font becomes immaterial, and there's often more to draw when zoomed out so the
// performance gain becomes more significant.
#define BITMAP_FONT_SIZE_THRESHOLD 3


static const std::vector<KICAD_T> labelTypes = { SCH_LABEL_LOCATE_ANY_T };


/* Constructor and destructor for SCH_ITEM */
/* They are not inline because this creates problems with gcc at linking time in debug mode */

SCH_ITEM::SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType, int aUnit, int aBodyStyle ) :
        EDA_ITEM( aParent, aType, true, false ),
        m_unit( aUnit ),
        m_bodyStyle( aBodyStyle ),
        m_private( false )
{
    m_layer              = LAYER_WIRE;   // It's only a default, in fact
    m_fieldsAutoplaced   = AUTOPLACE_NONE;
    m_connectivity_dirty = false;        // Item is unconnected until it is placed, so it's clean
}


SCH_ITEM::SCH_ITEM( const SCH_ITEM& aItem ) :
    EDA_ITEM( aItem )
{
    m_layer              = aItem.m_layer;
    m_unit               = aItem.m_unit;
    m_bodyStyle          = aItem.m_bodyStyle;
    m_private            = aItem.m_private;
    m_fieldsAutoplaced   = aItem.m_fieldsAutoplaced;
    m_connectivity_dirty = aItem.m_connectivity_dirty;
}


SCH_ITEM& SCH_ITEM::operator=( const SCH_ITEM& aItem )
{
    m_layer              = aItem.m_layer;
    m_unit               = aItem.m_unit;
    m_bodyStyle          = aItem.m_bodyStyle;
    m_private            = aItem.m_private;
    m_fieldsAutoplaced   = aItem.m_fieldsAutoplaced;
    m_connectivity_dirty = aItem.m_connectivity_dirty;

    return *this;
}


SCH_ITEM::~SCH_ITEM()
{
    for( const auto& it : m_connection_map )
        delete it.second;

    // Do not try to modify SCHEMATIC::ConnectionGraph()
    // if the schematic does not exist
    if( !SCHEMATIC::m_IsSchematicExists )
        return;

    SCHEMATIC* sch = Schematic();

    if( sch != nullptr )
        sch->ConnectionGraph()->RemoveItem( this );
}


bool SCH_ITEM::IsGroupableType() const
{
    switch( Type() )
    {
    case SCH_SYMBOL_T:
    case SCH_PIN_T:
    case SCH_SHAPE_T:
    case SCH_BITMAP_T:
    case SCH_FIELD_T:
    case SCH_TEXT_T:
    case SCH_TEXTBOX_T:
    case SCH_TABLE_T:
    case SCH_GROUP_T:
    case SCH_LINE_T:
    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_BUS_BUS_ENTRY_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_RULE_AREA_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_SHEET_PIN_T:
    case SCH_SHEET_T:
        return true;
    default:
        return false;
    }
}


SCH_ITEM* SCH_ITEM::Duplicate( bool addToParentGroup, SCH_COMMIT* aCommit, bool doClone ) const
{
    SCH_ITEM* newItem = (SCH_ITEM*) Clone();

    if( !doClone )
        const_cast<KIID&>( newItem->m_Uuid ) = KIID();

    newItem->ClearFlags( SELECTED | BRIGHTENED );

    newItem->RunOnChildren(
            []( SCH_ITEM* aChild )
            {
                aChild->ClearFlags( SELECTED | BRIGHTENED );
            },
            RECURSE_MODE::NO_RECURSE );

    if( addToParentGroup )
    {
        wxCHECK_MSG( aCommit, newItem, "Must supply a commit to update parent group" );

        if( EDA_GROUP* group = newItem->GetParentGroup() )
        {
            aCommit->Modify( group->AsEdaItem(), nullptr, RECURSE_MODE::NO_RECURSE );
            group->AddItem( newItem );
        }
    }

    return newItem;
}


wxString SCH_ITEM::GetUnitDisplayName( int aUnit, bool aLabel ) const
{
    if( aUnit == 0 )
        return aLabel ? _( "All units" ) : wxString( _HKI( "All units" ) );
    else if( const SYMBOL* symbol = GetParentSymbol() )
        return symbol->GetUnitDisplayName( aUnit, aLabel );

    return wxEmptyString;
}


wxString SCH_ITEM::GetBodyStyleDescription( int aBodyStyle, bool aLabel ) const
{
    if( aBodyStyle == 0 )
        return aLabel ? _( "All body styles" ) : wxString( _HKI( "All body styles" ) );
    else if( const SYMBOL* symbol = GetParentSymbol() )
        return symbol->GetBodyStyleDescription( aBodyStyle, aLabel );

    return wxEmptyString;
}

void SCH_ITEM::SetUnitString( const wxString& aUnit )
{
    if( aUnit == _HKI( "All units" ) )
    {
        m_unit = 0;
        return;
    }

    if( SYMBOL* symbol = GetParentSymbol() )
    {
        for( int ii = 1; ii <= symbol->GetUnitCount(); ii++ )
        {
            if( symbol->GetUnitDisplayName( ii, false ) == aUnit )
            {
                m_unit = ii;
                return;
            }
        }
    }
}


wxString SCH_ITEM::GetUnitString() const
{
    return GetUnitDisplayName( m_unit, false );
}

void SCH_ITEM::SetBodyStyleProp( const wxString& aBodyStyle )
{
    if( aBodyStyle == _HKI( "All body styles" ) )
    {
        m_bodyStyle = 0;
        return;
    }

    if( SYMBOL* symbol = GetParentSymbol() )
    {
        for( int bodyStyle : { BODY_STYLE::BASE, BODY_STYLE::DEMORGAN } )
        {
            if( symbol->GetBodyStyleDescription( bodyStyle, false ) == aBodyStyle )
            {
                m_bodyStyle = bodyStyle;
                return;
            }
        }
    }
}


wxString SCH_ITEM::GetBodyStyleProp() const
{
    return GetBodyStyleDescription( m_bodyStyle, false );
}


SCHEMATIC* SCH_ITEM::Schematic() const
{
    return static_cast<SCHEMATIC*>( findParent( SCHEMATIC_T ) );
}


const SYMBOL* SCH_ITEM::GetParentSymbol() const
{
    if( SYMBOL* sch_symbol = static_cast<SCH_SYMBOL*>( findParent( SCH_SYMBOL_T ) ) )
        return sch_symbol;

    if( SYMBOL* lib_symbol = static_cast<LIB_SYMBOL*>( findParent( LIB_SYMBOL_T ) ) )
        return lib_symbol;

    return nullptr;
}


SYMBOL* SCH_ITEM::GetParentSymbol()
{
    if( SYMBOL* sch_symbol = static_cast<SCH_SYMBOL*>( findParent( SCH_SYMBOL_T ) ) )
        return sch_symbol;

    if( SYMBOL* lib_symbol = static_cast<LIB_SYMBOL*>( findParent( LIB_SYMBOL_T ) ) )
        return lib_symbol;

    return nullptr;
}


bool SCH_ITEM::ResolveExcludedFromSim( const SCH_SHEET_PATH* aInstance,
                                       const wxString& aVariantName ) const
{
    if( GetExcludedFromSim( aInstance, aVariantName ) )
        return true;

    for( SCH_RULE_AREA* area : m_rule_areas_cache )
    {
        if( area->GetExcludedFromSim( aInstance, aVariantName ) )
            return true;
    }

    return false;
}


bool SCH_ITEM::ResolveExcludedFromBOM( const SCH_SHEET_PATH* aInstance,
                                       const wxString& aVariantName ) const
{
    if( GetExcludedFromBOM( aInstance, aVariantName ) )
        return true;

    for( SCH_RULE_AREA* area : m_rule_areas_cache )
    {
        if( area->GetExcludedFromBOM( aInstance, aVariantName ) )
            return true;
    }

    return false;
}


bool SCH_ITEM::ResolveExcludedFromBoard( const SCH_SHEET_PATH* aInstance,
                                         const wxString& aVariantName ) const
{
    if( GetExcludedFromBoard( aInstance, aVariantName ) )
        return true;

    for( SCH_RULE_AREA* area : m_rule_areas_cache )
    {
        if( area->GetExcludedFromBoard( aInstance, aVariantName ) )
            return true;
    }

    return false;
}


bool SCH_ITEM::ResolveExcludedFromPosFiles( const SCH_SHEET_PATH* aInstance,
                                            const wxString& aVariantName ) const
{
    if( GetExcludedFromPosFiles( aInstance, aVariantName ) )
        return true;

    for( SCH_RULE_AREA* area : m_rule_areas_cache )
    {
        if( area->GetExcludedFromPosFiles( aInstance, aVariantName ) )
            return true;
    }

    return false;
}


bool SCH_ITEM::ResolveDNP( const SCH_SHEET_PATH* aInstance, const wxString& aVariantName ) const
{
    if( GetDNP( aInstance, aVariantName ) )
        return true;

    for( SCH_RULE_AREA* area : m_rule_areas_cache )
    {
        if( area->GetDNP( aInstance, aVariantName ) )
            return true;
    }

    return false;
}


wxString SCH_ITEM::ResolveText( const wxString& aText, const SCH_SHEET_PATH* aPath, int aDepth ) const
{
    // Use local depth counter so each text element starts fresh
    int depth = 0;

    std::function<bool( wxString* )> libSymbolResolver =
            [&]( wxString* token ) -> bool
            {
                LIB_SYMBOL* symbol = static_cast<LIB_SYMBOL*>( m_parent );
                return symbol->ResolveTextVar( token, depth + 1 );
            };

    std::function<bool( wxString* )> symbolResolver =
            [&]( wxString* token ) -> bool
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( m_parent );
                return symbol->ResolveTextVar( aPath, token, depth + 1 );
            };

    std::function<bool( wxString* )> schematicResolver =
            [&]( wxString* token ) -> bool
            {
                if( !aPath )
                    return false;

                if( SCHEMATIC* schematic = Schematic() )
                    return schematic->ResolveTextVar( aPath, token, depth + 1 );

                return false;
            };

    std::function<bool( wxString* )> sheetResolver =
            [&]( wxString* token ) -> bool
            {
                if( !aPath )
                    return false;

                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( m_parent );

                SCHEMATIC*     schematic = Schematic();
                SCH_SHEET_PATH path = *aPath;
                path.push_back( sheet );

                bool retval = sheet->ResolveTextVar( &path, token, depth + 1 );

                if( schematic )
                    retval |= schematic->ResolveTextVar( &path, token, depth + 1 );

                return retval;
            };

    std::function<bool( wxString* )> labelResolver =
            [&]( wxString* token ) -> bool
            {
                if( !aPath )
                    return false;

                SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( m_parent );
                return label->ResolveTextVar( aPath, token, depth + 1 );
            };

    wxString variantName;

    if( SCHEMATIC* schematic = Schematic() )
        variantName = schematic->GetCurrentVariant();

    // Create a unified resolver that delegates to the appropriate resolver based on parent type
    std::function<bool( wxString* )> fieldResolver =
            [&]( wxString* token ) -> bool
            {
                bool resolved = false;

                if( m_parent && m_parent->Type() == LIB_SYMBOL_T )
                    resolved = libSymbolResolver( token );
                else if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
                    resolved = symbolResolver( token );
                else if( m_parent && m_parent->Type() == SCH_SHEET_T )
                    resolved = sheetResolver( token );
                else if( m_parent && m_parent->IsType( labelTypes ) )
                    resolved = labelResolver( token );
                else if( Schematic() )
                {
                    // Project-level and schematic-level variables
                    resolved = Schematic()->Project().TextVarResolver( token );
                    resolved |= schematicResolver( token );
                }

                return resolved;
            };

    return ResolveTextVars( aText, &fieldResolver, depth );
}


std::vector<int> SCH_ITEM::ViewGetLayers() const
{
    // Basic fallback
    return { LAYER_DEVICE, LAYER_DEVICE_BACKGROUND, LAYER_SELECTION_SHADOWS };
}


bool SCH_ITEM::IsConnected( const VECTOR2I& aPosition ) const
{
    if(( m_flags & STRUCT_DELETED ) || ( m_flags & SKIP_STRUCT ) )
        return false;

    return doIsConnected( aPosition );
}


SCH_CONNECTION* SCH_ITEM::Connection( const SCH_SHEET_PATH* aSheet ) const
{
    if( !IsConnectable() )
        return nullptr;

    if( !aSheet )
    {
        SCHEMATIC* sch = Schematic();

        if( !sch )
            return nullptr; // Item has been removed from schematic (e.g. SCH_PIN during symbol deletion)

        aSheet = &sch->CurrentSheet();
    }

    auto it = m_connection_map.find( *aSheet );

    if( it == m_connection_map.end() )
        return nullptr;
    else
        return it->second;
}


void SCH_ITEM::SetConnectionGraph( CONNECTION_GRAPH* aGraph )
{
    for( auto& [path, conn] : m_connection_map )
    {
        conn->SetGraph( aGraph );

        for( auto& member : conn->AllMembers() )
            member->SetGraph( aGraph );
    }
}


std::shared_ptr<NETCLASS> SCH_ITEM::GetEffectiveNetClass( const SCH_SHEET_PATH* aSheet ) const
{
    static std::shared_ptr<NETCLASS> nullNetclass = std::make_shared<NETCLASS>( wxEmptyString );

    SCHEMATIC* schematic = Schematic();

    if( schematic )
    {
        std::shared_ptr<NET_SETTINGS>& netSettings = schematic->Project().GetProjectFile().m_NetSettings;
        SCH_CONNECTION* connection = Connection( aSheet );

        if( connection )
            return netSettings->GetEffectiveNetClass( connection->Name() );
        else
            return netSettings->GetDefaultNetclass();
    }

    return nullNetclass;
}


void SCH_ITEM::ClearConnectedItems( const SCH_SHEET_PATH& aSheet )
{
    auto it = m_connected_items.find( aSheet );

    if( it != m_connected_items.end() )
        it->second.clear();
}


const SCH_ITEM_VEC& SCH_ITEM::ConnectedItems( const SCH_SHEET_PATH& aSheet )
{
    return m_connected_items[ aSheet ];
}


void SCH_ITEM::AddConnectionTo( const SCH_SHEET_PATH& aSheet, SCH_ITEM* aItem )
{
    SCH_ITEM_VEC& vec = m_connected_items[ aSheet ];

    // The vector elements are small, so reserve 1k at a time to prevent re-allocations
    if( vec.size() == vec.capacity() )
        vec.reserve( vec.size() + 4096 );

    // Add item to the correct place in the sorted vector if it is not already there
    auto it = std::lower_bound( vec.begin(), vec.end(), aItem );

    if( it == vec.end() || *it != aItem )
        vec.insert( it, aItem );
}


SCH_CONNECTION* SCH_ITEM::InitializeConnection( const SCH_SHEET_PATH& aSheet,
                                                CONNECTION_GRAPH* aGraph )
{
    SCH_CONNECTION* connection = Connection( &aSheet );

    // N.B. Do not clear the dirty connectivity flag here because we may need
    // to create a connection for a different sheet, and we don't want to
    // skip the connection creation because the flag is cleared.
    if( connection )
    {
        connection->Reset();
    }
    else
    {
        connection = new SCH_CONNECTION( this );
        m_connection_map.insert( std::make_pair( aSheet, connection ) );
    }

    connection->SetGraph( aGraph );
    connection->SetSheet( aSheet );
    return connection;
}


SCH_CONNECTION* SCH_ITEM::GetOrInitConnection( const SCH_SHEET_PATH& aSheet,
                                               CONNECTION_GRAPH* aGraph )
{
    if( !IsConnectable() )
        return nullptr;

    SCH_CONNECTION* connection = Connection( &aSheet );

    if( connection )
        return connection;
    else
        return InitializeConnection( aSheet, aGraph );
}


const wxString& SCH_ITEM::GetCachedDriverName() const
{
    static wxString s_empty;
    return s_empty;
}


void SCH_ITEM::swapData( SCH_ITEM* aItem )
{
    UNIMPLEMENTED_FOR( GetClass() );
}


void SCH_ITEM::SwapItemData( SCH_ITEM* aImage )
{
    if( aImage == nullptr )
        return;

    EDA_ITEM* parent = GetParent();

    SwapFlags( aImage );
    std::swap( m_layer, aImage->m_layer );
    std::swap( m_unit, aImage->m_unit );
    std::swap( m_bodyStyle, aImage->m_bodyStyle );
    std::swap( m_private, aImage->m_private );
    std::swap( m_fieldsAutoplaced, aImage->m_fieldsAutoplaced );
    std::swap( m_group, aImage->m_group );
    swapData( aImage );

    SetParent( parent );
}


void SCH_ITEM::SwapFlags( SCH_ITEM* aItem )
{
    EDA_ITEM_FLAGS editFlags = GetEditFlags();
    EDA_ITEM_FLAGS tempFlags = GetTempFlags();
    EDA_ITEM_FLAGS aItem_editFlags = aItem->GetEditFlags();
    EDA_ITEM_FLAGS aItem_tempFlags = aItem->GetTempFlags();

    std::swap( m_flags, aItem->m_flags );

    ClearEditFlags();
    SetFlags( editFlags );
    ClearTempFlags();
    SetFlags( tempFlags );

    aItem->ClearEditFlags();
    aItem->SetFlags( aItem_editFlags );
    aItem->ClearTempFlags();
    aItem->SetFlags( aItem_tempFlags );
}


void SCH_ITEM::ClearCaches()
{
    auto clearTextCaches =
            []( SCH_ITEM* aItem )
            {
                EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

                if( text )
                {
                    text->ClearBoundingBoxCache();
                    text->ClearRenderCache();
                }
            };

    clearTextCaches( this );

    RunOnChildren( clearTextCaches, RECURSE_MODE::NO_RECURSE );
}


bool SCH_ITEM::operator==( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return false;

    return compare( aOther, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) == 0;
}


bool SCH_ITEM::operator<( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return Type() < aOther.Type();

    return ( compare( aOther ) < 0 );
}


bool SCH_ITEM::cmp_items::operator()( const SCH_ITEM* aFirst, const SCH_ITEM* aSecond ) const
{
    return aFirst->compare( *aSecond, COMPARE_FLAGS::EQUALITY ) < 0;
}


int SCH_ITEM::compare( const SCH_ITEM& aOther, int aCompareFlags ) const
{
    if( Type() != aOther.Type() )
        return Type() - aOther.Type();

    if( !( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::UNIT ) && m_unit != aOther.m_unit )
        return m_unit - aOther.m_unit;

    if( !( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::UNIT ) && m_bodyStyle != aOther.m_bodyStyle )
        return m_bodyStyle - aOther.m_bodyStyle;

    if( IsPrivate() != aOther.IsPrivate() )
        return IsPrivate() ? 1 : -1;

    if( !( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::SKIP_TST_POS ) )
    {
        if( GetPosition().x != aOther.GetPosition().x )
            return GetPosition().x - aOther.GetPosition().x;

        if( GetPosition().y != aOther.GetPosition().y )
            return GetPosition().y - aOther.GetPosition().y;
    }

    if( ( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::EQUALITY )
        || ( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::ERC ) )
    {
        return 0;
    }

    if( m_Uuid < aOther.m_Uuid )
        return -1;

    if( m_Uuid > aOther.m_Uuid )
        return 1;

    return 0;
}


int SCH_ITEM::GetMaxError() const
{
    if( SCHEMATIC* schematic = Schematic() )
        return schematic->Settings().m_MaxError;
    else
        return schIUScale.mmToIU( ARC_LOW_DEF_MM );
}


const wxString& SCH_ITEM::GetDefaultFont( const RENDER_SETTINGS* aSettings ) const
{
    static wxString defaultName = KICAD_FONT_NAME;

    if( aSettings )
        return aSettings->GetDefaultFont();
    else if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        return cfg->m_Appearance.default_font;
    else
        return defaultName;
}


const KIFONT::METRICS& SCH_ITEM::GetFontMetrics() const
{
    if( SCHEMATIC* schematic = Schematic() )
        return schematic->Settings().m_FontMetrics;

    return KIFONT::METRICS::Default();
}


int SCH_ITEM::GetEffectivePenWidth( const SCH_RENDER_SETTINGS* aSettings ) const
{
    // For historical reasons, a stored value of 0 means "default width" and negative
    // numbers meant "don't stroke".

    if( GetPenWidth() < 0 )
    {
        return 0;
    }
    else if( GetPenWidth() == 0 )
    {
        if( GetParent() && GetParent()->Type() == LIB_SYMBOL_T )
            return std::max( aSettings->m_SymbolLineWidth, aSettings->GetMinPenWidth() );
        else
            return std::max( aSettings->GetDefaultPenWidth(), aSettings->GetMinPenWidth() );
    }
    else
    {
        return std::max( GetPenWidth(), aSettings->GetMinPenWidth() );
    }
}


bool SCH_ITEM::RenderAsBitmap( double aWorldScale ) const
{
    if( HasHypertext() )
        return false;

    if( const EDA_TEXT* text = dynamic_cast<const EDA_TEXT*>( this ) )
        return text->GetTextHeight() * aWorldScale < BITMAP_FONT_SIZE_THRESHOLD;

    return false;
}


void SCH_ITEM::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    if( SYMBOL* symbol = GetParentSymbol() )
    {
        if( symbol->IsMultiUnit() )
            aList.emplace_back( _( "Unit" ), GetUnitDisplayName( GetUnit(), false ) );

        if( symbol->IsMultiBodyStyle() )
            aList.emplace_back( _( "Body Style" ), GetBodyStyleDescription( GetBodyStyle(), true ) );

        if( dynamic_cast<LIB_SYMBOL*>( symbol ) && IsPrivate() )
            aList.emplace_back( _( "Private" ), wxEmptyString );
    }
}


const std::vector<wxString>* SCH_ITEM::GetEmbeddedFonts()
{
    if( SCHEMATIC* schematic = Schematic() )
        return schematic->GetEmbeddedFiles()->GetFontFiles();

    if( SYMBOL* symbol = GetParentSymbol() )
    {
        if( EMBEDDED_FILES* symbolEmbeddedFiles = symbol->GetEmbeddedFiles() )
            return symbolEmbeddedFiles->UpdateFontFiles();
    }

    return nullptr;
}


static struct SCH_ITEM_DESC
{
    SCH_ITEM_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_ITEM );
        propMgr.InheritsAfter( TYPE_HASH( SCH_ITEM ), TYPE_HASH( EDA_ITEM ) );

#ifdef NOTYET
        // Not yet functional in UI
        propMgr.AddProperty( new PROPERTY<SCH_ITEM, bool>( _HKI( "Locked" ),
                &SCH_ITEM::SetLocked, &SCH_ITEM::IsLocked ) );
#endif

        auto multiUnit =
                [=]( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( aItem ) )
                    {
                        if( const SYMBOL* symbol = schItem->GetParentSymbol() )
                            return symbol->IsMultiUnit();
                    }

                    return false;
                };

        auto multiBodyStyle =
                [=]( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( aItem ) )
                    {
                        if( const SYMBOL* symbol = schItem->GetParentSymbol() )
                            return symbol->IsMultiBodyStyle();
                    }

                    return false;
                };

        propMgr.AddProperty( new PROPERTY<SCH_ITEM, wxString>( _HKI( "Unit" ),
                    &SCH_ITEM::SetUnitString, &SCH_ITEM::GetUnitString ) )
                .SetAvailableFunc( multiUnit )
                .SetIsHiddenFromDesignEditors()
                .SetChoicesFunc( []( INSPECTABLE* aItem )
                                 {
                                     wxPGChoices choices;
                                     choices.Add( _HKI( "All units" ), 0 );

                                     if( SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aItem ) )
                                     {
                                         if( SYMBOL* symbol = item->GetParentSymbol() )
                                         {
                                             for( int ii = 1; ii <= symbol->GetUnitCount(); ii++ )
                                                 choices.Add( symbol->GetUnitDisplayName( ii, false ), ii );
                                         }
                                     }

                                     return choices;
                                 } );


        propMgr.AddProperty( new PROPERTY<SCH_ITEM, wxString>( _HKI( "Body Style" ),
                    &SCH_ITEM::SetBodyStyleProp, &SCH_ITEM::GetBodyStyleProp ) )
                .SetAvailableFunc( multiBodyStyle )
                .SetIsHiddenFromDesignEditors()
                .SetChoicesFunc( []( INSPECTABLE* aItem )
                                 {
                                     wxPGChoices choices;
                                     choices.Add( _HKI( "All body styles" ) );

                                     if( SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aItem ) )
                                     {
                                         if( SYMBOL* symbol = item->GetParentSymbol() )
                                         {
                                             for( int ii : { BODY_STYLE::BASE, BODY_STYLE::DEMORGAN } )
                                                 choices.Add( symbol->GetBodyStyleDescription( ii, false ) );
                                         }
                                     }

                                     return choices;
                                  } );

        propMgr.AddProperty( new PROPERTY<SCH_ITEM, bool>( _HKI( "Private" ),
                    &SCH_ITEM::SetPrivate, &SCH_ITEM::IsPrivate ) )
                .SetIsHiddenFromDesignEditors();
    }
} _SCH_ITEM_DESC;

IMPLEMENT_ENUM_TO_WXANY( SCH_LAYER_ID )


static bool lessYX( const DANGLING_END_ITEM& a, const DANGLING_END_ITEM& b )
{
    const auto aPos = a.GetPosition();
    const auto bPos = b.GetPosition();
    return aPos.y < bPos.y ? true : ( aPos.y > bPos.y ? false : aPos.x < bPos.x );
};


static bool lessType( const DANGLING_END_ITEM& a, const DANGLING_END_ITEM& b )
{
    return a.GetType() < b.GetType();
};


std::vector<DANGLING_END_ITEM>::iterator
DANGLING_END_ITEM_HELPER::get_lower_pos( std::vector<DANGLING_END_ITEM>& aItemListByPos,
                                         const VECTOR2I&                 aPos )
{
    DANGLING_END_ITEM needle = DANGLING_END_ITEM( PIN_END, nullptr, aPos );
    auto              start = aItemListByPos.begin();
    auto              end = aItemListByPos.end();
    return std::lower_bound( start, end, needle, lessYX );
}


std::vector<DANGLING_END_ITEM>::iterator
DANGLING_END_ITEM_HELPER::get_lower_type( std::vector<DANGLING_END_ITEM>& aItemListByType,
                                          const DANGLING_END_T&           aType )
{
    DANGLING_END_ITEM needle = DANGLING_END_ITEM( aType, nullptr, VECTOR2I{} );
    auto              start = aItemListByType.begin();
    auto              end = aItemListByType.end();
    return std::lower_bound( start, end, needle, lessType );
}


void DANGLING_END_ITEM_HELPER::sort_dangling_end_items(
        std::vector<DANGLING_END_ITEM>& aItemListByType,
        std::vector<DANGLING_END_ITEM>& aItemListByPos )
{
    // WIRE_END pairs must be kept together. Hence stable sort.
    std::stable_sort( aItemListByType.begin(), aItemListByType.end(), lessType );

    // Sort by y first, pins are more likely to share x than y.
    std::sort( aItemListByPos.begin(), aItemListByPos.end(), lessYX );
}
