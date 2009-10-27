/************************************************************/
/** eeconfig.h : configuration: definition des structures  **/
/************************************************************/

#include "param_config.h"

#define GROUP       wxT( "/eeschema" )
#define GROUPCOMMON wxT( "/common" )
#define GROUPLIB    wxT( "libraries" )

#include "netlist.h" /* Definitions generales liees au calcul de netliste */

/* variables importees */
extern int g_PenMinWidth;

/* saving parameters option : */
#define INSETUP TRUE    // used when the parameter is saved in general config
                        // if not used, the parameter is saved in the loca config (project config)
