/*
 *  linux/kernel/tty_io.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * 'tty_io.c' gives an orthogonal feeling to tty's, be they consoles
 * or rs-channels. It also implements echoing, cooked mode etc.
 *
 * Kill-line thanks to John T Kohl, who also corrected VMIN = VTIME = 0.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/ctype.h>

#include <asm/io.h>
#include <asm/segment.h>
#include <asm/system.h>

#include <sys/kd.h>
#include "vt_kern.h"

#define QUEUES	(3*(NR_CONSOLES+NR_SERIALS+2*NR_PTYS))
static struct tty_queue * tty_queues;
struct tty_struct tty_table[256];

#define con_queues tty_queues
#define rs_queues ((3*NR_CONSOLES) + tty_queues)
#define mpty_queues ((3*(NR_CONSOLES+NR_SERIALS)) + tty_queues)
#define spty_queues ((3*(NR_CONSOLES+NR_SERIALS+NR_PTYS)) + tty_queues)

#define con_table tty_table
#define rs_table (64+tty_table)
#define mpty_table (128+tty_table)
#define spty_table (192+tty_table)

/*
 * fg_console is the current virtual console,
 * redirect is the pseudo-tty that console output
 * is redirected to if asked by TIOCCONS.
 */
int fg_console = 0;
struct tty_struct * redirect = NULL;

/*
 * these are the tables used by the machine code handlers.
 * you can implement virtual consoles.
 */
struct tty_queue * table_list[] = { NULL, NULL };

void inline put_tty_queue(char c, struct tty_queue * queue)
{
	int head;
	unsigned long flags;

	__asm__ __volatile__("pushfl ; popl %0 ; cli":"=r" (flags));
	head = (queue->head + 1) & (TTY_BUF_SIZE-1);
	if (head != queue->tail) {
		queue->buf[queue->head] = c;
		queue->head = head;
	}
	__asm__ __volatile__("pushl %0 ; popfl"::"r" (flags));
}

int inline get_tty_queue(struct tty_queue * queue)
{
	int result = -1;
	unsigned long flags;

	__asm__ __volatile__("pushfl ; popl %0 ; cli":"=r" (flags));
	if (queue->tail != queue->head) {
		result = 0xff & queue->buf[queue->tail];
		queue->tail = (queue->tail + 1) & (TTY_BUF_SIZE-1);
	}
	__asm__ __volatile__("pushl %0 ; popfl"::"r" (flags));
	return result;
}

void inline tty_write_flush(struct tty_struct * tty)
{
	if (EMPTY(tty->write_q))
		return;
	if (set_bit(TTY_WRITE_BUSY,&tty->flags))
		return;
	tty->write(tty);
	if (clear_bit(TTY_WRITE_BUSY,&tty->flags))
		printk("tty_write_flush: bit already cleared\n");
}

void tty_read_flush(struct tty_struct * tty)
{
	if (EMPTY(tty->read_q))
		return;
	if (set_bit(TTY_READ_BUSY, &tty->flags))
		return;
	copy_to_cooked(tty);
	if (clear_bit(TTY_READ_BUSY, &tty->flags))
		printk("tty_read_flush: bit already cleared\n");
}

void change_console(unsigned int new_console)
{
	if (vt_cons[fg_console].vt_mode == KD_GRAPHICS)
		return;
	if (new_console == fg_console || new_console >= NR_CONSOLES)
		return;
	table_list[0] = con_queues + 0 + new_console*3;
	table_list[1] = con_queues + 1 + new_console*3;
	update_screen(new_console);
}

