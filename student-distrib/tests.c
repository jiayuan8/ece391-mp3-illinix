#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "int_linkage.h"
#include "idt.h"
#include "rtc.h"
#include "terminal.h"
#include "filesystem.h"

#define PASS 1
#define FAIL 0
#define FILE_BUF_LEN 40000

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* int divide_zero_test()
 *
 * Check that divide by zero exception works
 * Inputs: None
 * Outputs: blue screen if success
 * Side Effects: exception will occur
 * Coverage: divide by zero exception
 * Files: idt.h/c
 */
int divide_zero_test() {
	TEST_HEADER;

	int i = 1;
	int j = 0;
	int k = i/j;
	return k;
}

/* int deref_null()
 *
 * Check whether dereference NULL will raise exception
 * Inputs: None
 * Outputs: blue screen if success
 * Side Effects: will raise exception
 * Files: paging.h/c
 */

int deref_null() {
	TEST_HEADER;

	int * a = NULL;
	int b = *a;
	//printf("This is B: %x\n: ", b);
	return b;
}

/* int deref_an_address()
 *
 * Check a address at 4mb to 8 mb to see if paging works
 * Inputs: None
 * Outputs: print test header without bluescreen
 * Side Effects: exception will occur if wrong address has been input
 * Files: paging.h/c
 */
int deref_an_address()
{
	TEST_HEADER;
	int *a = (int *)0x00400080;
	int b = *a;
	//printf("This is B: %x\n: ", b);
	return (b == 0);
}

/* int deref_wrong_address()
 *
 * Check a address at 4mb to 8 mb to see if paging works
 * Inputs: None
 * Outputs: blue screen if success
 * Side Effects: will raise exception
 * Files: paging.h/c
 */
int deref_wrong_address()
{
	TEST_HEADER;
	int *a = (int *)0x00000034;
	int b = *a;
	//printf("This is B: %x\n: ", b);
	return b;
}

/* int deref_video_mem()
 *
 * Check whether dereferencing 0x000B8000 (mem address of video mem) works
 * Inputs: None
 * Outputs: print test header without bluescreen
 * Side Effects: exception will occur if wrong address has been input
 * Files: paging.h/c
 */
int deref_video_mem()
{
	TEST_HEADER;
	int *a = (int *)0x000B8000;
	int b = *a;
	//printf("This is B: %x\n: ", b);
	return b;
}

/* Checkpoint 2 tests */

/* int test_terminal_write()
 *
 * Test terminal write function
 * Inputs: None
 * Outputs: print text input after press enter
 * Side Effects: none
 * Files: terminal.h/c / keyboard.h/c / lib.h/c
 */

void test_terminal_write(){
	//TEST_HEADER;
	uint8_t testbuffer[129];
	while(1){
		terminal_read(0, &testbuffer[0], 128);
		terminal_write(0, &testbuffer[0], 128);
	}
	return;
}

/* int test_rtc_driver()
 *
 * Test whether change frequency works
 * Inputs: None
 * Outputs: print 'a' at desired frequency
 * Side Effects: none
 * Files: rtc.h/c
 */

void test_rtc_driver(int32_t feq_test){
	TEST_HEADER;
	if(rtc_write(0, &feq_test, 4)!= 0){
		printf("Invalid Frequency!!!\n");
		return;
	}
	while(1){
		rtc_read(0, NULL, 0);
		putc('1');
	}
	return;
}

/* void test_read_dir()
 *
 * Test whether dir_read works
 * Inputs: None
 * Outputs: it will print all the file name, file type and file size of files in fsdir directory
 * Side Effects: none
 * Files: filesystem.h/c
 */

void test_read_dir(){
	TEST_HEADER;
	int32_t fd, cnt;
	uint8_t buf[33];
	dentry_t dentry;
	uint32_t f_size;

	buf[32] = '\0';
	fd = dir_open((uint8_t*)".");

	while (0 != (cnt = dir_read (fd, buf, 32))) {
        if (-1 == cnt) {
	        printf ((int8_t*)"directory entry read failed\n");
	        return;
	    }

		read_dentry_by_name(buf, &dentry);

		printf("filename: ");
	    print_width((int8_t*)buf, 35);
		printf(", filetype: %d", dentry.f_type);
		f_size = *((uint32_t*)(inode_addr + dentry.i_node * BLOCKSIZE));
		printf(", filesize: %d", f_size);
		putc('\n');

    }

	return;

}


