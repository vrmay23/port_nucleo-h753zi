#include "taskmgr.h"
#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

/* Global storage for scanned tasks */
static task_entry_t g_tasks[TASKMGR_MAX_TASKS];
static int g_task_count = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int task_compare(const task_entry_t *t1, const task_entry_t *t2, 
                       task_order_t order)
{
    switch (order)
    {
        case TASK_ORDER_PID:
            return t1->pid - t2->pid;
        case TASK_ORDER_NAME:
            return strcmp(t1->name, t2->name);
        case TASK_ORDER_RUNTIME:
            return (int)(t1->runtime_ms - t2->runtime_ms);
        case TASK_ORDER_STACK_USED:
            return (int)(t1->stack_used - t2->stack_used);
        default:
            return 0;
    }
}

static void task_sort(task_entry_t *tasks, int count, task_order_t order, bool ascending)
{
    for (int i = 0; i < count - 1; i++)
    {
        for (int j = 0; j < count - 1 - i; j++)
        {
            int cmp = task_compare(&tasks[j], &tasks[j + 1], order);
            
            if (ascending ? (cmp > 0) : (cmp < 0))
            {
                task_entry_t tmp = tasks[j];
                tasks[j] = tasks[j + 1];
                tasks[j + 1] = tmp;
            }
        }
    }
}

