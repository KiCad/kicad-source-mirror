/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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


#include <config_params.h>       // for PARAM_CFG_INT_WITH_SCALE, PARAM_CFG_...
#include <locale_io.h>
#include <math/util.h>             // for KiROUND
#include <wx/config.h>           // for wxConfigBase
#include <wx/debug.h>            // for wxASSERT

void wxConfigLoadParams( wxConfigBase* aCfg, const std::vector<std::unique_ptr<PARAM_CFG>>& aList,
                         const wxString& aGroup )
{
    wxASSERT( aCfg );

    for( const auto& param : aList )
    {
        if( !!param->m_Group )
            aCfg->SetPath( param->m_Group );
        else
            aCfg->SetPath( aGroup );

        if( param->m_Setup )
            continue;

        param->ReadParam( aCfg );
    }
}


void wxConfigLoadSetups( wxConfigBase* aCfg, const std::vector<std::unique_ptr<PARAM_CFG>>& aList )
{
    wxASSERT( aCfg );

    for( const auto& param : aList )
    {
        if( !param->m_Setup )
            continue;

        param->ReadParam( aCfg );
    }
}


void wxConfigSaveParams( wxConfigBase* aCfg, const std::vector<std::unique_ptr<PARAM_CFG>>& aList,
                         const wxString& aGroup )
{
    wxASSERT( aCfg );

    for( const auto& param : aList )
    {
        if( !!param->m_Group )
            aCfg->SetPath( param->m_Group );
        else
            aCfg->SetPath( aGroup );

        if( param->m_Setup )
            continue;

        if( param->m_Type == PARAM_COMMAND_ERASE )       // Erase all data
        {
            if( !!param->m_Ident )
                aCfg->DeleteGroup( param->m_Ident );
        }
        else
        {
            param->SaveParam( aCfg );
        }
    }
}


void wxConfigSaveSetups( wxConfigBase* aCfg, const std::vector<std::unique_ptr<PARAM_CFG>>& aList )
{
    wxASSERT( aCfg );

    for( const auto& param : aList )
    {
        if( !param->m_Setup )
            continue;

        if( param->m_Type == PARAM_COMMAND_ERASE )       // Erase all data
        {
            if( !!param->m_Ident )
                aCfg->DeleteGroup( param->m_Ident );
        }
        else
        {
            param->SaveParam( aCfg );
        }
    }
}


void ConfigBaseWriteDouble( wxConfigBase* aConfig, const wxString& aKey, double aValue )
{
    // Use a single strategy, regardless of wx version.
    // Want C locale float string.

    LOCALE_IO   toggle;
    wxString    tnumber = wxString::Format( wxT( "%.16g" ), aValue );

    aConfig->Write( aKey, tnumber );
}


PARAM_CFG::PARAM_CFG( const wxString& ident, const paramcfg_id type,
                      const wxChar* group, const wxString& legacy )
{
    m_Ident = ident;
    m_Type  = type;
    m_Group = group;
    m_Setup = false;

    m_Ident_legacy = legacy;
}


PARAM_CFG_INT::PARAM_CFG_INT( const wxString& ident, int* ptparam, int default_val,
                              int min, int max, const wxChar* group, const wxString& legacy ) :
        PARAM_CFG( ident, PARAM_INT, group, legacy )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Min = min;
    m_Max = max;
}


PARAM_CFG_INT::PARAM_CFG_INT( bool setup, const wxString& ident, int* ptparam, int default_val,
                              int min, int max, const wxChar* group, const wxString& legacy ) :
        PARAM_CFG( ident, PARAM_INT, group, legacy )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Min   = min;
    m_Max   = max;
    m_Setup = setup;
}


void PARAM_CFG_INT::ReadParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    int itmp = m_Default;

    if( !aConfig->Read( m_Ident, &itmp ) && m_Ident_legacy != wxEmptyString )
        aConfig->Read( m_Ident_legacy, &itmp );

    if( (itmp < m_Min) || (itmp > m_Max) )
        itmp = m_Default;

    *m_Pt_param = itmp;
}


void PARAM_CFG_INT::SaveParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    aConfig->Write( m_Ident, *m_Pt_param );
}


PARAM_CFG_INT_WITH_SCALE::PARAM_CFG_INT_WITH_SCALE( const wxString& ident, int* ptparam,
                                                    int default_val, int min, int max,
                                                    const wxChar* group, double aBiu2cfgunit,
                                                    const wxString& legacy_ident ) :
    PARAM_CFG_INT( ident, ptparam, default_val, min, max, group, legacy_ident )
{
    m_Type = PARAM_INT_WITH_SCALE;
    m_BIU_to_cfgunit = aBiu2cfgunit;
}


