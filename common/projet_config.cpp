/*********************/
/* projet_config.cpp */
/*********************/

#include <fctsys.h>
#include <gr_basic.h>
#include <appl_wxstruct.h>
#include <common.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <wxstruct.h>
#include <param_config.h>

#include <wx/apptrait.h>
#include <wx/stdpaths.h>

#include <wildcards_and_files_ext.h>

#include <boost/foreach.hpp>


#define CONFIG_VERSION 1
#define FORCE_LOCAL_CONFIG true


bool EDA_APP::ReCreatePrjConfig( const wxString& fileName,
                                 const wxString& GroupName,
                                 bool            ForceUseLocalConfig )
{
    wxFileName fn = fileName;

    // Free old config file.
    if( m_projectSettings )
    {
        delete m_projectSettings;
        m_projectSettings = NULL;
    }

    /* Force the file extension.
     * This allows the user to enter a filename without extension
     * or use an existing name to create the project file
     */
    if( fn.GetExt() != ProjectFileExtension )
    {
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
        m_CurrentOptionFile = fn.GetFullPath();
        m_projectSettings = new wxFileConfig( wxEmptyString, wxEmptyString,
                                              m_CurrentOptionFile, wxEmptyString );
        m_projectSettings->DontCreateOnDemand();

        if( ForceUseLocalConfig )
            return true;

        /* Check the application version against the version saved in the
         * project file.
         *
         * TODO: Push the version test up the stack so that when one of the
         *       KiCad application version changes, the other applications
         *       settings do not get updated.  Practically, this can go away.
         *       It isn't used anywhere as far as I know (WLS).
         */
        int version = -1;
        int def_version = 0;

        m_projectSettings->SetPath( GroupName );
        version = m_projectSettings->Read( wxT( "version" ), def_version );
        m_projectSettings->SetPath( wxCONFIG_PATH_SEPARATOR );

        if( version > 0 )
        {
            return true;
        }
        else
        {
            delete m_projectSettings;    // Version incorrect
        }
    }

    wxString defaultFileName;
    defaultFileName = m_libSearchPaths.FindValidPath( wxT( "kicad.pro" ) );

    if( defaultFileName.IsEmpty() )
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
    m_CurrentOptionFile = fn.GetFullPath();
    m_projectSettings = new wxFileConfig( wxEmptyString, wxEmptyString,
                                          wxEmptyString, fn.GetFullPath() );
    m_projectSettings->DontCreateOnDemand();

    return false;
}


void EDA_APP::WriteProjectConfig( const wxString&  fileName,
                                  const wxString&  GroupName,
                                  const PARAM_CFG_ARRAY& params )
{
    ReCreatePrjConfig( fileName, GroupName, FORCE_LOCAL_CONFIG );

    /* Write date ( surtout pour eviter bug de wxFileConfig
     * qui se trompe de rubrique si declaration [xx] en premiere ligne
     * (en fait si groupe vide) */
    m_projectSettings->SetPath( wxCONFIG_PATH_SEPARATOR );

    m_projectSettings->Write( wxT( "update" ), DateAndTime() );
    m_projectSettings->Write( wxT( "last_client" ), GetAppName() );

    /* Save parameters */
    m_projectSettings->DeleteGroup( GroupName );   // Erase all data
    m_projectSettings->Flush();

    m_projectSettings->SetPath( GroupName );
    m_projectSettings->Write( wxT( "version" ), CONFIG_VERSION );
    m_projectSettings->SetPath( wxCONFIG_PATH_SEPARATOR );

    BOOST_FOREACH( const PARAM_CFG_BASE& param, params )
    {
        if( param.m_Group )
            m_projectSettings->SetPath( param.m_Group );
        else
            m_projectSettings->SetPath( GroupName );

        if( param.m_Setup )
            continue;

        if ( param.m_Type == PARAM_COMMAND_ERASE )    // Erase all data
        {
            if( param.m_Ident )
                m_projectSettings->DeleteGroup( param.m_Ident );
        }
        else
        {
            param.SaveParam( m_projectSettings );
        }
    }

    m_projectSettings->SetPath( UNIX_STRING_DIR_SEP );

    delete m_projectSettings;
    m_projectSettings = NULL;
}

