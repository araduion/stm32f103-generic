#include "task.h"

unsigned char cr_stack[TASK_NUM][STACK_SIZE] __attribute__ ((aligned(4)));
task_t *ts_start = NULL, *ts_end = NULL;
task_t *cr_task = NULL;
bool task_list_busy = false;
bool pend_op_q = false;
task_op_t task_ops[TASK_NUM][2];
task_t tasks[TASK_NUM];

void task_destroy(void);

void task_create(void (*entry_point)(void *arg), void *arg_value)
{
    hw_stack_frame_t *pframe;
    tasks[task_index].stack = cr_stack[task_index + 1];
    pframe = (hw_stack_frame_t *)&tasks[task_index].stack[STACK_SIZE - sizeof(hw_stack_frame_t)];
    tasks[task_index].sp = pframe;
    pframe->r0 = (unsigned int)arg_value;
    pframe->r1 = 0;
    pframe->r2 = 0;
    pframe->r3 = 0;
    pframe->r12 = 0;
    pframe->pc = (unsigned int)entry_point - 1;
    pframe->lr = (unsigned int)task_destroy;
    pframe->psr = 0x21000000;
    tasks[task_index].sp -= sizeof(sw_stack_frame_t);
    if (0 == task_index) {
        cr_task = &tasks[0];
        tasks[0].next = &tasks[0];
        tasks[0].prev = &tasks[0];
    } else {
        tasks[task_index].prev = &tasks[task_index - 1];
        tasks[task_index - 1].next = &tasks[task_index];
        tasks[task_index].next = &tasks[0];
        cr_task = &tasks[task_index];
        tasks[0].prev = cr_task;
    }
    tasks[task_index].task_id = task_index;
    task_index++;
    uart_printf("%s: task_num %d >= task_index %d\r\n", __func__, TASK_NUM, task_index);
}

void task_next(void)
{
    *((uint32_t volatile *)0xE000ED04) = 1 << 28; // trigger PendSV
}

void task_destroy(void)
{
    uart_printf("task exit!\r\n");
}

/* thread 0 content */
void check_pend_op(void)
{
    int i,j;
    if (pend_op_q) {
        pend_op_q = false;
        for (i = 1; i < TASK_NUM; i++) {
            for (j = 0; j < 2; j++) {
                task_op_t *cr_op = &task_ops[i][j];
                if (cr_op->op != 0) {
                    cr_op->op(cr_op->task_id);
                    cr_op->op = 0;
                    cr_op->task_id = 0;
                }
            }
        }
    }
    task_next();
}

bool task_suspend(uint8_t id)
{
    task_t *next, *prev;
    if (TASK_RUNNING != tasks[id].flags) {
        /* task not suspended */
        return false;
    }
    if (task_list_busy || id == cr_task->task_id) {
        task_ops[id][0].task_id = id;
        task_ops[id][0].op = task_suspend;
        pend_op_q = true;
        return true;
    }
    task_list_busy = true;
    next = tasks[id].next;
    prev = tasks[id].prev;
    if (NULL == ts_start) {
        ts_start = &tasks[id];
        ts_end = ts_start;
        ts_start->prev = NULL;
    } else {
        ts_end->next = &tasks[id];
        tasks[id].prev = ts_end;
        ts_end = ts_end->next;
    }
    ts_end->next = NULL;
    prev->next = next;
    next->prev = prev;
    tasks[id].flags = TASK_SUSPENDED;
    task_list_busy = false;
    return true;
}

bool task_resume(uint8_t id)
{
    task_t *prev, *next;
    if (TASK_SUSPENDED != tasks[id].flags) {
        /* task not suspended yet */
        return false;
    }
    if (task_list_busy) {
        task_ops[id][1].task_id = id;
        task_ops[id][1].op = task_resume;
        pend_op_q = true;
        return true;
    }
    task_list_busy = true;
    /* remove from suspended tasks */
    prev = tasks[id].prev;
    next = tasks[id].next;
    if (prev != NULL) {
        prev->next = next;
    } else {
        ts_start = ts_start->next;
    }
    if (next != NULL) {
        next->prev = prev;
    } else {
        ts_end = ts_end->prev;
        ts_end->next = NULL;
    }
    /* insert process in queue */
    next = cr_task->next;
    cr_task->next = &tasks[id];
    tasks[id].next = next;
    next->prev = &tasks[id];
    tasks[id].prev = cr_task;
    tasks[id].flags = TASK_RUNNING;
    task_list_busy = false;
    return true;
}

