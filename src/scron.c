// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <scron.h>

#include <time.h>
#include <string.h>
#include <stdlib.h>

time_t scron_schedule_next_time(const struct scron_schedule *sched, time_t now)
{
	struct tm next = *gmtime(&now);
	if (sched->second > 0)
	{
		next.tm_sec += sched->second;
	}
	if (sched->minute > 0)
	{
		next.tm_min += sched->minute;
	}
	if (sched->hour > 0)
	{
		next.tm_hour += sched->hour;
	}
	if (sched->weekday > 0)
	{
		next.tm_wday += sched->weekday;
	}
	if (sched->day > 0)
	{
		next.tm_mday += sched->day;
	}
	if (sched->month > 0)
	{
		next.tm_mon += sched->month;
	}
	return mktime(&next);
}

void scron_init(struct scron *scron, struct scron_tasks *static_tasks)
{
	scron->static_tasks = *static_tasks;
	memset(&scron->runtime_tasks, 0, sizeof(scron->runtime_tasks));
	scron->history = malloc(sizeof(scron->history[0]) * static_tasks->size);
	memset(scron->history, 0, sizeof(scron->history[0]) * static_tasks->size);
	scron->runtime_capacity = 0;
}

void scron_delete(struct scron *scron)
{
	if (scron->history)
	{
		free(scron->history);
		scron->history = NULL;
	}

	if (scron->runtime_tasks.tasks)
	{
		free(scron->runtime_tasks.tasks);
		scron->runtime_tasks.tasks = NULL;
		scron->runtime_tasks.size = 0;
	}
}

void scron_add_task(struct scron *scron, const struct scron_task *task)
{
	if (scron->runtime_tasks.tasks == NULL)
	{
		scron->runtime_tasks.tasks = malloc(sizeof(scron->runtime_tasks.tasks[0]) * 2);
		scron->runtime_capacity = 2;
		scron->runtime_tasks.size = 0;
	}
	size_t index = scron->runtime_tasks.size;
	if (index >= scron->runtime_capacity)
	{
		struct scron_task *tasks = scron->runtime_tasks.tasks;
		size_t new_size = sizeof(tasks[0]) * scron->runtime_capacity * 2;
		scron->runtime_tasks.tasks = realloc(tasks, new_size);
		// FIXME what if alloc failed?
		scron->runtime_capacity *= 2;
	}
	scron->runtime_tasks.tasks[index] = *task;
	scron->runtime_tasks.size += 1;
}

void scron_del_task(struct scron *scron, size_t index)
{
	// FIXME
}

size_t scron_get_task_count(const struct scron *scron)
{
	return scron->static_tasks.size + scron->runtime_tasks.size;
}

time_t scron_next_time(const struct scron *scron)
{
	size_t count = scron_get_task_count(scron);
	if (!count)
		return 0;

	time_t result;
	if (scron->static_tasks.size)
		result = scron_schedule_next_time(&scron->static_tasks.tasks[0].schedule, scron->history[0].last_run);
	else // Because count != 0, if we're here, we know we have runtime tasks
		result = scron_schedule_next_time(&scron->runtime_tasks.tasks[0].schedule, scron->history[0].last_run);

	for (size_t i = 0; i < scron->static_tasks.size; ++i)
	{
		time_t last_run = scron->history[i].last_run;
		time_t next = scron_schedule_next_time(&scron->static_tasks.tasks[i].schedule, last_run);
		if (result > next)
			result = next;
	}

	for (size_t i = 0; i < scron->runtime_tasks.size; ++i)
	{
		time_t last_run = scron->history[i + scron->static_tasks.size].last_run;
		time_t next = scron_schedule_next_time(&scron->runtime_tasks.tasks[i].schedule, last_run);
		if (result > next)
			result = next;
	}

	return result;
}

void scron_save(const struct scron *scron, scron_save_callback callback)
{
	for (size_t i = 0; i < scron->static_tasks.size; ++i)
	{
		time_t last_run = scron->history[i].last_run;
		callback(scron->static_tasks.tasks[i].name, last_run);
	}

	for (size_t i = 0; i < scron->runtime_tasks.size; ++i)
	{
		time_t last_run = scron->history[i + scron->static_tasks.size].last_run;
		callback(scron->runtime_tasks.tasks[i].name, last_run);
	}
}

void scron_load(const struct scron *scron, scron_load_callback callback)
{
	for (size_t i = 0; i < scron->static_tasks.size; ++i)
	{
		time_t last_run = 0;
		callback(scron->static_tasks.tasks[i].name, &last_run);
	}

	for (size_t i = 0; i < scron->runtime_tasks.size; ++i)
	{
		time_t last_run = 0;
		callback(scron->runtime_tasks.tasks[i].name, &last_run);
	}
}

struct scron_task *scron_get_task(struct scron *scron, size_t index)
{
	if (index > scron_get_task_count(scron))
		return NULL;

	if (index < scron->static_tasks.size)
		return &scron->static_tasks.tasks[index];
	return &scron->runtime_tasks.tasks[index - scron->static_tasks.size];
}

struct scron_task *scron_get_task_by_name(struct scron *scron, const char *name)
{
	for (size_t i = 0; i < scron->static_tasks.size; ++i)
	{
		if (!strcmp(name, scron->static_tasks.tasks[i].name))
			return &scron->static_tasks.tasks[i];
	}

	for (size_t i = 0; i < scron->runtime_tasks.size; ++i)
	{
		if (!strcmp(name, scron->runtime_tasks.tasks[i].name))
			return &scron->runtime_tasks.tasks[i];
	}

	return NULL;
}
