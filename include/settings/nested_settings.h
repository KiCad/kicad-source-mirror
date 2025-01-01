/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#ifndef NESTED_SETTINGS_H_
#define NESTED_SETTINGS_H_

#include <settings/json_settings.h>


/**
 * NESTED_SETTINGS is a JSON_SETTINGS that lives inside a JSON_SETTINGS.
 * Instead of being backed by a JSON file on disk, it loads and stores to its parent.
 */
class KICOMMON_API NESTED_SETTINGS : public JSON_SETTINGS
{
public:
    NESTED_SETTINGS( const std::string& aName, int aSchemaVersion, JSON_SETTINGS* aParent,
                     const std::string& aPath, bool aLoadFromFile = true );

    virtual ~NESTED_SETTINGS();

    /**
     * Loads the JSON document from the parent and then calls Load()
     * @param aDirectory
     */
    bool LoadFromFile( const wxString& aDirectory = "" ) override;

    /**
     * Calls Store() and then saves the JSON document contents into the parent JSON_SETTINGS
     * @param aDirectory is ignored
     * @return true if the document contents were updated
     */
    bool SaveToFile( const wxString& aDirectory = "", bool aForce = false ) override;

    void SetParent( JSON_SETTINGS* aParent, bool aLoadFromFile = true );

    JSON_SETTINGS* GetParent()
    {
        return m_parent;
    }

protected:

    /// A pointer to the parent object to load and store from
    JSON_SETTINGS* m_parent;

    /// The path (in pointer format) of where to store this document in the parent
    std::string m_path;
};

#endif
