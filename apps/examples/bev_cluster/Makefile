##############################################################################
# apps/examples/bev_cluster/Makefile
##############################################################################

include $(APPDIR)/Make.defs

# Application info

PROGNAME  = bev_cluster
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_BEV_CLUSTER)

# Source files

MAINSRC = app_main.c
CSRCS   = can_handler.c can_db.c button_handler.c ui_widgets.c
CSRCS  += notifications.c

include $(APPDIR)/Application.mk