static void safe_strncpy(char *dest, const char *src, size_t dest_size)
{
    if (dest == NULL || src == NULL || dest_size == 0)
        return;
        
    size_t i;
    for (i = 0; i < dest_size - 1 && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static int parse_proc_cmdline(int pid, task_entry_t *task)
{
    char path[64];
    int fd;
    ssize_t bytes_read;
    char buffer[64];
    
    /* Initialize with safe default name */
    snprintf(task->name, TASK_NAME_MAX, "task_%d", pid);
    
    /* Try to read command line - be very careful */
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    fd = open(path, O_RDONLY);
    if (fd >= 0)
    {
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0';
            
            /* Find first space and null-terminate there */
            for (int i = 0; i < bytes_read; i++)
            {
                if (buffer[i] == ' ' || buffer[i] == '\0')
                {
                    buffer[i] = '\0';
                    break;
                }
            }
            
            /* Extract basename if it's a path */
            char *basename = buffer;
            for (int i = 0; buffer[i] != '\0'; i++)
            {
                if (buffer[i] == '/')
                {
                    basename = &buffer[i + 1];
                }
            }
            
            if (basename[0] != '\0')
            {
                safe_strncpy(task->name, basename, TASK_NAME_MAX);
            }
        }
        close(fd);
    }
    
    return 0;
}

static int parse_simple_proc_entry(int pid, task_entry_t *task)
{
    /* Zero out the entire structure first */
    memset(task, 0, sizeof(task_entry_t));
    
    /* Set safe defaults */
    task->pid = pid;
    task->priority = 100;
    task->stack_size = 2048;
    task->stack_used = 512;
    task->runtime_ms = 0;
    task->status = TASK_RUNNING;
    safe_strncpy(task->state_str, "Running", sizeof(task->state_str));
    
    /* Try to get name */
    parse_proc_cmdline(pid, task);
    
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

const char* taskmgr_status_to_string(task_status_t status)
{
    switch (status)
    {
        case TASK_RUNNING:   return "RUNNING";
        case TASK_SLEEPING:  return "SLEEPING"; 
        case TASK_WAITING:   return "WAITING";
        case TASK_STOPPED:   return "STOPPED";
        default:             return "UNKNOWN";
    }
}

task_status_t taskmgr_parse_nuttx_state(const char* state_str)
{
    if (state_str == NULL) 
        return TASK_RUNNING;
    
    if (strstr(state_str, "Running") || strstr(state_str, "Ready"))
        return TASK_RUNNING;
    else if (strstr(state_str, "Sleep"))
        return TASK_SLEEPING;
    else if (strstr(state_str, "Wait"))
        return TASK_WAITING;
    else if (strstr(state_str, "Stop"))
        return TASK_STOPPED;
    else
        return TASK_RUNNING;
}

int taskmgr_scan_system(void)
{
    DIR *proc_dir;
    struct dirent *entry;
    int pid;
    int count = 0;
    
    /* Clear task list */
    memset(g_tasks, 0, sizeof(g_tasks));
    g_task_count = 0;
    
    /* Try /proc */
    proc_dir = opendir("/proc");
    if (proc_dir == NULL)
    {
        printf("Warning: /proc not accessible\n");
        printf("Adding current process only\n");
        
        /* Fallback: current process only */
        task_entry_t *task = &g_tasks[0];
        task->pid = getpid();
        safe_strncpy(task->name, "taskmgr", TASK_NAME_MAX);
        task->status = TASK_RUNNING;
        task->stack_size = 4096;
        task->stack_used = 1024;
        task->priority = 100;
        
        g_task_count = 1;
        return 1;
    }
    
    printf("Scanning /proc...\n");
    
    /* Scan entries */
    while ((entry = readdir(proc_dir)) != NULL && count < TASKMGR_MAX_TASKS)
    {
        if (entry->d_name == NULL)
            continue;
            
        /* Check if name is numeric (PID) */
        char *endptr;
        pid = (int)strtol(entry->d_name, &endptr, 10);
        
        if (endptr == entry->d_name || *endptr != '\0' || pid <= 0)
        {
            continue; /* Skip non-PID entries */
        }
        
        /* Parse safely */
        if (parse_simple_proc_entry(pid, &g_tasks[count]) == 0)
        {
            printf("Found PID %d: %s\n", pid, g_tasks[count].name);
            count++;
        }
    }
    
    closedir(proc_dir);
    g_task_count = count;
    
    printf("Scanned %d tasks\n", count);
    return count;
}

int taskmgr_get_task_count(void)
{
    return g_task_count;
}

task_entry_t* taskmgr_get_tasks(void)
{
    return g_tasks;
}

int taskmgr_kill_task(int pid)
{
    if (pid <= 0)
    {
        printf("Invalid PID\n");
        return -1;
    }
    
    if (kill(pid, SIGTERM) == 0)
    {
        printf("Terminated PID %d\n", pid);
        return 0;
    }
    
    printf("Failed to terminate PID %d\n", pid);
    return -1;
}

void taskmgr_list(task_order_t order, bool ascending)
{
    if (g_task_count == 0)
    {
        printf("No tasks. Run 'taskmgr scan' first\n");
        return;
    }

    /* Sort */
    task_entry_t sorted[TASKMGR_MAX_TASKS];
    memcpy(sorted, g_tasks, g_task_count * sizeof(task_entry_t));
    task_sort(sorted, g_task_count, order, ascending);

    /* Simple, safe output */
    printf("\nTask Manager: %d tasks found\n", g_task_count);
    printf("PID  | NAME                | STATUS\n");
    printf("-----+---------------------+---------\n");

    for (int i = 0; i < g_task_count; i++)
    {
        /* Ultra-safe printf - only basic types */
        printf("%4d | ", sorted[i].pid);
        
        /* Print name character by character if needed to avoid issues */
        int name_len = strlen(sorted[i].name);
        if (name_len > 19) name_len = 19;
        
        for (int j = 0; j < name_len; j++)
        {
            printf("%c", sorted[i].name[j]);
        }
        
        /* Pad with spaces */
        for (int j = name_len; j < 19; j++)
        {
            printf(" ");
        }
        
        printf(" | %s\n", taskmgr_status_to_string(sorted[i].status));
    }
    
    printf("\nUse 'ps' for detailed system info\n");
}

/****************************************************************************
 * Main entry point
 ****************************************************************************/

int taskmgr_main(int argc, FAR char *argv[])
{
    if (argc < 2)
    {
        printf("NuttX Task Manager\n");
        printf("Commands:\n");
        printf("  scan   - Scan for tasks\n");
        printf("  status - Show task list\n");
        printf("  auto   - Scan and show\n");
        printf("  kill PID - Kill task\n");
        return 0;
    }

    if (strcmp(argv[1], "scan") == 0)
    {
        int count = taskmgr_scan_system();
        printf("Found %d tasks\n", count);
        return 0;
    }
    else if (strcmp(argv[1], "auto") == 0)
    {
        printf("Auto scan...\n");
        int count = taskmgr_scan_system();
        if (count > 0)
        {
            taskmgr_list(TASK_ORDER_PID, true);
        }
        return 0;
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        taskmgr_list(TASK_ORDER_PID, true);
        return 0;
    }
    else if (strcmp(argv[1], "kill") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: taskmgr kill PID\n");
            return 1;
        }

        int pid = atoi(argv[2]);
        return taskmgr_kill_task(pid);
    }
    else
    {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }
}
