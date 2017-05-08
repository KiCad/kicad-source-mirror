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

#ifndef _EESCHEMA_BOM_TABLE_MODEL_H_
#define _EESCHEMA_BOM_TABLE_MODEL_H_

#include <wx/dataview.h>
#include <wx/regex.h>

#include <vector>
#include <map>

#include <sch_component.h>
#include <sch_reference_list.h>
#include <class_netlist_object.h>
#include <template_fieldnames.h>

#include "bom_table_column.h"

// Forward-declare classes
class BOM_TABLE_ROW;        // Base-class for table row data model
class BOM_TABLE_GROUP;      // Class for displaying a group of components
class BOM_TABLE_COMPONENT;  // Class for displaying a single component

// Map column IDs to field values (for quick lookup)
typedef std::map<unsigned int, wxString> FIELD_VALUE_MAP;


/**
 * The BOM_FIELD_VALUES class provides quick lookup of component values
 * (based on Field ID)
 * This is done for the following reasons:
 * - Increase lookup speed for table values
 * - Allow field values to be reverted to original values
 * - Allow duplicate components to reference the same data
 */
class BOM_FIELD_VALUES
{
public:
    BOM_FIELD_VALUES( wxString aRefDes, FIELD_VALUE_MAP* aTemplate );

    bool GetFieldValue( unsigned int aFieldId, wxString& aValue ) const;
    bool GetBackupValue( unsigned int aFieldId, wxString& aValue ) const;
    bool GetTemplateValue( unsigned int aFieldId, wxString& aValue ) const;

    void SetFieldValue( unsigned int aFieldId, wxString aValue, bool aOverwrite = false );

    wxString GetReference() const { return m_refDes; }

    bool HasValueChanged( unsigned int aFieldId ) const;

    void RevertChanges( unsigned int aFieldId );

    void SetBackupPoint();

protected:
    //! The RefDes to which these values correspond
    wxString m_refDes;

    //! Current values for each column
    FIELD_VALUE_MAP m_currentValues;

    //! Backup values for each column
    FIELD_VALUE_MAP m_backupValues;

    //! Template values for each column
    FIELD_VALUE_MAP* m_templateValues;
};

/**
 * Virtual base class determining how a row is displayed
 * There are three types of rows:
 * GROUP - Displays a group of (one or more) components
 * COMPONENT - Displays a single component
 * UNIT - Child of COMPONENT for multi-unit components
 */
class BOM_TABLE_ROW
{
public:
    BOM_TABLE_ROW();
    virtual ~BOM_TABLE_ROW() {}

    /// Set display properties for a cell
    virtual bool GetAttr( unsigned int aFieldId, wxDataViewItemAttr& aAttr ) const;

    /// Get the row value associated with provided field ID
    virtual wxString GetFieldValue( unsigned int aFieldId ) const = 0;

    /// Set the field value associated with the provided field ID
    virtual bool SetFieldValue( unsigned int aFieldId, const wxString aValue, bool aOverwrite = false ) = 0;

    /// Return parent item
    virtual BOM_TABLE_ROW* GetParent() const { return nullptr; }

    /// Test if row has any child rows
    virtual bool HasChildren() const { return false; }

    /// Return any child rows
    virtual unsigned int GetChildren( wxDataViewItemArray& aChildren ) const { return 0; }

    /// Determine if a value has changed
    virtual bool HasValueChanged( BOM_COLUMN* aField ) const { return false; }

    /// Determine if any values have changed
    bool HasChanged() const;

protected:

    /// Pointer to list of columns
    BOM_COLUMN_LIST* m_columnList;
};

/**
 * BOM_TABLE_GROUP class displays a group of similar components
 * If the group contains more than one component,
 * they are each displayed as child items of the group
 */
class BOM_TABLE_GROUP : public BOM_TABLE_ROW
{
public:
    // List of components stored in this group
    std::vector<BOM_TABLE_COMPONENT*> Components;

    BOM_TABLE_GROUP( BOM_COLUMN_LIST* aColumnList );
    virtual ~BOM_TABLE_GROUP() {}

    // Set display properties for a group row
    virtual bool GetAttr( unsigned int aFieldId, wxDataViewItemAttr& aAttr ) const override;

    // Get group row value
    virtual wxString GetFieldValue( unsigned int aFieldId ) const override;

    // Set group row value
    virtual bool SetFieldValue( unsigned int aFieldId, const wxString aValue, bool aOverwrite = false ) override;

    // Attempt to add a new component to the group
    bool AddComponent( BOM_TABLE_COMPONENT* aComponent );

    // Test if this group should display children
    virtual bool HasChildren() const override { return Components.size() > 1; }

    // Return a list of children items of this group
    virtual unsigned int GetChildren( wxDataViewItemArray& aChildren ) const override;

    // Test if any children have changed
    virtual bool HasValueChanged( BOM_COLUMN* aField ) const override;

    // Return the number of child items in this group
    unsigned int GroupSize( void ) const { return Components.size(); }

    // Return a sorted, concatenated list of references
    wxArrayString GetReferences( bool aSort = true ) const;

    // Function for sorting two reference strings
    static int SortReferences( const wxString& aFirst, const wxString& aSecond );

    // Function for sorting two value strings
    static int SortValues( const wxString& aFirst, const wxString& aSecond );

protected:
    // Test if a particular field matches against another component
    bool TestField( BOM_COLUMN* aField, BOM_TABLE_COMPONENT* aComponent ) const;
};

