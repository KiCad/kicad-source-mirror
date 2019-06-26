/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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


#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <wx/confbase.h>
#include <config_params.h>


/**
 * Class TOOL_SETTINGS
 *
 * Manages persistent settings for a tool (just a simple wrapper to wxConfigBase)
 */
class SETTINGS
{
    public:
        SETTINGS()
            {}

        virtual ~SETTINGS()
            {}

        /** Set a prefix that will be prepent to the keywords when adding a setting in list
         * @param aPrefix is the string to prepend to the keywords
         */
        void SetConfigPrefix( const wxString& aPrefix )
        {
            m_prefix = aPrefix;
        }

        /** @return the current prefix
         */
        const wxString& GetConfigPrefix()
        {
            return m_prefix;
        }

        virtual void Load( wxConfigBase *aConfig );
        virtual void Save( wxConfigBase *aConfig );

        #if 0
        template<class T>
        T Get( const wxString& aName, T aDefaultValue ) const
        {
            wxConfigBase* config = getConfigBase();

            if( !config )
                return aDefaultValue;

            T tmp = aDefaultValue;

            config->Read( getKeyName( aName ), &tmp );
            return tmp;
        }

        template<class T>
        void Set( const wxString& aName, const T &aValue )
        {
            wxConfigBase* config = getConfigBase();

            if( !config )
                return;

            config->Write( getKeyName( aName ), aValue );
        }
        #endif


        void Add( const wxString& name, int* aPtr, int aDefaultValue )
        {
            m_params.push_back( new PARAM_CFG_INT ( m_prefix+name, aPtr, aDefaultValue ) );
        }

        void Add( const wxString& name, bool* aPtr, bool aDefaultValue )
        {
            m_params.push_back( new PARAM_CFG_BOOL ( m_prefix+name, aPtr, aDefaultValue ) );
        }

        void Add( const wxString& name, KIGFX::COLOR4D* aPtr, KIGFX::COLOR4D aDefaultValue )
        {
            m_params.push_back( new PARAM_CFG_SETCOLOR ( m_prefix+name, aPtr, aDefaultValue ) );
        }

        void Add( const wxString& name, KIGFX::COLOR4D* aPtr, EDA_COLOR_T aDefaultValue )
        {
            m_params.push_back( new PARAM_CFG_SETCOLOR ( m_prefix+name, aPtr, aDefaultValue ) );
        }


    protected:

        virtual wxString getKeyName( const wxString& aEntryName ) const
        {
            return aEntryName;
        }

        wxString m_prefix;
        PARAM_CFG_ARRAY m_params;
};



#endif
