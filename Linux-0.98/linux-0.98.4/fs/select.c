/*
 * This file contains the procedures for the handling of select
 *
 * Created for Linux based loosely upon Mathius Lattner's minix
 * patches by Peter MacDonald. Heavily edited by Linus.
 */

#include <linux/types.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/signal.h>
#include <linux/errno.h>

#include <asm/segment.h>
#include <asm/system.h>

/*
 * Ok, Peter made a complicated, but straightforward multiple_wait() function.
 * I have rewritten this, taking some shortcuts: This code may not be easy to
 * follow, but it should be free of race-conditions, and it's practical. If you
 * understand what I'm doing here, then you understand how the linux
 * sleep/wakeup mechanism works.
 *
 * Two very simple procedures, select_wait() and free_wait() make all the work.
 * select_wait() is a inline-function defined in <linux/sched.h>, as all select
 * functions have to call it to add an entry to the select table.
 */

/*
 * I rewrote this again to make the select_table size variable, take some
 * more shortcuts, improve responsiveness, and remove another race that
 * Linus noticed.  -- jrs
 */

static void free_wait(select_table * p)
{
	struct select_table_entry * entry = p->entry + p->nr;

	while (p->nr > 0) {
		p->nr--;
		entry--;
		remove_wait_queue(entry->wait_address,&entry->wait);
	}
}

/*
 * The check function checks the ready status of a file using the vfs layer.
 *
 * If the file was not ready we were added to its wait queue.  But in
 * case it became ready just after the check and just before it called
 * select_wait, we call it again, knowing we are already on its
 * wait queue this time.  The second call is not necessary if the
 * select_table is NULL indicating an earlier file check was ready
 * and we aren't going to sleep on the select_table.  -- jrs
 */

static int check(int flag, select_table * wait, struct file * file)
{
	struct inode * inode;
	struct file_operations *fops;
	int (*select) (struct inode *, struct file *, int, select_table *);

	inode = file->f_inode;
	if ((fops = file->f_op) && (select = fops->select))
		return select(inode, file, flag, wait)
		    || (wait && select(inode, file, flag, NULL));
	if (S_ISREG(inode->i_mode))
		return 1;
	return 0;
}

int do_select(int n, fd_set *in, fd_set *out, fd_set *ex,
	fd_set *res_in, fd_set *res_out, fd_set *res_ex)
{
	int count;
	select_table wait_table, *wait;
	struct select_table_entry *entry;
	int i;
	int max;

	max = -1;
	for (i = 0 ; i < n ; i++) {
		if (!FD_ISSET(i, in) &&
		    !FD_ISSET(i, out) &&
		    !FD_ISSET(i, ex))
			continue;
		if (!current->filp[i])
			return -EBADF;
		if (!current->filp[i]->f_inode)
			return -EBADF;
		max = i;
	}
	n = max + 1;
	entry = (struct select_table_entry *) get_free_page(GFP_KERNEL);
	if (!entry)
		return -ENOMEM;
	FD_ZERO(res_in);
	FD_ZERO(res_out);
	FD_ZERO(res_ex);
	count = 0;
repeat:
	wait_table.nr = 0;
	wait_table.entry = entry;
	current->state = TASK_INTERRUPTIBLE;
	wait = &wait_table;
	for (i = 0 ; i < n ; i++) {
		if (FD_ISSET(i,in) && check(SEL_IN,wait,current->filp[i])) {
			FD_SET(i, res_in);
			count++;
			wait = NULL;
		}
		if (FD_ISSET(i,out) && check(SEL_OUT,wait,current->filp[i])) {
			FD_SET(i, res_out);
			count++;
			wait = NULL;
		}
		if (FD_ISSET(i,ex) && check(SEL_EX,wait,current->filp[i])) {
			FD_SET(i, res_ex);
			count++;
			wait = NULL;
		}
	}
	if (!count && current->timeout
	    && !(current->signal & ~current->blocked)) {
		schedule();
		free_wait(&wait_table);
		goto repeat;
	}
	free_wait(&wait_table);
	free_page((unsigned long) entry);
	current->state = TASK_RUNNING;
	return count;
}

static void __get_fd_set(int nr, unsigned long * fs_pointer, unsigned long * fdset)
{
	FD_ZERO(fdset);
	if (!fs_pointer)
		return;
	while (nr > 0) {
		*fdset = get_fs_long(fs_pointer);
		fdset++;
		fs_pointer++;
		nr -= 32;
	}
}

static void __set_fd_set(int nr, unsigned long * fs_pointer, unsigned long * fdset)
{
	if (!fs_pointer)
		return;
	verify_area(fs_pointer, sizeof(fd_set));
	while (nr > 0) {
		put_fs_long(*fdset, fs_pointer);
		fdset++;
		fs_pointer++;
		nr -= 32;
	}
}

#define get_fd_set(nr,fsp,fdp) \
__get_fd_set(nr, (unsigned long *) (fsp), (unsigned long *) (fdp))

#define set_fd_set(nr,fsp,fdp) \
__set_fd_set(nr, (unsigned long *) (fsp), (unsigned long *) (fdp))

/*
 * We can actually return ERESTARTSYS insetad of EINTR, but I'd
 * like to be certain this leads to no problems. So I return
 * EINTR just for safety.
 */
int sys_select( unsigned long *buffer )
{
/* Perform the select(nd, in, out, ex, tv) system call. */
	int i;
	fd_set res_in, in, *inp;
	fd_set res_out, out, *outp;
	fd_set res_ex, ex, *exp;
	int n;
	struct timeval *tvp;
	unsigned long timeout;

	n = get_fs_long(buffer++);
	if (n < 0)
		return -EINVAL;
	if (n > NR_OPEN)
		n = NR_OPEN;
	inp = (fd_set *) get_fs_long(buffer++);
	outp = (fd_set *) get_fs_long(buffer++);
	exp = (fd_set *) get_fs_long(buffer++);
	tvp = (struct timeval *) get_fs_long(buffer);
	get_fd_set(n, inp, &in);
	get_fd_set(n, outp, &out);
	get_fd_set(n, exp, &ex);
	timeout = 0xffffffff;
	if (tvp) {
		timeout = get_fs_long((unsigned long *)&tvp->tv_usec)/(1000000/HZ);
		timeout += get_fs_long((unsigned long *)&tvp->tv_sec) * HZ;
		timeout += jiffies;
	}
	current->timeout = timeout;
	i = do_select(n, &in, &out, &ex, &res_in, &res_out, &res_ex);
	if (current->timeout > jiffies)
		timeout = current->timeout - jiffies;
	else
		timeout = 0;
	current->timeout = 0;
	if (tvp) {
		verify_area(tvp, sizeof(*tvp));
		put_fs_long(timeout/HZ, (unsigned long *) &tvp->tv_sec);
		timeout %= HZ;
		timeout *= (1000000/HZ);
		put_fs_long(timeout, (unsigned long *) &tvp->tv_usec);
	}
	if (i < 0)
		return i;
	if (!i && (current->signal & ~current->blocked))
		return -EINTR;
	set_fd_set(n, inp, &res_in);
	set_fd_set(n, outp, &res_out);
	set_fd_set(n, exp, &res_ex);
	return i;
}
