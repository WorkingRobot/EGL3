/*
 * support.h - Useful definitions and macros. Originated from the Linux-NTFS project.
 *
 * Copyright (c) 2000-2004 Anton Altaparmakov
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the NTFS-3G
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _NTFS_SUPPORT_H
#define _NTFS_SUPPORT_H

 /*
  * Generic macro to convert pointers to values for comparison purposes.
  */
#ifndef p2n
#define p2n(p)		((ptrdiff_t)((ptrdiff_t*)(p)))
#endif

  /*
   * The classic min and max macros.
   */
#ifndef min
#define min(a,b)	((a) <= (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)	((a) >= (b) ? (a) : (b))
#endif

/*
 * Useful macro for determining the offset of a struct member.
 */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

 /*
  * Simple bit operation macros. NOTE: These are NOT atomic.
  */
#define test_bit(bit, var)	      ((var) & (1 << (bit)))
#define set_bit(bit, var)	      (var) |= 1 << (bit)
#define clear_bit(bit, var)	      (var) &= ~(1 << (bit))

#endif /* defined _NTFS_SUPPORT_H */