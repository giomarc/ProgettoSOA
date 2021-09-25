all: multi_read_write_multi_level test_driver reader writer test_wakeupall test_create_open_remove reader_custom multi_read_write_multi_tag writer_custom

multi_read_write_multi_level: multi_read_write_multi_level.o common_function.o
	gcc -Wall -Wextra -pthread multi_read_write_multi_level.o common_function.o -o multi_read_write_multi_level

multi_read_write_multi_level.o: multi_read_write_multi_level.c
	gcc -Wall -Wextra -c multi_read_write_multi_level.c

common_function.o: common_function.c
	gcc -Wall -Wextra -c common_function.c

test_driver: test_driver.o common_function.o
	gcc -Wall -Wextra -pthread test_driver.o common_function.o -o test_driver

test_driver.o: test_driver.c
	gcc  -Wall -Wextra -c -pthread test_driver.c

reader_custom: reader_custom.o common_function.o
	gcc -Wall -Wextra reader_custom.o common_function.o -o reader_custom

reader_custom.o: reader_custom.c
	gcc -Wall -Wextra -c reader_custom.c

writer_custom: writer_custom.o writer_custom.o
	gcc writer_custom.o common_function.o -o writer_custom

writer_custom.o: writer_custom.c
	gcc -c writer_custom.c

reader: reader.o common_function.o
	gcc reader.o common_function.o -o reader

reader.o:reader.c 
	gcc -c reader.c

writer: writer.o common_function.o
	gcc  writer.o common_function.o -o writer

writer.o:writer.c 
	gcc -c writer.c

test_wakeupall: test_wakeupall.o common_function.o
	gcc -Wall -Wextra -pthread test_wakeupall.o common_function.o -o test_wakeupall

test_wakeupall.o:test_wakeupall.c
	gcc -Wall -Wextra -c -pthread test_wakeupall.c

test_create_open_remove: test_create_open_remove.o common_function.o
	gcc test_create_open_remove.o common_function.o -o test_create_open_remove

test_create_open_remove.o: test_create_open_remove.c
	gcc -c test_create_open_remove.c

multi_read_write_multi_tag: multi_read_write_multi_tag.o common_function.o
	gcc -Wall -Wextra -pthread multi_read_write_multi_tag.o common_function.o -o multi_read_write_multi_tag

multi_read_write_multi_tag.o: multi_read_write_multi_tag.c
	gcc -Wall -Wextra -c multi_read_write_multi_tag.c

clean:
	rm *.o multi_read_write_multi_level test_driver reader writer test_wakeupall test_create_open_remove reader_custom multi_read_write_multi_tag writer_custom

