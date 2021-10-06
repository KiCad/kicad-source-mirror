/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014-2020 KiCad Developers, see AUTHORS.TXT for contributors.
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


#ifndef _TEMPLATE_FIELDNAME_H_
#define _TEMPLATE_FIELDNAME_H_

#include <richio.h>
#include <template_fieldnames_lexer.h>

class TEMPLATE_FIELDNAMES_LEXER;


/**
 * The set of all field indices assuming an array like sequence that a SCH_COMPONENT or
 * LIB_PART can hold.
 *
 * The first fields are fixed fields and are defined by #MANDATORY_FIELDS.  After that come
 * an unlimited number of user defined fields, only some of which have indices defined here.
 */
enum  MANDATORY_FIELD_T {
    REFERENCE_FIELD = 0,          ///< Field Reference of part, i.e. "IC21"
    VALUE_FIELD,                  ///< Field Value of part, i.e. "3.3K"
    FOOTPRINT_FIELD,              ///< Field Name Module PCB, i.e. "16DIP300"
    DATASHEET_FIELD,              ///< name of datasheet

    /// The first 4 are mandatory, and must be instantiated in SCH_COMPONENT
    /// and LIB_PART constructors
    MANDATORY_FIELDS
};


/**
 * Hold a name of a symbol's field, field value, and default visibility.
 *
 * Template fieldnames are wanted field names for use in the symbol property editors.
 */
struct TEMPLATE_FIELDNAME
{
    TEMPLATE_FIELDNAME() :
            m_Visible( false ),
            m_URL( false )
    {
    }

    TEMPLATE_FIELDNAME( const wxString& aName ) :
            m_Name( aName ),
            m_Visible( false ),
            m_URL( false )
    {
    }

    TEMPLATE_FIELDNAME( const TEMPLATE_FIELDNAME& ref )
    {
            m_Name = ref.m_Name;
            m_Visible = ref.m_Visible;
            m_URL = ref.m_URL;
    }

    /**
     * Serialize this object out as text into the given #OUTPUTFORMATTER.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const ;

    /**
     * Fill this object from information in the input stream \a aSpec, which is a
     * #TEMPLATE_FIELDNAMES_LEXER.
     *
     * The entire textual element spec is <br>(field (name _yourfieldname_)(value _yourvalue_)
     * visible))</br>.  The presence of value is optional, the presence of visible is optional.
     * When this function is called, the input token stream given by \a aSpec is assumed to be
     * positioned at the '^' in the following example, i.e. just after the identifying keyword
     * and before the content specifying stuff.<br>(field ^ (....) )</br>.
     *
     * @param aSpec is the input token stream of keywords and symbols.
     */
    void Parse( TEMPLATE_FIELDNAMES_LEXER* aSpec );

    /**
     * Return a default symbol field name for field \a aFieldNdx for all components.
     *
     * These field names are not modifiable but template field names are.
     *
     * @param aFieldNdx The field number index, > 0.
     * @param aTranslate If true, return the translated field name, else get the canonical name.
     */
    static const wxString GetDefaultFieldName( int aFieldNdx, bool aTranslate = true );

    wxString    m_Name;         // The field name
    bool        m_Visible;      // Field defaults to being visible in schematic.
    bool        m_URL;          // If field should have a browse button
};

typedef std::vector< TEMPLATE_FIELDNAME > TEMPLATE_FIELDNAMES;


class TEMPLATES
{
public:
    TEMPLATES() :
            m_resolvedDirty( true )
    { }

    /**
     * Serialize this object out as text into the given #OUTPUTFORMATTER.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel, bool aGlobal ) const ;

    /**
     * Fill this object from information in the input stream handled by
     * #TEMPLATE_FIELDNAMES_LEXER.
     */
    void Parse( TEMPLATE_FIELDNAMES_LEXER* in, bool aGlobal );


    /**
     * Insert or append a wanted symbol field name into the field names template.
     *
     * Should be used for any symbol property editor.  If the name already exists, it
     * overwrites the same name.
     *
     * @param aFieldName is a full description of the wanted field, and it must not match
     *                   any of the default field names.
     * @param aGlobal indicates whether to add to the global or project table.
     */
    void AddTemplateFieldName( const TEMPLATE_FIELDNAME& aFieldName, bool aGlobal );

    /**
     * Delete the entire contents.
     */
    void DeleteAllFieldNameTemplates( bool aGlobal );

    /**
     * Return a template field name list for read only access.
     */
    const TEMPLATE_FIELDNAMES& GetTemplateFieldNames();

    /**
     * Return a specific list (global or project) for read only access.
     */
    const TEMPLATE_FIELDNAMES& GetTemplateFieldNames( bool aGlobal );

    /**
     * Search for \a aName in the template field name list.
     *
     * @param aName A wxString object containing the field name to search for.
     * @return the template field name if found; NULL otherwise.
     */
    const TEMPLATE_FIELDNAME* GetFieldName( const wxString& aName );

protected:
    void resolveTemplates();

private:
    TEMPLATE_FIELDNAMES     m_globals;
    TEMPLATE_FIELDNAMES     m_project;

    // Combined list.  Project templates override global ones.
    TEMPLATE_FIELDNAMES     m_resolved;
    bool                    m_resolvedDirty;
};

#endif // _TEMPLATE_FIELDNAME_H_
