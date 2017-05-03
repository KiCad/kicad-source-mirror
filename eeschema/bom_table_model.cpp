/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include "bom_table_model.h"

// Indicator that multiple values exist in child rows
#define ROW_MULT_ITEMS wxString( "<...>" )

static const wxColor ROW_COLOUR_ITEM_CHANGED( 200, 0, 0 );
static const wxColor ROW_COLOUR_MULTIPLE_ITEMS( 60, 90, 200 );

/**
 * Convert BOM_TABLE_ROW -> wxDataViewItem
 */
static wxDataViewItem RowToItem( BOM_TABLE_ROW const* aRow )
{
    return wxDataViewItem( const_cast<void*>( static_cast<void const*>( aRow ) ) );
}

/**
 * Convert wxDataViewItem -> BOM_TABEL_ROW
 */
static BOM_TABLE_ROW const* ItemToRow( wxDataViewItem aItem )
{
    if( !aItem.IsOk() )
    {
        return nullptr;
    }
    else
    {
        return static_cast<BOM_TABLE_ROW const*>( aItem.GetID() );
    }
}

BOM_FIELD_VALUES::BOM_FIELD_VALUES( wxString aRefDes ) : m_refDes( aRefDes )
{
}

/**
 * Return the current value for the provided field ID
 */
bool BOM_FIELD_VALUES::GetFieldValue( unsigned int aFieldId, wxString& aValue ) const
{
    auto search = m_currentValues.find( aFieldId );

    if( search == m_currentValues.end() )
        return false;

    aValue = search->second;

    return true;
}

/**
 * Return the backup value for the provided field ID
 */
bool BOM_FIELD_VALUES::GetBackupValue( unsigned int aFieldId, wxString& aValue ) const
{
    auto search = m_backupValues.find( aFieldId );

    if( search == m_backupValues.end() )
        return false;

    aValue = search->second;

    return true;
}

/**
 * Set the value for the provided field ID
 * Field value is set under any of the following conditions:
 * - param aOverwrite is true
 * - There is no current value
 * - The current value is empty
 */
void BOM_FIELD_VALUES::SetFieldValue( unsigned int aFieldId, wxString aValue, bool aOverwrite )
{
    if( aOverwrite || m_currentValues.count( aFieldId ) == 0 || m_currentValues[aFieldId].IsEmpty() )
    {
        m_currentValues[aFieldId] = aValue;
    }
}

/**
 * Set the backup value for the provided field ID
 * If the backup value is already set, new value is ignored
 */
void BOM_FIELD_VALUES::SetBackupValue( unsigned int aFieldId, wxString aValue )
{
    if( m_backupValues.count( aFieldId ) == 0 || m_backupValues[aFieldId].IsEmpty() )
    {
        m_backupValues[aFieldId] = aValue;
    }
}

bool BOM_FIELD_VALUES::HasValueChanged( unsigned int aFieldId) const
{
    wxString currentValue, backupValue;

    GetFieldValue( aFieldId, currentValue );
    GetBackupValue( aFieldId, backupValue );

    return currentValue.Cmp( backupValue ) != 0;
}

void BOM_FIELD_VALUES::RevertChanges( unsigned int aFieldId )
{
    wxString backupValue;

    GetBackupValue( aFieldId, backupValue );

    SetFieldValue( aFieldId, backupValue, true );
}

BOM_TABLE_ROW::BOM_TABLE_ROW() : m_columnList( nullptr )
{
}

/**
 * Update cell attributes based on parameters of the cell
 * Default implementation highlights cells that have been altered
 */
bool BOM_TABLE_ROW::GetAttr( unsigned int aFieldId, wxDataViewItemAttr& aAttr ) const
{
    auto field = m_columnList->GetColumnById( aFieldId );

    if( HasValueChanged( field ) )
    {
        aAttr.SetBold( true );
        aAttr.SetItalic( true );
        aAttr.SetColour( ROW_COLOUR_ITEM_CHANGED );
        return true;
    }

    return false;
}

bool BOM_TABLE_ROW::HasChanged() const
{
    if( !m_columnList )
        return false;

    for( auto& column : m_columnList->Columns )
    {
        if( column && HasValueChanged( column ) )
        {
            return true;
        }
    }

    return false;
}

/**
 * Create a new group (which contains one or more components)
 */
BOM_TABLE_GROUP::BOM_TABLE_GROUP( BOM_COLUMN_LIST* aColumnList )
{
    m_columnList = aColumnList;
}

