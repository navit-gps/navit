/*
	Copyright (C) 2007  Alexander Atanasov      <aatanasov@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
	MA  02110-1301  USA

	Garmin and MapSource are registered trademarks or trademarks
	of Garmin Ltd. or one of its subsidiaries.

*/

#define dlog(x,y ...) logfn(__FILE__,__LINE__,x, ##y)
#ifdef HARDDEBUG
#define ddlog(x,y ...) logfn(__FILE__,__LINE__,x, ##y)
#else
#define ddlog(x,y ...)
#endif

extern int garmin_debug;
void logfn(char *file, int line, int level, char *fmt, ...);

