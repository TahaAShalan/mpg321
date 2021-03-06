/*
    mpg321 - a fully free clone of mpg123.
    Copyright (C) 2001 Joe Drew
    
    Originally based heavily upon:
    plaympeg - Sample MPEG player using the SMPEG library
    Copyright (C) 1999 Loki Entertainment Software
    
    Also uses some code from
    mad - MPEG audio decoder
    Copyright (C) 2000-2001 Robert Leslie
    
    Original playlist code contributed by Tobias Bengtsson <tobbe@tobbe.nu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define _LARGEFILE_SOURCE 1

#include "mpg321.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>

playlist * new_playlist()
{
    playlist *pl = (playlist *) malloc(sizeof(playlist));

    srandom(time(NULL));
        
    pl->files = (char **) malloc(DEFAULT_PLAYLIST_SIZE * sizeof(char *));
    pl->files_size = DEFAULT_PLAYLIST_SIZE;
    pl->numfiles = 0;
    pl->random_play = 0;
    strcpy(pl->remote_file, "");
    
    return pl;
}

/* not needed - one static playlist object per instance 
void delete_playlist(playlist *pl)
{
    int i;
    for (i = 0; i < pl->numfiles; i++)
        free(pl->files[i]);

    free(pl->files);
    free(pl);
} */

void resize_playlist(playlist *pl)
{
    char ** temp = (char **) malloc ((pl->files_size *= 2) * sizeof(char *));
    register int i;
    
    for (i = 0; i < pl->numfiles; i++)
        temp [i] = pl->files[i];
        
    free (pl->files);
    
    pl->files = temp;
}

void set_random_play(playlist *pl)
{
    pl->random_play = 1;
}

void shuffle_files (playlist *pl)
{
    int i;
    int a, b;
    char *swap;

    for (i = 0; i < 100 * pl->numfiles; i++)
    {
        a = pl->numfiles * ((double)random()/RAND_MAX);
        b = pl->numfiles * ((double)random()/RAND_MAX);

        /* because if random()/RAND_MAX == 1, we will be 
           referencing an index outside the array */
        if (a == pl->numfiles)
            a--;
        if (b == pl->numfiles)
            b--;

        swap = pl->files[a];
        pl->files[a] = pl->files[b];
        pl->files[b] = swap;
    }
}

void trim_whitespace(char *str)
{
    char *ptr = str;
    register int pos = strlen(str)-1;

    while(isspace(ptr[pos]))
        ptr[pos--] = '\0';
    
    while(isspace(*ptr))
        ptr++;
    
    strncpy(str, ptr, pos+1);
}    
    
char * get_next_file(playlist *pl, buffer *buf)
{
    static int i = 0;
    
    if (options.opt & MPG321_REMOTE_PLAY)
    {
        while (strlen(pl->remote_file) == 0 && !quit_now)
            remote_get_input_wait(buf);
            
        return pl->remote_file;
    }    
    
    if (!pl->random_play)
    {
        if (i == pl->numfiles)
            return NULL;
        
        return pl->files[i++];
    }
    
    else
    {
        if (!pl->numfiles)
            return NULL;

        i = pl->numfiles * ((double)random()/RAND_MAX);
        
        if (i == pl->numfiles) i--;
        
        return pl->files[i];
    }
}

void add_cmdline_files(playlist *pl, char *argv[])
{
    int i;
    for (i = optind; argv[i]; ++i )
    {
        if (pl->numfiles == pl->files_size)
            resize_playlist(pl);

        pl->files[(pl->numfiles)++] = strdup(argv[i]);
    }
}

void play_remote_file(playlist *pl, char *file)
{
    strncpy(pl->remote_file, file, PATH_MAX);
    pl->remote_file[PATH_MAX-1]='\0';
}

void clear_remote_file(playlist *pl)
{
    pl->remote_file[0] = '\0';
}

void add_file(playlist *pl, char *file)
{
    if (pl->numfiles == pl->files_size)
         resize_playlist(pl);

    pl->files[pl->numfiles++] = strdup(file);
}

void load_playlist(playlist *pl, char *filename)
{
    FILE *plist;
    char playlist_buf[PATH_MAX];
    char *directory;
    int dirlen;

    if (strncmp(filename, "-", 1) == 0)
    {
        plist = stdin;
    }

    else if(!(plist = fopen(filename,"r")))
    {
        mpg321_error(filename);
        exit(1);
    }

    /* prepend the directory of the playlist file on to each filename. */
    directory = strdup(filename);
    directory = dirname(directory);

    dirlen = strlen(directory);

    while (fgets(playlist_buf, PATH_MAX, plist) != NULL)
    {
        trim_whitespace(playlist_buf);

        if (strlen(playlist_buf) == 0)
            continue;                

        if (pl->numfiles == pl->files_size)
            resize_playlist(pl);

        /* I hate special cases... */
        if(playlist_buf[0] != '/' && !strstr(playlist_buf, "://")) /* relative path  or network path */
        {
            /* +2 because of null terminating character plus / */
            pl->files[pl->numfiles] = malloc(strlen(playlist_buf) + dirlen + 2);
            strcpy(pl->files[pl->numfiles], directory);
            strcat(pl->files[pl->numfiles], "/");
            strcat(pl->files[pl->numfiles], playlist_buf);
        }

        else /* absolute path */
        {
            pl->files[pl->numfiles] = strdup(playlist_buf);
        }

        pl->numfiles++;
    }
    
    if (plist != stdin)
        fclose(plist);
}