bool BOM_TABLE_GROUP::GetAttr( unsigned int aFieldId, wxDataViewItemAttr& aAttr ) const
{
    if( GetFieldValue( aFieldId ).Cmp( ROW_MULT_ITEMS ) == 0 )
    {
        aAttr.SetItalic( true );
        aAttr.SetColour( ROW_COLOUR_MULTIPLE_ITEMS );
        return true;
    }

    return BOM_TABLE_ROW::GetAttr( aFieldId, aAttr );
}

/**
 * Return the value associated with a given field in the group.
 * Some fields require special attention.
 */
wxString BOM_TABLE_GROUP::GetFieldValue( unsigned int aFieldId ) const
{
    wxString value;

    // Account for special cases
    switch( aFieldId )
    {
    // QUANTITY returns the size of the group
    case BOM_COL_ID_QUANTITY:
        value = wxString::Format( "%u", (unsigned int) GroupSize() );
        break;
    // REFERENCE field returns consolidated list of references
    case BOM_COL_ID_REFERENCE:
        value = wxJoin( GetReferences(), ' ' );
        break;
    // Otherwise, return component data
    default:
        if( Components.size() == 0 )
        {
            value = wxEmptyString;
        }
        else
        {
            // If the components in this group contain multiple items,
            // display a special string indicating this
            for( unsigned int i=0; i<Components.size(); i++ )
            {
                auto const& cmp = Components[i];

                if( i == 0 )
                {
                    value = cmp->GetFieldValue( aFieldId );
                }
                // Mismatch found
                else if( value.Cmp( cmp->GetFieldValue( aFieldId ) ) != 0 )
                {
                    value = ROW_MULT_ITEMS;
                    break;
                }
            }
        }
        break;
    }

    return value;
}

/**
 * Set the value of a field in a group
 * The new value is pushed to all components that are children of this group
 */
bool BOM_TABLE_GROUP::SetFieldValue( unsigned int aFieldId, const wxString aValue, bool aOverwrite )
{
    bool result = false;

    for( auto& cmp : Components )
    {
        result |= cmp->SetFieldValue( aFieldId, aValue, aOverwrite );
    }

    return result;
}

/**
 * Determines if a given component matches against a particular field.
 *
 * - Tests each field in turn; all fields must match
 * - Some fields require special checking
 *
 * @aFieldId - The ID of the field
 * @aCmp - The component being tested
 */
bool BOM_TABLE_GROUP::TestField( BOM_COLUMN* aField, BOM_TABLE_COMPONENT* aComponent ) const
{
    if( !aField || !aComponent )
        return false;

    if( Components.size() == 0 )
        return true;

    wxString componentValue;
    wxString comparisonValue;

    // Some fields are handled in a special manner
    // (handle these first)
    switch( aField->Id() )
    {
    // These fields should NOT be compared (return True)
    case BOM_COL_ID_QUANTITY:
        return true;
    // Reference matching is done only on prefix
    case BOM_COL_ID_REFERENCE:
        componentValue = aComponent->GetPrefix();
        comparisonValue = Components[0]->GetPrefix();
        break;
    default:
        componentValue = aComponent->GetFieldValue( aField->Id() );
        comparisonValue = Components[0]->GetFieldValue( aField->Id() );
        break;
    }

    bool result = componentValue.Cmp( comparisonValue ) == 0;

    return result;
}

/**
 * Add a new component to the group.
 * It is assumed at this stage that the component is a good match for the group.
 * @param aCmp is the new component to add
 */
bool BOM_TABLE_GROUP::AddComponent( BOM_TABLE_COMPONENT* aComponent )
{
    if( !aComponent )
        return false;

    // To be a match, all fields must match!
    bool match = true;

    for( auto* column : m_columnList->Columns )
    {
        match = TestField( column, aComponent );

        // Escape on first mismatch
        if( !match )
        {
            break;
        }
    }

    if( match )
    {
        aComponent->SetParent( this );
        Components.push_back( aComponent );
        return true;
    }
    else
    {
        return false;
    }

    return false;
}


/**
 * Adds each child row to the supplied list, and returns the total child count
 */
unsigned int BOM_TABLE_GROUP::GetChildren( wxDataViewItemArray& aChildren ) const
{
    // Show drop-down for child components
    for( auto& row : Components )
    {
        if( row )
        {
            aChildren.push_back( RowToItem( &*row ) );
        }
    }

    return aChildren.size();
}

/**
 * Test if any components in this group have a new value in the provided field
 * @param aField is the field to test
 * @return true if any children have changed else false
 */
