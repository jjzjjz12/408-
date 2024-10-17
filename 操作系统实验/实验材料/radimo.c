/*
 * Sample Ram DIsk Module, Radimo
 *
 */
#define __KERNEL__
#define MODULE

#include <linux/module.h>

#if defined(CONFIG_SMP)
#define __SMP__
#endif

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>

#include "radimo.h"

/*以上定义了一些常用的头文件*/

#define MAJOR_NR		RADIMO_MAJOR		// 定义了RADIMO的主设备号
#define DEVICE_NAME		"radimo"		// 定义了RADIMO的设备名
#define DEVICE_NR(device)	(MINOR(device))		
#define DEVICE_ON(device)
#define DEVICE_OFF(device)
#define DEVICE_NO_RANDOM

#include <linux/blk.h>

#define RADIMO_HARDS_BITS	9	/* 2**9 byte hardware sector */
#define RADIMO_BLOCK_SIZE	1024	/* block size */	//定义了一个块的大小，以字节为单位
#define RADIMO_TOTAL_SIZE	2048	/* size in blocks */	//定义了这个虚拟盘的容量，以块为单位

/* the storage pool */
static char *radimo_storage;		//这个指针是全局变量，指向用于虚拟盘的内存

static int radimo_hard = 1 << RADIMO_HARDS_BITS;
static int radimo_soft = RADIMO_BLOCK_SIZE;
static int radimo_size = RADIMO_TOTAL_SIZE;

static int radimo_readahead = 4;

struct timer_list radimo_timer;		//定义了一个定时器，这个程序中我们并没有用到。这里作为一个使用定时器的例子

/* forward declarations for _fops */
static int radimo_open(struct inode *inode, struct file *file);		
static int radimo_release(struct inode *inode, struct file *file);
static int radimo_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg);	//定义了三个常用的接口函数

static struct block_device_operations radimo_fops = {
	open:		radimo_open,
	release:	radimo_release,
	ioctl:		radimo_ioctl,
};					//用一个结构把三个接口函数组装起来，以便通知操作系统


static int radimo_request(request_queue_t *request_queue, int rw, struct buffer_head *sbh)
{	//【重要】这是块设备驱动程序处理读写请求的函数，是本程序中最重要的部分
	unsigned long offset, total;
	//int *make_oops = NULL;    // CR: access to this pointer will cause Oops, for debug only.	

	MSG(RADIMO_REQUEST, "%s sector rsector = %lu, blocknr = %lu\n",
				rw == READ ? "read" : "write",
				sbh->b_rsector,
				sbh->b_blocknr);	//MSG宏的定义在radimo.h中，相当于printk，
							//用于在log文件中打印一行，以便调试
	
	offset = sbh->b_rsector * radimo_hard;		
	total = (unsigned long)sbh->b_size;		//计算需要访问的地址和大小
	
	/* access beyond end of the device */
	if (total+offset > radimo_size * (radimo_hard << 1)) {
		MSG(RADIMO_REQUEST, "Error: access beyond end of the device");
		/* error in request  */
		buffer_IO_error(sbh);
		return 0;
	}						//判断访问地址是否越界

	MSG(RADIMO_REQUEST, "offset = %lu, total = %lu\n", offset, total);
	
	if (rw == READ) {	//如果是读操作，从虚拟盘的内存中复制数据到缓冲区中
		memcpy(bh_kmap(sbh), radimo_storage+offset, total);
	} else if (rw == WRITE) { //如果是写操作，从缓冲区中复制数据到虚拟盘的内存中
		memcpy(radimo_storage+offset, bh_kmap(sbh), total);
	} else {	/* can't happen */
		MSG(RADIMO_ERROR, "cmd == %d is invalid\n", rw);
	}
	/* successful */
	
	sbh->b_end_io(sbh,1);	//结束读写操作
	
	return 0;
}

void radimo_timer_fn(unsigned long data)
{	//这是用于定时器处理的函数，本程序中没有用到，但是可以作为使用定时器的框架
	/* set it up again */
	radimo_timer.expires = RADIMO_TIMER_DELAY + jiffies;
	add_timer(&radimo_timer);
}

static int radimo_release(struct inode *inode, struct file *file)
{	//关闭设备时调用
	MSG(RADIMO_OPEN, "closed\n");
	MOD_DEC_USE_COUNT;	//减少引用计数
	return 0;
}

