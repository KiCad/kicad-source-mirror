/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
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

#ifndef CONFIG_PARAMS_H_
#define CONFIG_PARAMS_H_

#include <kicommon.h>
#include <set>
#include <limits>

#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <boost/ptr_container/ptr_vector.hpp>


/**
 * A helper function to write doubles in configuration file.
 *
 * We cannot use wxConfigBase->Write for a double, because this function uses a format with
 * very few digits in mantissa and truncation issues are frequent.  We use here a better
 * floating format.
 *
 * @note Everything in this file is deprecated, it only remains because advanced_config depends on
 * it for the moment.
 */
KICOMMON_API void ConfigBaseWriteDouble( wxConfigBase* aConfig, const wxString& aKey, double aValue );


/** Type of parameter in the configuration file */
enum paramcfg_id {
    PARAM_INT,
    PARAM_INT_WITH_SCALE,
    PARAM_DOUBLE,
    PARAM_BOOL,
    PARAM_LIBNAME_LIST,
    PARAM_WXSTRING,
    PARAM_WXSTRING_SET,
    PARAM_FILENAME,
    PARAM_COMMAND_ERASE,
    PARAM_LAYERS,
    PARAM_TRACKWIDTHS,
    PARAM_VIADIMENSIONS,
    PARAM_DIFFPAIRDIMENSIONS,
    PARAM_NETCLASSES,
    PARAM_SEVERITIES
};


/**
 * A base class which establishes the interface functions ReadParam and SaveParam,
 * which are implemented by a number of derived classes, and these function's
 * doxygen comments are inherited also.
 *
 * See kicad.odt or kicad.pdf, chapter 2 :
 * "Installation and configuration/Initialization of the default config".
 */
class KICOMMON_API PARAM_CFG
{
public:
    PARAM_CFG( const wxString& ident, const paramcfg_id type, const wxChar* group = nullptr,
               const wxString& legacy_ident = wxEmptyString );
    virtual ~PARAM_CFG() {}

    /**
     * Read the value of the parameter stored in \a aConfig.
     *
     * @param aConfig the wxConfigBase that holds the parameter.
     */
    virtual void ReadParam( wxConfigBase* aConfig ) const {};

    /**
     * Save the value of the parameter stored in \a aConfig.
     *
     * @param aConfig the wxConfigBase that can store the parameter.
     */
    virtual void SaveParam( wxConfigBase* aConfig ) const {};

    wxString    m_Ident;  ///<  Keyword in config data
    paramcfg_id m_Type;   ///<  Type of parameter
    wxString    m_Group;  ///<  Group name (this is like a path in the config data)
    bool        m_Setup;  ///<  Install or Project based parameter, true == install

    // If the m_Ident keyword isn't found, fall back and read values from m_Ident_legacy.
    // Note that values are always written to the current, non-legacy keyword.
    wxString    m_Ident_legacy;
};


/**
 * Configuration object for integers.
 */
class KICOMMON_API PARAM_CFG_INT : public PARAM_CFG
{
public:
    PARAM_CFG_INT( const wxString& ident, int* ptparam, int default_val = 0,
                   int min = std::numeric_limits<int>::min(),
                   int max = std::numeric_limits<int>::max(),
                   const wxChar* group = nullptr,
                   const wxString& legacy_ident = wxEmptyString );
    PARAM_CFG_INT( bool Insetup, const wxString& ident, int* ptparam, int default_val = 0,
                   int min = std::numeric_limits<int>::min(),
                   int max = std::numeric_limits<int>::max(),
                   const wxChar* group = nullptr,
                   const wxString& legacy_ident = wxEmptyString );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;

    int* m_Pt_param;    ///<  Pointer to the parameter value
    int  m_Min, m_Max;  ///<  Minimum and maximum values of the param type
    int  m_Default;     ///<  The default value of the parameter
};

/**
 * Configuration for integers with unit conversion.
 *
 * Mainly used to store an integer value in millimeters (or inches) and retrieve it in
 * internal units.  The stored value is a floating number.
 */
class KICOMMON_API PARAM_CFG_INT_WITH_SCALE : public PARAM_CFG_INT
{
public:
    PARAM_CFG_INT_WITH_SCALE( const wxString& ident, int* ptparam, int default_val = 0,
                              int min = std::numeric_limits<int>::min(),
                              int max = std::numeric_limits<int>::max(),
                              const wxChar* group = nullptr, double aBiu2cfgunit = 1.0,
                              const wxString& legacy_ident = wxEmptyString );
    PARAM_CFG_INT_WITH_SCALE( bool insetup, const wxString& ident, int* ptparam,
                              int default_val = 0,
                              int min = std::numeric_limits<int>::min(),
                              int max = std::numeric_limits<int>::max(),
                              const wxChar* group = nullptr, double aBiu2cfgunit = 1.0,
                              const wxString& legacy_ident = wxEmptyString );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;

public:
    double   m_BIU_to_cfgunit;   ///<  the factor to convert the saved value in internal value
};


/**
 * Configuration object for double precision floating point numbers.
 */
class KICOMMON_API PARAM_CFG_DOUBLE : public PARAM_CFG
{
public:
    PARAM_CFG_DOUBLE( const wxString& ident, double* ptparam,
                      double default_val = 0.0, double min = 0.0, double max = 10000.0,
                      const wxChar* group = nullptr );
    PARAM_CFG_DOUBLE( bool Insetup, const wxString& ident, double* ptparam,
                      double default_val = 0.0, double min = 0.0, double max = 10000.0,
                      const wxChar* group = nullptr );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;

