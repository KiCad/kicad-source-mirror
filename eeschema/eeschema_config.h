/**
 * @file eeschema_config.h
 */

#include <param_config.h>

#define GROUP       wxT( "/eeschema" )
#define GROUPCOMMON wxT( "/common" )
#define GROUPLIB    wxT( "libraries" )

/**
 * The width given to bus drawings that do not have a specific width
 */
extern int g_DefaultBusWidth;

#if defined(KICAD_GOST)

/** In a GOST build, the bus width is set and cannot be changed */
#define GOST_BUS_WIDTH      22

#endif

/* saving parameters option : */
#define INSETUP true    /* used when the parameter is saved in general config
                         * if not used, the parameter is saved in the local
                         * config (project config) */
