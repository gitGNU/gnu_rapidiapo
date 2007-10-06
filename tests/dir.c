/**
 * Display a directory's content

 * Copyright (C) 2007  Sylvain Beucler

 * This file is part of Rapidiapo

 * Rapidiapo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.

 * Rapidiapo is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/* #include <sys/types.h> */
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

int main(int argc, char *argv[])
{
  DIR *dir;
  struct dirent *entry;
  dir = opendir(".");
  while((entry = readdir(dir)) != NULL)
    {
      char *name = entry->d_name;
      /* Match trailing ".jpg" */
      char *occurrence, *last_occurrence;
      char *cur_string = name;
      while ((occurrence = (strstr(cur_string, ".jpg"))) != NULL)
	{
	  last_occurrence = occurrence;
	  cur_string = occurrence + 1;
	}
      if ((last_occurrence - name) == (strlen(name) - 4))
	printf("%s\n", name);
    }
}
