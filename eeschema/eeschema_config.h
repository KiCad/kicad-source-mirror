/**
 * @file eeschema_config.h
 */

#ifndef EESCHEMA_CONFIG_H
#define EESCHEMA_CONFIG_H

#include <config_params.h>

// a key to read write in user config the visibility of the rescue library dialog
#define RESCUE_NEVER_SHOW_KEY wxT("RescueNeverShow")

// define autoplace key here to avoid having to take the long trip to get at the SCH_EDIT_FRAME
#define AUTOPLACE_JUSTIFY_KEY wxT("AutoplaceJustify")
#define AUTOPLACE_ALIGN_KEY wxT("AutoplaceAlign")

#endif      // EESCHEMA_CONFIG_H
