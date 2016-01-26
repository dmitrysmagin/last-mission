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

#ifndef _VIDEO_H_
#define _VIDEO_H_

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define ACTION_SCREEN_HEIGHT 136
#define GAME_SCREEN_BPP 16

int gfx_init();
void gfx_quit();
void gfx_flip();
void ClearScreen();

#endif /* _VIDEO_H_ */
