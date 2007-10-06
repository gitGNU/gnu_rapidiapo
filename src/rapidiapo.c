/**
 * Rapidiapo - nice and easy display for your pictures

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
#include <dirent.h>
#include <string.h>

#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
# define WIN32_LEAN_AND_MEAN
# include <windows.h>

#define strcasestr strstr
#endif


#include "SDL.h"
#include "SDL_image.h"
#include "SDL_rotozoom.h"
#include "SDL_framerate.h"


#define error printf
#define debug printf
#define info printf
#define _(a) a

/* [0,N-1] uniformly distributed */
/* http://www.thinkage.ca/english/gcos/expl/c/lib/rand.html */
static inline int myrand(int limit)
{
  int val;
#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
  /* Woe return a max of 32767, which is far too small */
  /* TODO: #ifdef RANDOM < 4000000 ? */
  int max = RAND_MAX<<15; /* 32767 == 2^15 */
  do {
    val = rand();
    val <<= 15;
    val |= rand();
    val /= (max/limit);
  } while(val >= limit);
#else
  while((val = rand() / (RAND_MAX/limit)) >= limit);
#endif
  return val;
}

/* Loading, resizing and displaying an image */
/* 2160x1440x24 -> 1280x1024x32 */
/*
  Time(loading): 368
  Image to screen ratio (0.592593x0.711111)
  Time(zooming): 256
  Time(blitting): 110
  Time(flipping): 10
  Total time: 745
*/

static SDL_Surface *screen;
static SDL_Surface *previous_screen, *next_screen;
static SDL_Surface *image1, *image2,
  *resized1, *resized2;
static int last_update = 0;

/* load_image() */
/* { */
static SDL_Surface *image = NULL;
static SDL_Surface *resized = NULL;
//char *dirname = "/home/me/Desktop/photos";
char *dirname;
char **filenames;
int nb_images = 0;
/* } */

/* TODO: the memcpy part is not portable, it assumes the values are in
   little-endian */
void get_pixel(SDL_Surface *surface, int x, int y, Uint8 *pr, Uint8 *pg, Uint8 *pb)
{
  char *pixels = (char*) surface->pixels;
  Uint32 pixel = 0;
  memcpy(&pixel,
	 &(pixels[y * surface->pitch + x * surface->format->BytesPerPixel]),
	 surface->format->BytesPerPixel);
  SDL_GetRGB(pixel, surface->format, pr, pg, pb);
}

void set_pixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
  char *pixels = (char*) surface->pixels;
  Uint32 pixel = SDL_MapRGB(surface->format, r, g, b);
  memcpy(&(pixels[y * surface->pitch + x * surface->format->BytesPerPixel]),
	 &pixel,
	 surface->format->BytesPerPixel);
}


SDL_Surface *resize_surface(SDL_Surface *surface)
{
  double factor_x, factor_y;
  factor_x = 1.0 * screen->w / surface->w;
  factor_y = 1.0 * screen->h / surface->h;
  if (factor_x > factor_y)
    factor_x = factor_y;
  if (factor_y > factor_x)
    factor_y = factor_x;
  
  return zoomSurface(surface, factor_x, factor_y, SMOOTHING_ON);
}

Uint32 StopPauseCallback(Uint32 interval, void *param)
{
  SDL_Event event;
  SDL_UserEvent userevent;
  
  userevent.type = SDL_USEREVENT;
  userevent.code = 0;
  userevent.data1 = NULL;
  userevent.data2 = NULL;
  
  event.type = SDL_USEREVENT;
  event.user = userevent;
  
  SDL_PushEvent(&event);
  return(interval);
}

int load_image(void *param_filename)
{
  int index = (int) param_filename;
  if (index > nb_images)
    return 0;

  int status = 0;
  char *fullpath = malloc(strlen(dirname) + 1 + strlen(filenames[index]) + 1);
  sprintf(fullpath, "%s/%s", dirname, filenames[index]);
  
  if (image != NULL)
    SDL_FreeSurface(image);
  printf("Loading #%d %s\n", index, fullpath);
  image = IMG_Load(fullpath);

  if (image == NULL)
    {
      printf("Could not load %s: %s\n", fullpath, SDL_GetError());
      status = 0;
    }
  else
    {
      if (resized != NULL)
	SDL_FreeSurface(resized);
      resized = resize_surface(image);
      status = 1;
    }
  free(fullpath);
  return status;
}

