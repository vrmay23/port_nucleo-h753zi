#ifndef TASKMGR_H
#define TASKMGR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define TASKMGR_MAX_TASKS 64
#define TASK_NAME_MAX 32

typedef enum {
    TASK_INVALID = 0,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_WAITING,
    TASK_STOPPED
} task_status_t;

typedef enum {
    TASK_ORDER_PID,
    TASK_ORDER_NAME,
    TASK_ORDER_RUNTIME,
    TASK_ORDER_STACK_USED
} task_order_t;

typedef struct {
    int pid;
    char name[TASK_NAME_MAX];
    size_t stack_size;
    size_t stack_used;
    uint32_t runtime_ms;
    uint8_t priority;
    task_status_t status;
    char state_str[16];  /* Estado textual do NuttX */
} task_entry_t;

/* Public API Functions */
int taskmgr_scan_system(void);
int taskmgr_get_task_count(void);
task_entry_t* taskmgr_get_tasks(void);
int taskmgr_kill_task(int pid);
void taskmgr_list(task_order_t order, bool ascending);

/* Utility functions */
const char* taskmgr_status_to_string(task_status_t status);
task_status_t taskmgr_parse_nuttx_state(const char* state_str);

#endif // TASKMGR_H