static int radimo_open(struct inode *inode, struct file *file)
{	//打开设备时调用
	MSG(RADIMO_OPEN, "opened\n");
	MOD_INC_USE_COUNT;	//增加引用计数

	/* timer function needs device to invalidate buffers. pass it as
	   data. */
	radimo_timer.data = inode->i_rdev;
	radimo_timer.expires = RADIMO_TIMER_DELAY + jiffies;
	radimo_timer.function = &radimo_timer_fn;

	if (!timer_pending(&radimo_timer))
		add_timer(&radimo_timer);
	//以上几行是设置定时器，本程序中没有用到定时器

	return 0;
}

static int radimo_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{	//响应一些特殊的操作，这些操作可以自己定义
	unsigned int minor;
	
	if (!inode || !inode->i_rdev) 	
		return -EINVAL;

	minor = MINOR(inode->i_rdev);

	switch (cmd) {

		case BLKFLSBUF: {	//将缓冲写回存储区的操作
			/* flush buffers */
			MSG(RADIMO_IOCTL, "ioctl: BLKFLSBUF\n");
			/* deny all but root */
			if (!capable(CAP_SYS_ADMIN))
				return -EACCES;
			fsync_dev(inode->i_rdev);
			invalidate_buffers(inode->i_rdev);
			break;
		}

         	case BLKGETSIZE: {	//得到设备容量的操作
			/* return device size */
			MSG(RADIMO_IOCTL, "ioctl: BLKGETSIZE\n");
			if (!arg)
				return -EINVAL;
			return put_user(radimo_size*2, (long *) arg);
		}
		
		case BLKRASET: {	//设置设备预读值的操作
			/* set read ahead value */
			int tmp;
			MSG(RADIMO_IOCTL, "ioctl: BLKRASET\n");
			if (get_user(tmp, (long *)arg))
				return -EINVAL;
			if (tmp > 0xff)
				return -EINVAL;
			read_ahead[RADIMO_MAJOR] = tmp;
			return 0;
		}

		case BLKRAGET: {	//得到设备预读值的操作
			/* return read ahead value */
			MSG(RADIMO_IOCTL, "ioctl: BLKRAGET\n");
			if (!arg)
				return -EINVAL;
			return put_user(read_ahead[RADIMO_MAJOR], (long *)arg);
		}

		case BLKSSZGET: {	//得到设备块大小的操作
			/* return block size */
			MSG(RADIMO_IOCTL, "ioctl: BLKSSZGET\n");
			if (!arg)
				return -EINVAL;
			return put_user(radimo_soft, (long *)arg);
		}

		default: {		//其他操作
			MSG(RADIMO_ERROR, "ioctl wanted %u\n", cmd);
			return -ENOTTY;
		}
	}
	return 0;
}
	
int init_module(void)
{	//在模块被加载的时候调用
	int res;
	
	/* 块大小必须是扇区大小的整数倍 */
	if (radimo_soft & ((1 << RADIMO_HARDS_BITS)-1)) {
		MSG(RADIMO_ERROR, "Block size not a multiple of sector size\n");
		return -EINVAL;
	}
	
	/* 分配存储空间 */
	radimo_storage = (char *) vmalloc(1024*radimo_size);
	if (radimo_storage == NULL) {
		MSG(RADIMO_ERROR, "Not enough memory. Try a smaller size.\n");
		return -ENOMEM;
	}
	memset(radimo_storage, 0, 1024*radimo_size);
	
	/* 【重要】向系统注册块设备 */
	res = register_blkdev(RADIMO_MAJOR, "radimo", &radimo_fops);
	if (res) {
		MSG(RADIMO_ERROR, "couldn't register block device\n");
		return res;
	}
	
	init_timer(&radimo_timer);
	
	/* 在系统中注册块的大小、存储容量等参数 */
	hardsect_size[RADIMO_MAJOR] = &radimo_hard;
	blksize_size[RADIMO_MAJOR] = &radimo_soft;
	blk_size[RADIMO_MAJOR] = &radimo_size;
	
	/* 在系统中注册响应读写请求的函数 */
	blk_queue_make_request(BLK_DEFAULT_QUEUE(RADIMO_MAJOR), &radimo_request);
	read_ahead[RADIMO_MAJOR] = radimo_readahead;
	
	MSG(RADIMO_INFO, "loaded\n");
	MSG(RADIMO_INFO, "sector size of %d, block size of %d, total size = %dKb\n",
					radimo_hard, radimo_soft, radimo_size);
	
	return 0;
}

void cleanup_module(void)
{	//在模块被卸载的时候调用
	unregister_blkdev(RADIMO_MAJOR, "radimo");
	del_timer(&radimo_timer);

	invalidate_buffers(MKDEV(RADIMO_MAJOR,0));

	/* remove our request function */
	blk_dev[RADIMO_MAJOR].request_queue.request_fn = 0;
	
	vfree(radimo_storage);

	MSG(RADIMO_INFO, "unloaded\n");
}	