bool BOM_TABLE_GROUP::HasValueChanged( BOM_COLUMN* aField ) const
{

    bool changed = false;

    if( !aField )
        return false;

    for( auto const& row : Components )
    {
        if( row->HasValueChanged( aField ) )
        {
            changed = true;
            break;
        }
    }

    // If the value has changed, the group field data must be updated
    if( changed )
    {
        //TODO
    }

    return changed;
}

/**
 * Return a list of (ordered) references
 * for all the components in this group
 *
 * @aSort - Sort the references
 */
wxArrayString BOM_TABLE_GROUP::GetReferences( bool aSort ) const
{
    wxArrayString refs;

    for( auto const& cmp : Components )
    {
        if( cmp )
        {
            refs.Add( cmp->GetFieldValue( BOM_COL_ID_REFERENCE ) );
        }
    }

    if( aSort )
    {
        refs.Sort( SortReferences );
    }

    return refs;
}

/**
 * Compare two references (e.g. "R100", "R19") and perform a 'natural' sort
 * This sorting must preserve numerical order rather than alphabetical
 * e.g. "R100" is lower (alphabetically) than "R19"
 * BUT should be placed after R19
 */
int BOM_TABLE_GROUP::SortReferences( const wxString& aFirst, const wxString& aSecond )
{
    // Default sorting
    int defaultSort = aFirst.Cmp( aSecond );

    static const wxString REGEX_STRING = "^([a-zA-Z]+)(\\d+)$";

    // Compile regex statically
    static wxRegEx regexFirst( REGEX_STRING, wxRE_ICASE | wxRE_ADVANCED );
    static wxRegEx regexSecond( REGEX_STRING, wxRE_ICASE | wxRE_ADVANCED );

    if( !regexFirst.Matches( aFirst ) || !regexSecond.Matches( aSecond ) )
    {
        return defaultSort;
    }

    // First priority is to order by prefix
    wxString prefixFirst  = regexFirst.GetMatch( aFirst, 1 );
    wxString prefixSecond = regexSecond.GetMatch( aSecond, 1 );

    if( prefixFirst.CmpNoCase( prefixSecond ) != 0 ) // Different prefixes!
    {
        return defaultSort;
    }

    wxString numStrFirst   = regexFirst.GetMatch( aFirst, 2 );
    wxString numStrSecond  = regexSecond.GetMatch( aSecond, 2 );

    // If either match failed, just return normal string comparison
    if( numStrFirst.IsEmpty() || numStrSecond.IsEmpty() )
    {
        return defaultSort;
    }

    // Convert each number string to an integer
    long numFirst    = 0;
    long numSecond   = 0;

    // If either conversion fails, return normal string comparison
    if( !numStrFirst.ToLong( &numFirst ) || !numStrSecond.ToLong( &numSecond ) )
    {
        return defaultSort;
    }

    return (int) (numFirst - numSecond);
}

/**
 * Compare two VALUE fields.
 * A value field can reasonably be expected to be one of:
 * a) Purely numerical e.g. '22'
 * b) Numerical with included units e.g. '15uF'
 * c) Numerical with included prefix but no units e.g. '20n'
 * d) Numerical with prefix inside number e.g. '4K7'
 * e) Other, e.g. 'MAX232'
 *
 * Cases a) to d) should be detected and converted to a common representation
 * Values that do not match this pattern should revert to standard string comparison
 */
int BOM_TABLE_GROUP::SortValues( const wxString& aFirst, const wxString& aSecond )
{
    //TODO - Intelligent comparison of component values
    // e.g. 4K > 499
    // e.g. 1nF < 0.1u

    // For now, just return default comparison

    return aFirst.CmpNoCase( aSecond );
}

/**
 * Create a new COMPONENT row
 * Each COMPONENT row is associated with a single component item.
 */
BOM_TABLE_COMPONENT::BOM_TABLE_COMPONENT( BOM_TABLE_GROUP* aParent,
                                          BOM_COLUMN_LIST* aColumnList,
                                          BOM_FIELD_VALUES* aFieldValues )
{
    m_parent = aParent;
    m_columnList = aColumnList;
    m_fieldValues = aFieldValues;
}

/**
 * Try to add a unit to this component
 * If the references match, it will be added
 */
