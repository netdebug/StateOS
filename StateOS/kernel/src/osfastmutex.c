/******************************************************************************

    @file    StateOS: osfastmutex.c
    @author  Rajmund Szymanski
    @date    07.10.2018
    @brief   This file provides set of functions for StateOS.

 ******************************************************************************

   Copyright (c) 2018 Rajmund Szymanski. All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.

 ******************************************************************************/

#include "inc/osfastmutex.h"
#include "inc/oscriticalsection.h"
#include "osalloc.h"

/* -------------------------------------------------------------------------- */
void mut_init( mut_t *mut )
/* -------------------------------------------------------------------------- */
{
	assert_tsk_context();
	assert(mut);

	sys_lock();
	{
		memset(mut, 0, sizeof(mut_t));

		core_obj_init(&mut->obj);
	}
	sys_unlock();
}

/* -------------------------------------------------------------------------- */
mut_t *mut_create( void )
/* -------------------------------------------------------------------------- */
{
	mut_t *mut;

	assert_tsk_context();

	sys_lock();
	{
		mut = sys_alloc(sizeof(mut_t));
		mut_init(mut);
		mut->obj.res = mut;
	}
	sys_unlock();

	return mut;
}

/* -------------------------------------------------------------------------- */
void mut_kill( mut_t *mut )
/* -------------------------------------------------------------------------- */
{
	assert_tsk_context();
	assert(mut);
	assert(mut->obj.res!=RELEASED);

	sys_lock();
	{
		core_all_wakeup(mut->obj.queue, E_STOPPED);
	}
	sys_unlock();
}

/* -------------------------------------------------------------------------- */
void mut_delete( mut_t *mut )
/* -------------------------------------------------------------------------- */
{
	assert_tsk_context();
	assert(mut);
	assert(mut->obj.res!=RELEASED);

	sys_lock();
	{
		mut_kill(mut);
		core_res_free(&mut->obj.res);
	}
	sys_unlock();
}

/* -------------------------------------------------------------------------- */
static
unsigned priv_mut_take( mut_t *mut )
/* -------------------------------------------------------------------------- */
{
	if (mut->owner == 0)
	{
		mut->owner = System.cur;
		return E_SUCCESS;
	}

	if (mut->owner != System.cur)
		return E_TIMEOUT;

	return E_FAILURE;
}

/* -------------------------------------------------------------------------- */
unsigned mut_take( mut_t *mut )
/* -------------------------------------------------------------------------- */
{
	unsigned event;

	assert_tsk_context();
	assert(mut);
	assert(mut->obj.res!=RELEASED);

	sys_lock();
	{
		event = priv_mut_take(mut);
	}
	sys_unlock();

	return event;
}

/* -------------------------------------------------------------------------- */
unsigned mut_waitFor( mut_t *mut, cnt_t delay )
/* -------------------------------------------------------------------------- */
{
	unsigned event;

	assert_tsk_context();
	assert(mut);
	assert(mut->obj.res!=RELEASED);

	sys_lock();
	{
		event = priv_mut_take(mut);

		if (event == E_TIMEOUT)
			event = core_tsk_waitFor(&mut->obj.queue, delay);
	}
	sys_unlock();

	return event;
}

/* -------------------------------------------------------------------------- */
unsigned mut_waitUntil( mut_t *mut, cnt_t time )
/* -------------------------------------------------------------------------- */
{
	unsigned event;

	assert_tsk_context();
	assert(mut);
	assert(mut->obj.res!=RELEASED);

	sys_lock();
	{
		event = priv_mut_take(mut);

		if (event == E_TIMEOUT)
			event = core_tsk_waitUntil(&mut->obj.queue, time);
	}
	sys_unlock();

	return event;
}

/* -------------------------------------------------------------------------- */
static
unsigned priv_mut_give( mut_t *mut )
/* -------------------------------------------------------------------------- */
{
	if (mut->owner == System.cur)
	{
		mut->owner = core_one_wakeup(mut->obj.queue, E_SUCCESS);
		return E_SUCCESS;
	}

	return E_FAILURE;
}

/* -------------------------------------------------------------------------- */
unsigned mut_give( mut_t *mut )
/* -------------------------------------------------------------------------- */
{
	unsigned event;

	assert_tsk_context();
	assert(mut);
	assert(mut->obj.res!=RELEASED);

	sys_lock();
	{
		event = priv_mut_give(mut);
	}
	sys_unlock();

	return event;
}

/* -------------------------------------------------------------------------- */
