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

/*���϶�����һЩ���õ�ͷ�ļ�*/

#define MAJOR_NR		RADIMO_MAJOR		// ������RADIMO�����豸��
#define DEVICE_NAME		"radimo"		// ������RADIMO���豸��
#define DEVICE_NR(device)	(MINOR(device))		
#define DEVICE_ON(device)
#define DEVICE_OFF(device)
#define DEVICE_NO_RANDOM

#include <linux/blk.h>

#define RADIMO_HARDS_BITS	9	/* 2**9 byte hardware sector */
#define RADIMO_BLOCK_SIZE	1024	/* block size */	//������һ����Ĵ�С�����ֽ�Ϊ��λ
#define RADIMO_TOTAL_SIZE	2048	/* size in blocks */	//��������������̵��������Կ�Ϊ��λ

/* the storage pool */
static char *radimo_storage;		//���ָ����ȫ�ֱ�����ָ�����������̵��ڴ�

static int radimo_hard = 1 << RADIMO_HARDS_BITS;
static int radimo_soft = RADIMO_BLOCK_SIZE;
static int radimo_size = RADIMO_TOTAL_SIZE;

static int radimo_readahead = 4;

struct timer_list radimo_timer;		//������һ����ʱ����������������ǲ�û���õ���������Ϊһ��ʹ�ö�ʱ��������

/* forward declarations for _fops */
static int radimo_open(struct inode *inode, struct file *file);		
static int radimo_release(struct inode *inode, struct file *file);
static int radimo_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg);	//�������������õĽӿں���

static struct block_device_operations radimo_fops = {
	open:		radimo_open,
	release:	radimo_release,
	ioctl:		radimo_ioctl,
};					//��һ���ṹ�������ӿں�����װ�������Ա�֪ͨ����ϵͳ


static int radimo_request(request_queue_t *request_queue, int rw, struct buffer_head *sbh)
{	//����Ҫ�����ǿ��豸�����������д����ĺ������Ǳ�����������Ҫ�Ĳ���
	unsigned long offset, total;
	//int *make_oops = NULL;    // CR: access to this pointer will cause Oops, for debug only.	

	MSG(RADIMO_REQUEST, "%s sector rsector = %lu, blocknr = %lu\n",
				rw == READ ? "read" : "write",
				sbh->b_rsector,
				sbh->b_blocknr);	//MSG��Ķ�����radimo.h�У��൱��printk��
							//������log�ļ��д�ӡһ�У��Ա����
	
	offset = sbh->b_rsector * radimo_hard;		
	total = (unsigned long)sbh->b_size;		//������Ҫ���ʵĵ�ַ�ʹ�С
	
	/* access beyond end of the device */
	if (total+offset > radimo_size * (radimo_hard << 1)) {
		MSG(RADIMO_REQUEST, "Error: access beyond end of the device");
		/* error in request  */
		buffer_IO_error(sbh);
		return 0;
	}						//�жϷ��ʵ�ַ�Ƿ�Խ��

	MSG(RADIMO_REQUEST, "offset = %lu, total = %lu\n", offset, total);
	
	if (rw == READ) {	//����Ƕ��������������̵��ڴ��и������ݵ���������
		memcpy(bh_kmap(sbh), radimo_storage+offset, total);
	} else if (rw == WRITE) { //�����д�������ӻ������и������ݵ������̵��ڴ���
		memcpy(radimo_storage+offset, bh_kmap(sbh), total);
	} else {	/* can't happen */
		MSG(RADIMO_ERROR, "cmd == %d is invalid\n", rw);
	}
	/* successful */
	
	sbh->b_end_io(sbh,1);	//������д����
	
	return 0;
}

void radimo_timer_fn(unsigned long data)
{	//�������ڶ�ʱ������ĺ�������������û���õ������ǿ�����Ϊʹ�ö�ʱ���Ŀ��
	/* set it up again */
	radimo_timer.expires = RADIMO_TIMER_DELAY + jiffies;
	add_timer(&radimo_timer);
}

static int radimo_release(struct inode *inode, struct file *file)
{	//�ر��豸ʱ����
	MSG(RADIMO_OPEN, "closed\n");
	MOD_DEC_USE_COUNT;	//�������ü���
	return 0;
}

