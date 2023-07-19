// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#ifndef SCRON_H_
#define SCRON_H_

#include <time.h>
#include <stdint.h>

// FIXME what is data?
typedef int (*scron_task_function)(void *data);

/** scron schedule.
 *
 * This describes when a task/event should take place. Negative numbers are
 * ignored. Scheduled events take place when all non-negative elements match
 * the current time. For example, for a structure where only the minute element
 * is non-negative, this scheduled event will fire every hour when the system
 * clock matches the minute value in the schedule.
 */
struct scron_schedule
{
	int8_t month;
	int8_t day;
	int8_t weekday;
	int8_t hour;
	int8_t minute;
	int8_t second;
};

/** scron task control structure.
 *
 * scron allows for tasks to be registered to be run when their schedule
 * dictates. This struct contains static metadata about the task including:
 *  - name: Name of the task, less than 31 characters long
 *  - minimum_voltage: the empirically derived safe voltage at which the task
 *    should terminate before expending all available energy.
 *  - function: the pointer to the actual task function
 *  - schedule: the schedule describing when the task should run
 *  - exact_timing: indicating whether the task should only be run at specific
 *    times, or just any time after the schedule is triggered.
 */
struct scron_task
{
	char name[32];
	double minimum_voltage;
	scron_task_function function;
	struct scron_schedule schedule;
	bool exact_timing;
};

/** scron task history structure.
 *
 * This structure is meant to contain non-static information regarding tasks.
 * These should be written to non-volatile or persistent memory.
 */
struct scron_task_history
{
	time_t last_run;
};

/** scron tasks table. */
struct scron_tasks
{
	size_t size;
	struct scron_task *tasks;
};

/** scron control structure.
 *
 * This contains two tables of tasks-- a static one that is meant to exist in
 * on-chip flash constant memory, and another that can be added to dynamically
 * at runtime.
 */
struct scron
{
	struct scron_tasks static_tasks;
	struct scron_tasks runtime_tasks;
	size_t runtime_capacity;
	struct scron_task_history *history;
};

/** Initializes the scron object.
 *
 * @param[out] scron scron object to initialize.
 * @param[in] static_tasks Pointer to the constant/compile-time tasks in memory.
 */
void scron_init(struct scron *scron, struct scron_tasks *static_tasks);

/** Releases any resources being used by scron.
 *
 * @param[out] scron scron object to delete.
 */
void scron_delete(struct scron *scron);

/** Adds a new task to the runtime task table.
 *
 * @param[in,out] scron scron object to add the task to.
 * @param[in] scron_task Task structure to copy into the runtime table.
 */
void scron_add_task(struct scron *scron,
	const struct scron_task *task);

//FIXME what if we fail, or ask for a bad index?
/** Removes a task from the runtime task table.
 *
 * @param[in,out] scron scron object to remove the task from.
 * @param[in] index scron task index to remove.
 */
void scron_del_task(struct scron *scron, size_t index);

/** Gets the number of scron tasks in total (static and runtime).
 *
 * @param[in,out] scron scron object to query.
 *
 * @returns The number of tasks registered total, both static and runtime.
 */
size_t scron_get_task_count(const struct scron *scron);

/** Given a schedule, computes the next time the event should occur based on
 *  the current time.
 *
 * @param[in] sched Schedule to use.
 * @param[in] now Current time in time_t (usually seconds from POSIX epoch).
 *
 * @returns The time_t when the next scheduled event should take place.
 */
time_t scron_schedule_next_time(const struct scron_schedule *sched, time_t now);

/** Computes the next time the event should occur based on the time the tasks
 *  last ran.
 *
 * @param[in] scron scron to use.
 *
 * @returns The time_t when the next scheduled event should take place. FIXME do I want to return which task too???
 */
time_t scron_next_time(const struct scron *scron);

/** Callback called by save to save the key,value pair of {name: last_run}
 *  somewhere. The specific details of saving are left to the callback to
 *  manage.
 *
 * @param[in] name Name of the task to be saved.
 * @param[in] last_run Time the task last ran.
 */
typedef void (*scron_save_callback)(const char *name, time_t last_run);

/** Saves the scron history through the use of a save callback function.
 *
 * @param[in] scron The scron with the data to save.
 * @param[in] callback A function that accepts a (name, time) pair to save the
 *  scron state to disk iteratively.
 */
void scron_save(const struct scron *scron, scron_save_callback callback);

/** Callback called by load to load the key,value pair of {name: last_run} from
 *  somewhere. The specific details of loading are left to the callback to
 *  manage.
 *
 * @param[in] name Name of the task to be loaded.
 * @param[out] last_run Time the task last ran from storage if found, else it
 *  is left unmodified..
 */
typedef void (*scron_load_callback)(const char *name, time_t *last_run);

/** Loads the scron history through the use of a load callback function.
 *
 * @param[in] scron The scron to load.
 * @param[in] callback A function that takes a task name, and modifies the
 *  provided pointer if found in storage.
 */
void scron_load(const struct scron *scron, scron_load_callback callback);

#endif//SCRON_H_
