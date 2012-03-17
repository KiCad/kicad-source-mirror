/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_component.h
 * @brief Definition the SCH_COMPONENT class for Eeschema.
 */

#ifndef COMPONENT_CLASS_H
#define COMPONENT_CLASS_H


#include <sch_field.h>
#include <transform.h>
#include <general.h>


class SCH_SHEET_PATH;
class LIB_ITEM;
class LIB_PIN;
class LIB_COMPONENT;


/// A container for several SCH_FIELD items
typedef std::vector<SCH_FIELD> SCH_FIELDS;


/**
 * Class SCH_COMPONENT
 * describes a real schematic component
 */
class SCH_COMPONENT : public SCH_ITEM
{
    friend class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC;

    wxPoint m_Pos;
    wxString m_ChipName;    ///< Name to look for in the library, i.e. "74LS00".
    int      m_unit;        ///< The unit for multiple part per package components.
    int      m_convert;     ///< The alternate body style for components that have more than
                            ///< one body style defined.  Primarily used for components that
                            ///< have a De Morgan conversion.
    wxString m_prefix;      ///< C, R, U, Q etc - the first character which typically indicates
                            ///< what the component is. Determined, upon placement, from the
                            ///< library component.  Created upon file load, by the first
                            ///<  non-digits in the reference fields.
    TRANSFORM m_transform;  ///< The rotation/mirror transformation matrix.
    SCH_FIELDS m_Fields;    ///< Variable length list of fields.

    /**
     * Defines the hierarchical path and reference of the component.  This allows support
     * for hierarchical sheets that reference the same schematic.  The format for the path
     * is /&ltsheet time stamp&gt/&ltsheet time stamp&gt/.../&lscomponent time stamp&gt.
     * A single / denotes the root sheet.
     */
    wxArrayString m_PathsAndReferences;

    void Init( const wxPoint& pos = wxPoint( 0, 0 ) );

    EDA_RECT GetBodyBoundingBox() const;

public:
    SCH_COMPONENT( const wxPoint& pos = wxPoint( 0, 0 ), SCH_ITEM* aParent = NULL );

    /**
     * Create schematic component from library component object.
     *
     * @param libComponent - Component library object to create schematic
     *                       component from.
     * @param sheet - Schematic sheet the component is place into.
     * @param unit - Part for components that have multiple parts per
     *               package.
     * @param convert - Use the alternate body style for the schematic
     *                  component.
     * @param pos - Position to place new component.
     * @param setNewItemFlag - Set the component IS_NEW and IS_MOVED flags.
     */
    SCH_COMPONENT( LIB_COMPONENT& libComponent, SCH_SHEET_PATH* sheet,
                   int unit = 0, int convert = 0,
                   const wxPoint& pos = wxPoint( 0, 0 ),
                   bool setNewItemFlag = false );

    /**
     * Copy Constructor
     * clones \a aComponent into a new object.  All fields are copied as is except
     * for the linked list management pointers which are set to NULL, and the
     * SCH_FIELD's m_Parent pointers which are set to the new parent,
     * i.e. this new object.
     */
    SCH_COMPONENT( const SCH_COMPONENT& aComponent );