static int radimo_open(struct inode *inode, struct file *file)
{	//���豸ʱ����
	MSG(RADIMO_OPEN, "opened\n");
	MOD_INC_USE_COUNT;	//�������ü���

	/* timer function needs device to invalidate buffers. pass it as
	   data. */
	radimo_timer.data = inode->i_rdev;
	radimo_timer.expires = RADIMO_TIMER_DELAY + jiffies;
	radimo_timer.function = &radimo_timer_fn;

	if (!timer_pending(&radimo_timer))
		add_timer(&radimo_timer);
	//���ϼ��������ö�ʱ������������û���õ���ʱ��

	return 0;
}

static int radimo_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{	//��ӦһЩ����Ĳ�������Щ���������Լ�����
	unsigned int minor;
	
	if (!inode || !inode->i_rdev) 	
		return -EINVAL;

	minor = MINOR(inode->i_rdev);

	switch (cmd) {

		case BLKFLSBUF: {	//������д�ش洢���Ĳ���
			/* flush buffers */
			MSG(RADIMO_IOCTL, "ioctl: BLKFLSBUF\n");
			/* deny all but root */
			if (!capable(CAP_SYS_ADMIN))
				return -EACCES;
			fsync_dev(inode->i_rdev);
			invalidate_buffers(inode->i_rdev);
			break;
		}

         	case BLKGETSIZE: {	//�õ��豸�����Ĳ���
			/* return device size */
			MSG(RADIMO_IOCTL, "ioctl: BLKGETSIZE\n");
			if (!arg)
				return -EINVAL;
			return put_user(radimo_size*2, (long *) arg);
		}
		
		case BLKRASET: {	//�����豸Ԥ��ֵ�Ĳ���
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

		case BLKRAGET: {	//�õ��豸Ԥ��ֵ�Ĳ���
			/* return read ahead value */
			MSG(RADIMO_IOCTL, "ioctl: BLKRAGET\n");
			if (!arg)
				return -EINVAL;
			return put_user(read_ahead[RADIMO_MAJOR], (long *)arg);
		}

		case BLKSSZGET: {	//�õ��豸���С�Ĳ���
			/* return block size */
			MSG(RADIMO_IOCTL, "ioctl: BLKSSZGET\n");
			if (!arg)
				return -EINVAL;
			return put_user(radimo_soft, (long *)arg);
		}

		default: {		//��������
			MSG(RADIMO_ERROR, "ioctl wanted %u\n", cmd);
			return -ENOTTY;
		}
	}
	return 0;
}
	
int init_module(void)
{	//��ģ�鱻���ص�ʱ�����
	int res;
	
	/* ���С������������С�������� */
	if (radimo_soft & ((1 << RADIMO_HARDS_BITS)-1)) {
		MSG(RADIMO_ERROR, "Block size not a multiple of sector size\n");
		return -EINVAL;
	}
	
	/* ����洢�ռ� */
	radimo_storage = (char *) vmalloc(1024*radimo_size);
	if (radimo_storage == NULL) {
		MSG(RADIMO_ERROR, "Not enough memory. Try a smaller size.\n");
		return -ENOMEM;
	}
	memset(radimo_storage, 0, 1024*radimo_size);
	
	/* ����Ҫ����ϵͳע����豸 */
	res = register_blkdev(RADIMO_MAJOR, "radimo", &radimo_fops);
	if (res) {
		MSG(RADIMO_ERROR, "couldn't register block device\n");
		return res;
	}
	
	init_timer(&radimo_timer);
	
	/* ��ϵͳ��ע���Ĵ�С���洢�����Ȳ��� */
	hardsect_size[RADIMO_MAJOR] = &radimo_hard;
	blksize_size[RADIMO_MAJOR] = &radimo_soft;
	blk_size[RADIMO_MAJOR] = &radimo_size;
	
	/* ��ϵͳ��ע����Ӧ��д����ĺ��� */
	blk_queue_make_request(BLK_DEFAULT_QUEUE(RADIMO_MAJOR), &radimo_request);
	read_ahead[RADIMO_MAJOR] = radimo_readahead;
	
	MSG(RADIMO_INFO, "loaded\n");
	MSG(RADIMO_INFO, "sector size of %d, block size of %d, total size = %dKb\n",
					radimo_hard, radimo_soft, radimo_size);
	
	return 0;
}

void cleanup_module(void)
{	//��ģ�鱻ж�ص�ʱ�����
	unregister_blkdev(RADIMO_MAJOR, "radimo");
	del_timer(&radimo_timer);

	invalidate_buffers(MKDEV(RADIMO_MAJOR,0));

	/* remove our request function */
	blk_dev[RADIMO_MAJOR].request_queue.request_fn = 0;
	
	vfree(radimo_storage);

	MSG(RADIMO_INFO, "unloaded\n");
}	
