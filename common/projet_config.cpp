/**************************************************/
/* projet_config : routines de trace du cartouche */
/**************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "wxstruct.h"
#include "param_config.h"

#include <wx/apptrait.h>
#include <wx/stdpaths.h>


#define CONFIG_VERSION 1

#define FORCE_LOCAL_CONFIG true


#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY( PARAM_CFG_ARRAY );


/**
 * Cree ou recree la configuration locale de kicad (filename.pro)
 * initialise:
 *     g_Prj_Config
 *     g_Prj_Config_LocalFilename
 *     g_Prj_Default_Config_FullFilename
 * return:
 *     true si config locale
 *     false si default config
 */
bool WinEDA_App::ReCreatePrjConfig( const wxString& fileName,
                                    const wxString& GroupName,
                                    bool            ForceUseLocalConfig )
{
    wxFileName fn = fileName;
    wxString defaultFileName;

    // Free old config file.
    if( m_ProjectConfig )
    {
        delete m_ProjectConfig;
        m_ProjectConfig = NULL;
    }

    /* Check just in case the file name does not a kicad project extension. */
    if( fn.GetExt() != ProjectFileExtension )
    {
        wxLogDebug( _( "ReCreatePrjConfig() called with project file <%s> " \
                       "which does not have the correct file extension." ),
                    fn.GetFullPath().c_str() );
        fn.SetExt( ProjectFileExtension );
    }

    /* Update the library search path list if a new project file is loaded. */
    if( m_projectFileName != fn )
    {
        RemoveLibraryPath( m_projectFileName.GetPath() );
        InsertLibraryPath( fn.GetPath(), 0 );
        m_projectFileName = fn;
    }

    // Init local config filename
    if( ForceUseLocalConfig || fn.FileExists() )
    {
        m_ProjectConfig = new wxFileConfig( wxEmptyString, wxEmptyString,
                                            fn.GetFullPath(), wxEmptyString );
        m_ProjectConfig->DontCreateOnDemand();

        if( ForceUseLocalConfig )
            return true;

        /* Check the application version against the version saved in the
         * project file.
         *
         * TODO: Push the version test up the stack so that when one of the
         *       Kicad application version changes, the other applications
         *       settings do not get updated.  Practically, this can go away.
         *       It isn't used anywhere as far as I know (WLS).
         */
        int version = -1;
        int def_version = 0;

        m_ProjectConfig->SetPath( GroupName );
        version = m_ProjectConfig->Read( wxT( "version" ), def_version );
        m_ProjectConfig->SetPath( wxCONFIG_PATH_SEPARATOR );

        if( version > 0 )
            return true;
        else
        {
            delete m_ProjectConfig;    // Version incorrecte
        }
    }

    defaultFileName = m_libSearchPaths.FindValidPath( wxT( "kicad.pro" ) );

    if( !defaultFileName )
    {
        wxLogDebug( wxT( "Template file <kicad.pro> not found." ) );
        fn = wxFileName( GetTraits()->GetStandardPaths().GetDocumentsDir(),
                         wxT( "kicad" ), ProjectFileExtension );
    }
    else
    {
        fn = defaultFileName;
    }

    // Create new project file using the default name.
    m_ProjectConfig = new wxFileConfig( wxEmptyString, wxEmptyString,
                                        wxEmptyString, fn.GetFullPath() );
    m_ProjectConfig->DontCreateOnDemand();

    return false;
}


/**
 * Function WriteProjectConfig
 *  Save the current "projet" parameters
 *  saved parameters are parameters that have the .m_Setup member set to false
 *  saving file is the .pro file project
 */
