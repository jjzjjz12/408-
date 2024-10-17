#define	__KERNEL__
#define MODULE

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>	
#include <asm/uaccess.h>
//���ϼ�����ģ����������ͷ�ļ��ͺ꣬����Ҫ�

#define STRINGLEN 	  1024	

char global_buffer[STRINGLEN];	//����������һ���СΪ1024���ڴ�
struct proc_dir_entry *example_dir, *hello_file, *current_file, *symlink;

int proc_read_current(char *page, char **start, off_t off, int count,int *eof, void *data)
{	//���û���ȡ"current"�ļ�������ʱ���ں˵����������
        int len;
        MOD_INC_USE_COUNT;	//ģ�����ü�������1����֤ģ�鲻�ᱻж��
        len = sprintf(page,"current process usages:\nname:%s\ngid:%d\npid:%d\n",current->comm,current->pgrp,current->pid);
        //�����û����������ļ�������һ���ַ�����˵���˵�ǰ���̵����ֺ�pid�ȵȡ�
        MOD_DEC_USE_COUNT;	//ģ�����ü�����1��
        return len;
}

int proc_read_hello(char *page, char **start,off_t off, int count, int *eof, void *data)
{	//���û���ȡ"hello"�ļ�������ʱ���ں˵����������
        int len;
        
        MOD_INC_USE_COUNT;
	len = sprintf(page, "hello message:\n%s write:%s\n", current->comm, global_buffer);
	//��global_buffer��ǰ�������1024��С���ڴ棩��ǰ������������֣���Ϊ�û�����������
        MOD_DEC_USE_COUNT;
        
        return len;
}

static int proc_write_hello(struct file *file,const char *buffer, unsigned long count,void *data)
{	//���û�д"hello"�ļ�������ʱ���ں˵����������
        int len;

        MOD_INC_USE_COUNT;
        if(count > STRINGLEN)		//���Ҫд�����ݶ���1024���ַ����ͰѶ�����Ĳ��ֽض�
		len = STRINGLEN;
        else
		len = count;

        copy_from_user(global_buffer, buffer, len);	//���û��Ļ��������Ҫд�����ݣ�ע�������copy_from_user
        global_buffer[len] = '\0';	//���ַ��������һ��0����ʾ�ַ�������
        
        MOD_DEC_USE_COUNT;
        return len;
}

int init_module()
{	//����������Ϥ��init_module�����������ڼ���ģ���ʱ�򱻵��ã�������Ҫ�ڼ���ģ���ʱ�򴴽�proc�ļ�
	example_dir = proc_mkdir("proc_test", NULL);
        example_dir->owner = THIS_MODULE;	//����Ŀ¼���൱��Windows���ļ��У� /proc/proc_test
        
        current_file = create_proc_read_entry("current",0666,example_dir,proc_read_current, NULL);
        current_file->owner = THIS_MODULE;	//����ֻ���ļ�  /proc/proc_test/current

        hello_file = create_proc_entry("hello", 0644, example_dir);
        strcpy(global_buffer, "hello");
        hello_file->read_proc = proc_read_hello;
        hello_file->write_proc = proc_write_hello;
        hello_file->owner = THIS_MODULE;	//����һ�����Զ�д���ļ� /proc/proc_test/hello               
        symlink = proc_symlink("current_too", example_dir,"current");
        symlink->owner = THIS_MODULE;	//����һ�����ӣ��൱��Windows�Ŀ�ݷ�ʽ��/proc/proc_test/current_too��ָ��current

        return 0;
}

void cleanup_module()
{	//Ҫж��ģ���ˣ������Ҫ�Ѵ�����Ŀ¼���ļ����������
        remove_proc_entry("current_too", example_dir);
        remove_proc_entry("hello", example_dir);
        remove_proc_entry("current", example_dir);
        remove_proc_entry("proc_test", NULL);
}

