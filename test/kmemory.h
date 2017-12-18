#ifndef __KMEMORY_H
#define __KMEMORY_H

void init_page_map();
void *get_free_pages(unsigned int flag, int order );
void put_free_pages(void *addr,int order );
#endif // _PRINT_H