    double* m_Pt_param;     ///<  Pointer to the parameter value
    double  m_Default;      ///<  The default value of the parameter
    double  m_Min, m_Max;   ///<  Minimum and maximum values of the param type
};


/**
 * Configuration object for booleans.
 */
class KICOMMON_API PARAM_CFG_BOOL : public PARAM_CFG
{
public:
    PARAM_CFG_BOOL( const wxString& ident, bool* ptparam,
                    int default_val = false, const wxChar* group = nullptr,
                    const wxString& legacy_ident = wxEmptyString );
    PARAM_CFG_BOOL( bool Insetup, const wxString& ident, bool* ptparam,
                    int default_val = false, const wxChar* group = nullptr,
                    const wxString& legacy_ident = wxEmptyString );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;

    bool* m_Pt_param;       ///<  Pointer to the parameter value
    int   m_Default;        ///<  The default value of the parameter
};


/**
 * Configuration object for wxString objects.
 */
class KICOMMON_API PARAM_CFG_WXSTRING : public PARAM_CFG
{
public:
    PARAM_CFG_WXSTRING( const wxString& ident, wxString* ptparam, const wxChar* group = nullptr );

    PARAM_CFG_WXSTRING( bool            Insetup,
                        const wxString& ident,
                        wxString*       ptparam,
                        const wxString& default_val = wxEmptyString,
                        const wxChar* group = nullptr );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;

    wxString* m_Pt_param;       ///<  Pointer to the parameter value
    wxString  m_default;        ///<  The default value of the parameter
};


/**
 * Configuration object for a set of wxString objects.
 *
 */
class KICOMMON_API PARAM_CFG_WXSTRING_SET : public PARAM_CFG
{
public:
    PARAM_CFG_WXSTRING_SET( const wxString& ident, std::set<wxString>* ptparam,
                            const wxChar* group = nullptr );

    PARAM_CFG_WXSTRING_SET( bool                Insetup,
                            const wxString&     ident,
                            std::set<wxString>* ptparam,
                            const wxChar* group = nullptr );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;

    std::set<wxString>* m_Pt_param;       ///<  Pointer to the parameter value
};


/**
 * Configuration object for a file name object.
 *
 * Same as #PARAM_CFG_WXSTRING but stores "\" as "/" and replace "/" by "\" under Windows.
 */
class KICOMMON_API PARAM_CFG_FILENAME : public PARAM_CFG
{
public:
    PARAM_CFG_FILENAME( const wxString& ident, wxString* ptparam,
            const wxChar* group = nullptr );
    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;

    wxString* m_Pt_param;    ///<  Pointer to the parameter value
};


class KICOMMON_API PARAM_CFG_LIBNAME_LIST : public PARAM_CFG
{
public:
    wxArrayString* m_Pt_param;     ///<  Pointer to the parameter value

public:
    PARAM_CFG_LIBNAME_LIST( const wxChar* ident, wxArrayString* ptparam,
                            const wxChar* group = nullptr );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;
};


/**
 * Writes @a aList of #PARAM_CFG objects to @a aCfg.
 *
 * Only elements with m_Setup set true will be saved, hence the function name.
 *
 * @param aCfg where to save.
 * @param aList holds some configuration parameters, not all of which will necessarily be saved.
 */
KICOMMON_API void wxConfigSaveSetups( wxConfigBase* aCfg, const std::vector<std::unique_ptr<PARAM_CFG>>& aList );

/**
 * Write @a aList of #PARAM_CFG objects @a aCfg.
 *
 * Only elements with m_Setup set false will be saved, hence the function name.
 *
 * @param aCfg where to save.
 * @param aList holds some configuration parameters, not all of which will necessarily be saved.
 * @param aGroup indicates in which group the value should be saved, unless the PARAM_CFG provides
 *               its own group, in which case it will take precedence.  aGroup may be empty.
 */
KICOMMON_API void wxConfigSaveParams( wxConfigBase* aCfg, const std::vector<std::unique_ptr<PARAM_CFG>>& aList,
                                      const wxString& aGroup );

/**
 * Use @a aList of #PARAM_CFG object to load configuration values from @a aCfg.
 *
 * Only elements whose m_Setup field is true will be loaded.
 *
 * @param aCfg where to load from.
 * @param aList holds some configuration parameters, not all of which will necessarily be loaded.
 */
KICOMMON_API void wxConfigLoadSetups( wxConfigBase* aCfg, const std::vector<std::unique_ptr<PARAM_CFG>>& aList );

/**
 * Use @a aList of #PARAM_CFG objects to load configuration values from @a aCfg.
 * Only elements whose m_Setup field is false will be loaded.
 *
 * @param aCfg where to load from.
 * @param aList holds some configuration parameters, not all of which will necessarily be loaded.
 * @param aGroup indicates in which group the value should be saved, unless the PARAM_CFG provides
 *               its own group, in which case it will take precedence.  aGroup may be empty.
 */
KICOMMON_API void wxConfigLoadParams( wxConfigBase* aCfg, const std::vector<std::unique_ptr<PARAM_CFG>>& aList,
                                      const wxString& aGroup );


#endif  // CONFIG_PARAMS_H_
