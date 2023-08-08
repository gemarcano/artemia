// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <artemia.h>
#include <scron.h>

#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

// FIXME HACK testing printf
#include <stdio.h>

struct sort_task
{
	struct scron_task *task;
	time_t *last_run;
};

static int qsort_tasks(const void * a, const void *b)
{
	const struct sort_task *a_ = a;
	const struct sort_task *b_ = b;
	if (*a_->last_run < *b_->last_run)
		return -1;
	if (*a_->last_run > *b_->last_run)
		return 1;
	return 0;
}

bool artemia_scheduler(struct scron *scron, double voltage, time_t now)
{
	const size_t task_count = scron_get_task_count(scron);
	// Iterate through all tasks...
	struct sort_task *order = malloc(task_count * sizeof(*order));
	for (size_t i = 0; i < task_count; ++i)
	{
		order[i].task = scron_get_task(scron, i);
		order[i].last_run = &scron->history[i].last_run;
	}
	qsort(order, task_count, sizeof(*order), qsort_tasks);

	for (size_t i = 0; i < task_count; ++i)
	{
		struct scron_task *task = order[i].task;
		// Only select a task if we're at a voltage higher than the minimum...
		if (task->minimum_voltage <= voltage)
		{
			time_t last_run = *order[i].last_run;
			time_t next_run = scron_schedule_next_time(&task->schedule, last_run);
			double diff = difftime(now, next_run);
			// And if we're past the scheduled time, and if delta is set, that
			// we're within that delta from the schedule
			if (diff >= 0.0 && (task->delta <= 0 || task->delta > diff))
			{
				printf("running: %s, last: %lu, next: %lu, now: %lu, diff: %lu\r\n", task->name, (uint32_t)last_run, (uint32_t)next_run, (uint32_t)now, (uint32_t)diff);
				task->function(&now);
				// After the task, update history
				*order[i].last_run = now;
				return true;
			}
		}
	}
	return false;
}