bool BOM_TABLE_COMPONENT::AddUnit( SCH_REFERENCE aUnit )
{
    // Addition is successful if the references match or there are currently no units in the group
    if( Units.size() == 0  || Units[0].GetRef().Cmp( aUnit.GetRef() ) == 0 )
    {
        Units.push_back( aUnit );

        wxString value;

        // Extract the component data
        for( auto column : m_columnList->Columns )
        {
            auto cmp = aUnit.GetComp();

            switch( column->Id() )
            {
            case BOM_COL_ID_QUANTITY:
                value = wxEmptyString;
                break;
            case BOM_COL_ID_DESCRIPTION:
                value = cmp->GetAliasDescription();
                break;
            case BOM_COL_ID_DATASHEET:
                value = cmp->GetField( DATASHEET )->GetText();
                if( value.IsEmpty() )
                {
                    value = cmp->GetAliasDocumentation();
                }
                break;
            case BOM_COL_ID_REFERENCE:
                value = aUnit.GetRef();
                break;
            case BOM_COL_ID_VALUE:
                value = cmp->GetField( VALUE )->GetText();
                break;
            case BOM_COL_ID_FOOTPRINT:
                value = cmp->GetField( FOOTPRINT )->GetText();
                break;

            // User fields
            default:
                value = cmp->GetFieldText( column->Title() );
                break;
            }

            m_fieldValues->SetFieldValue( column->Id(), value );
            m_fieldValues->SetBackupValue( column->Id(), value );
        }

        return true;
    }

    return false;
}

/**
 * Return the value associated with a particular field
 * If no field is found, return an empty string
 */
wxString BOM_TABLE_COMPONENT::GetFieldValue( unsigned int aFieldId ) const
{
    wxString value;

    switch ( aFieldId )
    {
    case BOM_COL_ID_QUANTITY:
        return wxEmptyString;
    case BOM_COL_ID_REFERENCE:
        return GetReference();
    default:
        break;
    }

    if( m_fieldValues )
        m_fieldValues->GetFieldValue( aFieldId, value );

    return value;
}

/**
 * Set the value of a field in the component
 * @param aFieldId is the unique ID of the field to update
 * @param aValue is the new value
 * @param aOverwrite enforces writing even if a value exists
 */
bool BOM_TABLE_COMPONENT::SetFieldValue( unsigned int aFieldId, const wxString aValue, bool aOverwrite )
{
    if( m_fieldValues )
    {
        m_fieldValues->SetFieldValue( aFieldId, aValue, aOverwrite );
        return true;
    }

    return false;
}

/**
 * Return the prefix of a component e.g. "R23" -> "R"
 */
wxString BOM_TABLE_COMPONENT::GetPrefix() const
{
    if( Units.size() == 0 )
        return wxEmptyString;

    return Units[0].GetComp()->GetPrefix();
}

/**
 * Return the reference of a component e.g. "R23"
 */
wxString BOM_TABLE_COMPONENT::GetReference() const
{
    if( Units.size() == 0 )
        return wxEmptyString;

    return Units[0].GetRef();
}

/**
 * Determines if the given field has been changed for this component
 */
bool BOM_TABLE_COMPONENT::HasValueChanged( BOM_COLUMN* aField ) const
{
    if( !aField )
    {
        return false;
    }

    return m_fieldValues->HasValueChanged( aField->Id() );
}

/**
 * If any changes have been made to this component,
 * they are now applied to the schematic component
 */
void BOM_TABLE_COMPONENT::ApplyFieldChanges()
{
    for( auto& unit : Units )
    {
        auto cmp = unit.GetComp();

        if( !cmp )
            continue;

        // Iterate over each column
        SCH_FIELD* field;

        for( auto& column : m_columnList->Columns )
        {
            if( column && HasValueChanged( column ) )
            {
                wxString value = GetFieldValue( column->Id() );

                switch( column->Id() )
                {
                // Ignore read-only fields
                case BOM_COL_ID_REFERENCE:
                case BOM_COL_ID_QUANTITY:
                    continue;
                // Special field considerations
                case BOM_COL_ID_FOOTPRINT:
                    field = cmp->GetField( FOOTPRINT );
                    break;
                case BOM_COL_ID_VALUE:
                    field = cmp->GetField( VALUE );
                    break;
                case BOM_COL_ID_DATASHEET:
                    field = cmp->GetField( DATASHEET );
                    break;
                default:
                    field = cmp->FindField( column->Title() );
                    break;
                }

                // New field needs to be added?
                if( !field && !value.IsEmpty() )
                {
                    SCH_FIELD newField( wxPoint( 0, 0 ), -1, cmp, column->Title() );
                    field = cmp->AddField( newField );
                }

                if( field )
                {
                    field->SetText( value );
                }
            }
        }
    }
}

/**
 * Revert the displayed fields for this component
 * to their original values (matching the schematic data)
 */