void WinEDA_App::WriteProjectConfig( const wxString&  fileName,
                                     const wxString&  GroupName,
                                     PARAM_CFG_BASE** List )
{
    PARAM_CFG_BASE* pt_cfg;
    wxString        msg;

    ReCreatePrjConfig( fileName, GroupName, FORCE_LOCAL_CONFIG );

    /* Write date ( surtout pour eviter bug de wxFileConfig
     * qui se trompe de rubrique si declaration [xx] en premiere ligne
     * (en fait si groupe vide) */
    m_ProjectConfig->SetPath( wxCONFIG_PATH_SEPARATOR );

    msg = DateAndTime();
    m_ProjectConfig->Write( wxT( "update" ), msg );

    msg = GetAppName();
    m_ProjectConfig->Write( wxT( "last_client" ), msg );

    /* Save parameters */
    m_ProjectConfig->DeleteGroup( GroupName );   // Erase all datas
    m_ProjectConfig->Flush();

    m_ProjectConfig->SetPath( GroupName );
    m_ProjectConfig->Write( wxT( "version" ), CONFIG_VERSION );
    m_ProjectConfig->SetPath( wxCONFIG_PATH_SEPARATOR );

    for( ; List != NULL && *List != NULL; List++ )
    {
        pt_cfg = *List;
        if( pt_cfg->m_Group )
            m_ProjectConfig->SetPath( pt_cfg->m_Group );
        else
            m_ProjectConfig->SetPath( GroupName );

        if( pt_cfg->m_Setup )
            continue;

        if ( pt_cfg->m_Type == PARAM_COMMAND_ERASE )    // Erase all data
        {
            if( pt_cfg->m_Ident )
                m_ProjectConfig->DeleteGroup( pt_cfg->m_Ident );
        }
        else
            pt_cfg->SaveParam( m_ProjectConfig );
    }

    m_ProjectConfig->SetPath( UNIX_STRING_DIR_SEP );
    delete m_ProjectConfig;
    m_ProjectConfig = NULL;
}


void WinEDA_App::WriteProjectConfig( const wxString&  fileName,
                                     const wxString&  GroupName,
                                     const PARAM_CFG_ARRAY& params )
{
    PARAM_CFG_BASE* param;
    wxString        msg;
    size_t          i;

    ReCreatePrjConfig( fileName, GroupName, FORCE_LOCAL_CONFIG );

    /* Write date ( surtout pour eviter bug de wxFileConfig
     * qui se trompe de rubrique si declaration [xx] en premiere ligne
     * (en fait si groupe vide) */
    m_ProjectConfig->SetPath( wxCONFIG_PATH_SEPARATOR );

    msg = DateAndTime();
    m_ProjectConfig->Write( wxT( "update" ), msg );

    msg = GetAppName();
    m_ProjectConfig->Write( wxT( "last_client" ), msg );

    /* Save parameters */
    m_ProjectConfig->DeleteGroup( GroupName );   // Erase all datas
    m_ProjectConfig->Flush();

    m_ProjectConfig->SetPath( GroupName );
    m_ProjectConfig->Write( wxT( "version" ), CONFIG_VERSION );
    m_ProjectConfig->SetPath( wxCONFIG_PATH_SEPARATOR );

    for( i = 0; i < params.GetCount(); i++ )
    {
        param = &params[i];
        if( param->m_Group )
            m_ProjectConfig->SetPath( param->m_Group );
        else
            m_ProjectConfig->SetPath( GroupName );

        if( param->m_Setup )
            continue;

        if ( param->m_Type == PARAM_COMMAND_ERASE )    // Erase all data
        {
            if( param->m_Ident )
                m_ProjectConfig->DeleteGroup( param->m_Ident );
        }
        else
            param->SaveParam( m_ProjectConfig );
    }

    m_ProjectConfig->SetPath( UNIX_STRING_DIR_SEP );
    delete m_ProjectConfig;
    m_ProjectConfig = NULL;
}


/*****************************************************************/
void WinEDA_App::SaveCurrentSetupValues( PARAM_CFG_BASE** aList )
/*****************************************************************/