int main(int argc, char *argv[])
{
  FPSmanager framerate_manager;
  int quit = 0;
  int start;
  SDL_Thread* background_loading;

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
  screen = SDL_SetVideoMode(0, 0, 0, SDL_HWSURFACE | SDL_ANYFORMAT | SDL_FULLSCREEN);
  if (screen == NULL)
    {
      error(_("Could not start the graphics display: %s\n"), SDL_GetError());
      exit(1);
    }
  if (screen->flags & SDL_HWSURFACE)
    printf("INFO: Using hardware video mode.\n");
  else
    printf("INFO: Not using a hardware video mode.\n");

  /* Copy */
  previous_screen = SDL_DisplayFormat(screen);
  next_screen = SDL_DisplayFormat(screen);

  SDL_initFramerate(&framerate_manager);
  SDL_setFramerate(&framerate_manager, 50);
  /*   { */
  /*     SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0)); */
  /*     SDL_Flip(screen); */
  /*   } */

#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
  char myself[MAX_PATH];
  int k;

  /* Current directory */
  GetModuleFileName(NULL, myself, MAX_PATH);
  for (k = strlen(myself); myself[k] != '\\'; k--)
    ;
  dirname = myself;
  dirname[k] = '\0';
#else /* Unix */
  if (argc > 1)
    dirname = argv[1];
  else
    dirname = ".";
#endif

  DIR *dir;
  struct dirent *entry;

  printf("Opening %s\n", dirname);
  dir = opendir(dirname);
  filenames = malloc(1);
  while((entry = readdir(dir)) != NULL)
    {
      char *name = entry->d_name;
      int len = strlen(name);

      char *name_lc = malloc(len + 1);
      strncpy(name_lc, name, len + 1);
      char *pc = name_lc;
      printf("lc1 = %s\n", name_lc);
      while (*pc != '\0')
	{
	  *pc = tolower(*pc);
	  pc++;
	}
      printf("lc2 = %s\n", name_lc);

      /* Match trailing ".jpg" */
      char *occurrence, *last_occurrence, *cur_string;
      occurrence = cur_string = name_lc;
      last_occurrence = NULL;
      while (printf("cur_string = %s\n", cur_string), (occurrence = (strstr(cur_string, ".jpg"))) != NULL)
	{
	  last_occurrence = occurrence;
	  cur_string = occurrence + 1;
	}
      if ((last_occurrence != NULL) && ((last_occurrence - name_lc) == (len - 4)))
	{
	  /* This is a .jpg !!! */
	  printf("%s is a .jpg !!!\n", name);
	  nb_images++;
	  filenames = realloc(filenames, sizeof(char*) * nb_images);
	  filenames[nb_images-1] = malloc(len + 1);
	  strncpy(filenames[nb_images-1], name, len + 1);
	}
      free(name_lc);
    }

  if (nb_images > 0)
    background_loading = SDL_CreateThread(load_image, 0);

  int i = 0;
  while(i < nb_images && !quit)
    {
      // Load next image, if not already
      {
	int *pstatus = malloc(sizeof(int));
	SDL_WaitThread(background_loading, pstatus);
	int status = *pstatus;
	free(pstatus);
	if (status == 0)
	  /* Failed to load, skip to next one */
	  continue;
      }

      SDL_BlitSurface(resized, NULL, next_screen, NULL);
      SDL_BlitSurface(screen, NULL, previous_screen, NULL);

      last_update = start = SDL_GetTicks();
      
      if (SDL_MUSTLOCK(screen))
	SDL_LockSurface(screen);
      
      unsigned int total = 0;
      unsigned int pos = 0;

      int skip = 0;
      while (!quit && !skip && ((SDL_GetTicks() - start) < 3000))
      // while (!quit && !skip)
	{
	  SDL_Event event;
	  while (SDL_PollEvent(&event))
	    {
	      switch (event.type)
		{
		case SDL_QUIT:
		  quit = 1;
		  break;
		case SDL_KEYDOWN:
		  if (event.key.keysym.sym == 'q'
		      || event.key.keysym.sym == SDLK_ESCAPE)
		    quit = 1;
		  else
		    skip = 1;
		  break;
		}
	    }

	  /* Transition 1 */
	  int randx, randy;
	  int c1 = RAND_MAX / next_screen->w, c2 = RAND_MAX / next_screen->h;
	  int pos_limit = (screen->h * screen->w - 1) / 4;
	  /* int increment = (2000 + myrand(2000)) * next_screen->format->BytesPerPixel; */
	  int increment = screen->pitch * screen->h / 4;
	  for (int i = 0; i < 100; i++)
	    {
	      Uint8 r=0, g=0, b=0;
	      unsigned int array_index;
	      
	      /* 1x1 pixel each time - too slow */
	      /* randx = (int) round(1.0 * (next_screen->w-1) * rand() / RAND_MAX); */
	      /* randy = (int) round(1.0 * (next_screen->h-1) * rand() / RAND_MAX); */
	      /* get_pixel(next_screen, randx, randy, &r, &g, &b); */
	      /* set_pixel(screen, randx, randy, r, g, b); */
	      
	      /* 	  randx = myrand(next_screen->w); */
	      /* 	  randy = myrand(next_screen->h); */
	      
	      /* 	  while((randx = rand() / c1) >= next_screen->w); */
	      /* 	  while((randy = rand() / c2) >= next_screen->h); */
	      
	      /* 	  pos = randy * next_screen->pitch + randx * next_screen->format->BytesPerPixel; */
	      
	      /* 	  pos = (unsigned)total * (screen->w / 10) % (screen->h * screen->w - 1) * next_screen->format->BytesPerPixel; */
	      /* 	  randx = pos % screen->w; */
	      /* 	  randy = pos / screen->w; */
	      
	      /* 	  pos = randy * next_screen->pitch + randx * next_screen->format->BytesPerPixel; */
	      /* fast pseudo random */
	      /* pos += pos + (total % 10); */
	      
	      /* 	  pos += increment; */
	      /* 	  pos %= pos_limit; */
	      /* 	  if (pos > pos_limit) pos = 0; */
	      
	      pos = myrand(pos_limit);
	      
	      /* 	  if (pos > pos_limit) pos = 0; */
	      array_index = pos * next_screen->format->BytesPerPixel;
	      {
		char *address1;
		char *address2;
		
		address1 = &(((char*)screen->pixels)[array_index]);
		address2 = &(((char*)next_screen->pixels)[array_index]);
		memcpy(address1, address2,
		       next_screen->format->BytesPerPixel);
		
		address1 += increment;
		address2 += increment;
		memcpy(address1, address2,
		       next_screen->format->BytesPerPixel);
		
		address1 += increment;
		address2 += increment;
		memcpy(address1, address2,
		       next_screen->format->BytesPerPixel);
		
		address1 += increment;
		address2 += increment;
		memcpy(address1, address2,
		       next_screen->format->BytesPerPixel);
	      }
	      /* 	  SDL_Rect src_dst = {randx, randy, 1, 1}; */
	      /* 	  SDL_BlitSurface(next_screen, &src_dst, screen, &src_dst); */
	      total++;
	      
	      /* Byte by byte, color per color */
	      /* 	  { */
	      /* 	    int pos = round(1.0 * (next_screen->pitch * next_screen->h - 1) * rand() / RAND_MAX); */
	      /* 	    ((char*)screen->pixels)[pos] = ((char*)next_screen->pixels)[pos]; */
	      /* 	  } */
	      
	      /* 2x2 pixels each time */
	      /* randx = (int) round(1.0 * (next_screen->w-1-1) * rand() / RAND_MAX); */
	      /* randy = (int) round(1.0 * (next_screen->h-1-1) * rand() / RAND_MAX); */
	      
	      /* 2x2 pixels each time, starting point every 2 pixels (finishes faster but uglier) */
	      /* 	  randx = (int) round(1.0 * (next_screen->w/2-1) * rand() / RAND_MAX); */
	      /* 	  randy = (int) round(1.0 * (next_screen->h/2-1) * rand() / RAND_MAX); */
	      /* 	  randx *= 2; */
	      /* 	  randy *= 2; */
	      
	      /* 	  get_pixel(next_screen, randx, randy, &r, &g, &b); */
	      /* 	  set_pixel(screen, randx, randy, r, g, b); */
	      
	      /* 	  get_pixel(next_screen, randx+1, randy, &r, &g, &b); */
	      /* 	  set_pixel(screen, randx+1, randy, r, g, b); */
	      
	      /* 	  get_pixel(next_screen, randx, randy+1, &r, &g, &b); */
	      /* 	  set_pixel(screen, randx, randy+1, r, g, b); */
	      
	      /* 	  get_pixel(next_screen, randx+1, randy+1, &r, &g, &b); */
	      /* 	  set_pixel(screen, randx+1, randy+1, r, g, b); */
	      
	      /*  	  randx = (int) round(1.0 * (next_screen->w/2-1) * rand() / RAND_MAX); */
	      /* 	  randy = (int) round(1.0 * (next_screen->h/2-1) * rand() / RAND_MAX); */
	      /* 	  randx *= 2; */
	      /* 	  randy *= 2; */
	      /* 	  { */
	      /* 	    SDL_Rect src_dst = {randx, randy, 2, 2}; */
	      /* 	    SDL_BlitSurface(next_screen, &src_dst, screen, &src_dst); */
	      /* 	  } */
	      
	      /*  	  randx = (int) round(1.0 * (next_screen->w/3-1) * rand() / RAND_MAX); */
	      /* 	  randy = (int) round(1.0 * (next_screen->h/3-1) * rand() / RAND_MAX); */
	      /* 	  randx *= 3; */
	      /* 	  randy *= 3; */
	      /* 	  { */
	      /* 	    SDL_Rect src_dst = {randx, randy, 3, 3}; */
	      /* 	    SDL_BlitSurface(next_screen, &src_dst, screen, &src_dst); */
	      /* 	  } */
	      
	    }
	  
	  if (SDL_GetTicks() - last_update > (1000 / 50))
	    {
	      if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);
	      SDL_Flip(screen);
	      if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);
	      last_update = SDL_GetTicks();
	    }
	}
      if (SDL_MUSTLOCK(screen))
	SDL_UnlockSurface(screen);
      printf("Modified pixels: %d\n", total);
      printf("Elapsed time: %d\n", SDL_GetTicks() - start);
      SDL_BlitSurface(next_screen, NULL, screen, NULL);
      SDL_Flip(screen);

      /* Preload next image if we're not at the last image already */
      if (i < (nb_images-1) && !quit)
	  background_loading = SDL_CreateThread(load_image, (void*)(i+1));

      SDL_Event event;
      SDL_TimerID timer_id = SDL_AddTimer(5000, StopPauseCallback, NULL);
      skip = 0;
      while (!quit && !skip && SDL_WaitEvent(&event))
	{
	  switch (event.type)
	    {
	    case SDL_USEREVENT:
	      skip = 1;
	      break;
	    case SDL_QUIT:
	      quit = 1;
	      break;
	    case SDL_KEYDOWN:
	      if (event.key.keysym.sym == 'q'
		  || event.key.keysym.sym == SDLK_ESCAPE)
		quit = 1;
	      else
		skip = 1;
	      SDL_RemoveTimer(timer_id);
	      break;
	    }
	}
      i++;
    }
  /* All images were displayed */
  //SDL_KillThread
  if (image != NULL)
    SDL_FreeSurface(image);
  if (resized != NULL)
    SDL_FreeSurface(resized);
  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER);
  //SDL_Quit();
  return 0;
}