/* void test_read_file_by_name(int8_t* filename)
 *
 * Test whether file_read() and read_dentry_by_name works
 * Inputs: filename: a string of filename, 32 characters at most
 * Outputs: If the file is larger than 400 bytes, it will print first and last 200 bytes separately.
 * 			Otherwise, it will print all characters in the file
 * 			In the end, it will print filename and filesize
 * 			If it is invalid file name, the function will print "read failed!!!"
 * 			If the file is not a regular file, the function will print "Not Regular Type File For Read!!!"
 * 			and "read failed!!!"
 * Side Effects: none
 * Files: filesystem.h/c
 */
void test_read_file_by_name(int8_t* filename){
	TEST_HEADER;
	uint32_t i = 0;
	int8_t buf[FILE_BUF_LEN + 1];

	buf[FILE_BUF_LEN] = '\0';

	for(i = 0; i < FILE_BUF_LEN; i ++){
		buf[i] = '\0';
	}

	if(file_open((uint8_t *)(&filename[0])) != -1){

		file_read(0, &buf, FILE_BUF_LEN);

		if(opened_file_size >= 400){
			printf("\n\n/********First 200 bytes: ********/\n\n");
			for(i = 0; i < 200; i ++){
				putc(buf[i]);
			}
			printf("\n\n/********Last 200 bytes: ********/\n\n");
			for(i = 201; i >= 1; i --){
				putc(buf[opened_file_size - i]);
			}
		}else{
			for(i = 0; i < opened_file_size; i++){
				putc(buf[i]);
			}
		}
		printf("\n\nFile size: %d bytes", opened_file_size);
	}else{
		printf("\n Read Failed!!!\n");
	}
	printf("\nFile name: %s\n", filename);

	return;
}

/* void test_read_file_by_name(int8_t* filename)
 *
 * Test whether file_read() and read_dentry_by_name works
 * Inputs: filename: a string of filename, 32 characters at most
 * Outputs: If the file is larger than 400 bytes, it will print first and last 200 bytes separately.
 * 			Otherwise, it will print all characters in the file
 * 			In the end, it will print filename and filesize
 * 			If it is invalid file name, the function will print "read failed!!!"
 * 			If the file is not a regular file, the function will print "Not Regular Type File For Read!!!"
 * 			and "read failed!!!"
 * Side Effects: none
 * Files: filesystem.h/c
 */
void test_read_whole_file_by_name(int8_t* filename){
	TEST_HEADER;
	uint32_t i = 0;
	int8_t buf[FILE_BUF_LEN + 1];

	buf[FILE_BUF_LEN] = '\0';

	for(i = 0; i < FILE_BUF_LEN; i ++){
		buf[i] = '\0';
	}

	if(file_open((uint8_t *)(&filename[0])) != -1){
		file_read(0, &buf, FILE_BUF_LEN);

		for(i = 0; i < opened_file_size; i++){
			putc(buf[i]);
		}

		cursor_helper();
		printf("\n\nFile size: %d bytes", opened_file_size);
	}else{
		printf("\n Read Failed!!!\n");
	}
	printf("\nFile name: %s\n", filename);

	return;
}


/* void test_read_file_by_index(uint32_t index)
 *
 * Test whether file_read() and read_dentry_by_index() works
 * Inputs: index: index of a directory entry in boot block
 * Outputs: If the file is larger than 400 bytes, it will print first and last 200 bytes separately.
 * 			Otherwise, it will print all characters in the file
 * 			In the end, it will print filename and filesize
 * 			If it is invalid directory entry index, the function will print "read failed!!!"
 * 			If the file is not a regular file, the function will print "Not Regular Type File For Read!!!"
 * 			and "read failed!!!"
 * Side Effects: none
 * Files: filesystem.h/c
 */