/** Function SaveCurrentSetupValues()
 * Save the current setup values in m_EDA_Config
 * saved parameters are parameters that have the .m_Setup member set to true
 * @param aList = array of PARAM_CFG_BASE pointers
 */
{
    PARAM_CFG_BASE* pt_cfg;
    wxString        msg;

    if( m_EDA_Config == NULL )
        return;

    for( ; *aList != NULL; aList++ )
    {
        pt_cfg = *aList;
        if( pt_cfg->m_Setup == false )
            continue;

        if ( pt_cfg->m_Type == PARAM_COMMAND_ERASE )    // Erase all data
        {
            if( pt_cfg->m_Ident )
                m_EDA_Config->DeleteGroup( pt_cfg->m_Ident );
        }
        else
            pt_cfg->SaveParam( m_EDA_Config );
    }
}


/** Function ReadProjectConfig
 *  Read the current "projet" parameters
 *  Parameters are parameters that have the .m_Setup member set to false
 *  read file is the .pro file project
 *
 * if Load_Only_if_New == true, this file is read only if it diders from
 * the current config (different dates )
 *
 * @return      true if read.
 * Also set:
 *     wxGetApp().m_CurrentOptionFileDateAndTime
 *     wxGetApp().m_CurrentOptionFile
 */
bool WinEDA_App::ReadProjectConfig( const wxString&  local_config_filename,
                                    const wxString&  GroupName,
                                    PARAM_CFG_BASE** List,
                                    bool             Load_Only_if_New )
{
    PARAM_CFG_BASE* pt_cfg;
    wxString        timestamp;

    ReCreatePrjConfig( local_config_filename, GroupName, false );

    m_ProjectConfig->SetPath( wxCONFIG_PATH_SEPARATOR );
    timestamp = m_ProjectConfig->Read( wxT( "update" ) );
    if( Load_Only_if_New && ( !timestamp.IsEmpty() )
       && (timestamp == m_CurrentOptionFileDateAndTime) )
    {
        return false;
    }

    m_CurrentOptionFileDateAndTime = timestamp;

    if( !g_Prj_Default_Config_FullFilename.IsEmpty() )
        m_CurrentOptionFile = g_Prj_Default_Config_FullFilename;
    else
    {
        if( wxPathOnly( g_Prj_Config_LocalFilename ).IsEmpty() )
            m_CurrentOptionFile = wxGetCwd() + STRING_DIR_SEP +
                g_Prj_Config_LocalFilename;
        else
            m_CurrentOptionFile = g_Prj_Config_LocalFilename;
    }

    for( ; List != NULL && *List != NULL; List++ )
    {
        pt_cfg = *List;
        if( pt_cfg->m_Group )
            m_ProjectConfig->SetPath( pt_cfg->m_Group );
        else
            m_ProjectConfig->SetPath( GroupName );

        if( pt_cfg->m_Setup )
            continue;

        pt_cfg->ReadParam( m_ProjectConfig );
    }

    delete m_ProjectConfig;
    m_ProjectConfig = NULL;

    return true;
}


bool WinEDA_App::ReadProjectConfig( const wxString&        local_config_filename,
                                    const wxString&        GroupName,
                                    const PARAM_CFG_ARRAY& params,
                                    bool                   Load_Only_if_New )
{
    size_t          i;
    PARAM_CFG_BASE* param;
    wxString        timestamp;

    ReCreatePrjConfig( local_config_filename, GroupName, false );

    m_ProjectConfig->SetPath( wxCONFIG_PATH_SEPARATOR );
    timestamp = m_ProjectConfig->Read( wxT( "update" ) );
    if( Load_Only_if_New && ( !timestamp.IsEmpty() )
       && (timestamp == m_CurrentOptionFileDateAndTime) )
    {
        return false;
    }

    m_CurrentOptionFileDateAndTime = timestamp;

    if( !g_Prj_Default_Config_FullFilename.IsEmpty() )
        m_CurrentOptionFile = g_Prj_Default_Config_FullFilename;
    else
    {
        if( wxPathOnly( g_Prj_Config_LocalFilename ).IsEmpty() )
            m_CurrentOptionFile = wxGetCwd() + STRING_DIR_SEP +
                g_Prj_Config_LocalFilename;
        else
            m_CurrentOptionFile = g_Prj_Config_LocalFilename;
    }

    for( i = 0; i < params.GetCount(); i++ )
    {
        param = &params[i];

        if( param->m_Group )
            m_ProjectConfig->SetPath( param->m_Group );
        else
            m_ProjectConfig->SetPath( GroupName );

        if( param->m_Setup )
            continue;

        param->ReadParam( m_ProjectConfig );
    }

    delete m_ProjectConfig;
    m_ProjectConfig = NULL;

    return true;
}