void EDA_APP::SaveCurrentSetupValues( const PARAM_CFG_ARRAY& List )
{
    if( m_settings == NULL )
        return;

    unsigned count = List.size();
    for( unsigned i=0; i<count; ++i )
    {
        const PARAM_CFG_BASE& param = List[i];

        if( param.m_Setup == false )
            continue;

        if( param.m_Type == PARAM_COMMAND_ERASE )       // Erase all data
        {
            if( param.m_Ident )
                m_settings->DeleteGroup( param.m_Ident );
        }
        else
        {
            param.SaveParam( m_settings );
        }
    }
}

bool EDA_APP::ReadProjectConfig( const wxString&  local_config_filename,
                                 const wxString&  GroupName,
                                 const PARAM_CFG_ARRAY& params,
                                 bool             Load_Only_if_New )
{
    ReCreatePrjConfig( local_config_filename, GroupName, false );

    m_projectSettings->SetPath( wxCONFIG_PATH_SEPARATOR );
    wxString timestamp = m_projectSettings->Read( wxT( "update" ) );

    if( Load_Only_if_New && ( !timestamp.IsEmpty() )
       && (timestamp == m_CurrentOptionFileDateAndTime) )
    {
        return false;
    }

    m_CurrentOptionFileDateAndTime = timestamp;

    BOOST_FOREACH( const PARAM_CFG_BASE& param, params )
    {
        if( param.m_Group )
            m_projectSettings->SetPath( param.m_Group );
        else
            m_projectSettings->SetPath( GroupName );

        if( param.m_Setup )
            continue;

        param.ReadParam( m_projectSettings );
    }

    delete m_projectSettings;
    m_projectSettings = NULL;

    return true;
}


void EDA_APP::ReadCurrentSetupValues( const PARAM_CFG_ARRAY& List )
{
    BOOST_FOREACH( const PARAM_CFG_BASE& param, List )
    {
        if( param.m_Setup == false )
            continue;

        param.ReadParam( m_settings );
    }
}


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


void PARAM_CFG_INT::ReadParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    int itmp = aConfig->Read( m_Ident, m_Default );

    if( (itmp < m_Min) || (itmp > m_Max) )
        itmp = m_Default;

    *m_Pt_param = itmp;
}


void PARAM_CFG_INT::SaveParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    aConfig->Write( m_Ident, *m_Pt_param );
}


PARAM_CFG_INT_WITH_SCALE::PARAM_CFG_INT_WITH_SCALE( const wxChar* ident, int* ptparam,
                              int default_val, int min, int max,
                              const wxChar* group, double aBiu2cfgunit ) :
    PARAM_CFG_INT( ident, ptparam, default_val, min, max, group )
{
    m_Type = PARAM_INT_WITH_SCALE;
    m_BIU_to_cfgunit = aBiu2cfgunit;
}


PARAM_CFG_INT_WITH_SCALE::PARAM_CFG_INT_WITH_SCALE( bool Insetup,
                              const wxChar* ident, int* ptparam,
                              int default_val, int min, int max,
                              const wxChar* group, double aBiu2cfgunit ) :
    PARAM_CFG_INT( Insetup, ident, ptparam, default_val, min, max, group )
{
    m_Type = PARAM_INT_WITH_SCALE;
    m_BIU_to_cfgunit = aBiu2cfgunit;
}


void PARAM_CFG_INT_WITH_SCALE::ReadParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    double dtmp = (double) m_Default * m_BIU_to_cfgunit;
    aConfig->Read( m_Ident, &dtmp );

    int itmp = KiROUND( dtmp / m_BIU_to_cfgunit );

    if( (itmp < m_Min) || (itmp > m_Max) )
        itmp = m_Default;

    *m_Pt_param = itmp;
}


void PARAM_CFG_INT_WITH_SCALE::SaveParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    // We cannot use aConfig->Write for a double, because
    // this function uses a format with very few digits in mantissa,
    // and truncature issues are frequent.
    // We uses our function.
    ConfigBaseWriteDouble( aConfig, m_Ident, *m_Pt_param * m_BIU_to_cfgunit );
}


PARAM_CFG_SETCOLOR::PARAM_CFG_SETCOLOR( const wxChar* ident, EDA_COLOR_T* ptparam,
                                        EDA_COLOR_T default_val,
                                        const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_SETCOLOR, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
}


PARAM_CFG_SETCOLOR::PARAM_CFG_SETCOLOR( bool          Insetup,
                                        const wxChar* ident,
                                        EDA_COLOR_T*          ptparam,
                                        EDA_COLOR_T           default_val,
                                        const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_SETCOLOR, group )
{
    m_Pt_param = ptparam;
    m_Default  = default_val;
    m_Setup    = Insetup;
}


