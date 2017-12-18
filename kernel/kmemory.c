#include "memory.h"
#define test
#ifdef S3C24X0  //�趨�ڴ��������
#define _MEM_END	0x30700000       //�������������ģʽ��ջ��ҳ���
#define _MEM_START	0x300f0000      //�����ַ��0X30000000��ʼ���趨����ϵͳС��60KB
#endif

#ifdef test
extern unsigned int table_p;
#define _MEM_END	(table_p+0x40000)      //�������������ģʽ��ջ��ҳ���
#define _MEM_START	(table_p)      //�����ַ��0X30000000��ʼ���趨����ϵͳС��60KB
#endif // test

/*page�ṹ���е�˫������������ͬ��С���ڴ��*/
struct list_head{
	struct list_head *prev,*next;
};
/*page�ṹ�� */
struct page{
	unsigned int vaddr;	/*�ڴ�����ʼλ��*/
	unsigned int flags;	/*�ṹ�弰ҳ���Ʊ�־*/
	int order;		/*ʹ�ô���*/
	struct knem_cache *cachep;
	struct list_head list;	/*˫���������Ӵ�С��ͬ���ڴ��*/
};


/*˫��������غ����������ڴ������������*/
static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next=list;
	list->prev=list;

}

/*�� prve next֮�����new_list*/
static inline void _list_add(struct list_head *new_list,struct list_head *prev,struct list_head * next)
{
	next->prev=new_list;
	new_list->next=next;
	new_list->prev=prev;
	prev->next=new_list;
}

/*head �����*/
static inline void list_add(struct list_head *new_list,struct list_head *head)
{
	_list_add(new_list,head,head->next);
}

/*head ǰ����*/
static inline void list_add_tail(struct list_head * new_list,struct list_head * head)
{
	_list_add(new_list,head->prev,head);
}


/*prev next������ɾ���м�*/
static inline void _list_del(struct list_head *prev,struct list_head *next)
{
	next->prev=prev;
	prev->next=next;
}


/*ɾ��*/
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

/*��ֵַ��Ϊƫ��ֵ*/
#define offsetof(TYPE,MEMBER)	((unsigned int )&((TYPE *)0)->MEMBER)

/*type ΪGNU��չ�ؼ��֣���÷���ֵ������
 *ptrΪ��Ա
 *���ؽṹ�׵�ַ
 */
#define container_of(ptr,type,member)	({	\
  const typeof(((type *)0)->member)*_mptr=(ptr);\
	(type *)((char *)_mptr-offsetof(type,member));})

#define list_entry(ptr,type,member)	container_of(ptr,type,member)

#define list_for_each(pos,head)	for(pos=(head)->next;pos!=(head);pos=pos->next)




/*
 *�ڴ������غ궨��
 * pageingΪ�ڴ��
 * pageΪ�ṹ��,��¼����ڴ����Ϣ
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
 *buddy��ʼ������
 * */
#define PAGE_AVAILABLE	0x00
#define PAGE_DIRTY	0x01
#define PAGE_PROTECT	0x02
#define PAGE_BUDDY_BUSY	0x04
#define PAGE_IN_CACHE	0x08

#define MAX_BUDDY_PAGE_NUM	(9)

#define AVERAGE_PAGE_NUM_PER_BUDDY	(KERNEL_PAGENUM/(MAX_BUDDY_PAGE_NUM-1))

#define PAGE_NUM_FOR_MAX_BUDDY	(1<<MAX_BUDDY_PAGE_NUM-1)

struct list_head page_buddy[MAX_BUDDY_PAGE_NUM];		/*�ҽ�����*/

/*��ʼ���ҽ�����*/
void init_page_buddy(void){
	int i;
	for(i=0;i<MAX_BUDDY_PAGE_NUM;i++){
		INIT_LIST_HEAD(&page_buddy[i]);
	}
}


