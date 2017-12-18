#include <stdio.h>
#include <stdlib.h>
#include "kmemory.h"

char table[0x10000];

unsigned int table_p=(unsigned int)table;

int main()
{

    init_page_map();

    char * p=(char*)get_free_pages(0,2);

    printf("address is %x\n",p);
    return 0;
}