void BOM_TABLE_COMPONENT::RevertFieldChanges()
{
    for( auto& column : m_columnList->Columns )
    {
        switch( column->Id() )
        {
        case BOM_COL_ID_REFERENCE:
        case BOM_COL_ID_QUANTITY:
            continue;
        default:
            break;
        }

        m_fieldValues->RevertChanges( column->Id() );
    }
}

BOM_TABLE_MODEL::BOM_TABLE_MODEL() :
        m_widget( nullptr ),
        m_sortingColumn( BOM_COL_ID_REFERENCE ),
        m_sortingOrder( true )
{
    //TODO
}

/**
 * Create a container for the BOM_TABLE_MODEL
 * This is required for reference counting by wxDataViewCtrl
 */
BOM_TABLE_MODEL::MODEL_PTR BOM_TABLE_MODEL::Create()
{
    auto model = new BOM_TABLE_MODEL();

    auto container = BOM_TABLE_MODEL::MODEL_PTR( model );

    return container;
}

BOM_TABLE_MODEL::~BOM_TABLE_MODEL()
{
   //TODO
}

wxDataViewColumn* BOM_TABLE_MODEL::AddColumn( BOM_COLUMN* aColumn, int aPosition )
{
    static const unsigned int flags = wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE;

    if( !m_widget || !aColumn || !aColumn->IsVisible() )
        return nullptr;

    wxDataViewCellMode editFlag = aColumn->IsReadOnly() ? wxDATAVIEW_CELL_INERT : wxDATAVIEW_CELL_EDITABLE;

    auto renderer = new wxDataViewTextRenderer( "string" , editFlag );

    auto column = new wxDataViewColumn( aColumn->Title(),
                                        renderer,
                                        aColumn->Id(),
                                        150, //TODO - variable default width?
                                        wxAlignment( wxALIGN_CENTER ),
                                        flags );

    // Work out where to insert the column
    std::set<unsigned int> columnsBefore;

    for( auto testCol : ColumnList.Columns )
    {
        if( testCol->Id() == aColumn->Id() )
        {
            break;
        }
        else
        {
            columnsBefore.insert( testCol->Id() );
        }
    }

    bool found = false;

    for( unsigned int ii=0; ii<m_widget->GetColumnCount(); ii++ )
    {
        auto col = m_widget->GetColumn( ii );

        if( !col )
            continue;

        // If the new column is already in the view, escape
        if( col->GetModelColumn() == aColumn->Id() )
        {
            return col;
        }

        // If we should insert the new column BEFORE this one
        if( columnsBefore.count( col->GetModelColumn() ) == 0 )
        {
            found = true;
            m_widget->InsertColumn( ii, column );
            break;
        }
    }

    if( !found )
        m_widget->AppendColumn( column );

    column->SetResizeable( true );

    return column;
}

/**
 * Gracefully remove the given column from the wxDataViewCtrl
 * Removing columns individually prevents bad redraw of entire table
 */
bool BOM_TABLE_MODEL::RemoveColumn( BOM_COLUMN* aColumn )
{
    if( !m_widget || !aColumn )
        return false;

    for( unsigned int ii=0; ii<m_widget->GetColumnCount(); ii++ )
    {
        auto col = m_widget->GetColumn( ii );

        if( col && col->GetModelColumn() == aColumn->Id() )
        {
            m_widget->DeleteColumn( col );
            return true;
        }
    }

    return false;
}

/**
 * Attach the MODEL to a particular VIEW
 * This function causes the view to be updated appropriately
 */
void BOM_TABLE_MODEL::AttachTo( wxDataViewCtrl* aView )
{
    if( !aView )
    {
        return;
    }

    m_widget = aView;
    aView->Freeze();
    Cleared();

    aView->AssociateModel( this );
    aView->ClearColumns();

    // Add all columns
    for( auto col : ColumnList.Columns )
    {
        AddColumn( col );
    }

    aView->Thaw();

    // Notify the view that the data needs to be redrawn
    aView->Update();
}

/**
 * Return the total number of components displayed by the model
 */
unsigned int BOM_TABLE_MODEL::ComponentCount() const
{
    unsigned int count = 0;

    for( auto& group : Groups )
    {
        if( group )
            count += group->GroupSize();
    }

    return count;
}

void BOM_TABLE_MODEL::ClearColumns()
{
    ColumnList.Clear();
}

/**
 * Add default columns to the table
 * These columns are ALWAYS available in the table
 * They are immutable - can be hidden by user but not removed
 */
