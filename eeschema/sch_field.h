/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file sch_field.h
 * @brief Definition of the SCH_FIELD class for Eeschema.
 */

#ifndef CLASS_SCH_FIELD_H
#define CLASS_SCH_FIELD_H


#include "sch_item_struct.h"
#include "general.h"


class SCH_EDIT_FRAME;
class SCH_COMPONENT;
class LIB_FIELD;


/**
 * Class SCH_FIELD
 * instances are attached to a component and provide a place for the component's value,
 * reference designator, footprint, and user definable name-value pairs of arbitrary purpose.
 *
 * <ul> <li>Field 0 is reserved for the component reference.</li>
 * <li>Field 1 is reserved for the component value.</li>
 * <li>Field 2 is reserved for the component footprint.</li>
 * <li>Field 3 is reserved for the component data sheet file.</li>
 * <li>Fields 4 and higher are user defineable.</li></ul>
 */

class SCH_FIELD : public SCH_ITEM, public EDA_TEXT
{
public:
    int      m_FieldId;         ///< Field index, @see enum NumFieldType

    wxString m_Name;

    bool     m_AddExtraText;    /**< for REFERENCE, add extra info
                                 * (for REFERENCE: add part selection text */

public:
    SCH_FIELD( const wxPoint& aPos, int aFieldId, SCH_COMPONENT* aParent,
               wxString aName = wxEmptyString );

    SCH_FIELD( const SCH_FIELD& aField );

    ~SCH_FIELD();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_FIELD" );
    }

    /**
     * Function GetName
     * returns the field name.  If the field name is empty, the default field name is
     * returned.  Field names are VALUE, REFERENCE, etc.
     * @return The name of the field.
     */
    wxString GetName() const;

    int GetId() const { return m_FieldId; }

    void Place( SCH_EDIT_FRAME* frame, wxDC* DC );

    EDA_RECT GetBoundingBox() const;

    /**
     * Function IsVoid
     * returns true if the field is either empty or holds "~".
     */
    bool IsVoid() const
    {
        size_t len = m_Text.Len();

        return len == 0 || ( len == 1 && m_Text[0] == wxChar( '~' ) );
    }

    /**
     * Function SwapData
     * exchanges the date between the field and \a aField.
     *
     * @param aItem The field to exchange data with.
     */
    virtual void SwapData( SCH_ITEM* aItem );

    /**
     * Function ImportValues
     * copy parameters from a source.
     * Pointers and specific values (position) are not copied
     * @param aSource = the LIB_FIELD to read
     */
    void ImportValues( const LIB_FIELD& aSource );

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    int GetPenSize() const;

    /**
     * Function IsVisible
     * @return true is this field is visible, false if flagged invisible
     */
    bool IsVisible() const
    {
        return (m_Attributs & TEXT_NO_VISIBLE) == 0 ? true : false;
    }


    /**
     * Function Draw
     */
    void Draw( EDA_DRAW_PANEL* aPanel,
               wxDC*           aDC,
               const wxPoint&  aOffset,
               int             aDrawMode,
               int             aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    virtual void Rotate( wxPoint rotationPoint );

    virtual void Mirror_X( int aXaxis_position )
    {
        /* Do Nothing: fields are never mirrored alone.
         * they are moved when the parent component is mirrored
         * this function is only needed by the virtual pure function of the
         * master class */
    }

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position )
    {
        /* Do Nothing: fields are never mirrored alone.
         * they are moved when the parent component is mirrored
         * this function is only needed by the virtual pure function of the
         * master class */
    }

    /**
     * Compare schematic field text against search string.
     *
     * @param aSearchData - Criteria to search against.
     * @param aAuxData - a pointer on auxiliary data, if needed.
     *              the sheet path is needed for REFERENCE field because m_Text
     *              value is just the value displayed, and in complex hierarchies
     *              this is only one of all references (one per sheet path)
     * @param aFindLocation - a wxPoint where to put the location of matched item. can be NULL.
     * @return True if this field text matches the search criteria.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData,
                          void* aAuxData, wxPoint* aFindLocation );

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const;

private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const;
    virtual bool doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const;
    virtual EDA_ITEM* doClone() const;
    virtual void doPlot( PLOTTER* aPlotter );
    virtual wxPoint doGetPosition() const { return m_Pos; }
    virtual void doSetPosition( const wxPoint& aPosition ) { m_Pos = aPosition; }
};


#endif /* CLASS_SCH_FIELD_H */
