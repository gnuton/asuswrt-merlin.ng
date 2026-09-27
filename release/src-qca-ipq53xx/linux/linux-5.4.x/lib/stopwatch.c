/*
 * lib/stopwatch.c
 *   Creates /proc/stopwatch/ and associated functions
 *
 * This file is part of the Ubicom32 Linux Kernel Port.
 *
 * The Ubicom32 Linux Kernel Port is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Ubicom32 Linux Kernel Port is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Ubicom32 Linux Kernel Port.  If not,
 * see <http://www.gnu.org/licenses/>
 *
 */

#include <linux/types.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#define __STOPWATCH_USE__
#include <linux/stopwatch.h>

#define STOPWATCH_MAX_WATCHES 10

static DEFINE_SPINLOCK(stopwatch_init_lock);
static DEFINE_SPINLOCK(stopwatch_lock);
static struct proc_dir_entry *stopwatch_root;

#define STOPWATCH_FLAG_INUSE	0x01
#define STOPWTACH_FLAG_CREATED	0x02

static struct stopwatch {
	int flags;
	const char *name;
	int count;
	stopwatch_show_t show;
} stopwatches[STOPWATCH_MAX_WATCHES];

/*
 * stopwatch_find()
 *	Find the stop watch.
 */
static struct stopwatch *stopwatch_find(const char *name)
{
	int idx;
	unsigned long flags;

	spin_lock_irqsave(&stopwatch_lock, flags);
	for (idx = 0; idx < STOPWATCH_MAX_WATCHES; idx++) {
		/*
		 * Trying to find a match.
		 */
		struct stopwatch *sw = &stopwatches[idx];
		if (strcmp(sw->name, name)) {
			continue;
		}
		spin_unlock_irqrestore(&stopwatch_lock, flags);
		return sw;
	}
	spin_unlock_irqrestore(&stopwatch_lock, flags);
	return NULL;
}

/*
 * stopwatch_alloc()
 *	Allocate an entry, check for duplicates
 */
static struct stopwatch *stopwatch_alloc(const char *name)
{
	int idx;
	unsigned long flags;
	struct stopwatch *free = NULL;

	spin_lock_irqsave(&stopwatch_lock, flags);
	for (idx = 0; idx < STOPWATCH_MAX_WATCHES; idx++) {
		struct stopwatch *sw = &stopwatches[idx];

		/*
		 * Looking for an empty slot?
		 */
		if (!(sw->flags & STOPWATCH_FLAG_INUSE)) {
			free = sw;
			continue;
		}

		/*
		 * Trying to find a match.
		 */
		if (strcmp(sw->name, name) == 0) {
			printk(KERN_WARNING "[%s]: duplicate found: %d\n",
				__func__, idx);
			spin_unlock_irqrestore(&stopwatch_lock, flags);
			return NULL;
		}
	}

	/*
	 * If we have a free slot, use it.
	 */
	if (free) {
		free->name = name;
		free->flags |= STOPWATCH_FLAG_INUSE;
	}
	spin_unlock_irqrestore(&stopwatch_lock, flags);
	return free;
}

/*
 * stopwatch_seq_show()
 *	Internal show (redirects to the callers show).
 */
static int stopwatch_seq_show(struct seq_file *f, void *v)
{
	struct stopwatch *sw = f->private;
	if (!sw) {
		return 0;
	}
	return sw->show(f, v);
}

/*
 * stopwatch_seq_start()
 *	Internal seq start (uses configured count).
 */
static void *stopwatch_seq_start(struct seq_file *f, loff_t *pos)
{
	struct stopwatch *sw = f->private;
	if (!sw) {
		return NULL;
	}
	return (*pos < sw->count) ? pos : NULL;
}

/*
 * stopwatch_seq_next()
 *	Internal seq next (uses configured count).
 */
static void *stopwatch_seq_next(struct seq_file *f, void *v, loff_t *pos)
{
	struct stopwatch *sw = f->private;
	if (!sw) {
		return NULL;
	}

	(*pos)++;
	if (*pos >= sw->count) {
		return NULL;
	}
	return pos;
}

/*
 * stopwatch_seq_start()
 *	Empty stop function.
 */
static void stopwatch_seq_stop(struct seq_file *f, void *v)
{
	/* Nothing to do */
}

/*
 * Provide our own internal sequence operations.
 */
static const struct seq_operations stopwatch_seq_ops = {
	.start = stopwatch_seq_start,
	.next  = stopwatch_seq_next,
	.stop  = stopwatch_seq_stop,
	.show  = stopwatch_seq_show,
};

/*
 * stopwatch_open()
 *	Bind the file to the proper sequence operations
 */
