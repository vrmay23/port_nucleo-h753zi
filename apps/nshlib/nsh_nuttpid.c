#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../taskmgr/taskmgr.h"

static void show_usage(void)
{
    printf("Usage:\n");
    printf("  nuttpid status [--order <type>] [--desc]  : list tasks\n");
    printf("          order types: pid, name, runtime\n");
    printf("  nuttpid kill <pid>                        : kill a task\n");
}

int nsh_nuttpid_main(int argc, char *argv[])
{
    if (argc < 2)
    {
        show_usage();
        return 1;
    }

    if (strcmp(argv[1], "status") == 0)
    {
        task_order_t order = TASK_ORDER_PID;
        bool ascending = true;

        for (int i = 2; i < argc; i++)
        {
            if (strcmp(argv[i], "--order") == 0 && i + 1 < argc)
            {
                i++;
                if (strcmp(argv[i], "pid") == 0)
                    order = TASK_ORDER_PID;
                else if (strcmp(argv[i], "name") == 0)
                    order = TASK_ORDER_NAME;
                else if (strcmp(argv[i], "runtime") == 0)
                    order = TASK_ORDER_RUNTIME;
                else
                {
                    printf("Invalid order type: %s\n", argv[i]);
                    return 1;
                }
            }
            else if (strcmp(argv[i], "--desc") == 0)
            {
                ascending = false;
            }
            else
            {
                printf("Unknown argument: %s\n", argv[i]);
                show_usage();
                return 1;
            }
        }

        taskmgr_list(order, ascending);
    }
    else if (strcmp(argv[1], "kill") == 0)
    {
        if (argc < 3)
        {
            printf("Error: PID required\n");
            return 1;
        }

        int pid = atoi(argv[2]);
        if (pid <= 0)
        {
            printf("Invalid PID\n");
            return 1;
        }

        if (taskmgr_stop(pid) == 0)
        {
            printf("Task %d stopped\n", pid);
        }
        else
        {
            printf("Failed to stop task %d\n", pid);
        }
    }
    else
    {
        show_usage();
        return 1;
    }

    return 0;
}
