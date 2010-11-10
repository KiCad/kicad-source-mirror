/*****************/
/** eeconfig.h  **/
/*****************/

#include "param_config.h"

#define GROUP       wxT( "/eeschema" )
#define GROUPCOMMON wxT( "/common" )
#define GROUPLIB    wxT( "libraries" )

extern int g_PenMinWidth;

/* saving parameters option : */
#define INSETUP TRUE    /* used when the parameter is saved in general config
                         * if not used, the parameter is saved in the local
                         * config (project config) */