PARAM_CFG_INT_WITH_SCALE::PARAM_CFG_INT_WITH_SCALE( bool setup, const wxString& ident, int* ptparam,
                                                    int default_val, int min, int max,
                                                    const wxChar* group, double aBiu2cfgunit,
                                                    const wxString& legacy_ident ) :
    PARAM_CFG_INT( setup, ident, ptparam, default_val, min, max, group, legacy_ident )
{
    m_Type = PARAM_INT_WITH_SCALE;
    m_BIU_to_cfgunit = aBiu2cfgunit;
}


void PARAM_CFG_INT_WITH_SCALE::ReadParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    double dtmp = (double) m_Default * m_BIU_to_cfgunit;
    if( !aConfig->Read( m_Ident, &dtmp ) && m_Ident_legacy != wxEmptyString )
        aConfig->Read( m_Ident_legacy, &dtmp );

    int itmp = KiROUND( dtmp / m_BIU_to_cfgunit );

    if( (itmp < m_Min) || (itmp > m_Max) )
        itmp = m_Default;

    *m_Pt_param = itmp;
}


void PARAM_CFG_INT_WITH_SCALE::SaveParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    // We cannot use aConfig->Write for a double, because
    // this function uses a format with very few digits in mantissa,
    // and truncate issues are frequent.
    // We uses our function.
    ConfigBaseWriteDouble( aConfig, m_Ident, *m_Pt_param * m_BIU_to_cfgunit );
}


PARAM_CFG_DOUBLE::PARAM_CFG_DOUBLE( const wxString& ident, double* ptparam,
                                    double default_val, double min, double max,
                                    const wxChar* group ) :
        PARAM_CFG( ident, PARAM_DOUBLE, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Min = min;
    m_Max = max;
}


PARAM_CFG_DOUBLE::PARAM_CFG_DOUBLE( bool          Insetup,
                                    const wxString& ident,
                                    double*       ptparam,
                                    double        default_val,
                                    double        min,
                                    double        max,
                                    const wxChar* group ) :
        PARAM_CFG( ident, PARAM_DOUBLE, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Min   = min;
    m_Max   = max;
    m_Setup = Insetup;
}


void PARAM_CFG_DOUBLE::ReadParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    double dtmp = m_Default;
    aConfig->Read( m_Ident, &dtmp );

    if( (dtmp < m_Min) || (dtmp > m_Max) )
        dtmp = m_Default;

    *m_Pt_param = dtmp;
}


void PARAM_CFG_DOUBLE::SaveParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    // We cannot use aConfig->Write for a double, because
    // this function uses a format with very few digits in mantissa,
    // and truncate issues are frequent.
    // We uses our function.
    ConfigBaseWriteDouble( aConfig, m_Ident, *m_Pt_param );
}


PARAM_CFG_BOOL::PARAM_CFG_BOOL( const wxString& ident, bool* ptparam, int default_val,
                                const wxChar* group, const wxString& legacy ) :
        PARAM_CFG( ident, PARAM_BOOL, group, legacy )
{
    m_Pt_param = ptparam;
    m_Default  = default_val ? true : false;
}


PARAM_CFG_BOOL::PARAM_CFG_BOOL( bool Insetup, const wxString& ident, bool* ptparam,
                                int default_val, const wxChar* group, const wxString& legacy ) :
        PARAM_CFG( ident, PARAM_BOOL, group, legacy )
{
    m_Pt_param = ptparam;
    m_Default  = default_val ? true : false;
    m_Setup    = Insetup;
}


void PARAM_CFG_BOOL::ReadParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    int itmp = (int) m_Default;

    if( !aConfig->Read( m_Ident, &itmp ) && m_Ident_legacy != wxEmptyString )
        aConfig->Read( m_Ident_legacy, &itmp );

    *m_Pt_param = itmp ? true : false;
}


void PARAM_CFG_BOOL::SaveParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    aConfig->Write( m_Ident, *m_Pt_param );
}


PARAM_CFG_WXSTRING::PARAM_CFG_WXSTRING( const wxString& ident, wxString* ptparam,
                                        const wxChar* group ) :
        PARAM_CFG( ident, PARAM_WXSTRING, group )
{
    m_Pt_param = ptparam;
}


