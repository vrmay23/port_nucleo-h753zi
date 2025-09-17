#ifndef __APPS_NSHLIB_NSH_NUTTPID_H
#define __APPS_NSHLIB_NSH_NUTTPID_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_NSH_NUTTPID
int nsh_nuttpid_main(int argc, char *argv[]);
#endif

#endif /* __APPS_NSHLIB_NSH_NUTTPID_H */