    ~SCH_COMPONENT() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_COMPONENT" );
    }

    wxString GetLibName() const { return m_ChipName; }

    void SetLibName( const wxString& aName );

    int GetUnit() const { return m_unit; }

    void SetUnit( int aUnit );

    int GetConvert() const { return m_convert; }

    void SetConvert( int aConvert );

    wxString GetPrefix() const { return m_prefix; }

    TRANSFORM& GetTransform() const { return const_cast< TRANSFORM& >( m_transform ); }

    void SetTransform( const TRANSFORM& aTransform );

    /**
     * Function GetPartCount
     * returns the number of parts per package of the component.
     *
     * @return The number of parts per package or zero if the library entry cannot be found.
     */
    int GetPartCount() const;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic component from \a aLine in a .sch file.
     *
     * @param aLine Essentially this is file to read the component from.
     * @param aErrorMsg Description of the error if an error occurs while loading the component.
     * @return True if the component loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function SetOrientation
     * computes the new transform matrix based on \a aOrientation for the component which is
     * applied to the current transform.
     * @param aOrientation The orientation to apply to the transform.
     */
    void SetOrientation( int aOrientation );

    /**
     * Function GetOrientation
     * Used to display component orientation (in dialog editor or info)
     * @return the orientation and mirror
     * Note: Because there are different ways to have a given orientation/mirror,
     * the orientation/mirror is not necessary what the used does
     * (example : a mirrorX then a mirrorY give no mirror but rotate the
     * component).
     * So this function find a rotation and a mirror value
     * ( CMP_MIRROR_X because this is the first mirror option tested)
     *  but can differs from the orientation made by an user
     * ( a CMP_MIRROR_Y is find as a CMP_MIRROR_X + orientation 180, because
     * they are equivalent)
     */
    int GetOrientation();

    /**
     * Function GetScreenCoord
     * Returns the coordinated point relative to the orientation of the component of \a aPoint.
     * The coordinates are always relative to the anchor position of the component.
     * @param aPoint The coordinates to transform.
     * @return The transformed point.
     */
    wxPoint GetScreenCoord( const wxPoint& aPoint );

    void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function ClearAnnotation
     * clears exiting component annotation ( i.i IC23 changed to IC? and part reset to 1)
     * @param aSheetPath: SCH_SHEET_PATH value: if NULL remove all annotations,
     *                    else remove annotation relative to this sheetpath
     */
    void ClearAnnotation( SCH_SHEET_PATH* aSheetPath );

    /**
     * Function SetTimeStamp
     * changes the time stamp to \a aNewTimeStamp updates the reference path.
     * @see m_PathsAndReferences
     * @param aNewTimeStamp = new time stamp
     */
    void SetTimeStamp( long aNewTimeStamp );

    /**
     * Function GetBoundingBox
     * returns the bounding box of this object for display purposes. This box should be an
     * enclosing perimeter for visible components of this object, and the units should be
     * in the pcb or schematic coordinate system.  It is OK to overestimate the size by a
     * few counts.
     * @return The bounding rectangle of the component.
     */
    EDA_RECT GetBoundingBox() const;

    //-----<Fields>-----------------------------------------------------------

    /**
     * Function GetField
     * returns a field.
     * @param aFieldNdx An index into the array of fields, not a field id.
     * @return SCH_FIELD* - the field value or NULL if does not exist
     */
    SCH_FIELD* GetField( int aFieldNdx ) const;

    /**
     * Function AddField
     * adds a field to the component.  The field is copied as it is put into
     * the component.
     * @param aField A const reference to the SCH_FIELD to add.
     * @return SCH_FIELD* - the newly inserted field.
     */
    SCH_FIELD* AddField( const SCH_FIELD& aField );

    /**
     * Function FindField
     * searches for SCH_FIELD with \a aFieldName and returns it if found, else NULL.
     */
    SCH_FIELD* FindField( const wxString& aFieldName );

    void SetFields( const SCH_FIELDS& aFields )
    {
        m_Fields = aFields;     // vector copying, length is changed possibly
    }

    //-----</Fields>----------------------------------------------------------

    /**
     * Function GetFieldCount
     * returns the number of fields in this component.
     */
    int GetFieldCount() const { return (int) m_Fields.size(); }

    /**
     * Function GetPin
     * finds a component pin by number.
     *
     * @param number - The number of the pin to find.
     * @return Pin object if found, otherwise NULL.
     */
    LIB_PIN* GetPin( const wxString& number );

    virtual void Draw( EDA_DRAW_PANEL* panel,
                       wxDC*           DC,
                       const wxPoint&  offset,
                       int             draw_mode,
                       int             Color = -1 )
    {
        Draw( panel, DC, offset, draw_mode, Color, true );
    }

    void Draw( EDA_DRAW_PANEL* panel,
               wxDC*           DC,
               const wxPoint&  offset,
               int             draw_mode,
               int             Color,
               bool            DrawPinText );

    virtual void SwapData( SCH_ITEM* aItem );

    // returns a unique ID, in the form of a path.
    wxString GetPath( const SCH_SHEET_PATH* sheet ) const;

    /**
     * Function IsReferenceStringValid (static)
     * Tests for an acceptable reference string
     * An acceptable reference string must support unannotation
     * i.e starts by letter
     * @param aReferenceString = the reference string to validate
     * @return true if OK
     */
    static bool IsReferenceStringValid( const wxString& aReferenceString );

    /**
     * Function GetRef
     * returns the reference, for the given sheet path.
     */
    const wxString GetRef( const SCH_SHEET_PATH* sheet );

    /**
     * Set the reference, for the given sheet path.
     */
    void SetRef( const SCH_SHEET_PATH* sheet, const wxString& ref );

    /**
     * Function AddHierarchicalReference
     * adds a full hierarchical reference (path + local reference)
     * @param aPath Hierarchical path (/&ltsheet timestamp&gt/&ltcomponent
     *              timestamp&gt like /05678E50/A23EF560)
     * @param aRef :local reference like C45, R56
     * @param aMulti Part selection, used in multi part per package (0 or 1 for non multi)
     */
    void AddHierarchicalReference( const wxString& aPath,
                                   const wxString& aRef,
                                   int             aMulti );

    // returns the unit selection, for the given sheet path.
    int GetUnitSelection( SCH_SHEET_PATH* aSheet );

    // Set the unit selection, for the given sheet path.
    void SetUnitSelection( SCH_SHEET_PATH* aSheet, int aUnitSelection );

    // Geometric transforms (used in block operations):

    /** @copydoc SCH_ITEM::Move() */
    virtual void Move( const wxPoint& aMoveVector )
    {
        if( aMoveVector == wxPoint( 0, 0 ) )
            return;

        m_Pos += aMoveVector;

        for( int ii = 0; ii < GetFieldCount(); ii++ )
            GetField( ii )->Move( aMoveVector );

        SetModified();
    }

    /** @copydoc SCH_ITEM::MirrorY() */
    virtual void MirrorY( int aYaxis_position );

    /** @copydoc SCH_ITEM::MirrorX() */
    virtual void MirrorX( int aXaxis_position );

    /** @copydoc SCH_ITEM::Rotate() */
    virtual void Rotate( wxPoint aPosition );

    /**
     * @copydoc EDA_ITEM::Matches()
     */
    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation );

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    wxPoint GetPinPhysicalPosition( LIB_PIN* Pin );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual bool IsConnectable() const { return true; }

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

    virtual SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData,
                                 const KICAD_T scanTypes[] );

    /**
     * Function GetDrawItem().
     * Return the component library item at \a aPosition that is part of this component.
     *
     * @param aPosition - Schematic position of the component library object.
     * @param aType - Type of component library object to find or any if set to TYPE_NOT_INIT.
     * @return A pointer to the component library object if found, otherwise NULL.
     */
    LIB_ITEM* GetDrawItem( const wxPoint& aPosition, KICAD_T aType = TYPE_NOT_INIT );

    /** @copydoc EDA_ITEM::GetSelectMenuText() */
    virtual wxString GetSelectMenuText() const;

    /** @copydoc EDA_ITEM::GetMenuImage() */
    virtual BITMAP_DEF GetMenuImage() const { return  add_component_xpm; }

    virtual void GetNetListItem( vector<NETLIST_OBJECT*>& aNetListItems,
                                 SCH_SHEET_PATH*          aSheetPath );

    virtual bool operator <( const SCH_ITEM& aItem ) const;

    bool operator==( const SCH_COMPONENT& aComponent) const;
    bool operator!=( const SCH_COMPONENT& aComponent) const;

    SCH_ITEM& operator=( const SCH_ITEM& aItem );

    /**
     * @copydoc EDA_ITEM::IsReplaceable()
     */
    virtual bool IsReplaceable() const { return true; }

    /** @copydoc SCH_ITEM::GetPosition() */
    virtual wxPoint GetPosition() const { return m_Pos; }

    /** @copydoc SCH_ITEM::SetPosition() */
    virtual void SetPosition( const wxPoint& aPosition ) { Move( aPosition - m_Pos ); }

    /** @copydoc SCH_ITEM::HitTest(const wxPoint&,int)const */
    virtual bool HitTest( const wxPoint& aPosition, int aAccuracy ) const;

    /** @copydoc SCH_ITEM::HitTest(const EDA_RECT&,bool,int)const */
    virtual bool HitTest( const EDA_RECT& aRect, bool aContained = false,
                          int aAccuracy = 0 ) const;

    /** @copydoc SCH_ITEM::Plot() */
    virtual void Plot( PLOTTER* aPlotter );

    /** @copydoc EDA_ITEM::Clone() */
    virtual EDA_ITEM* Clone() const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // override
#endif

private:
    /** @copydoc SCH_ITEM::doIsConnected() */
    virtual bool doIsConnected( const wxPoint& aPosition ) const;
};


#endif /* COMPONENT_CLASS_H */
