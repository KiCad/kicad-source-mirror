/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
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

#ifndef CONFIG_PARAMS_H_
#define CONFIG_PARAMS_H_

/**
 * The common library
 * @file config_params.h
 */

#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include <gal/color4d.h>
#include <limits>

using KIGFX::COLOR4D;

/// Names of sub sections where to store project info in *.pro project config files
#define GROUP_PCB           wxT( "/pcbnew" )            /// parameters for Pcbnew/Modedit
#define GROUP_SCH           wxT( "/eeschema" )          /// library list and lib paths list
#define GROUP_SCH_EDIT      wxT( "/schematic_editor" )  /// parameters for schematic editor
                                                        /// (and few for component editor).
                                                        /// Does not store libs list
#define GROUP_PCB_LIBS      wxT( "/pcbnew/libraries" )  /// PCB library list, should be removed soon
                                                        /// (Now in fp lib tables)
#define GROUP_SCH_LIBS      wxT( "/eeschema/libraries" )    /// library list section

#define GROUP_CVP           wxT("/cvpcb")
#define GROUP_CVP_EQU       wxT("/cvpcb/equfiles")


#define CONFIG_VERSION      1


/**
 * Function ConfigBaseWriteDouble
 * This is a helper function to write doubles in config
 * We cannot use wxConfigBase->Write for a double, because
 * this function uses a format with very few digits in mantissa,
 * and truncation issues are frequent.
 * We use here a better floating format.
 */
void ConfigBaseWriteDouble( wxConfigBase* aConfig, const wxString& aKey, double aValue );


/** Type of parameter in the configuration file */
enum paramcfg_id {
    PARAM_INT,
    PARAM_INT_WITH_SCALE,
    PARAM_SETCOLOR,
    PARAM_DOUBLE,
    PARAM_BOOL,
    PARAM_LIBNAME_LIST,
    PARAM_WXSTRING,
    PARAM_FILENAME,
    PARAM_COMMAND_ERASE,
    PARAM_FIELDNAME_LIST,
    PARAM_LAYERS,
    PARAM_TRACKWIDTHS,
    PARAM_VIADIMENSIONS,
    PARAM_DIFFPAIRDIMENSIONS,
    PARAM_NETCLASSES
};


/**
 * Class PARAM_CFG_BASE
 * is a base class which establishes the interface functions ReadParam and SaveParam,
 * which are implemented by a number of derived classes, and these function's
 * doxygen comments are inherited also.
 * <p>
 * See kicad.odt or kicad.pdf, chapter 2 :
 * "Installation and configuration/Initialization of the default config".
 */
class PARAM_CFG_BASE
{
public:
    wxString    m_Ident;  ///<  Keyword in config data
    paramcfg_id m_Type;   ///<  Type of parameter
    wxString    m_Group;  ///<  Group name (this is like a path in the config data)
    bool        m_Setup;  ///<  Install or Project based parameter, true == install

    // If the m_Ident keyword isn't found, fall back and read values from m_Ident_legacy.
    // Note that values are always written to the current, non-legacy keyword.
    wxString    m_Ident_legacy;

public:
    PARAM_CFG_BASE( const wxString& ident, const paramcfg_id type, const wxChar* group = NULL,
                    const wxString& legacy_ident = wxEmptyString );
    virtual ~PARAM_CFG_BASE() {}

    /**
     * Function ReadParam
     * reads the value of the parameter stored in aConfig
     * @param aConfig = the wxConfigBase that holds the parameter
     */
    virtual void ReadParam( wxConfigBase* aConfig ) const {};

    /**
     * Function SaveParam
     * saves the value of the parameter stored in aConfig
     * @param aConfig = the wxConfigBase that can store the parameter
     */
    virtual void SaveParam( wxConfigBase* aConfig ) const {};
};


/**
 * Configuration parameter - Integer Class
 *
 */
