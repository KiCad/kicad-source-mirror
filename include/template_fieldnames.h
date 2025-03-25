/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#pragma once

#include <wx/string.h>

class OUTPUTFORMATTER;
class TEMPLATE_FIELDNAMES_LEXER;


/**
 * The set of all field indices assuming an array like sequence that a SCH_COMPONENT or
 * LIB_PART can hold.
 *
 * The first fields are fixed fields and are defined by #MANDATORY_FIELDS.  After that come
 * an unlimited number of user defined fields, only some of which have indices defined here.
 *
 * NOTE: this must stay a enum class to prevent developers from trying to use it as an array
 * index.
 */
enum class FIELD_T : int
{
    USER,                   ///< The field ID hasn't been set yet; field is invalid
    REFERENCE,              ///< Field Reference of part, i.e. "IC21"
    VALUE,                  ///< Field Value of part, i.e. "3.3K"
    FOOTPRINT,              ///< Field Name Module PCB, i.e. "16DIP300"
    DATASHEET,              ///< name of datasheet
    DESCRIPTION,            ///< Field Description of part, i.e. "1/4W 1% Metal Film Resistor"
    INTERSHEET_REFS,        ///< Global label cross-reference page numbers
    SHEET_NAME,
    SHEET_FILENAME,
    SHEET_USER
};

#define MANDATORY_FIELDS { FIELD_T::REFERENCE,        \
                           FIELD_T::VALUE,            \
                           FIELD_T::FOOTPRINT,        \
                           FIELD_T::DATASHEET,        \
                           FIELD_T::DESCRIPTION }

#define GLOBALLABEL_MANDATORY_FIELDS { FIELD_T::INTERSHEET_REFS }

#define SHEET_MANDATORY_FIELDS { FIELD_T::SHEET_NAME,        \
                                 FIELD_T::SHEET_FILENAME }

// A helper to call GetDefaultFieldName with or without translation.
// Translation should be used only to display field names in dialogs
#define DO_TRANSLATE true


/**
 * Return a default symbol field name for a mandatory field type.
 *
 * These field names are not modifiable but template field names are.
 *
 * @param aTranslateForHI If true, return the translated field name,
 * else get the canonical name (defualt). Translation is intended only for dialogs
 */
wxString GetDefaultFieldName( FIELD_T aFieldId, bool aTranslateForHI );
wxString GetUserFieldName( int aFieldNdx, bool aTranslateForHI );


inline wxString GetCanonicalFieldName( FIELD_T aFieldType )
{
    return GetDefaultFieldName( aFieldType, !DO_TRANSLATE );
}


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
    void Format( OUTPUTFORMATTER* out ) const ;

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

    wxString    m_Name;         // The field name
    bool        m_Visible;      // Field defaults to being visible in schematic.
    bool        m_URL;          // If field should have a browse button
};


class TEMPLATES
{
public:
    TEMPLATES() :
            m_resolvedDirty( true )
    { }

    /**
     * Serialize this object out as text into the given #OUTPUTFORMATTER.
     */
    void Format( OUTPUTFORMATTER* out, bool aGlobal ) const ;

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
     * Add a serialized list of template field names.
     */
    void AddTemplateFieldNames( const wxString& aSerializedFieldNames );

    /**
     * Delete the entire contents.
     */
    void DeleteAllFieldNameTemplates( bool aGlobal );

    /**
     * Return a template field name list for read only access.
     */
    const std::vector<TEMPLATE_FIELDNAME>& GetTemplateFieldNames();

    /**
     * Return a specific list (global or project) for read only access.
     */
    const std::vector<TEMPLATE_FIELDNAME>& GetTemplateFieldNames( bool aGlobal );

    /**
     * Search for \a aName in the template field name list.
     *
     * @param aName A wxString object containing the field name to search for.
     * @return the template field name if found; NULL otherwise.
     */
    const TEMPLATE_FIELDNAME* GetFieldName( const wxString& aName );

protected:
    void resolveTemplates();

    void parse( TEMPLATE_FIELDNAMES_LEXER* in, bool aGlobal );

private:
    std::vector<TEMPLATE_FIELDNAME> m_globals;
    std::vector<TEMPLATE_FIELDNAME> m_project;

    // Combined list.  Project templates override global ones.
    std::vector<TEMPLATE_FIELDNAME> m_resolved;
    bool                            m_resolvedDirty;
};
