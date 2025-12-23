/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-09-20     Bernard      Change the name to components.c
 *                             And all components related header files.
 * 2012-12-23     Bernard      fix the pthread initialization issue.
 * 2013-06-23     Bernard      Add the init_call for components initialization.
 * 2013-07-05     Bernard      Remove initialization feature for MS VC++ compiler
 * 2015-02-06     Bernard      Remove the MS VC++ support and move to the kernel
 * 2015-05-04     Bernard      Rename it to components.c because compiling issue
 *                             in some IDEs.
 * 2015-07-29     Arda.Fu      Add support to use RT_USING_USER_MAIN with IAR
 * 2018-11-22     Jesven       Add secondary cpu boot up
 */

#include <rthw.h>
#include <rtthread.h>

#ifdef RT_USING_USER_MAIN
#ifndef RT_MAIN_THREAD_STACK_SIZE
#define RT_MAIN_THREAD_STACK_SIZE     2048
#endif /* RT_MAIN_THREAD_STACK_SIZE */
#ifndef RT_MAIN_THREAD_PRIORITY
#define RT_MAIN_THREAD_PRIORITY       (RT_THREAD_PRIORITY_MAX / 3)
#endif /* RT_MAIN_THREAD_PRIORITY */
#endif /* RT_USING_USER_MAIN */

#ifdef RT_USING_COMPONENTS_INIT
#define COMPONENTS_ARRAY_MAX 32
#define COMPONENTS_LEVEL 7

#if RT_DEBUG_INIT
#define COMPOENENTS_TYPE const struct rt_init_desc*
#else
#define COMPOENENTS_TYPE const init_fn_t*
#endif /* RT_DEBUG_INIT */
COMPOENENTS_TYPE components_array[COMPONENTS_LEVEL][COMPONENTS_ARRAY_MAX];
int components_num[COMPONENTS_LEVEL];
void register_components(COMPOENENTS_TYPE desc, char *level)
{
    int idx = level[0]-'0';
    if (components_num[idx] < COMPONENTS_ARRAY_MAX) {
        components_array[idx][components_num[idx]] = desc;
        components_num[idx]++;
    }
}

void __rt_components_init(int init_start, int init_end)
{
#if RT_DEBUG_INIT
    int result;
    const struct rt_init_desc *desc;

    rt_kprintf("do components initialization.\n");
    for (int i = init_start; i < init_end; i ++) {
        rt_kprintf("components_num[%d]:%d\n", i, components_num[i]);
        for (int j = 0; j < components_num[i]; j ++) {
            desc = components_array[i][j];
            rt_kprintf("initialize %s", desc->fn_name);
            result = desc->fn();
            rt_kprintf(":%d done\n", result);
        }
    }
#else
    volatile const init_fn_t *fn_ptr;

    for (int i = init_start; i < init_end; i ++) {
        for (int j = 0; j < components_num[i]; j ++) {
            fn_ptr = components_array[i][j];
            (*fn_ptr)();
        }
    }
#endif /* RT_DEBUG_INIT */
}

/**
 * @brief  Onboard components initialization. In this function, the board-level
 *         initialization function will be called to complete the initialization
 *         of the on-board peripherals.
 */
void rt_components_board_init(void)
{
    __rt_components_init(0,1);
    rt_system_timer_init();
    rt_system_timer_thread_init();
}

/**
 * @brief  RT-Thread Components Initialization.
 */
void rt_components_init(void)
{
    __rt_components_init(1,COMPONENTS_LEVEL);
}
#endif /* RT_USING_COMPONENTS_INIT */