class BOM_TABLE_COMPONENT : public BOM_TABLE_ROW
{
public:
    // List of units associated with this component
    std::vector<SCH_REFERENCE> Units;

    BOM_TABLE_COMPONENT( BOM_TABLE_GROUP* aParent, BOM_COLUMN_LIST* aColumnList, BOM_FIELD_VALUES* aValues );

    bool AddUnit( SCH_REFERENCE aUnit );

    virtual wxString GetFieldValue( unsigned int aFieldId ) const override;

    virtual bool SetFieldValue( unsigned int aFieldId, const wxString aValue, bool aOverwrite = false ) override;

    virtual bool HasValueChanged( BOM_COLUMN* aField ) const override;

    // Return the reference of the first unit (all units must be the same
    wxString GetReference() const;

    wxString GetPrefix() const;

    void ApplyFieldChanges();

    void RevertFieldChanges();

    void SetParent( BOM_TABLE_GROUP* aParent ) { m_parent = aParent; }

    virtual BOM_TABLE_ROW* GetParent() const override { return m_parent; }

protected:
    BOM_TABLE_GROUP* m_parent;

    BOM_FIELD_VALUES* m_fieldValues;
};

/**
 * BOM_TABLE_MODEL class
 *
 * Contains complete BOM information:
 * a) List of columns (fields) to display
 * b) List of groups of consolidated components
 */
class BOM_TABLE_MODEL : public wxDataViewModel
{
protected:
    BOM_TABLE_MODEL();

    // Vector of unique component rows
    std::vector<std::unique_ptr<BOM_TABLE_COMPONENT>> m_components;

    // Vector of field values mapped to field IDs
    std::vector<std::unique_ptr<BOM_FIELD_VALUES>> m_fieldValues;

    // Template field values
    FIELD_VALUE_MAP m_fieldTemplates;

    // BOM Preferences
    //! Group components based on values
    bool m_groupColumns = true;
    //! Filter components
    bool m_filterColumns = false;
    //! Allow blank fields to be merged with otherwise matching groups
    bool m_mergeBlankFields = false;

    wxDataViewCtrl* m_widget;

    //! ID of column to sort by
    unsigned int m_sortingColumn;
    bool m_sortingOrder;

    void AddDefaultColumns();
    void ClearColumns();

    virtual bool            HasContainerColumns( const wxDataViewItem& aItem ) const override { return true; }

    virtual bool            IsContainer( const wxDataViewItem& aItem ) const override;

    virtual wxDataViewItem  GetParent( const wxDataViewItem& aItem ) const override;

    virtual unsigned int    GetChildren( const wxDataViewItem& aItem, wxDataViewItemArray& aChildren ) const override;

    virtual unsigned int    GetColumnCount() const override { return ColumnList.ColumnCount( false ); }

    virtual wxString        GetColumnType( unsigned int aFieldId ) const override { return wxString( "string" ); }

    virtual void            GetValue( wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aFieldId ) const override;

    virtual bool            SetValue( const wxVariant& aVariant, const wxDataViewItem& item, unsigned int aFieldId ) override;

public:

    virtual ~BOM_TABLE_MODEL();

    BOM_COLUMN_LIST ColumnList;

    /// List of component groups
    std::vector<std::unique_ptr<BOM_TABLE_GROUP>> Groups;

    typedef wxObjectDataPtr<BOM_TABLE_MODEL> MODEL_PTR;

    static MODEL_PTR Create();

    void AttachTo( wxDataViewCtrl* aView );

    wxDataViewColumn* AddColumn( BOM_COLUMN* aColumn, int aPosition = -1 );
    bool RemoveColumn( BOM_COLUMN* aColumn );

    // wxDataViewModel functions
    virtual bool            GetAttr( const wxDataViewItem& aItem, unsigned int aFieldId, wxDataViewItemAttr& aAttr ) const override;
    virtual bool            HasDefaultCompare() const override { return false; }
    virtual int             Compare( const wxDataViewItem& aItem1,
                                     const wxDataViewItem& aItem2,
                                     unsigned int aColumnId,
                                     bool aAscending ) const override;

    void                    ReloadTable();

    unsigned int ColumnCount() const { return ColumnList.ColumnCount(); }
    unsigned int GroupCount() const { return (unsigned int) Groups.size(); }
    unsigned int ComponentCount() const;

    void SetColumnGrouping( const bool aGroup = true ) { m_groupColumns = aGroup; }
    bool GetColumnGrouping() const { return m_groupColumns; }

    void SetColumnFiltering( const bool aFilter = true ) { m_filterColumns = aFilter; }
    bool GetColumnFiltering() const { return m_filterColumns; }

    void SetBlankMerging( const bool aMerge = true ) { m_mergeBlankFields = aMerge; }
    bool GetBlankMerging() const { return m_mergeBlankFields; }

    wxArrayString GetRowData( unsigned int aRow, std::vector<BOM_COLUMN*> aColumns ) const;

    void SetComponents( SCH_REFERENCE_LIST aRefs, const TEMPLATE_FIELDNAMES& aTemplateFields );
    void AddComponentFields( SCH_COMPONENT* aCmp );

    void RevertFieldChanges();
    void ApplyFieldChanges();

    bool HaveFieldsChanged() const;

    void SetBackupPoint();

    std::vector<SCH_REFERENCE> GetChangedComponents();
    unsigned int CountChangedComponents();
};

#endif // _EESCHEMA_BOM_TABLE_MODEL_H_