/*�ҽ��ڴ�飬ʹϵͳ��ʼֻ�������С�����ڴ��*/
void init_page_map(){
	int i;
	struct page *pg=(struct page*)KERNEL_PAGE_START;
	init_page_buddy();
	for(i=0;i<(KERNEL_PAGE_NUM);i++){
		pg->vaddr=KERNEL_PAGEING_START+i*PAGE_SIZE;	/*pgָ���Ӧ�ڴ��*/
		pg->flags|=PAGE_AVAILABLE;
		INIT_LIST_HEAD(&pg->list);			/*��ʼ��pg->list*/
		if(i<(KERNEL_PAGE_NUM&(~PAGE_NUM_FOR_MAX_BUDDY))){			/*i������ڴ��ɻ��ֵ�ַ��*/
			if((i&PAGE_NUM_FOR_MAX_BUDDY)==0){/*����ڴ�黮���׵�ַ*/
					pg->order=MAX_BUDDY_PAGE_NUM-1;/*����ڴ�ͷֵ*/
			}else{
					pg->order=-1;
			}
			list_add_tail(&(pg->list),&page_buddy[MAX_BUDDY_PAGE_NUM-1]);
		}else					/*�����γ�max_buddy��ʣ���*/
		{
		pg->order=0;
		list_add_tail(&(pg->list),&page_buddy[0]);
		}
	}
}

/*����㷨�ڴ�������*/

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
			continue;		/*�����������order�ڴ�鲻����*/
		}else{
			pg=list_entry(page_buddy[neworder].next,struct page,list);/*pg->page�ṹ��*/
			tlst=&(BUDDY_END(pg,neworder)->list);	/*tlstָ����ڴ�ĩβ*/
			tlst->next->prev=&page_buddy[neworder];	/*�����ڴ��*/
			page_buddy[neworder].next=tlst->next;
			goto OUT_OK;
		}
	}
	return NULL;
OUT_OK:
	for(neworder--;neworder>=order;neworder--){
		tlst1=&(BUDDY_END(pg,neworder)->list);/*tlst1ָ�򽵽�order��Ϊ����ǰһ��end*/
		tlst=&(pg->list);			/*tlstָ���ڴ���ײ�page.list*/
		pg=NEXT_BUDDY_START(pg,neworder);	/*pgָ���벿��start*/
		list_entry(tlst,struct page,list)->order=neworder;/*��벿����order*/
		list_add_chain_tail(tlst,tlst1,&page_buddy[neworder]);/*�ҽ�ǰһ�뵽neworder*/
	}
	pg->flags|=PAGE_BUDDY_BUSY;  /*��1Ϊfree*/
	pg->order=order;
	return pg;
}


/*����㷨�ͷų���
 *��1Ϊbusy��־
 * */

void put_pages_to_list(struct page*pg,int order){
	struct page *tprev,*tnext;
	if((pg->flags&PAGE_BUDDY_BUSY)==0){
	return;	/*�ڴ��æ��ʧ�ܷ���*/
	}
	pg->flags&=~(PAGE_BUDDY_BUSY);/*��ɾ����*/
	for(;order<MAX_BUDDY_PAGE_NUM;order++){
		tnext=NEXT_BUDDY_START(pg,order);	/*tnext->���ں��ڴ���׵�ַ*/
		tprev=PREV_BUDDY_START(pg,order);	/*tprev->����ǰ�ڴ���׵�ַ*/
		if((!(tnext->flags&PAGE_BUDDY_BUSY))&&(tnext->order==order)){
			pg->order++;
			tnext->order=-1;
			list_remove_chain(&(tnext->list),&(BUDDY_END(pg,order)->list));	/*����tnext��*/
			BUDDY_END(pg,order)->list.next=&(tnext->list);			/*����ͷŵ��ڴ������*/
			tnext->list.prev=&(BUDDY_END(pg,order)->list);
			continue;
		}else if((!(tprev->flags&PAGE_BUDDY_BUSY))&&(tprev->order==order)){
			pg->order=-1;
			list_remove_chain(&(tprev->list),&(BUDDY_END(pg,order)->list));	/*����tprev��*/
			BUDDY_END(tprev,order)->list.next=&(pg->list);			/*����ͷŵ��ڴ������*/
			pg->list.prev=&(BUDDY_END(tprev,order)->list);
			pg=tprev;
			pg->order++;
			continue;
		}else{
			break;
		}

	}
	list_add_chain(&(pg->list),&(tnext-1)->list,&page_buddy[order]);			/*���չҽ�*/
}

/*
 *�ڴ��ַתpage �ṹ���ַ
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
