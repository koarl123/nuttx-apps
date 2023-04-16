############################################################################
# apps/examples/lvgldemo/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# LVGL demo Example lvgl_myview.c

MAINSRC = temprelaydisp.c
CSRCS = lvgl_myview.c data_collect.c gui_update.c controller.c


PROGNAME = temprelaydisp
PRIORITY = $(CONFIG_EXAMPLES_TEMPRELAYDISP_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_TEMPRELAYDISP_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_TEMPRELAYDISP)

include $(APPDIR)/Application.mk
