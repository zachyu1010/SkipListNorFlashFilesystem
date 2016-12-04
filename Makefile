CFLAG = -Wall -lc


all: main.o mdfs_raw_inode.o flash_drive.o utility.o
	gcc -o go $(CFLAG) main.o mdfs_raw_inode.o flash_drive.o utility.o 
main.o: main.c mdfs_node.h
	gcc -c main.c
mdfs_raw_inode.o: mdfs_raw_inode.c mdfs_node.h
	gcc -c mdfs_raw_inode.c
#mdfs_raw_data.o: mdfs_raw_data.c mdfs_node.h
#	gcc -c  mdfs_raw_data.c
#ISACC.o: ISACC.c internal.h rand.h standard.h
#	gcc -c ISACC.c
flash_drive.o: flash_drive.c
	gcc -c flash_drive.c
utility.o: utility.c
	gcc -c utility.c

pss:  poisson.c
	gcc -o pss -lm -lc poisson.c

clean:
	rm -rf *.o go pss

dist_clean:
	rm -rf go drive/* *.o

clean_drive:
	rm -rf drive/*
