/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS for(int A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "timer.h"

// Timer routines
static long long start_us = 0;
static int frame_count_us = 0, skipped_frames = 0;

static long long get_ticks_us()
{
	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	return (long long)current_time.tv_sec * 1000000 +
	       (long long)current_time.tv_usec;
}

static void sleep_us(int value)
{
	usleep(value);
}

void synchronize_us()
{
	long long now_us;
	long long lim, wait;

	if (!start_us)
		start_us = get_ticks_us();

	now_us = get_ticks_us();
	frame_count_us++;
	if(now_us - start_us >= 1000000) {
		start_us = now_us;
		//fps = frame_limit - skipped_frames; // frame_limit = 60
		skipped_frames = 0;
		frame_count_us = 0;
	}

	lim = frame_count_us * 16667; /* frametime */

	wait = lim - (now_us - start_us);
	if (wait > 0)
		sleep_us(wait);
}