/***************************************************************/
void WinEDA_App::ReadCurrentSetupValues( PARAM_CFG_BASE** aList )
/***************************************************************/

/** Function ReadCurrentSetupValues()
 * Raed the current setup values previously saved, from m_EDA_Config
 * saved parameters are parameters that have the .m_Setup member set to true
 * @param aList = array of PARAM_CFG_BASE pointers
 */
{
    PARAM_CFG_BASE* pt_cfg;

    for( ; *aList != NULL; aList++ )
    {
        pt_cfg = *aList;
        if( pt_cfg->m_Setup == false )
            continue;

        pt_cfg->ReadParam( m_EDA_Config );
    }
}


/**************************************************************/
/* Constructeurs des descripteurs de structs de configuration */
/**************************************************************/

PARAM_CFG_BASE::PARAM_CFG_BASE( const wxChar* ident, const paramcfg_id type,
                                const wxChar* group )
{
    m_Ident = ident;
    m_Type  = type;
    m_Group = group;
    m_Setup = false;
}


PARAM_CFG_INT::PARAM_CFG_INT( const wxChar* ident, int* ptparam,
                              int default_val, int min, int max,
                              const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_INT, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Min = min;
    m_Max = max;
}


PARAM_CFG_INT::PARAM_CFG_INT( bool Insetup, const wxChar* ident, int* ptparam,
                              int default_val, int min, int max,
                              const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_INT, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Min   = min;
    m_Max   = max;
    m_Setup = Insetup;
}


/** ReadParam
 * read the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that store the parameter
 */
void PARAM_CFG_INT::ReadParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    int itmp = aConfig->Read( m_Ident, m_Default );

    if( (itmp < m_Min) || (itmp > m_Max) )
        itmp = m_Default;

    *m_Pt_param = itmp;
}


/** SaveParam
 * save the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that can store the parameter
 */
void PARAM_CFG_INT::SaveParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    aConfig->Write( m_Ident, *m_Pt_param );
}


/**************************************************************************/

PARAM_CFG_SETCOLOR::PARAM_CFG_SETCOLOR( const wxChar* ident, int* ptparam,
                                        int default_val,
                                        const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_SETCOLOR, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
}


PARAM_CFG_SETCOLOR::PARAM_CFG_SETCOLOR( bool          Insetup,
                                        const wxChar* ident,
                                        int*          ptparam,
                                        int           default_val,
                                        const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_SETCOLOR, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Setup    = Insetup;
}


/** ReadParam
 * read the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that store the parameter
 */
void PARAM_CFG_SETCOLOR::ReadParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    int itmp = aConfig->Read( m_Ident, m_Default );

    if( (itmp < 0) || (itmp > MAX_COLOR) )
        itmp = m_Default;
    *m_Pt_param = itmp;
}


/** SaveParam
 * save the the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that can store the parameter
 */
void PARAM_CFG_SETCOLOR::SaveParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    aConfig->Write( m_Ident, *m_Pt_param );
}


PARAM_CFG_DOUBLE::PARAM_CFG_DOUBLE( const wxChar* ident, double* ptparam,
                                    double default_val, double min, double max,
                                    const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_DOUBLE, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Min = min;
    m_Max = max;
}


PARAM_CFG_DOUBLE::PARAM_CFG_DOUBLE( bool          Insetup,
                                    const wxChar* ident,
                                    double*       ptparam,
                                    double        default_val,
                                    double        min,
                                    double        max,
                                    const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_DOUBLE, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Min   = min;
    m_Max   = max;
    m_Setup = Insetup;
}