PARAM_CFG_WXSTRING::PARAM_CFG_WXSTRING( bool Insetup, const wxString& ident, wxString* ptparam,
                                        const wxString& default_val, const wxChar* group ) :
        PARAM_CFG( ident, PARAM_WXSTRING, group )
{
    m_Pt_param = ptparam;
    m_Setup    = Insetup;
    m_default  = default_val;
}


void PARAM_CFG_WXSTRING::ReadParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    *m_Pt_param = aConfig->Read( m_Ident, m_default );
}


void PARAM_CFG_WXSTRING::SaveParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    aConfig->Write( m_Ident, *m_Pt_param );
}


PARAM_CFG_WXSTRING_SET::PARAM_CFG_WXSTRING_SET( const wxString& ident, std::set<wxString>* ptparam,
                                                const wxChar* group ) :
        PARAM_CFG( ident, PARAM_WXSTRING_SET, group )
{
    m_Pt_param = ptparam;
}


PARAM_CFG_WXSTRING_SET::PARAM_CFG_WXSTRING_SET( bool Insetup, const wxString& ident,
                                                std::set<wxString>* ptparam, const wxChar* group ) :
        PARAM_CFG( ident, PARAM_WXSTRING, group )
{
    m_Pt_param = ptparam;
    m_Setup    = Insetup;
}


void PARAM_CFG_WXSTRING_SET::ReadParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    for( int i = 1; true; ++i )
    {
        wxString key, data;

        key = m_Ident;
        key << i;
        data = aConfig->Read( key, wxT( "" ) );

        if( data.IsEmpty() )
            break;

        m_Pt_param->insert( data );
    }
}


void PARAM_CFG_WXSTRING_SET::SaveParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    int i = 1;

    for( const wxString& str : *m_Pt_param )
    {
        wxString key;

        key = m_Ident;
        key << i++;

        aConfig->Write( key, str );
    }
}


PARAM_CFG_FILENAME::PARAM_CFG_FILENAME( const wxString& ident,
                                        wxString*     ptparam,
                                        const wxChar* group ) :
        PARAM_CFG( ident, PARAM_FILENAME, group )
{
    m_Pt_param = ptparam;
}


void PARAM_CFG_FILENAME::ReadParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    wxString prm = aConfig->Read( m_Ident );
    // file names are stored using Unix notation
    // under Window we must use \ instead of /
    // mainly if there is a server name in path (something like \\server\kicad)
#ifdef __WINDOWS__
    prm.Replace( wxT( "/" ), wxT( "\\" ) );
#endif
    *m_Pt_param = prm;
}


void PARAM_CFG_FILENAME::SaveParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    wxString prm = *m_Pt_param;

    // filenames are stored using Unix notation
    prm.Replace( wxT( "\\" ), wxT( "/" ) );
    aConfig->Write( m_Ident, prm );
}


PARAM_CFG_LIBNAME_LIST::PARAM_CFG_LIBNAME_LIST( const wxChar*  ident,
                                                wxArrayString* ptparam,
                                                const wxChar*  group ) :
        PARAM_CFG( ident, PARAM_LIBNAME_LIST, group )
{
    m_Pt_param = ptparam;
}


void PARAM_CFG_LIBNAME_LIST::ReadParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    int            indexlib = 1; // We start indexlib to 1 because first
                                 // lib name is LibName1
    wxString       libname, id_lib;
    wxArrayString* libname_list = m_Pt_param;

    while( 1 )
    {
        id_lib = m_Ident;
        id_lib << indexlib;
        indexlib++;
        libname = aConfig->Read( id_lib, wxT( "" ) );

        if( libname.IsEmpty() )
            break;

        // file names are stored using Unix notation
        // under Window we must use \ instead of /
        // mainly if there is a server name in path (something like \\server\kicad)
#ifdef __WINDOWS__
        libname.Replace( wxT( "/" ), wxT( "\\" ) );
#endif
        libname_list->Add( libname );
    }
}


void PARAM_CFG_LIBNAME_LIST::SaveParam( wxConfigBase* aConfig ) const
{
    if( !m_Pt_param || !aConfig )
        return;

    wxArrayString* libname_list = m_Pt_param;

    wxString       configkey;
    wxString       libname;

    for( unsigned indexlib = 0;  indexlib < libname_list->GetCount();  indexlib++ )
    {
        configkey = m_Ident;

        // We use indexlib+1 because first lib name is LibName1
        configkey << ( indexlib + 1 );
        libname = libname_list->Item( indexlib );

        // filenames are stored using Unix notation
        libname.Replace( wxT( "\\" ), wxT( "/" ) );
        aConfig->Write( configkey, libname );
    }
}
