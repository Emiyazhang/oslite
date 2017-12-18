#include "memory.h"
#define test
#ifdef S3C24X0  //设定内存分配区域
#define _MEM_END	0x30700000       //以上区域归属各模式堆栈、页表等
#define _MEM_START	0x300f0000      //物理地址从0X30000000开始，设定操作系统小于60KB
#endif

#ifdef test
extern unsigned int table_p;
#define _MEM_END	(table_p+0x40000)      //以上区域归属各模式堆栈、页表等
#define _MEM_START	(table_p)      //物理地址从0X30000000开始，设定操作系统小于60KB
#endif // test

/*page结构体中的双向链表，连接相同大小的内存块*/
struct list_head{
	struct list_head *prev,*next;
};
/*page结构体 */
struct page{
	unsigned int vaddr;	/*内存块的起始位置*/
	unsigned int flags;	/*结构体及页控制标志*/
	int order;		/*使用次数*/
	struct knem_cache *cachep;
	struct list_head list;	/*双向链表，连接大小相同的内存块*/
};


/*双向链表相关函数，进行内存块的连接与剥离*/
static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next=list;
	list->prev=list;

}

/*在 prve next之间插入new_list*/
static inline void _list_add(struct list_head *new_list,struct list_head *prev,struct list_head * next)
{
	next->prev=new_list;
	new_list->next=next;
	new_list->prev=prev;
	prev->next=new_list;
}

/*head 后插入*/
static inline void list_add(struct list_head *new_list,struct list_head *head)
{
	_list_add(new_list,head,head->next);
}

/*head 前插入*/
static inline void list_add_tail(struct list_head * new_list,struct list_head * head)
{
	_list_add(new_list,head->prev,head);
}


/*prev next相连，删除中间*/
static inline void _list_del(struct list_head *prev,struct list_head *next)
{
	next->prev=prev;
	prev->next=next;
}


/*删除*/
static inline void list_del(struct list_head *entry)
{
	_list_del(entry->prev,entry->next);
}


static inline void list_remove_chain(struct list_head *ch,struct list_head *ct)
{
	ch->prev->next=ct->next;
	ct->next->prev=ch->prev;
}

static inline void list_add_chain(struct list_head *ch,struct list_head *ct,struct list_head *head)
{
	ch->prev=head;
	ct->next=head->next;
	head->next->prev=ct;
	head->next=ct;
}
static inline void list_add_chain_tail(struct list_head *ch,struct list_head *ct,struct list_head *head)
{
	ch->prev=head->prev;
	head->prev->next=ch;
	head->prev=ct;
	ct->next=head;
}


static inline int list_empty(const struct list_head *head)
{
	return head->next==head;
}

/*地址值即为偏移值*/
#define offsetof(TYPE,MEMBER)	((unsigned int )&((TYPE *)0)->MEMBER)

/*type 为GNU拓展关键字，获得返回值的类型
 *ptr为成员
 *返回结构首地址
 */
#define container_of(ptr,type,member)	({	\
  const typeof(((type *)0)->member)*_mptr=(ptr);\
	(type *)((char *)_mptr-offsetof(type,member));})

#define list_entry(ptr,type,member)	container_of(ptr,type,member)

#define list_for_each(pos,head)	for(pos=(head)->next;pos!=(head);pos=pos->next)




/*
 *内存管理相关宏定义
 * pageing为内存块
 * page为结构体,记录相关内存块信息
 * */
#define PAGE_SHIFT	(12)
#define PAGE_SIZE	(1<<PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#define KERNEL_MEM_END	(_MEM_END)

#define KERNEL_PAGEING_START	((_MEM_START+(~PAGE_MASK))&((PAGE_MASK)))
#define KERNEL_PAGEING_END	(((KERNEL_MEM_END-KERNEL_PAGEING_START)/(PAGE_SIZE+\
				sizeof(struct page)))*(PAGE_SIZE)+KERNEL_PAGEING_START)

#define KERNEL_PAGE_NUM	((KERNEL_PAGEING_END-KERNEL_PAGEING_START)/PAGE_SIZE)

#define KERNEL_PAGE_END	(_MEM_END)
#define KERNEL_PAGE_START	(KERNEL_PAGE_END-KERNEL_PAGE_NUM*sizeof(struct page))

/*
 *buddy初始化部分
 * */
#define PAGE_AVAILABLE	0x00
#define PAGE_DIRTY	0x01
#define PAGE_PROTECT	0x02
#define PAGE_BUDDY_BUSY	0x04
#define PAGE_IN_CACHE	0x08

#define MAX_BUDDY_PAGE_NUM	(9)

#define AVERAGE_PAGE_NUM_PER_BUDDY	(KERNEL_PAGENUM/(MAX_BUDDY_PAGE_NUM-1))

#define PAGE_NUM_FOR_MAX_BUDDY	(1<<MAX_BUDDY_PAGE_NUM-1)

struct list_head page_buddy[MAX_BUDDY_PAGE_NUM];		/*挂接数组*/

/*初始化挂接数组*/
void init_page_buddy(void){
	int i;
	for(i=0;i<MAX_BUDDY_PAGE_NUM;i++){
		INIT_LIST_HEAD(&page_buddy[i]);
	}
}


