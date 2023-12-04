/****************************************************************************
 * apps/examples/temprelaydisp/lvglmain.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#ifndef SIMULATOR
#include <nuttx/config.h>

#include <sys/boardctl.h>
#include <sys/param.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <debug.h>
#include <semaphore.h>

#include <lvgl/lvgl.h>
#include <port/lv_port.h>
#include <lvgl/demos/lv_demos.h>
#include "lvgl_myview.h"
#ifdef CONFIG_LIBUV
#  include <uv.h>
#  include <port/lv_port_libuv.h>
#endif
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Should we perform board-specific driver initialization?  There are two
 * ways that board initialization can occur:  1) automatically via
 * board_late_initialize() during bootupif CONFIG_BOARD_LATE_INITIALIZE
 * or 2).
 * via a call to boardctl() if the interface is enabled
 * (CONFIG_BOARDCTL=y).
 * If this task is running as an NSH built-in application, then that
 * initialization has probably already been performed otherwise we do it
 * here.
 */

#undef NEED_BOARDINIT

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define NEED_BOARDINIT 1
#endif

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/
extern sem_t sem_lvgl;
/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void lvgl_initialize( void )
{
  /* LVGL initialization */
  lv_init();

  /* LVGL port initialization */

  lv_port_init();

  /* LVGL demo creation */

  lv_myview();

  /* Handle LVGL tasks */

  while (1)
    {
      uint32_t idle;
      nxsem_wait(&sem_lvgl);
      idle = lv_timer_handler();
      nxsem_post(&sem_lvgl);
      /* Minimum sleep of 1ms */

      idle = idle ? idle : 1;
      usleep(idle * 1000);
    }
}
/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main or temprelaydisp_main
 *
 * Description:
 *
 * Input Parameters:
 *   Standard argc and argv
 *
 * Returned Value:
 *   Zero on success; a positive, non-zero value on failure.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{

#ifdef NEED_BOARDINIT
  /* Perform board-specific driver initialization */

  boardctl(BOARDIOC_INIT, 0);

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif
#endif
  lvgl_initialize();
  return EXIT_SUCCESS;
}
#endif //SIMULATOR
