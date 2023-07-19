// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <scron.h>

#include <stdbool.h>
#include <time.h>

/** Artemia task scheduler, runs tasks based on the current voltage, time, and
 * the schedules of the tasks.
 *
 * This function executes a task depending on the current time, the schedule of
 * the task, whether the task cares about being run late or not, and whether
 * the main power storage cpaacitor has a high enough voltage.
 *
 * If a task is executed, its history metadata is updated in scron to reflect
 * the last time it ran.
 *
 * @param[in,out] scron scron that manages the tasks to be run.
 * @parampin] voltage Current storage voltage level.
 * @param[in] now The current time.
 *
 * @returns True if a task was run, false otherwise. If a task did not run,
 * this is an indication that there are no more tasks to schedule for the time
 * being.
 */
bool artemia_scheduler(struct scron *scron, double voltage, time_t now);