void BOM_TABLE_MODEL::AddDefaultColumns()
{
    // Reference column is read-only
    ColumnList.AddColumn( new BOM_COLUMN(
                BOM_COL_ID_REFERENCE,
                BOM_COL_TYPE_GENERATED,
                BOM_COL_TITLE_REFERENCE,
                true, true ) );

    ColumnList.AddColumn( new BOM_COLUMN(
               BOM_COL_ID_VALUE,
               BOM_COL_TYPE_KICAD,
               BOM_COL_TITLE_VALUE,
               true, false ) );

    ColumnList.AddColumn( new BOM_COLUMN(
               BOM_COL_ID_FOOTPRINT,
               BOM_COL_TYPE_KICAD,
               BOM_COL_TITLE_FOOTPRINT,
               true, false ) );

    ColumnList.AddColumn( new BOM_COLUMN(
               BOM_COL_ID_DATASHEET,
               BOM_COL_TYPE_KICAD,
               BOM_COL_TITLE_DATASHEET,
               true, false ) );

    // Description comes from .dcm file and is read-only
    ColumnList.AddColumn( new BOM_COLUMN(
               BOM_COL_ID_DESCRIPTION,
               BOM_COL_TYPE_LIBRARY,
               BOM_COL_TITLE_DESCRIPTION,
               true, true ) );

    // Quantity column is read-only
    ColumnList.AddColumn( new BOM_COLUMN(
               BOM_COL_ID_QUANTITY,
               BOM_COL_TYPE_GENERATED,
               BOM_COL_TITLE_QUANTITY,
               true, true ) );
}

/**
 * Extract field data from all components
 * Compiles an inclusive list of all field names from all components
 */
void BOM_TABLE_MODEL::AddComponentFields( SCH_COMPONENT* aCmp )
{
    std::vector< SCH_FIELD* > fields;

    SCH_FIELD* field;
    wxString fieldName;

    if( nullptr == aCmp )
        return;

    // Extract custom columns from component
    fields.clear();
    aCmp->GetFields( fields, false );

    // Iterate over custom field datas
    for( unsigned int i=MANDATORY_FIELDS; i<fields.size(); i++ )
    {
        field = fields[i];

        if( nullptr == field ) continue;

        fieldName = field->GetName();

        auto existing = ColumnList.GetColumnByTitle( fieldName );

        // As columns are sorted by ID, we can allow user to
        // create a column with a "special" name
        if( existing && existing->Id() >= MANDATORY_FIELDS)
            continue;

        ColumnList.AddColumn( new BOM_COLUMN( ColumnList.NextFieldId(),
                                         BOM_COL_TYPE_USER,
                                         field->GetName(),
                                         true, false ) );
    }
}

/**
 * Add a list of component items to the BOM manager
 * Creates consolidated groups of components as required
 */
void BOM_TABLE_MODEL::SetComponents( SCH_REFERENCE_LIST aRefs )
{
    // Add default columns
    AddDefaultColumns();

    // Extract all component fields
    for( unsigned int ii=0; ii<aRefs.GetCount(); ii++ )
    {
        auto ref = aRefs.GetItem( ii );
        auto cmp = ref.GetComp();

        if( cmp )
        {
            AddComponentFields( cmp );
        }
    }

    // Group multi-unit components together
    m_components.clear();
    m_fieldValues.clear();

    for( unsigned int ii=0; ii<aRefs.GetCount(); ii++ )
    {
        auto ref = aRefs.GetItem( ii );

        bool found = false;

        for( auto& cmp : m_components )
        {
            if( cmp->AddUnit( ref ) )
            {
                found = true;
                break;
            }
        }

        if( !found )
        {
            // Find the field:value map associated with this component
            wxString refDes = ref.GetComp()->GetField( REFERENCE )->GetText();

            bool dataFound = false;

            BOM_FIELD_VALUES* values;

            for( auto& data : m_fieldValues )
            {
                // Look for a match based on RefDes
                if( data->GetReference().Cmp( refDes ) == 0 )
                {
                    dataFound = true;
                    values = &*data;
                }
            }

            if( !dataFound )
            {
                values = new BOM_FIELD_VALUES( refDes );
                m_fieldValues.push_back( std::unique_ptr<BOM_FIELD_VALUES>( values ) );
            }

            auto* newComponent = new BOM_TABLE_COMPONENT( nullptr, &ColumnList, values );
            newComponent->AddUnit( ref );

            m_components.push_back( std::unique_ptr<BOM_TABLE_COMPONENT>( newComponent ) );
        }
    }
}

/**
 *  Recalculate grouping of components and reload table
 **/