static int stopwatch_open(struct inode *inode, struct file *f)
{
	struct seq_file *m;
	int ret;
	struct stopwatch *sw = PDE_DATA(inode);
	if (!sw) {
		printk(KERN_WARNING "[%s]: unable to find: %s\n",
		       __func__, f->f_path.dentry->d_name.name);
		return -ENOENT;
	}

	ret = seq_open(f, &stopwatch_seq_ops);
	if (ret < 0) {
		printk(KERN_WARNING "[%s]: unable to create sequence file: "
			"%s\n", __func__, f->f_path.dentry->d_name.name);
		return ret;
	}

	/*
	 * Transfer stopwatch pointer form inode to file.
	 */
	m = f->private_data;
	m->private = sw;
	return 0;
}

/*
 * List of file operations.
 */
static const struct file_operations stopwatch_fops = {
	.open		= stopwatch_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

/*
 * stopwatch_init()
 *	Initialize the stop watch service.
 */
static int stopwatch_init(void)
{
	/*
	 * Already inited, do nothing?
	 */
	spin_lock(&stopwatch_init_lock);
	if (stopwatch_root) {
		spin_unlock(&stopwatch_init_lock);
		return 0;
	}

	/*
	 * create /proc/stopwatch
	 */
	stopwatch_root = proc_mkdir("stopwatch", NULL);
	if (!stopwatch_root) {
		spin_unlock(&stopwatch_init_lock);
		printk(KERN_WARNING "[%s]: unable to create /proc/stopwatch\n",
			__func__);
		return -ENOMEM;
	}

	spin_unlock(&stopwatch_init_lock);
	return 0;
}
module_init(stopwatch_init);

/*
 * stopwtch_show()
 *	Print the stop watch values to the sequence file.
 */
void stopwatch_show(struct stopwatch_instance *si, struct seq_file *p,
			int precision)
{
	unsigned long flags;
	unsigned long long min, avg, max;
	unsigned long count;

	if (si->count == 0) {
		seq_printf(p, "   \t%u\t%u\t%u", 0, 0, 0);
		return;
	}

	spin_lock_irqsave(&si->lock, flags);

	min = si->min;
	avg = si->avg;
	max = si->max;
	count = si->count;

	si->min = 0;
	si->avg = 0;
	si->max = 0;
	si->count = 0;

	spin_unlock_irqrestore(&si->lock, flags);

	/* Calculate the actual average */
	do_div(avg, count);

	if (precision == STOPWATCH_MICRO) {
		do_div(min, 1000);
		do_div(avg, 1000);
		do_div(max, 1000);
	}

	seq_printf(p, "   \t%llu\t%llu\t%llu", min, avg, max);
}
EXPORT_SYMBOL(stopwatch_show);


/*
 * stopwatch_unregister()
 *	Remove the registered stopwatch.
 */
void stopwatch_unregister(const char *name)
{
	unsigned long flags;
	struct stopwatch *sw = stopwatch_find(name);
	if (!sw) {
		printk(KERN_WARNING "[%s]: unable to find: %s\n",
		       __func__, name);
		return;
	}
	remove_proc_entry(name, stopwatch_root);

	/*
	 * Remove an entry.
	 */
	spin_lock_irqsave(&stopwatch_lock, flags);
	sw->name = NULL;
	sw->flags = 0;
	spin_unlock_irqrestore(&stopwatch_lock, flags);
}
EXPORT_SYMBOL(stopwatch_unregister);

/*
 * stopwatch_register()
 *	Register a new stop watch counter group.
 */
unsigned int stopwatch_register(const char *name, int count,
					stopwatch_show_t show)
{
	unsigned long flags;
	struct stopwatch *sw;
	struct proc_dir_entry *pfe;

	/*
	 * If we try to register before the stopwatch init, just do
	 * the init first.
	 */
	if (!stopwatch_root) {
		stopwatch_init();
	}

	/*
	 * Check for duplicate name use.
	 */
	sw = stopwatch_alloc(name);
	if (!sw) {
		printk(KERN_WARNING "[%s]: unable to allocate empty slot\n",
		       __func__);
		return -ENOMEM;
	}

	/*
	 * Create a node in the stop watch directory.
	 */
	pfe = proc_create_data(name, 0, stopwatch_root, &stopwatch_fops, (void *)sw);
	if(pfe == NULL)


	spin_lock_irqsave(&stopwatch_lock, flags);
	sw->count = count;
	sw->show = show;
	sw->flags |= STOPWTACH_FLAG_CREATED;
	spin_unlock_irqrestore(&stopwatch_lock, flags);
	return 0;
}
EXPORT_SYMBOL(stopwatch_register);
