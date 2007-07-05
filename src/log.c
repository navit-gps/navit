#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "map_data.h"
#include "log.h"

void
log_write(char *message, struct file *file, void *data, int size)
{
	char log[4096];
	char *l;
	unsigned char *p=data;

	int fd=open("log.txt",O_RDWR|O_CREAT|O_APPEND|O_SYNC, 0644);
	sprintf(log,"# %s\n",message);
	l=log+strlen(log);
	sprintf(l, "%s 0x%x ", file->name, p-file->begin);
	l=log+strlen(log);
	while (size) {
		sprintf(l,"%02x ", *p++);
		l+=3;
		size--;
	}
	*l++='\n';
	*l='\0';
	write(fd, log, strlen(log));
	close(fd);
}

void
log_apply(struct map_data *map, int files)
{
	char buffer[4096],*p;
	char *filename, *addr;
	struct file *file;
	unsigned char *data;
	unsigned long dataval;
	struct map_data *curr;
	int j;

	FILE *f=fopen("log.txt","r");
	if (!f) 
		return;
	while (fgets(buffer, 4096, f)) {
		if (buffer[0] != '#') {
			buffer[strlen(buffer)-1]='\0';
			filename=buffer;
			p=buffer;
			while (*p && *p != ' ')
				p++;		
			if (! *p)
				continue;
			*p++=0;
			file=NULL;
			curr=map;
			while (curr) {
				for(j = 0 ; j < files ; j++) {
					if (curr->file[j] && !strcmp(curr->file[j]->name, filename)) {
						file=curr->file[j];
						break;
					}
				}
				if (file)
					break;
				curr=curr->next;
			}
			if (file) {
				addr=p;				
				while (*p && *p != ' ')
					p++;
				if (! *p)
					continue;
				*p++=0;	
				data=file->begin+strtoul(addr, NULL, 16);
				while (*p) {
					dataval=strtoul(p, NULL, 16);
					*data++=dataval;
					p+=2;					
					while (*p == ' ')
						p++;
				}	
			}
		}
	}	
	fclose(f);
}
