/**
 * The common library
 * @file param_config.h
 */

#ifndef __PARAM_CONFIG_H__
#define __PARAM_CONFIG_H__ 1

#include "wx/confbase.h"
#include "wx/fileconf.h"
#include <wx/dynarray.h>


/* definifition des types de parametre des files de configuration */
enum paramcfg_id    /* type du parametre dans la structure ParamConfig */
{
    PARAM_INT,
    PARAM_SETCOLOR,
    PARAM_DOUBLE,
    PARAM_BOOL,
    PARAM_LIBNAME_LIST,
    PARAM_WXSTRING,
    PARAM_COMMAND_ERASE
};

#define MAX_COLOR  0x8001F
#define IS_VALID_COLOR( c )  ( ( c >= 0 ) && ( c <= 0x8001F ) )

#define INT_MINVAL 0x80000000
#define INT_MAXVAL 0x7FFFFFFF

class PARAM_CFG_BASE
{
public:
    const wxChar* m_Ident;          /* Keyword in config data */
    paramcfg_id   m_Type;           /* Type of parameter */
    const wxChar* m_Group;          /* Group name (tjis is like a path in the config data) */
    bool          m_Setup;          /* TRUE -> setup parameter (used for all projects), FALSE = parameter relative to a project */

public:
    PARAM_CFG_BASE( const wxChar* ident, const paramcfg_id type, const wxChar* group = NULL );

    /** ReadParam
     * read the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that store the parameter
     */
    virtual void ReadParam( wxConfigBase* aConfig ) {};

    /** SaveParam
     * the the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that can store the parameter
     */
    virtual void SaveParam( wxConfigBase* aConfig ) {};
};

class PARAM_CFG_INT      : public PARAM_CFG_BASE
{
public:
    int* m_Pt_param;                /* pointeur sur le parametre a configurer */
    int  m_Min, m_Max;              /* valeurs extremes du parametre */
    int  m_Default;                 /* valeur par defaut */

public:
    PARAM_CFG_INT( const wxChar* ident, int* ptparam,
                   int default_val = 0, int min = INT_MINVAL, int max = INT_MAXVAL,
                   const wxChar* group = NULL );
    PARAM_CFG_INT( bool Insetup, const wxChar* ident, int* ptparam,
                   int default_val = 0, int min = INT_MINVAL, int max = INT_MAXVAL,
                   const wxChar* group = NULL );

    /** ReadParam
     * read the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that store the parameter
     */
    virtual void ReadParam( wxConfigBase* aConfig );

    /** SaveParam
     * the the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that can store the parameter
     */
    virtual void SaveParam( wxConfigBase* aConfig );
};

class PARAM_CFG_SETCOLOR : public PARAM_CFG_BASE
{
public:
    int* m_Pt_param;                /* pointeur sur le parametre a configurer */
    int  m_Default;                 /* valeur par defaut */

public:
    PARAM_CFG_SETCOLOR( const wxChar* ident, int* ptparam,
                        int default_val, const wxChar* group = NULL );
    PARAM_CFG_SETCOLOR( bool Insetup, const wxChar* ident, int* ptparam,
                        int default_val, const wxChar* group = NULL );

    /** ReadParam
     * read the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that store the parameter
     */
    virtual void ReadParam( wxConfigBase* aConfig );

    /** SaveParam
     * the the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that can store the parameter
     */
    virtual void SaveParam( wxConfigBase* aConfig );
};

class PARAM_CFG_DOUBLE   : public PARAM_CFG_BASE
{
public:
    double* m_Pt_param;                 /* pointeur sur le parametre a configurer */
    double  m_Default;                  /* valeur par defaut */
    double  m_Min, m_Max;               /* valeurs extremes du parametre */

public:
    PARAM_CFG_DOUBLE( const wxChar* ident, double* ptparam,
                      double default_val = 0.0, double min = 0.0, double max = 10000.0,
                      const wxChar* group = NULL );
    PARAM_CFG_DOUBLE( bool Insetup, const wxChar* ident, double* ptparam,
                      double default_val = 0.0, double min = 0.0, double max = 10000.0,
                      const wxChar* group = NULL );

    /** ReadParam
     * read the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that store the parameter
     */
    virtual void ReadParam( wxConfigBase* aConfig );

    /** SaveParam
     * the the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that can store the parameter
     */
    virtual void SaveParam( wxConfigBase* aConfig );
};

class PARAM_CFG_BOOL     : public PARAM_CFG_BASE
{
public:
    bool* m_Pt_param;               /* pointeur sur le parametre a configurer */
    int   m_Default;                /* valeur par defaut */

public:
    PARAM_CFG_BOOL( const wxChar* ident, bool* ptparam,
                    int default_val = FALSE, const wxChar* group = NULL );
    PARAM_CFG_BOOL( bool Insetup, const wxChar* ident, bool* ptparam,
                    int default_val = FALSE, const wxChar* group = NULL );

    /** ReadParam
     * read the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that store the parameter
     */
    virtual void ReadParam( wxConfigBase* aConfig );

    /** SaveParam
     * the the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that can store the parameter
     */
    virtual void SaveParam( wxConfigBase* aConfig );
};


class PARAM_CFG_WXSTRING     : public PARAM_CFG_BASE
{
public:
    wxString* m_Pt_param;              /* pointeur sur le parametre a configurer */

public:
    PARAM_CFG_WXSTRING( const wxChar* ident, wxString* ptparam, const wxChar* group = NULL );
    PARAM_CFG_WXSTRING( bool          Insetup,
                        const wxChar* ident,
                        wxString*     ptparam,
                        const wxChar* group = NULL );
    /** ReadParam
     * read the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that store the parameter
     */
    virtual void ReadParam( wxConfigBase* aConfig );

    /** SaveParam
     * the the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that can store the parameter
     */
    virtual void SaveParam( wxConfigBase* aConfig );
};

class PARAM_CFG_LIBNAME_LIST : public PARAM_CFG_BASE
{
public:
    wxArrayString* m_Pt_param;     /* pointeur sur le parametre a configurer */

public:
    PARAM_CFG_LIBNAME_LIST( const wxChar*  ident,
                            wxArrayString* ptparam,
                            const wxChar*  group = NULL );

    /** ReadParam
     * read the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that store the parameter
     */
    virtual void ReadParam( wxConfigBase* aConfig );

    /** SaveParam
     * the the value of parameter thi stored in aConfig
     * @param aConfig = the wxConfigBase that can store the parameter
     */
    virtual void SaveParam( wxConfigBase* aConfig );
};

WX_DECLARE_OBJARRAY( PARAM_CFG_BASE, PARAM_CFG_ARRAY );

#endif  /* __PARAM_CONFIG_H__ */