class PARAM_CFG_INT      : public PARAM_CFG_BASE
{
public:
    int* m_Pt_param;    ///<  Pointer to the parameter value
    int  m_Min, m_Max;  ///<  Minimum and maximum values of the param type
    int  m_Default;     ///<  The default value of the parameter

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
};

/**
 * Configuration parameter - Integer Class
 * with unit conversion.
 * Mainly used to store an integer value in millimeters (or inches)
 * and retrieve it in internal units
 * the stored value is a floating number
 */
class PARAM_CFG_INT_WITH_SCALE : public PARAM_CFG_INT
{
public:
    double   m_BIU_to_cfgunit;   ///<  the factor to convert the saved value in internal value

public:
    PARAM_CFG_INT_WITH_SCALE( const wxString& ident, int* ptparam, int default_val = 0,
                              int min = std::numeric_limits<int>::min(),
                              int max = std::numeric_limits<int>::max(),
                              const wxChar* group = NULL, double aBiu2cfgunit = 1.0,
                              const wxString& legacy_ident = wxEmptyString );
    PARAM_CFG_INT_WITH_SCALE( bool insetup, const wxString& ident, int* ptparam,
                              int default_val = 0,
                              int min = std::numeric_limits<int>::min(),
                              int max = std::numeric_limits<int>::max(),
                              const wxChar* group = NULL, double aBiu2cfgunit = 1.0,
                              const wxString& legacy_ident = wxEmptyString );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;
};


/**
 * Configuration parameter - SetColor Class
 *
 */
class PARAM_CFG_SETCOLOR : public PARAM_CFG_BASE
{
public:
    COLOR4D* m_Pt_param;    ///<  Pointer to the parameter value
    COLOR4D  m_Default;     ///<  The default value of the parameter

public:
    PARAM_CFG_SETCOLOR( const wxString& ident, COLOR4D* ptparam,
                        COLOR4D default_val, const wxChar* group = NULL );
    PARAM_CFG_SETCOLOR( bool Insetup, const wxString& ident, COLOR4D* ptparam,
                        COLOR4D default_val, const wxChar* group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;
};


/**
 * Configuration parameter - Double Precision Class
 *
 */
class PARAM_CFG_DOUBLE   : public PARAM_CFG_BASE
{
public:
    double* m_Pt_param;     ///<  Pointer to the parameter value
    double  m_Default;      ///<  The default value of the parameter
    double  m_Min, m_Max;   ///<  Minimum and maximum values of the param type

public:
    PARAM_CFG_DOUBLE( const wxString& ident, double* ptparam,
                          double default_val = 0.0, double min = 0.0, double max = 10000.0,
                          const wxChar* group = NULL );
    PARAM_CFG_DOUBLE( bool Insetup, const wxString& ident, double* ptparam,
                      double default_val = 0.0, double min = 0.0, double max = 10000.0,
                      const wxChar* group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;
};


/**
 * Configuration parameter - Boolean Class
 *
 */
class PARAM_CFG_BOOL     : public PARAM_CFG_BASE
{
public:
    bool* m_Pt_param;       ///<  Pointer to the parameter value
    int   m_Default;        ///<  The default value of the parameter

public:
    PARAM_CFG_BOOL( const wxString& ident, bool* ptparam,
                    int default_val = false, const wxChar* group = NULL,
                    const wxString& legacy_ident = wxEmptyString );
    PARAM_CFG_BOOL( bool Insetup, const wxString& ident, bool* ptparam,
                    int default_val = false, const wxChar* group = NULL,
                    const wxString& legacy_ident = wxEmptyString );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;
};


/**
 * Configuration parameter - wxString Class
 *
 */
class PARAM_CFG_WXSTRING : public PARAM_CFG_BASE
{
public:
    wxString* m_Pt_param;       ///<  Pointer to the parameter value
    wxString  m_default;        ///<  The default value of the parameter

public:
    PARAM_CFG_WXSTRING( const wxString& ident, wxString* ptparam, const wxChar* group = NULL );