void BOM_TABLE_MODEL::ReloadTable()
{
    if( m_widget )
    {
        m_widget->Freeze();
    }

    // Alert the view that the model data has changed
    Cleared();

    Groups.clear();

    for( auto& cmp : m_components )
    {
        bool grouped = false;

        if( m_groupColumns )
        {
            for( auto& group : Groups )
            {
                if( group->AddComponent( &*cmp ) )
                {
                    grouped = true;
                    break;
                }
            }
        }

        // No suitable group was found for this component
        if( !grouped )
        {
            auto* newGroup = new BOM_TABLE_GROUP( &ColumnList );

            newGroup->AddComponent( &*cmp );

            Groups.push_back( std::unique_ptr<BOM_TABLE_GROUP>( newGroup ) );
        }
    }

    // Update the display
    if( m_widget )
    {
        //Cleared();
        m_widget->AssociateModel( this );
        m_widget->Thaw();
    }
}

/**
 * Return a string array of data from a given row
 */
wxArrayString BOM_TABLE_MODEL::GetRowData( unsigned int aRow, std::vector<BOM_COLUMN*> aColumns ) const
{
    wxArrayString row;

    wxString data;

    if( Groups.size() <= aRow )
        return row;

    auto const& group = Groups[aRow];

    if ( !group )
        return row;

    for( auto const col : aColumns )
    {
        if( !col )
        {
            row.Add( wxEmptyString );
            continue;
        }

        row.Add( group->GetFieldValue( col->Id() ) );
    }

    return row;
}

/**
 * Get the value of a particular item in the model
 */
void BOM_TABLE_MODEL::GetValue(
        wxVariant& aVariant,
        const wxDataViewItem& aItem,
        unsigned int aFieldId ) const
{
    auto row = ItemToRow( aItem );

    if( row )
    {
        aVariant = row->GetFieldValue( aFieldId );
    }
}

/**
 * Set the value of a particular item in the model
 */
bool BOM_TABLE_MODEL::SetValue(
        const wxVariant& aVariant,
        const wxDataViewItem& aItem,
        unsigned int aFieldId )
{
    if( !aItem.IsOk() || !m_widget )
    {
        return false;
    }

    // Extract the value to be set
    if( aVariant.GetType().Cmp( "string" ) == 0 )
    {
        wxString value = aVariant.GetString();

        bool result = false;

        wxDataViewItemArray selectedItems;
        m_widget->GetSelections( selectedItems );

        // Set the row value for all selected rows

        for( auto item : selectedItems )
        {
            auto selectedRow = static_cast<BOM_TABLE_ROW*>( item.GetID() );

            if( selectedRow )
            {
                result |= selectedRow->SetFieldValue( aFieldId, value, true );
            }
        }

        if( m_widget )
        {
            m_widget->Update();
        }

        return result;
    }

    // Default
    return false;
}

/**
 * Return the parent item for a given item in the model.
 * If no parent is found (or the item is invalid) return an invalid item.
 */
wxDataViewItem BOM_TABLE_MODEL::GetParent( const wxDataViewItem& aItem ) const
{
    auto row = ItemToRow( aItem );
    auto parent = row ? row->GetParent() : nullptr;

    if( parent )
    {
        return RowToItem( parent );
    }

    // Return an invalid item
    return wxDataViewItem();
}

/**
 * Returns true if the supplied item has children
 */
bool BOM_TABLE_MODEL::IsContainer( const wxDataViewItem& aItem ) const
{
    auto row = ItemToRow( aItem );

    if( row )
    {
        return row->HasChildren();
    }

    return true;
}

/**
 * Push all children of the supplied item into the list
 * If the supplied item is invalid, push all the top-level items
 */
unsigned int BOM_TABLE_MODEL::GetChildren(
        const wxDataViewItem& aItem,
        wxDataViewItemArray& aChildren ) const
{
    auto row = aItem.IsOk() ? ItemToRow( aItem ) : nullptr;

    // Valid row, return its children
    if( row )
    {
        return row->GetChildren( aChildren );
    }
    else
    {
        for( auto& group : Groups )
        {
            aChildren.Add( RowToItem( &*group ) );
        }

        return aChildren.size();
    }
}

bool BOM_TABLE_MODEL::GetAttr( const wxDataViewItem& aItem,
                           unsigned int aFieldId,
                           wxDataViewItemAttr& aAttr ) const
{
    auto row = aItem.IsOk() ? ItemToRow( aItem ) : nullptr;

    if( row )
    {
        return row->GetAttr( aFieldId, aAttr );
    }
    else
    {
        return false;
    }
}

/**
 * Custom comparison function for improved column sorting
 * Alphanumeric sorting is not sufficient for correct ordering of some fields
 * Some columns are sorted numerically, others with more complex rules.
 */