void PARAM_CFG_SETCOLOR::ReadParam( wxConfigBase* aConfig ) const
{
    static const int MAX_COLOR = 0x8001F;

    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    EDA_COLOR_T itmp = ColorFromInt( aConfig->Read( m_Ident, m_Default ) );

    if( (itmp < 0) || (itmp > MAX_COLOR) )
        itmp = m_Default;
    *m_Pt_param = itmp;
}


void PARAM_CFG_SETCOLOR::SaveParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    aConfig->Write( m_Ident, (long) *m_Pt_param );
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


void PARAM_CFG_DOUBLE::ReadParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    double dtmp = m_Default;
    aConfig->Read( m_Ident, &dtmp );

    if( (dtmp < m_Min) || (dtmp > m_Max) )
        dtmp = m_Default;

    *m_Pt_param = dtmp;
}


void PARAM_CFG_DOUBLE::SaveParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    // We cannot use aConfig->Write for a double, because
    // this function uses a format with very few digits in mantissa,
    // and truncature issues are frequent.
    // We uses our function.
    ConfigBaseWriteDouble( aConfig, m_Ident, *m_Pt_param );
}


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


void PARAM_CFG_BOOL::ReadParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    int itmp = aConfig->Read( m_Ident, (int) m_Default );

    *m_Pt_param = itmp ? true : false;
}


void PARAM_CFG_BOOL::SaveParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    aConfig->Write( m_Ident, *m_Pt_param );
}


PARAM_CFG_WXSTRING::PARAM_CFG_WXSTRING( const wxChar* ident,
                                        wxString*     ptparam,
                                        const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_WXSTRING, group )
{
    m_Pt_param = ptparam;
}


PARAM_CFG_WXSTRING::PARAM_CFG_WXSTRING( bool Insetup, const wxChar* ident,
                                        wxString* ptparam,
                                        const wxString& default_val,
                                        const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_WXSTRING, group )
{
    m_Pt_param = ptparam;
    m_Setup    = Insetup;
    m_default = default_val;
}


void PARAM_CFG_WXSTRING::ReadParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    *m_Pt_param = aConfig->Read( m_Ident, m_default );
}


void PARAM_CFG_WXSTRING::SaveParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    aConfig->Write( m_Ident, *m_Pt_param );
}



PARAM_CFG_FILENAME::PARAM_CFG_FILENAME( const wxChar* ident,
                                        wxString*     ptparam,
                                        const wxChar* group ) :
    PARAM_CFG_BASE( ident, PARAM_FILENAME, group )
{
    m_Pt_param = ptparam;
}


void PARAM_CFG_FILENAME::ReadParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    wxString prm = aConfig->Read( m_Ident );
    // file names are stored using Unix notation
    // under Window we must use \ instead of /
    // mainly if there is a server name in path (something like \\server\kicad)
#ifdef __WINDOWS__
    prm.Replace(wxT("/"), wxT("\\"));
#endif
    *m_Pt_param = prm;
}


void PARAM_CFG_FILENAME::SaveParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;

    wxString prm = *m_Pt_param;
    // filenames are stored using Unix notation
    prm.Replace(wxT("\\"), wxT("/") );
    aConfig->Write( m_Ident, prm );
}


PARAM_CFG_LIBNAME_LIST::PARAM_CFG_LIBNAME_LIST( const wxChar*  ident,
                                                wxArrayString* ptparam,
                                                const wxChar*  group ) :
    PARAM_CFG_BASE( ident, PARAM_LIBNAME_LIST, group )
{
    m_Pt_param = ptparam;
}


void PARAM_CFG_LIBNAME_LIST::ReadParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
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
        libname.Replace(wxT("/"), wxT("\\"));
#endif
       libname_list->Add( libname );
    }
}


void PARAM_CFG_LIBNAME_LIST::SaveParam( wxConfigBase* aConfig ) const
{
    if( m_Pt_param == NULL || aConfig == NULL )
        return;
    wxArrayString* libname_list = m_Pt_param;

    unsigned       indexlib = 0;
    wxString       configkey;
    wxString       libname;

    for( ; indexlib < libname_list->GetCount(); indexlib++ )
    {
        configkey = m_Ident;

        // We use indexlib+1 because first lib name is LibName1
        configkey << (indexlib + 1);
        libname = libname_list->Item( indexlib );

        // filenames are stored using Unix notation
        libname.Replace(wxT("\\"), wxT("/") );
        aConfig->Write( configkey, libname );
    }
}
