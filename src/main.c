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

#include "video.h"
#include "sound.h"
#include "engine.h"
#include "world.h"

#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save-as") == 0 && i + 1 < argc) {
            WORLD *world;
            char *filename = argv[++i];

            world = load_world("data/lastmission.dat");
            if (!world) {
                printf("Failed to load data/lastmission.dat\n");
                return -1;
            }

            if (save_world(filename, world)) {
                printf("Saved to %s\n", filename);
            } else {
                printf("Failed to save to %s\n", filename);
            }

            free_world(world);
            return 0;
        }
    }

    if (!gfx_init())
        return -1;

    snd_init();

    GameLoop();

    snd_quit();
    gfx_quit();

    return 0;
}
