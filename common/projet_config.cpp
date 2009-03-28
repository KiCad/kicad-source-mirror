/**************************************************/
/* projet_config : routines de trace du cartouche */
/**************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "param_config.h"

#define CONFIG_VERSION 1

#define FORCE_LOCAL_CONFIG true


/*********************************************************************/
static bool ReCreatePrjConfig( const wxString& local_config_filename,
                               const wxString& GroupName,
                               bool            ForceUseLocalConfig )
/*********************************************************************/

/* Cree ou recree la configuration locale de kicad (filename.pro)
 * initialise:
 *     g_Prj_Config
 *     g_Prj_Config_LocalFilename
 *     g_Prj_Default_Config_FullFilename
 * return:
 *     true si config locale
 *     false si default config
 */
{
    // free old config
    if( g_Prj_Config )
        delete g_Prj_Config;
    g_Prj_Config = NULL;

    // Init local Config filename
    if( local_config_filename.IsEmpty() )
        g_Prj_Config_LocalFilename = wxT( "kicad" );
    else
        g_Prj_Config_LocalFilename = local_config_filename;

    ChangeFileNameExt( g_Prj_Config_LocalFilename, g_Prj_Config_Filename_ext );

    // Init local config filename
    if( ForceUseLocalConfig || wxFileExists( g_Prj_Config_LocalFilename ) )
    {
        g_Prj_Default_Config_FullFilename.Empty();
        g_Prj_Config = new wxFileConfig( wxEmptyString,
                                         wxEmptyString,
                                         g_Prj_Config_LocalFilename,
                                         wxEmptyString,
                                         wxCONFIG_USE_RELATIVE_PATH );

        g_Prj_Config->DontCreateOnDemand();

        if( ForceUseLocalConfig )
            return true;

        // Test de la bonne version du fichier (ou groupe) de configuration
        int version = -1, def_version = 0;
        g_Prj_Config->SetPath( GroupName );
        version = g_Prj_Config->Read( wxT( "version" ), def_version );
        g_Prj_Config->SetPath( UNIX_STRING_DIR_SEP );
        if( version > 0 )
            return true;
        else
            delete g_Prj_Config;    // Version incorrecte
    }


    // Fichier local non trouve ou invalide
    g_Prj_Config_LocalFilename.Empty();
    g_Prj_Default_Config_FullFilename =
        ReturnKicadDatasPath() +
        wxT( "template/kicad" ) +
        g_Prj_Config_Filename_ext;

    // Recreate new config
    g_Prj_Config = new wxFileConfig( wxEmptyString,
                                     wxEmptyString,
                                     wxEmptyString,
                                     g_Prj_Default_Config_FullFilename,
                                     wxCONFIG_USE_RELATIVE_PATH );

    g_Prj_Config->DontCreateOnDemand();

    return false;
}


/***************************************************************************************/
void WinEDA_App::WriteProjectConfig( const wxString&  local_config_filename,
                                     const wxString&  GroupName,
                                     PARAM_CFG_BASE** List )
/***************************************************************************************/

