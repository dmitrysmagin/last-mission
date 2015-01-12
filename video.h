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

extern unsigned char *pScreenBuffer;

int LM_Init(unsigned char **pScreenBuffer);
void LM_Deinit();
void LM_GFX_Flip(unsigned char *p);
void LM_GFX_SetScale(int scale);

#endif /* _VIDEO_H_ */