    PARAM_CFG_WXSTRING( bool            Insetup,
                        const wxString& ident,
                        wxString*       ptparam,
                        const wxString& default_val = wxEmptyString,
                        const wxChar* group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;
};


/**
 * Configuration parameter - PARAM_CFG_FILENAME Class
 * Same as PARAM_CFG_WXSTRING, but stores "\" as "/".
 * and replace "/" by "\" under Windows.
 * Used to store paths and filenames in config files
 */
class PARAM_CFG_FILENAME     : public PARAM_CFG_BASE
{
public:
    wxString* m_Pt_param;    ///<  Pointer to the parameter value

public:
    PARAM_CFG_FILENAME( const wxString& ident, wxString* ptparam,
            const wxChar* group = NULL );
    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;
};


class PARAM_CFG_LIBNAME_LIST : public PARAM_CFG_BASE
{
public:
    wxArrayString* m_Pt_param;     ///<  Pointer to the parameter value

public:
    PARAM_CFG_LIBNAME_LIST( const wxChar*  ident,
                                wxArrayString* ptparam,
                                const wxChar*  group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const override;
    virtual void SaveParam( wxConfigBase* aConfig ) const override;
};


/** A list of parameters type */
//typedef boost::ptr_vector<PARAM_CFG_BASE> PARAM_CFG_ARRAY;
class  PARAM_CFG_ARRAY : public boost::ptr_vector<PARAM_CFG_BASE>
{
};


/**
 * Function wxConfigSaveSetups
 * writes @a aList of PARAM_CFG_ARRAY elements to save configuration values
 * to @a aCfg.  Only elements with m_Setup set true will be saved, hence the
 * function name.
 *
 * @param aCfg where to save
 * @param aList holds some configuration parameters, not all of which will
 *  necessarily be saved.
 */
void wxConfigSaveSetups( wxConfigBase* aCfg, const PARAM_CFG_ARRAY& aList );

/**
 * Function wxConfigSaveParams
 * writes @a aList of PARAM_CFG_ARRAY elements to save configuration values
 * to @a aCfg.  Only elements with m_Setup set false will be saved, hence the
 * function name.
 *
 * @param aCfg where to save
 * @param aList holds some configuration parameters, not all of which will
 *  necessarily be saved.
 * @param aGroup indicates in which group the value should be saved,
 *  unless the PARAM_CFG_ARRAY element provides its own group, in which case it will
 *  take precedence.  aGroup may be empty.
 */
void wxConfigSaveParams( wxConfigBase* aCfg,
            const PARAM_CFG_ARRAY& aList, const wxString& aGroup );

/**
 * Function wxConfigLoadSetups
 * uses @a aList of PARAM_CFG_ARRAY elements to load configuration values
 * from @a aCfg.  Only elements whose m_Setup field is true will be loaded.
 *
 * @param aCfg where to load from.
 * @param aList holds some configuration parameters, not all of which will
 *  necessarily be loaded.
 */
void wxConfigLoadSetups( wxConfigBase* aCfg, const PARAM_CFG_ARRAY& aList );

/**
 * Function wxConfigLoadParams
 * uses @a aList of PARAM_CFG_ARRAY elements to load configuration values
 * from @a aCfg.  Only elements whose m_Setup field is false will be loaded.
 *
 * @param aCfg where to load from.
 *
 * @param aList holds some configuration parameters, not all of which will
 *  necessarily be loaded.
 *
 * @param aGroup indicates in which group the value should be saved,
 *  unless the PARAM_CFG_ARRAY element provides its own group, in which case it will
 *  take precedence.  aGroup may be empty.
 */
void wxConfigLoadParams( wxConfigBase* aCfg,
        const PARAM_CFG_ARRAY& aList, const wxString& aGroup );


#endif  // CONFIG_PARAMS_H_