/*挂接内存块，使系统初始只有最大最小两种内存块*/
void init_page_map(){
	int i;
	struct page *pg=(struct page*)KERNEL_PAGE_START;
	init_page_buddy();
	for(i=0;i<(KERNEL_PAGE_NUM);i++){
		pg->vaddr=KERNEL_PAGEING_START+i*PAGE_SIZE;	/*pg指向对应内存块*/
		pg->flags|=PAGE_AVAILABLE;
		INIT_LIST_HEAD(&pg->list);			/*初始化pg->list*/
		if(i<(KERNEL_PAGE_NUM&(~PAGE_NUM_FOR_MAX_BUDDY))){			/*i在最大内存块可划分地址内*/
			if((i&PAGE_NUM_FOR_MAX_BUDDY)==0){/*最大内存块划分首地址*/
					pg->order=MAX_BUDDY_PAGE_NUM-1;/*标记内存头值*/
			}else{
					pg->order=-1;
			}
			list_add_tail(&(pg->list),&page_buddy[MAX_BUDDY_PAGE_NUM-1]);
		}else					/*不可形成max_buddy的剩余块*/
		{
		pg->order=0;
		list_add_tail(&(pg->list),&page_buddy[0]);
		}
	}
}

/*伙伴算法内存块的申请*/

#define BUDDY_END(x,order)	((x)+(1<<(order))-1)
#define NEXT_BUDDY_START(x,order)	((x)+(1<<(order)))
#define PREV_BUDDY_START(x,order)	((x)-(1<<(order)))

struct page *get_pages_from_list(int order){
	//unsigned int vaddr;
	int neworder=order;
	struct page *pg;
	struct list_head *tlst,*tlst1;
	for(;neworder<MAX_BUDDY_PAGE_NUM;neworder++){
		if(list_empty(&page_buddy[neworder])){
			continue;		/*向上申请更大order内存块不存在*/
		}else{
			pg=list_entry(page_buddy[neworder].next,struct page,list);/*pg->page结构体*/
			tlst=&(BUDDY_END(pg,neworder)->list);	/*tlst指向块内存末尾*/
			tlst->next->prev=&page_buddy[neworder];	/*剥离内存块*/
			page_buddy[neworder].next=tlst->next;
			goto OUT_OK;
		}
	}
	return NULL;
OUT_OK:
	for(neworder--;neworder>=order;neworder--){
		tlst1=&(BUDDY_END(pg,neworder)->list);/*tlst1指向降阶order块为，即前一半end*/
		tlst=&(pg->list);			/*tlst指向内存块首部page.list*/
		pg=NEXT_BUDDY_START(pg,neworder);	/*pg指向后半部分start*/
		list_entry(tlst,struct page,list)->order=neworder;/*后半部分新order*/
		list_add_chain_tail(tlst,tlst1,&page_buddy[neworder]);/*挂接前一半到neworder*/
	}
	pg->flags|=PAGE_BUDDY_BUSY;  /*置1为free*/
	pg->order=order;
	return pg;
}


/*伙伴算法释放程序
 *置1为busy标志
 * */

void put_pages_to_list(struct page*pg,int order){
	struct page *tprev,*tnext;
	if((pg->flags&PAGE_BUDDY_BUSY)==0){
	return;	/*内存块忙，失败返回*/
	}
	pg->flags&=~(PAGE_BUDDY_BUSY);/*可删除吧*/
	for(;order<MAX_BUDDY_PAGE_NUM;order++){
		tnext=NEXT_BUDDY_START(pg,order);	/*tnext->相邻后内存块首地址*/
		tprev=PREV_BUDDY_START(pg,order);	/*tprev->相邻前内存块首地址*/
		if((!(tnext->flags&PAGE_BUDDY_BUSY))&&(tnext->order==order)){
			pg->order++;
			tnext->order=-1;
			list_remove_chain(&(tnext->list),&(BUDDY_END(pg,order)->list));	/*剥离tnext块*/
			BUDDY_END(pg,order)->list.next=&(tnext->list);			/*与待释放的内存块连接*/
			tnext->list.prev=&(BUDDY_END(pg,order)->list);
			continue;
		}else if((!(tprev->flags&PAGE_BUDDY_BUSY))&&(tprev->order==order)){
			pg->order=-1;
			list_remove_chain(&(tprev->list),&(BUDDY_END(pg,order)->list));	/*剥离tprev块*/
			BUDDY_END(tprev,order)->list.next=&(pg->list);			/*与待释放的内存块连接*/
			pg->list.prev=&(BUDDY_END(tprev,order)->list);
			pg=tprev;
			pg->order++;
			continue;
		}else{
			break;
		}

	}
	list_add_chain(&(pg->list),&(tnext-1)->list,&page_buddy[order]);			/*最终挂接*/
}

/*
 *内存地址转page 结构体地址
 * */
struct page *virt_to_page(unsigned int addr){
	unsigned int i;
	i=((addr)-KERNEL_PAGEING_START)>>PAGE_SHIFT;
	if(i>KERNEL_PAGE_NUM)
		return NULL;
	return (struct page *)KERNEL_PAGE_START+i;
}
void *page_address(struct page *pg){
	return (void *)(pg->vaddr);
}

struct page *alloc_pages(unsigned int flag,int order){
	struct page *pg;
    int i;
    pg =get_pages_from_list(order);
    if(pg==NULL)
    {
        return NULL;
    }
    for(i=0;i<(i<<order);i++){
        (pg+i)->flags|=PAGE_DIRTY;
    }

return pg;
}


void free_pages(struct page *pg,int order){
    int i;
    for(i=0;i<(1<<order);i++){
        (pg+i)->flags&=~PAGE_DIRTY;

    }
    put_pages_to_list(pg,order) ;

}

void *get_free_pages(unsigned int flag, int order ){
    struct page*page;
    page =alloc_pages(flag,order);
    if(!page)
        return NULL;
    return page_address(page);
}

void put_free_pages(void *addr,int order ){
    free_pages(virt_to_page((unsigned int)addr),order);
}
