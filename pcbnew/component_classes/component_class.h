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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef PCBNEW_COMPONENT_CLASS_H
#define PCBNEW_COMPONENT_CLASS_H

#include <wx/string.h>
#include <vector>

/**
 * A lightweight representation of a component class. The membership within
 * m_consituentClasses allows determination of the type of class this is:
 *
 * m_constituentClasses.size() == 0: This is a null class (no assigment).
 *     m_name is empty.
 * m_constituentClasses.size() == 1: This is an atomic class. The constituent class
 *     pointer refers to itself. m_name contains the name of the atomic class
 * m_constituentClasses.size() > 1: This is a composite class. The constituent class
 *     pointers refer to all atomic members. m_name contains a comma-delimited list of
 *     all atomic member class names.
 */
class COMPONENT_CLASS
{
public:
    /// The assignment context in which this component class is used
    enum class USAGE
    {
        STATIC,
        DYNAMIC,
        STATIC_AND_DYNAMIC,
        EFFECTIVE
    };

    /// Construct a new component class
    explicit COMPONENT_CLASS( const wxString& name, const USAGE aUsageContext ) :
            m_name( name ), m_usageContext( aUsageContext )
    {
    }

    /// @brief Gets the consolidated name of this component class (which may be an aggregate). This is
    /// intended for display to users (e.g. in infobars or messages). WARNING: Do not use this
    /// to compare equivalence, or to export to other tools)
    wxString GetHumanReadableName() const;

    /// Fetches the full name of this component class
    const wxString& GetName() const { return m_name; }

    /// Adds a constituent component class to an effective component class
    void AddConstituentClass( COMPONENT_CLASS* componentClass );

    /// Returns a named constituent class of this component class, or nullptr if not found
    const COMPONENT_CLASS* GetConstituentClass( const wxString& className ) const;

    /// Determines if this (effective) component class contains a specific constituent class
    bool ContainsClassName( const wxString& className ) const;

    /// Determines if this (effective) component class is empty (i.e. no classes defined)
    bool IsEmpty() const;

    /// Fetches a vector of the constituent classes for this (effective) class
    const std::vector<COMPONENT_CLASS*>& GetConstituentClasses() const
    {
        return m_constituentClasses;
    }

    /// Gets the assignment context in which this component class is being used
    USAGE GetUsageContext() const { return m_usageContext; }

    /// Sets the assignment context in which this component class is being used
    void SetUsageContext( const USAGE aUsageContext ) { m_usageContext = aUsageContext; }

    /// Tests two component classes for equality based on full class name
    bool operator==( const COMPONENT_CLASS& aComponent ) const;

    /// Tests two component classes for inequality based on full class name
    bool operator!=( const COMPONENT_CLASS& aComponent ) const;

private:
    /// The full name of the component class
    wxString m_name;

    /// The COMPONENT_CLASS objects contributing to this complete component class
    std::vector<COMPONENT_CLASS*> m_constituentClasses;

    /// The assignment context in which this component class is being used
    USAGE m_usageContext;
};

#endif //PCBNEW_COMPONENT_CLASS_H
