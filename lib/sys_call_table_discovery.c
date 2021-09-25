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
#include "../include/sys_call_table_discovery.h"

unsigned long *hacked_ni_syscall=NULL;
EXPORT_SYMBOL(hacked_ni_syscall);
unsigned long **hacked_syscall_tbl=NULL;
EXPORT_SYMBOL(hacked_syscall_tbl);

unsigned long sys_call_table_address = 0x0;
EXPORT_SYMBOL(sys_call_table_address);
unsigned long sys_ni_syscall_address = 0x0;
EXPORT_SYMBOL(sys_ni_syscall_address);

extern int sys_vtpmo(unsigned long vaddr);

int good_area(unsigned long * addr){

	int i;
	
	for(i = 1; i < FIRST_NI_SYSCALL; i++){
		if(addr[i] == addr[FIRST_NI_SYSCALL]) goto bad_area;
	}
	return 1;

bad_area:
	return 0;
}


/* This routine checks if the page contains the begin of the syscall_table.  */
int validate_page(unsigned long *addr){
	int i = 0;
	unsigned long page = (unsigned long) addr;
	unsigned long new_page = (unsigned long) addr;

	for( ; i < PAGE_SIZE; i += sizeof(void*)){
		new_page = page+i+FOURTH_NI_SYSCALL*sizeof(void*);

		// If the table occupies 2 pages check if the second one is materialized in a frame
		if( 
			((page+PAGE_SIZE) == (new_page & ADDRESS_MASK))
			&& sys_vtpmo(new_page) == NO_MAP
		) break;

		// go for patter matching
		addr = (unsigned long*) (page + i);
		if(
		        ( (addr[FIRST_NI_SYSCALL] & 0x3 ) == 0 )
		        && (addr[FIRST_NI_SYSCALL] != 0x0 )			        // not points to 0x0
			    && (addr[FIRST_NI_SYSCALL] > 0xffffffff00000000 )	// not points to a location lower than 0xffffffff00000000
			    && (addr[FIRST_NI_SYSCALL] == addr[SECOND_NI_SYSCALL])
			    && (addr[FIRST_NI_SYSCALL] == addr[THIRD_NI_SYSCALL])
			    && (addr[FIRST_NI_SYSCALL] == addr[FOURTH_NI_SYSCALL])
			    && (good_area(addr))
		){
			hacked_ni_syscall = (void*)(addr[FIRST_NI_SYSCALL]);	// save ni_syscall
			sys_ni_syscall_address = (unsigned long)hacked_ni_syscall;
			hacked_syscall_tbl = (void*)(addr);	                    // save syscall_table address
			sys_call_table_address = (unsigned long) hacked_syscall_tbl;
			return 1;
		}
	}
	return 0;
}

void syscall_table_finder(void){
	unsigned long k; // current page
	unsigned long candidate; // current* page

	for(k = START; k < MAX_ADDR; k += 4096){
		candidate = k;
		if( (sys_vtpmo(candidate) != NO_MAP))
		{
			// check if candidate maintains the syscall_table
			if(validate_page( (unsigned long *)(candidate)) ){
				printk("%s: syscall table found at %px\n","USCTM",(void*)(hacked_syscall_tbl));
				printk("%s: sys_ni_syscall found at %px\n","USCTM",(void*)(hacked_ni_syscall));
				break;
			}
		}
	}
	
}