static void sleep_if_empty(struct tty_queue * queue)
{
	cli();
	while (!(current->signal & ~current->blocked) && EMPTY(queue))
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

void wait_for_keypress(void)
{
	sleep_if_empty(tty_table[fg_console].secondary);
	flush_input(&tty_table[fg_console]);
}

void copy_to_cooked(struct tty_struct * tty)
{
	int c;

	if (!(tty && tty->write && tty->read_q &&
	    tty->write_q && tty->secondary)) {
		printk("copy_to_cooked: missing queues\n\r");
		return;
	}
	while (1) {
		if (FULL(tty->secondary))
			break;
		c = get_tty_queue(tty->read_q);
		if (c < 0)
			break;
		if (I_STRP(tty))
			c &= 0x7f;
		if (c==13) {
			if (I_CRNL(tty))
				c=10;
			else if (I_NOCR(tty))
				continue;
		} else if (c==10 && I_NLCR(tty))
			c=13;
		if (I_UCLC(tty))
			c=tolower(c);
		if (L_CANON(tty)) {
			if ((KILL_CHAR(tty) != __DISABLED_CHAR) &&
			    (c==KILL_CHAR(tty))) {
				/* deal with killing the input line */
				while(!(EMPTY(tty->secondary) ||
					(c=LAST(tty->secondary))==10 ||
					((EOF_CHAR(tty) != __DISABLED_CHAR) &&
					 (c==EOF_CHAR(tty))))) {
					if (L_ECHO(tty)) {
						if (c<32) {
							put_tty_queue(8,tty->write_q);
							put_tty_queue(' ',tty->write_q);
							put_tty_queue(8,tty->write_q);
						}
						put_tty_queue(8,tty->write_q);
						put_tty_queue(' ',tty->write_q);
						put_tty_queue(8,tty->write_q);
					}
					DEC(tty->secondary->head);
				}
				continue;
			}
			if ((ERASE_CHAR(tty) != __DISABLED_CHAR) &&
			    (c==ERASE_CHAR(tty))) {
				if (EMPTY(tty->secondary) ||
				   (c=LAST(tty->secondary))==10 ||
				   ((EOF_CHAR(tty) != __DISABLED_CHAR) &&
				    (c==EOF_CHAR(tty))))
					continue;
				if (L_ECHO(tty)) {
					if (c<32) {
						put_tty_queue(8,tty->write_q);
						put_tty_queue(' ',tty->write_q);
						put_tty_queue(8,tty->write_q);
					}
					put_tty_queue(8,tty->write_q);
					put_tty_queue(32,tty->write_q);
					put_tty_queue(8,tty->write_q);
				}
				DEC(tty->secondary->head);
				continue;
			}
		}
		if (I_IXON(tty)) {
			if ((STOP_CHAR(tty) != __DISABLED_CHAR) &&
			    (c==STOP_CHAR(tty))) {
			        tty->status_changed = 1;
				tty->ctrl_status |= TIOCPKT_STOP;
				tty->stopped=1;
				continue;
			}
			if ((START_CHAR(tty) != __DISABLED_CHAR) &&
			    (c==START_CHAR(tty))) {
			        tty->status_changed = 1;
				tty->ctrl_status |= TIOCPKT_START;
				tty->stopped=0;
				continue;
			}
		}
		if (L_ISIG(tty)) {
			if ((INTR_CHAR(tty) != __DISABLED_CHAR) &&
			    (c==INTR_CHAR(tty))) {
				kill_pg(tty->pgrp, SIGINT, 1);
				flush_input(tty);
				continue;
			}
			if ((QUIT_CHAR(tty) != __DISABLED_CHAR) &&
			    (c==QUIT_CHAR(tty))) {
				kill_pg(tty->pgrp, SIGQUIT, 1);
				flush_input(tty);
				continue;
			}
			if ((SUSPEND_CHAR(tty) != __DISABLED_CHAR) &&
			    (c==SUSPEND_CHAR(tty))) {
				if (!is_orphaned_pgrp(tty->pgrp))
					kill_pg(tty->pgrp, SIGTSTP, 1);
				continue;
			}
		}
		if (c==10 || (EOF_CHAR(tty) != __DISABLED_CHAR &&
		    c==EOF_CHAR(tty)))
			tty->secondary->data++;
		if ((c==10) && (L_ECHO(tty) || (L_CANON(tty) && L_ECHONL(tty)))) {
			put_tty_queue(10,tty->write_q);
			put_tty_queue(13,tty->write_q);
		} else if (L_ECHO(tty)) {
			if (c<32 && L_ECHOCTL(tty)) {
				put_tty_queue('^',tty->write_q);
				put_tty_queue(c+64,tty->write_q);
			} else
				put_tty_queue(c,tty->write_q);
		}
		put_tty_queue(c,tty->secondary);
	}
	TTY_WRITE_FLUSH(tty);
	if (!EMPTY(tty->secondary))
		wake_up(&tty->secondary->proc_list);
	if (tty->write_q->proc_list && LEFT(tty->write_q) > TTY_BUF_SIZE/2)
		wake_up(&tty->write_q->proc_list);
}

int is_ignored(int sig)
{
	return ((current->blocked & (1<<(sig-1))) ||
	        (current->sigaction[sig-1].sa_handler == SIG_IGN));
}

/*
 * Called when we need to send a SIGTTIN or SIGTTOU to our process
 * group
 * 
 * We only request that a system call be restarted if there was if the 
 * default signal handler is being used.  The reason for this is that if
 * a job is catching SIGTTIN or SIGTTOU, the signal handler may not want 
 * the system call to be restarted blindly.  If there is no way to reset the
 * terminal pgrp back to the current pgrp (perhaps because the controlling
 * tty has been released on logout), we don't want to be in an infinite loop
 * while restarting the system call, and have it always generate a SIGTTIN
 * or SIGTTOU.  The default signal handler will cause the process to stop
 * thus avoiding the infinite loop problem.  Presumably the job-control
 * cognizant parent will fix things up before continuging its child process.
 */
int tty_signal(int sig, struct tty_struct *tty)
{
	(void) kill_pg(current->pgrp,sig,1);
	return -ERESTARTSYS;
}

static void wait_for_canon_input(struct tty_struct * tty)
{
	while (1) {
		TTY_READ_FLUSH(tty);
		if (tty->link)
			if (tty->link->count)
				TTY_WRITE_FLUSH(tty->link);
			else
				return;
		if (current->signal & ~current->blocked)
			return;
		if (FULL(tty->read_q))
			return;
		if (tty->secondary->data)
			return;
		cli();
		if (!tty->secondary->data)
			interruptible_sleep_on(&tty->secondary->proc_list);
		sti();
	}
}

static int read_chan(unsigned int channel, struct file * file, char * buf, int nr)
{
	struct tty_struct * tty;
	int c;
	char * b=buf;
	int minimum,time;

	if (channel > 255)
		return -EIO;
	tty = TTY_TABLE(channel);
	if (!(tty->read_q && tty->secondary))
		return -EIO;
	if ((tty->pgrp > 0) &&
	    (current->tty == channel) &&
	    (tty->pgrp != current->pgrp))
		if (is_ignored(SIGTTIN) || is_orphaned_pgrp(current->pgrp))
			return -EIO;
		else
			return(tty_signal(SIGTTIN, tty));
	if (L_CANON(tty))
		minimum = time = current->timeout = 0;
	else {
		time = 10L*tty->termios.c_cc[VTIME];
		minimum = tty->termios.c_cc[VMIN];
		if (minimum)
			current->timeout = 0xffffffff;
		else {
			if (time)
				current->timeout = time + jiffies;
			else
				current->timeout = 0;
			time = 0;
			minimum = 1;
		}
	}
	if (file->f_flags & O_NONBLOCK)
		time = current->timeout = 0;
	else if (L_CANON(tty)) {
		wait_for_canon_input(tty);
		if (current->signal & ~current->blocked)
			return -ERESTARTSYS;
	}
	if (minimum>nr)
		minimum = nr;

	/* deal with packet mode:  First test for status change */
	if (tty->packet && tty->link && tty->link->status_changed)
	  {
	     put_fs_byte (tty->link->ctrl_status, b);
	     tty->link->status_changed = 0;
	     return (1);
	  }
	  
	/* now bump the buffer up one. */
	if (tty->packet)
	  {
	     put_fs_byte (0,b++);
	     nr --;
	     /* this really shouldn't happen, but we need to 
		put it here. */
	     if (nr == 0) return (1);
	  }

	while (nr>0) {
		TTY_READ_FLUSH(tty);
		if (tty->link)
			TTY_WRITE_FLUSH(tty->link);
		while (nr > 0 && ((c = get_tty_queue(tty->secondary)) >= 0)) {
			if ((EOF_CHAR(tty) != __DISABLED_CHAR &&
			     c==EOF_CHAR(tty)) || c==10)
				tty->secondary->data--;
			if ((EOF_CHAR(tty) != __DISABLED_CHAR &&
			     c==EOF_CHAR(tty)) && L_CANON(tty))
				break;
			put_fs_byte(c,b++);
			nr--;
			if (time)
				current->timeout = time+jiffies;
			if (c==10 && L_CANON(tty))
				break;
		};
		wake_up(&tty->read_q->proc_list);
		if (b-buf >= minimum || !current->timeout)
			break;
		if (current->signal & ~current->blocked) 
			break;
		if (tty->link && !tty->link->count)
			break;
		TTY_READ_FLUSH(tty);
		if (tty->link)
			TTY_WRITE_FLUSH(tty->link);
		cli();
		if (EMPTY(tty->secondary))
			interruptible_sleep_on(&tty->secondary->proc_list);
		sti();
	}
	TTY_READ_FLUSH(tty);
	if (tty->link && tty->link->write)
		TTY_WRITE_FLUSH(tty->link);
	current->timeout = 0;

	/* packet mode sticks in an extra 0.  If that's all we've got,
	   we should count it a zero bytes. */
	if (tty->packet)
	  {
	     if ((b-buf) > 1)
	       return b-buf;
	  }
	else
	  {
	     if (b-buf)
	       return b-buf;
	  }

	if (current->signal & ~current->blocked)
		return -ERESTARTSYS;
	if (file->f_flags & O_NONBLOCK)
		return -EAGAIN;
	return 0;
}

static int write_chan(unsigned int channel, struct file * file, char * buf, int nr)
{
	struct tty_struct * tty;
	char c, *b=buf;

	if (channel > 255)
		return -EIO;
	tty = TTY_TABLE(channel);
	if (L_TOSTOP(tty) && (tty->pgrp > 0) &&
	    (current->tty == channel) && (tty->pgrp != current->pgrp)) {
		if (is_orphaned_pgrp(tty->pgrp))
			return -EIO;
		if (!is_ignored(SIGTTOU))
			return tty_signal(SIGTTOU, tty);
	}
	if (nr < 0)
		return -EINVAL;
	if (!nr)
		return 0;
	if (redirect && tty == TTY_TABLE(0))
		tty = redirect;
	if (!(tty->write_q && tty->write))
		return -EIO;
	while (nr>0) {
		if (current->signal & ~current->blocked)
			break;
		if (tty->link && !tty->link->count) {
			send_sig(SIGPIPE,current,0);
			break;
		}
		if (FULL(tty->write_q)) {
			TTY_WRITE_FLUSH(tty);
			cli();
			if (FULL(tty->write_q))
				interruptible_sleep_on(&tty->write_q->proc_list);
			sti();
			continue;
		}
		while (nr>0 && !FULL(tty->write_q)) {
			c=get_fs_byte(b);
			if (O_POST(tty)) {
				if (c=='\r' && O_CRNL(tty))
					c='\n';
				else if (c=='\n' && O_NLRET(tty))
					c='\r';
				if (c=='\n' && O_NLCR(tty) &&
				    !set_bit(TTY_CR_PENDING,&tty->flags)) {
					put_tty_queue(13,tty->write_q);
					continue;
				}
				if (O_LCUC(tty))
					c=toupper(c);
			}
			b++; nr--;
			clear_bit(TTY_CR_PENDING,&tty->flags);
			put_tty_queue(c,tty->write_q);
		}
		if (nr>0)
			schedule();
	}
	TTY_WRITE_FLUSH(tty);
	if (b-buf)
		return b-buf;
	if (tty->link && !tty->link->count)
		return -EPIPE;
	if (current->signal & ~current->blocked)
		return -ERESTARTSYS;
	return 0;
}

static int tty_read(struct inode * inode, struct file * file, char * buf, int count)
{
	int i;

	if (MAJOR(file->f_rdev) != 4) {
		printk("tty_read: pseudo-major != 4\n");
		return -EINVAL;
	}
	i = read_chan(MINOR(file->f_rdev),file,buf,count);
	if (i > 0)
		inode->i_atime = CURRENT_TIME;
	return i;
}

static int tty_write(struct inode * inode, struct file * file, char * buf, int count)
{
	int i;
	
	if (MAJOR(file->f_rdev) != 4) {
		printk("tty_write: pseudo-major != 4\n");
		return -EINVAL;
	}
	i = write_chan(MINOR(file->f_rdev),file,buf,count);
	if (i > 0)
		inode->i_mtime = CURRENT_TIME;
	return i;
}

static int tty_lseek(struct inode * inode, struct file * file, off_t offset, int orig)
{
	return -EBADF;
}

/*
 * tty_open and tty_release keep up the tty count that contains the
 * number of opens done on a tty. We cannot use the inode-count, as
 * different inodes might point to the same tty.
 *
 * Open-counting is needed for pty masters, as well as for keeping
 * track of serial lines: DTR is dropped when the last close happens.
 */
static int tty_open(struct inode * inode, struct file * filp)
{
	struct tty_struct *tty;
	int dev, retval;

	dev = inode->i_rdev;
	if (MAJOR(dev) == 5)
		dev = current->tty;
	else
		dev = MINOR(dev);
	if (dev < 0)
		return -ENODEV;
	filp->f_rdev = 0x0400 | dev;
	tty = TTY_TABLE(dev);
	if (!tty->count && !(tty->link && tty->link->count)) {
		flush_input(tty);
		flush_output(tty);
		tty->stopped = 0;
	}
	if (IS_A_PTY_MASTER(dev)) {
		if (tty->count)
			return -EAGAIN;
		if (tty->link)
			tty->link->count++;

		/* perhaps user applications that don't take care of
		   this deserve what the get, but I think my system
		   has hung do to this, esp. in X. -RAB */
		tty->termios.c_lflag &= ~ECHO;
	}
	tty->count++;
	retval = 0;

	/* clean up the packet stuff. */
	tty->status_changed = 0;
	tty->ctrl_status = 0;
	tty->packet = 0;

	if (!(filp->f_flags & O_NOCTTY) &&
	    current->leader &&
	    current->tty<0 &&
	    tty->session==0) {
		current->tty = dev;
		tty->session = current->session;
		tty->pgrp = current->pgrp;
	}
	if (IS_A_SERIAL(dev) && tty->count < 2)
		retval = serial_open(dev-64,filp);
	else if (IS_A_PTY(dev))
		retval = pty_open(dev,filp);
	if (retval) {
		tty->count--;
		if (IS_A_PTY_MASTER(dev) && tty->link)
			tty->link->count--;
	}
	return retval;
}

/*
 * Note that releasing a pty master also releases the child, so
 * we have to make the redirection checks after that and on both
 * sides of a pty.
 */
static void tty_release(struct inode * inode, struct file * filp)
{
	int dev;
	struct tty_struct * tty;

	dev = filp->f_rdev;
	if (MAJOR(dev) != 4) {
		printk("tty_close: tty pseudo-major != 4\n");
		return;
	}
	dev = MINOR(filp->f_rdev);
	tty = TTY_TABLE(dev);
	if (IS_A_PTY_MASTER(dev) && tty->link)
		tty->link->count--;
	tty->count--;
	if (tty->count)
		return;
	if (IS_A_SERIAL(dev)) {
		wait_until_sent(tty);
		serial_close(dev-64,filp);
	} else if (IS_A_PTY(dev))
		pty_close(dev,filp);
	if (!tty->count && (tty == redirect))
		redirect = NULL;
	if (tty = tty->link)
		if (!tty->count && (tty == redirect))
			redirect = NULL;
}

static int tty_select(struct inode * inode, struct file * filp, int sel_type, select_table * wait)
{
	int dev;
	struct tty_struct * tty;

	dev = filp->f_rdev;
	if (MAJOR(dev) != 4) {
		printk("tty_select: tty pseudo-major != 4\n");
		return 0;
	}
	dev = MINOR(filp->f_rdev);
	tty = TTY_TABLE(dev);
	switch (sel_type) {
		case SEL_IN:
			if (!EMPTY(tty->secondary))
				return 1;
			if (tty->link && !tty->link->count)
				return 1;

			/* see if the status byte can be read. */
			if (tty->packet && tty->link &&
			    tty->link->status_changed)
			  return 1;

			select_wait(&tty->secondary->proc_list, wait);
			return 0;
		case SEL_OUT:
			if (!FULL(tty->write_q))
				return 1;
			select_wait(&tty->write_q->proc_list, wait);
			return 0;
		case SEL_EX:
			if (tty->link && !tty->link->count)
				return 1;
			return 0;
	}
	return 0;
}

static struct file_operations tty_fops = {
	tty_lseek,
	tty_read,
	tty_write,
	NULL,		/* tty_readdir */
	tty_select,
	tty_ioctl,
	tty_open,
	tty_release
};

long tty_init(long kmem_start)
{
	int i;

	tty_queues = (struct tty_queue *) kmem_start;
	kmem_start += QUEUES * (sizeof (struct tty_queue));
	table_list[0] = con_queues + 0;
	table_list[1] = con_queues + 1;
	chrdev_fops[4] = &tty_fops;
	chrdev_fops[5] = &tty_fops;
	for (i=0 ; i < QUEUES ; i++)
		tty_queues[i] = (struct tty_queue) {0,0,0,0,""};
	for (i=0 ; i<256 ; i++) {
		tty_table[i] =  (struct tty_struct) {
		 	{0, 0, 0, 0, 0, INIT_C_CC},
			-1, 0, 0, 0, 0, 0, 0, 0, 0, {0,0,0,0},
			NULL, NULL, NULL, NULL, NULL
		};
	}
	kmem_start = con_init(kmem_start);
	for (i = 0 ; i<NR_CONSOLES ; i++) {
		con_table[i] = (struct tty_struct) {
		 	{ICRNL,		/* change incoming CR to NL */
			OPOST|ONLCR,	/* change outgoing NL to CRNL */
			B38400 | CS8,
			IXON | ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,
			0,		/* console termio */
			INIT_C_CC},
			-1,		/* initial pgrp */
			0,			/* initial session */
			0,			/* initial stopped */
			0,			/* initial status_changed */
			0,			/* initial packet */
			0,			/* initial ctrl_status */
			0,			/* initial unused */
			0,			/* initial flags */
			0,			/* initial count */
			{video_num_lines,video_num_columns,0,0},
			con_write,
			NULL,		/* other-tty */
			con_queues+0+i*3,con_queues+1+i*3,con_queues+2+i*3
		};
	}
	for (i = 0 ; i<NR_SERIALS ; i++) {
		rs_table[i] = (struct tty_struct) {
			{0, /* no translation */
			0,  /* no translation */
			B2400 | CS8,
			0,
			0,
			INIT_C_CC},
			-1,
			0,
			0, 0, 0, 0, 0,
			0,
			0,
			{24,80,0,0},
			rs_write,
			NULL,		/* other-tty */
			rs_queues+0+i*3,rs_queues+1+i*3,rs_queues+2+i*3
		};
	}
	for (i = 0 ; i<NR_PTYS ; i++) {
		mpty_table[i] = (struct tty_struct) {
			{0, /* no translation */
			0,  /* no translation */
			B9600 | CS8,
			0,
			0,
			INIT_C_CC},
			-1,
			0,
			0, 0, 0, 0, 0,
			0,
			0,
			{24,80,0,0},
			mpty_write,
			spty_table+i,
			mpty_queues+0+i*3,mpty_queues+1+i*3,mpty_queues+2+i*3
		};
		spty_table[i] = (struct tty_struct) {
			{0, /* no translation */
			0,  /* no translation */
			B9600 | CS8,
			IXON | ISIG | ICANON,
			0,
			INIT_C_CC},
			-1,
			0,
			0, 0, 0, 0, 0,
			0,
			0,
			{24,80,0,0},
			spty_write,
			mpty_table+i,
			spty_queues+0+i*3,spty_queues+1+i*3,spty_queues+2+i*3
		};
	}
	kmem_start = rs_init(kmem_start);
	printk("%d virtual consoles\n\r",NR_CONSOLES);
	printk("%d pty's\n\r",NR_PTYS);
	return kmem_start;
}