/** Function WriteProjectConfig
 *  Save the current "projet" parameters
 *  saved parameters are parameters that have the .m_Setup member set to false
 *  saving file is the .pro file project
 */
{
    PARAM_CFG_BASE* pt_cfg;
    wxString        msg;

    ReCreatePrjConfig( local_config_filename, GroupName,
                       FORCE_LOCAL_CONFIG );

    /* Write date ( surtout pour eviter bug de wxFileConfig
     * qui se trompe de rubrique si declaration [xx] en premiere ligne
     * (en fait si groupe vide) */
    g_Prj_Config->SetPath( UNIX_STRING_DIR_SEP );
    msg = DateAndTime();

    g_Prj_Config->Write( wxT( "update" ), msg );
    msg = GetAppName();

    g_Prj_Config->Write( wxT( "last_client" ), msg );

    /* Save parameters */
    g_Prj_Config->DeleteGroup( GroupName );   // Erase all datas
    g_Prj_Config->Flush();

    g_Prj_Config->SetPath( GroupName );
    g_Prj_Config->Write( wxT( "version" ), CONFIG_VERSION );
    g_Prj_Config->SetPath( UNIX_STRING_DIR_SEP );

    for( ; *List != NULL; List++ )
    {
        pt_cfg = *List;
        if( pt_cfg->m_Group )
            g_Prj_Config->SetPath( pt_cfg->m_Group );
        else
            g_Prj_Config->SetPath( GroupName );

        if( pt_cfg->m_Setup )
            continue;

        if ( pt_cfg->m_Type == PARAM_COMMAND_ERASE )    // Erase all data
        {
            if( pt_cfg->m_Ident )
                g_Prj_Config->DeleteGroup( pt_cfg->m_Ident );
        }
        else
            pt_cfg->SaveParam( g_Prj_Config );
    }

    g_Prj_Config->SetPath( UNIX_STRING_DIR_SEP );
    delete g_Prj_Config;
    g_Prj_Config = NULL;
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


/***************************************************************************************/
bool WinEDA_App::ReadProjectConfig( const wxString&  local_config_filename,
                                    const wxString&  GroupName,
                                    PARAM_CFG_BASE** List,
                                    bool             Load_Only_if_New )
/***************************************************************************************/

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
{
    PARAM_CFG_BASE* pt_cfg;
    wxString        timestamp;

    if( List == NULL )
        return false;

    ReCreatePrjConfig( local_config_filename, GroupName, false );

    g_Prj_Config->SetPath( UNIX_STRING_DIR_SEP );
    timestamp = g_Prj_Config->Read( wxT( "update" ) );
    if( Load_Only_if_New && ( !timestamp.IsEmpty() )
       && (timestamp == wxGetApp().m_CurrentOptionFileDateAndTime) )
    {
        return false;
    }

    wxGetApp().m_CurrentOptionFileDateAndTime = timestamp;

    if( !g_Prj_Default_Config_FullFilename.IsEmpty() )
        wxGetApp().m_CurrentOptionFile = g_Prj_Default_Config_FullFilename;
    else
    {
        if( wxPathOnly( g_Prj_Config_LocalFilename ).IsEmpty() )
            wxGetApp().m_CurrentOptionFile =
                wxGetCwd() + STRING_DIR_SEP + g_Prj_Config_LocalFilename;
        else
            wxGetApp().m_CurrentOptionFile = g_Prj_Config_LocalFilename;
    }

    for( ; *List != NULL; List++ )
    {
        pt_cfg = *List;
        if( pt_cfg->m_Group )
            g_Prj_Config->SetPath( pt_cfg->m_Group );
        else
            g_Prj_Config->SetPath( GroupName );

        if( pt_cfg->m_Setup )
            continue;

        pt_cfg->ReadParam( g_Prj_Config );
    }

    delete g_Prj_Config;
    g_Prj_Config = NULL;

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
 * read the value of parameter thi stored in aConfig
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
 * the the value of parameter thi stored in aConfig
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
 * read the value of parameter thi stored in aConfig
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
 * the the value of parameter thi stored in aConfig
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
 * read the value of parameter thi stored in aConfig
 * @param aConfig = the wxConfigBase that store the parameter
 */
void PARAM_CFG_DOUBLE::ReadParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    double   ftmp = 0;
    wxString msg;
    msg = g_Prj_Config->Read( m_Ident, wxT( "" ) );

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
 * the the value of parameter thi stored in aConfig
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
 * read the value of parameter thi stored in aConfig
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
 * the the value of parameter thi stored in aConfig
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
 * read the value of parameter thi stored in aConfig
 * @param aConfig = the wxConfigBase that store the parameter
 */
void PARAM_CFG_WXSTRING::ReadParam( wxConfigBase* aConfig )
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    *m_Pt_param = aConfig->Read( m_Ident );
}


/** SaveParam
 * the the value of parameter thi stored in aConfig
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
 * read the value of parameter thi stored in aConfig
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
 * the the value of parameter thi stored in aConfig
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
