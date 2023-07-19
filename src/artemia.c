// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <artemia.h>
#include <scron.h>

#include <stdbool.h>
#include <time.h>

bool artemia_scheduler(struct scron *scron, double voltage, time_t now)
{
	const size_t task_count = scron_get_task_count(scron);
	// Iterate through all tasks...
	for (size_t i = 0; i < task_count; ++i)
	{
		struct scron_task *task = scron_get_task(scron, i);
		// Only select a task if we're at a voltage higher than the minimum...
		if (task->minimum_voltage <= voltage)
		{
			time_t last_run = scron->history[i].last_run;
			time_t next_run = scron_schedule_next_time(&task->schedule, last_run);
			double diff = difftime(now, next_run);
			// And if we're past the scheduled time, and if delta is set, that
			// we're within that delta from the schedule
			if (diff >= 0.0 && (task->delta <= 0 || task->delta > diff))
			{
				task->function(&now);
				// After the task, update history
				scron->history[i].last_run = now;
				return true;
			}
		}
	}
	return false;
}
