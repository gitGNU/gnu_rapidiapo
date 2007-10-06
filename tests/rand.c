/**
 * Benchmarks for plotting random points

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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "SDL.h"


/* [0,N-1] uniformly distributed */
/* http://www.thinkage.ca/english/gcos/expl/c/lib/rand.html */
static inline int myrand(int limit)
{
  int val;
  while((val = rand() / (RAND_MAX/limit)) >= limit);
  return val;
}

static inline int myrand2(int limit)
{
  return (int) round(1.0 * (limit-1) * rand() / RAND_MAX);
}

void test()
{
  int start = SDL_GetTicks();
  int w = 1280, h = 1024, pitch = 1280, BytesPerPixel = 3, randx, randy;
  void *mem = malloc(w*h*BytesPerPixel);
  for (int i = 0; i < 1; i++)
    {
      int c1 = RAND_MAX/w;
      int c2 = RAND_MAX/h;
      for (int j = 0; j < 4000000; j++)
	{
	  randx = rand() / c1;
	  randy = rand() / c2;
/* 	  randx = myrand(w); */
/* 	  randy = myrand(h); */
/* 	  int pos = randy * pitch + randx * BytesPerPixel; */
/* 	  memcpy(&(((char*)mem)[pos]), */
/* 		 &(((char*)mem)[pos]), */
/* 		 BytesPerPixel); */
	}
    }

  int end = SDL_GetTicks();
  printf("%d\n", end - start);
}

int main(int argc, char *argv[])
{
  SDL_Init(SDL_INIT_TIMER);
  atexit(SDL_Quit);
  test();
  return 0;
}
