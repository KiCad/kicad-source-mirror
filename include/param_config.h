/**
 * The common library
 * @file param_config.h
 */

#ifndef PARAM_CONFIG_H_
#define PARAM_CONFIG_H_

#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <boost/ptr_container/ptr_vector.hpp>


/** Type of parameter in the configuration file */
enum paramcfg_id {
    PARAM_INT,
    PARAM_SETCOLOR,
    PARAM_DOUBLE,
    PARAM_BOOL,
    PARAM_LIBNAME_LIST,
    PARAM_WXSTRING,
    PARAM_FILENAME,
    PARAM_COMMAND_ERASE,
    PARAM_FIELDNAME_LIST
};

#define MAX_COLOR 0x8001F
#define IS_VALID_COLOR( c ) ( ( c >= 0 ) && ( c <= 0x8001F ) )

#define INT_MINVAL 0x80000000
#define INT_MAXVAL 0x7FFFFFFF


/**
 * Class PARAM_CFG_BASE
 * is a base class which establishes the virtual functions ReadParam and SaveParam,
 * which are re-implemented by a number of base classes, and these function's
 * doxygen comments are inherited also.
 */
class PARAM_CFG_BASE
{
public:
    const wxChar* m_Ident;  ///<  Keyword in config data
    paramcfg_id   m_Type;   ///<  Type of parameter
    const wxChar* m_Group;  ///<  Group name (this is like a path in the config data)
    bool          m_Setup;  ///<  Install or Project based parameter, true == install

public:
    PARAM_CFG_BASE( const wxChar* ident, const paramcfg_id type, const wxChar* group = NULL );
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
    PARAM_CFG_INT( const wxChar* ident, int* ptparam,
                       int default_val = 0, int min = INT_MINVAL, int max = INT_MAXVAL,
                       const wxChar* group = NULL );
    PARAM_CFG_INT( bool Insetup, const wxChar* ident, int* ptparam,
                   int default_val = 0, int min = INT_MINVAL, int max = INT_MAXVAL,
                   const wxChar* group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const;
    virtual void SaveParam( wxConfigBase* aConfig ) const;
};


/**
 * Configuration parameter - SetColor Class
 *
 */
class PARAM_CFG_SETCOLOR : public PARAM_CFG_BASE
{
public:
    int* m_Pt_param;    ///<  Pointer to the parameter value
    int  m_Default;     ///<  The default value of the parameter

public:
    PARAM_CFG_SETCOLOR( const wxChar* ident, int* ptparam,
                            int default_val, const wxChar* group = NULL );
    PARAM_CFG_SETCOLOR( bool Insetup, const wxChar* ident, int* ptparam,
                        int default_val, const wxChar* group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const;
    virtual void SaveParam( wxConfigBase* aConfig ) const;
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
    PARAM_CFG_DOUBLE( const wxChar* ident, double* ptparam,
                          double default_val = 0.0, double min = 0.0, double max = 10000.0,
                          const wxChar* group = NULL );
    PARAM_CFG_DOUBLE( bool Insetup, const wxChar* ident, double* ptparam,
                      double default_val = 0.0, double min = 0.0, double max = 10000.0,
                      const wxChar* group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const;
    virtual void SaveParam( wxConfigBase* aConfig ) const;
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
    PARAM_CFG_BOOL( const wxChar* ident, bool* ptparam,
                        int default_val = false, const wxChar* group = NULL );
    PARAM_CFG_BOOL( bool Insetup, const wxChar* ident, bool* ptparam,
                    int default_val = false, const wxChar* group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const;
    virtual void SaveParam( wxConfigBase* aConfig ) const;
};


/**
 * Configuration parameter - wxString Class
 *
 */
class PARAM_CFG_WXSTRING     : public PARAM_CFG_BASE
{
public:
    wxString* m_Pt_param;       ///<  Pointer to the parameter value
    wxString  m_default;        ///<  The default value of the parameter

public:
    PARAM_CFG_WXSTRING( const wxChar* ident, wxString* ptparam, const wxChar* group = NULL );
    PARAM_CFG_WXSTRING( bool            Insetup,
                        const wxChar*   ident,
                        wxString*       ptparam,
                        const wxString& default_val = wxEmptyString,
                        const wxChar*   group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const;
    virtual void SaveParam( wxConfigBase* aConfig ) const;
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
    PARAM_CFG_FILENAME( const wxChar* ident, wxString* ptparam, const wxChar* group = NULL );
    virtual void ReadParam( wxConfigBase* aConfig ) const;
    virtual void SaveParam( wxConfigBase* aConfig ) const;
};


class PARAM_CFG_LIBNAME_LIST : public PARAM_CFG_BASE
{
public:
    wxArrayString* m_Pt_param;     ///<  Pointer to the parameter value

public:
    PARAM_CFG_LIBNAME_LIST( const wxChar*  ident,
                                wxArrayString* ptparam,
                                const wxChar*  group = NULL );

    virtual void ReadParam( wxConfigBase* aConfig ) const;
    virtual void SaveParam( wxConfigBase* aConfig ) const;
};


/** A list of parameters type */
typedef boost::ptr_vector<PARAM_CFG_BASE> PARAM_CFG_ARRAY;

#endif  // PARAM_CONFIG_H_