void test_read_file_by_index(uint32_t index){
	TEST_HEADER;
	uint32_t file_len;
	uint32_t i = 0;
	int8_t buf[FILE_BUF_LEN + 1];
	dentry_t dentry;
	uint8_t fname_buf[33];

	buf[FILE_BUF_LEN] = '\0';

	for(i = 0; i < FILE_BUF_LEN; i ++){
		buf[i] = '\0';
	}

	if(read_dentry_by_index(index, &dentry) == -1){
		printf("\n Read Failed!!!\n");
		return;
	}

	file_len = *((uint32_t*)(inode_addr + dentry.i_node * BLOCKSIZE));
	strncpy((int8_t*)fname_buf, (int8_t*)(&(dentry.f_name)), 32);
	fname_buf[32] = '\0';

	if(file_open((uint8_t *)((uint8_t *)(&fname_buf[0]))) != -1){
		file_read(0, &buf, FILE_BUF_LEN);
		if(file_len >= 400){

			printf("\n\n/********First 200 bytes: ********/\n\n");

			for(i = 0; i < 200; i ++){
				putc(buf[i]);
			}

			printf("\n\n/********Last 200 bytes: ********/\n\n");

			for(i = 201; i >= 1; i --){
				putc(buf[file_len - i]);
			}

		}else{
			for(i = 0; i < file_len; i++){
				putc(buf[i]);
			}
		}
	}else{
		printf("\n Read Failed!!!\n");
	}
	printf("\n\nFile size: %d bytes", file_len);
	printf("\nFile name: %s\n", (&fname_buf[0]));

	return;
}

/* void test_read_whole_file_by_index(uint32_t index)
 *
 * Test whether file_read() and read_dentry_by_index() works
 * Inputs: index: index of a directory entry in boot block
 * Outputs: Print the whole file if success
 *
 * Side Effects: none
 * Files: filesystem.h/c
 */
void test_read_whole_file_by_index(uint32_t index){
	TEST_HEADER;
	uint32_t file_len;
	uint32_t i = 0;
	int8_t buf[FILE_BUF_LEN + 1];
	dentry_t dentry;
	uint8_t fname_buf[33];

	buf[FILE_BUF_LEN] = '\0';

	for(i = 0; i < FILE_BUF_LEN; i ++){
		buf[i] = '\0';
	}

	if(read_dentry_by_index(index, &dentry) == -1){
		printf("\n Read Failed!!!!\n");
		return;
	}
	file_len = *((uint32_t*)(inode_addr + dentry.i_node * BLOCKSIZE));

	strncpy((int8_t*)fname_buf, (int8_t*)(&(dentry.f_name)), 32);
	fname_buf[32] = '\0';

	if(file_open((uint8_t *)(&fname_buf[0])) != -1){

		file_read(0, &buf, FILE_BUF_LEN);
		for(i = 0; i < file_len; i++){
			putc(buf[i]);
		}

	}else{
		printf("\n Read Failed!!!\n");
	}
	printf("\n\nFile size: %d bytes", file_len);
	printf("\nFile name: %s\n", (&fname_buf[0]));

	return;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	/* 3.1 tests */

	// TEST_OUTPUT("idt_test", idt_test());
	//TEST_OUTPUT("divide_by_zero_test", divide_zero_test());
	//TEST_OUTPUT("dereference_null_test", deref_null());
	//TEST_OUTPUT("dereference_add1_test", deref_an_address());
	//TEST_OUTPUT("dereference_video_mem", deref_video_mem());
	//TEST_OUTPUT("dereference_wrong_mem", deref_wrong_address());

	/* 3.2 tests */
	test_terminal_write();

	/*
		Possible valid input for rtc test

		0,1,2,4,8,16,32,64,128,256,512,1024

	*/

	//test_rtc_driver(512);
	//test_read_dir();

	/*
		possible valid input for filename:

		"cat"
		"counter"
		"fish"
		"frame0.txt"
		"frame1.txt"
		"grep"
		"hello"
		"ls"
		"pingpong"
		"shell"
		"sigtest"
		"syserr"
		"testprint"
		"verylargetextwithverylongname.tx"
		"created.txt"
		"."
		"rtc"
	*/

	//test_read_file_by_name("verylargetextwithverylongname.tx");
	//test_read_whole_file_by_name("fish");

	/*
		possible valid input for index:

		0 -- 16

	*/

	//test_read_file_by_index(5);
	//test_read_whole_file_by_index(11);

	/* 3.3 tests */
	/* 3.4 tests */
	/* 3.5 tests */
}
