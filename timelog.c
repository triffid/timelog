/*
 * Timelog
 *
 * a basic program to prepend high resolution timestamps to lines of text
 *
 *    Copyright (C) 2017 Michael Moon
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <stdio.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>

#include <errno.h>

#define fprintf(...) do {} while (0)

struct timespec curtime;
struct tm calendartime;
struct timespec starttime;

char timebuf[64];

char linebuf[4096];

int main(int argc, char** argv) {
	if (clock_gettime(CLOCK_REALTIME, &starttime))
		return 1;
	if (clock_gettime(CLOCK_MONOTONIC, &curtime))
		return 1;

	// subtract to estimate wall-clock start time of CLOCK_MONOTONIC
	{
		// starttime -= curtime;
		starttime.tv_nsec -= curtime.tv_nsec;
		starttime.tv_sec  -= curtime.tv_sec;
		while (starttime.tv_nsec < 0) {
			starttime.tv_nsec += 1000000000;
			starttime.tv_sec  -= 1;
		}
	}

	int fd = STDIN_FILENO;

	if (argc >= 2) {
		fd = open(argv[1], O_RDONLY | O_NONBLOCK);
		fd = open(argv[1], O_RDONLY);
		if (fd == -1) {
			perror("open");
			return 1;
		}
	}
	else {
		int flags = fcntl(fd, F_GETFL, 0);
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}

	struct pollfd pollinfo = {
		fd,
		POLLIN,
		0
	};

	int h = 0;
	int r = 0;
	int ok = 1;
	while (ok) {
		r = read(fd, linebuf + h, sizeof(linebuf) - h);
		{
			if (clock_gettime(CLOCK_MONOTONIC, &curtime))
				return 1;
			// add start time to get wall-clock time but monotonic vs program start
			curtime.tv_nsec += starttime.tv_nsec;
			curtime.tv_sec  += starttime.tv_sec;
			while (curtime.tv_nsec >= 1000000000) {
				curtime.tv_nsec -= 1000000000;
				curtime.tv_sec++;
			}
		}

		if ((r == -1) && (errno == EAGAIN)) {
			r = 0;
			ok = 1;
		}
		else if (r == 0) {
			return 0;
		}

		if (r > 0) {
			linebuf[h + r] = 0;
			fprintf(stderr, "\n\tRead %d bytes at %d: %s\n", r, h, linebuf + h);
			fprintf(stderr, "\tBuffer is now '%s'\n", linebuf);
			h += r;
			char* p;
			do {
				p = index(linebuf, '\n');
				int i = p - linebuf;

				fprintf(stderr, "\tFound newline at %p, %d bytes into buffer\n", p, i);

				if (p) {
					if (strftime(timebuf, sizeof(timebuf), "%F %T", localtime_r(&curtime.tv_sec, &calendartime)) <= 0)
						return 1;

					linebuf[i] = 0;
					printf("[%s.%09ld] %s\n", timebuf, curtime.tv_nsec, linebuf);

					fprintf(stderr, "\tChanging h to %d (now %d)\n", h - i - 1, h);

					h -= i + 1;
					memmove(linebuf, linebuf + i + 1, h);
					linebuf[h] = 0;
				}
				else {
					break;
				}
			} while (p);
			fprintf(stderr, "\th is %d\n", h);
			fflush(stdout);
		}
		else {
			poll(&pollinfo, 1, -1);
		}
	}
}
