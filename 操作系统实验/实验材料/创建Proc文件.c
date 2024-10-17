#define	__KERNEL__
#define MODULE

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>	
#include <asm/uaccess.h>
//以上几行是模块必须包含的头文件和宏，不需要深究

#define STRINGLEN 	  1024	

char global_buffer[STRINGLEN];	//这里申请了一块大小为1024的内存
struct proc_dir_entry *example_dir, *hello_file, *current_file, *symlink;

int proc_read_current(char *page, char **start, off_t off, int count,int *eof, void *data)
{	//当用户读取"current"文件的内容时，内核调用这个函数
        int len;
        MOD_INC_USE_COUNT;	//模块引用计数增加1，保证模块不会被卸载
        len = sprintf(page,"current process usages:\nname:%s\ngid:%d\npid:%d\n",current->comm,current->pgrp,current->pid);
        //告诉用户，读出的文件内容是一行字符串，说明了当前进程的名字和pid等等。
        MOD_DEC_USE_COUNT;	//模块引用计数减1。
        return len;
}

int proc_read_hello(char *page, char **start,off_t off, int count, int *eof, void *data)
{	//当用户读取"hello"文件的内容时，内核调用这个函数
        int len;
        
        MOD_INC_USE_COUNT;
	len = sprintf(page, "hello message:\n%s write:%s\n", current->comm, global_buffer);
	//给global_buffer（前面申请的1024大小的内存）的前面加上两行文字，作为用户读出的内容
        MOD_DEC_USE_COUNT;
        
        return len;
}

static int proc_write_hello(struct file *file,const char *buffer, unsigned long count,void *data)
{	//当用户写"hello"文件的内容时，内核调用这个函数
        int len;

        MOD_INC_USE_COUNT;
        if(count > STRINGLEN)		//如果要写的内容多于1024个字符，就把多出来的部分截断
		len = STRINGLEN;
        else
		len = count;

        copy_from_user(global_buffer, buffer, len);	//从用户的缓冲区获得要写的数据，注意必须用copy_from_user
        global_buffer[len] = '\0';	//给字符串后面加一个0，表示字符串结束
        
        MOD_DEC_USE_COUNT;
        return len;
}

int init_module()
{	//这是我们熟悉的init_module函数，将会在加载模块的时候被调用，我们需要在加载模块的时候创建proc文件
	example_dir = proc_mkdir("proc_test", NULL);
        example_dir->owner = THIS_MODULE;	//创建目录（相当于Windows的文件夹） /proc/proc_test
        
        current_file = create_proc_read_entry("current",0666,example_dir,proc_read_current, NULL);
        current_file->owner = THIS_MODULE;	//创建只读文件  /proc/proc_test/current

        hello_file = create_proc_entry("hello", 0644, example_dir);
        strcpy(global_buffer, "hello");
        hello_file->read_proc = proc_read_hello;
        hello_file->write_proc = proc_write_hello;
        hello_file->owner = THIS_MODULE;	//创建一个可以读写的文件 /proc/proc_test/hello               
        symlink = proc_symlink("current_too", example_dir,"current");
        symlink->owner = THIS_MODULE;	//创建一个链接（相当于Windows的快捷方式）/proc/proc_test/current_too，指向current

        return 0;
}

void cleanup_module()
{	//要卸载模块了，因此需要把创建的目录和文件都清除掉。
        remove_proc_entry("current_too", example_dir);
        remove_proc_entry("hello", example_dir);
        remove_proc_entry("current", example_dir);
        remove_proc_entry("proc_test", NULL);
}

