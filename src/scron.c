// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <scron.h>

#include <time.h>
#include <string.h>
#include <stdlib.h>

time_t scron_schedule_next_time(const struct scron_schedule *sched, time_t now)
{
	time_t next = now;
	struct tm now_tm = *gmtime(&now);
	if (sched->hour >= 0)
	{
		int diff = (sched->hour - now_tm.tm_hour) * 3600;
		if (diff < 0)
		{
			diff += 3600 * 24;
		}
		next += diff;
	}
	if (sched->minute >= 0)
	{
		int diff = (sched->minute - now_tm.tm_min) * 60;
		if (diff < 0)
		{
			diff += 3600;
		}
		next += diff;
	}
	if (sched->second >= 0)
	{
		int diff = (sched->second - now_tm.tm_sec);
		if (diff < 0)
		{
			diff += 60;
		}
		next += diff;
	}
	// If everything is negative, trigger the next second
	if (now == next)
	{
		next++;
	}
	return next;
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