int BOM_TABLE_MODEL::Compare( const wxDataViewItem& aItem1,
                          const wxDataViewItem& aItem2,
                          unsigned int aColumnId,
                          bool aAscending ) const
{
    if( !aItem1.IsOk() || !aItem2.IsOk() )
        return 0;

    int result = 0;

    auto row1 = ItemToRow( aItem1 );
    auto row2 = ItemToRow( aItem2 );

    if( !row1 || !row2 )
        return 0;

    if( row1->GetParent() != row2->GetParent() )
        return 0;

    wxString strVal1 = row1->GetFieldValue( aColumnId );
    wxString strVal2 = row2->GetFieldValue( aColumnId );

    long numVal1;
    long numVal2;

    switch( aColumnId )
    {
    // Reference column sorted by reference val
    case BOM_COL_ID_REFERENCE:
        result = BOM_TABLE_GROUP::SortReferences( strVal1, strVal2 );
        break;
    case BOM_COL_ID_VALUE:
        result = BOM_TABLE_GROUP::SortValues( strVal1, strVal2 );
        break;
    // These columns are sorted numerically
    case BOM_COL_ID_QUANTITY:
        if( strVal1.ToLong( &numVal1 ) && strVal2.ToLong( &numVal2 ) )
        {
            result = numVal1 - numVal2;
        }
        else
        {
            result = strVal1.Cmp( strVal2 );
        }
        break;
    default:
        // Default comparison (no special case)
        result = strVal1.Cmp( strVal2 );
        break;
    }

    // If initial sorting failed, sort secondly by reference
    if( result == 0 && aColumnId != BOM_COL_ID_REFERENCE )
    {
        result = BOM_TABLE_GROUP::SortReferences(
                row1->GetFieldValue( BOM_COL_ID_REFERENCE ),
                row2->GetFieldValue( BOM_COL_ID_REFERENCE ) );
    }

    // If sorting still failed, sort thirdly by value
    if( result == 0 && aColumnId != BOM_COL_ID_VALUE )
    {
        result = BOM_TABLE_GROUP::SortValues(
                row1->GetFieldValue( BOM_COL_ID_VALUE ),
                row2->GetFieldValue( BOM_COL_ID_VALUE ) );
    }

    if( !aAscending )
    {
        result *= -1;
    }

    return result;
}

/**
 * Revert all component data back to the original values.
 * The table view is updated accordingly
 */
void BOM_TABLE_MODEL::RevertFieldChanges()
{
    for( auto& group : Groups )
    {
        if( !group )
            continue;

        bool changed = false;

        for( auto& component : group->Components )
        {
            if( !component )
                continue;

            if( component->HasChanged() )
            {
                component->RevertFieldChanges();
                ItemChanged( RowToItem( &*component ) );
                changed = true;
            }
        }

        // Update the group if any components changed
        if( changed )
        {
            ItemChanged( RowToItem( &*group ) );
        }
    }
}

/**
 * Apply all outstanding field changes.
 * This is performed only when the window is closed
 */
void BOM_TABLE_MODEL::ApplyFieldChanges()
{
    for( auto& group : Groups )
    {
        if( !group )
            continue;

        for( auto& component : group->Components )
        {
            if( !component )
                continue;

            if( component->HasChanged() )
            {
                component->ApplyFieldChanges();
            }
        }
    }
}

/**
 * Tests if any component values in the table have been altered
 */
bool BOM_TABLE_MODEL::HaveFieldsChanged() const
{
    for( auto const& group : Groups )
    {
        if( !group )
            continue;

        for( auto const& cmp : group->Components )
        {
            if( !cmp )
                continue;

            if( cmp->HasChanged() )
            {
                return true;
            }
        }
    }

    return false;
}

/**
 * Returns a list of only those components that have been changed
 */
std::vector<SCH_REFERENCE> BOM_TABLE_MODEL::GetChangedComponents()
{
    std::vector<SCH_REFERENCE> components;

    for( auto& group : Groups )
    {
        if( !group )
            continue;

        for( auto& component : group->Components )
        {
            if( !component )
                continue;

            if( component->HasChanged() )
            {
                for( auto& unit : component->Units )
                {
                    components.push_back( unit );
                }
            }
        }
    }

    return components;
}

/**
 * Returns a count of the components that have been changed
 */
unsigned int BOM_TABLE_MODEL::CountChangedComponents()
{
    unsigned int count = 0;

    for( auto& group : Groups )
    {
        if( !group )
            continue;

        for( auto& component : group->Components )
        {
            if( component && component->HasChanged() )
            {
                count++;
            }
        }
    }

    return count;
}