/** ReadParam
 * read the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that store the parameter
 */
void PARAM_CFG_DOUBLE::ReadParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    double   ftmp = 0;
    wxString msg;
    msg = aConfig->Read( m_Ident, wxT( "" ) );

    if( msg.IsEmpty() )
        ftmp = m_Default;
    else
    {
        msg.ToDouble( &ftmp );
        if( (ftmp < m_Min) || (ftmp > m_Max) )
            ftmp = m_Default;
    }
    *m_Pt_param = ftmp;
}


/** SaveParam
 * save the the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that can store the parameter
 */
void PARAM_CFG_DOUBLE::SaveParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    aConfig->Write( m_Ident, *m_Pt_param );
}


/***********************************************************************/

PARAM_CFG_BOOL::PARAM_CFG_BOOL( const wxChar* ident, bool* ptparam,
                                int default_val, const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_BOOL, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val ? true : false;
}


PARAM_CFG_BOOL::PARAM_CFG_BOOL( bool          Insetup,
                                const wxChar* ident,
                                bool*         ptparam,
                                int           default_val,
                                const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_BOOL, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val ? true : false;
    m_Setup    = Insetup;
}


/** ReadParam
 * read the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that store the parameter
 */
void PARAM_CFG_BOOL::ReadParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    int itmp = aConfig->Read( m_Ident, (int) m_Default );

    *m_Pt_param = itmp ? true : false;
}


/** SaveParam
 * save the the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that can store the parameter
 */
void PARAM_CFG_BOOL::SaveParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    aConfig->Write( m_Ident, *m_Pt_param );
}


/*********************************************************************/
PARAM_CFG_WXSTRING::PARAM_CFG_WXSTRING( const wxChar* ident,
                                        wxString*     ptparam,
                                        const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_WXSTRING, group )
{
    m_Pt_param = ptparam;
}


PARAM_CFG_WXSTRING::PARAM_CFG_WXSTRING( bool Insetup, const wxChar* ident,
                                        wxString* ptparam,
                                        const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_WXSTRING, group )
{
    m_Pt_param = ptparam;
    m_Setup    = Insetup;
}


/** ReadParam
 * read the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that store the parameter
 */
void PARAM_CFG_WXSTRING::ReadParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    *m_Pt_param = aConfig->Read( m_Ident );
}


/** SaveParam
 * save the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that can store the parameter
 */
void PARAM_CFG_WXSTRING::SaveParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    aConfig->Write( m_Ident, *m_Pt_param );
}


/***************************************************************************/
PARAM_CFG_LIBNAME_LIST::PARAM_CFG_LIBNAME_LIST( const wxChar*  ident,
                                                wxArrayString* ptparam,
                                                const wxChar*  group ) :
    PARAM_CFG_BASE( ident, PARAM_LIBNAME_LIST, group )
{
    m_Pt_param = ptparam;
}


/** ReadParam
 * read the value of parameter this stored in aConfig
 * @param aConfig = the wxConfigBase that store the parameter
 */
void PARAM_CFG_LIBNAME_LIST::ReadParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    int            indexlib = 1; // We start indexlib to 1 because first lib name is LibName1
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
        libname_list->Add( libname );
    }
}


/** SaveParam
 * save the value of parameter this in aConfig (list of parameters)
 * @param aConfig = the wxConfigBase that can store the parameter
 */
void PARAM_CFG_LIBNAME_LIST::SaveParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    wxArrayString* libname_list = m_Pt_param;

    unsigned       indexlib = 0;
    wxString       cle_config;
    for( ; indexlib < libname_list->GetCount(); indexlib++ )
    {
        cle_config = m_Ident;

        // We use indexlib+1 because first lib name is LibName1
        cle_config << (indexlib + 1);
        aConfig->Write( cle_config, libname_list->Item( indexlib ) );
    }
}
