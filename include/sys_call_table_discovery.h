#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <linux/syscalls.h>
#include "vtpmo.h"
#include "custom_structures.h"
#include "custom_errors.h"


#define ADDRESS_MASK 0xfffffffffffff000//to migrate

#define START 			0xffffffff00000000ULL		// use this as starting address --> this is a biased search since does not start from 0xffff000000000000
#define MAX_ADDR		0xfffffffffff00000ULL
#define FIRST_NI_SYSCALL	134 //get
#define SECOND_NI_SYSCALL	174 //send
#define THIRD_NI_SYSCALL	177 //receive
#define FOURTH_NI_SYSCALL	178 //ctl


#define ENTRIES_TO_EXPLORE 256

extern unsigned long *hacked_ni_syscall;
extern unsigned long **hacked_syscall_tbl;
extern unsigned long sys_call_table_address;
extern unsigned long sys_ni_syscall_address;


int good_area(unsigned long * addr);
int validate_page(unsigned long *addr);
void syscall_table_finder(void);



