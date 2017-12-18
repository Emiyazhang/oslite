//printk��������Ҫ�������ַ����def
//��#define STD_OUT_ADD	50000020(ARM9 24X0)
//��ѡ������ʽ��const or ++ ͨ������һ��ֵ
#include "print.h"
extern char * table_P;
#define STD_OUT_ADD table_P
typedef char * va_list;
#define _INTSIZEOF(n)   ((sizeof(n)+sizeof(int)-1)&~(sizeof(int)-1))	//��ȡ����n��int�� ��������+size(int)-1ʹ������ȡ��
#define va_start(ap,v)	(ap = (va_list)&v + _INTSIZEOF(v))				//ָ�������V�ĸߵ�ַ�Ӷ�ʵ��ָ��������ڵĵ�ַ
#define va_arg(ap,t)	(* (t*) ((ap+=_INTSIZEOF(t))-_INTSIZEOF(t)) )	//��õ�ǰ*ap����ʹ��apָ����һ��
#define va_end(ap)		( ap = (va_list)0 )								//NULL

const char * digits="0123456789abcdef"		;	//��������
extern char numbers[68];			//��ֵ�����ݴ����棬long8*8=64+����+0x+'\0'
static char print_buff[1024];			//print_k()���������

#define FORMAT_TYPE_MASK		0XFF00
#define FORMAT_TYPE_SIGN_BIT	0X0100
#define FORMAT_TYPE_NONE		0X000
#define FORMAT_TYPE_CHAR		0X100
#define FORMAT_TYPE_UCHAR		0X200
#define FORMAT_TYPE_SHORT		0X300
#define FORMAT_TYPE_USHORT		0X400
#define FORMAT_TYPE_INT			0X500
#define FORMAT_TYPE_UINT		0X600
#define FORMAT_TYPE_LONG		0X700
#define FORMAT_TYPE_ULONG		0X800
#define FORMAT_TYPE_STR			0Xd00
#define FORMAT_TYPE_PTR			0X900
#define FORMAT_TYPE_SIZE_T		0Xb00


#define FORMAT_TYPE(X)			((X)&(FORMAT_TYPE_MASK))
#define SET_FORMAT_TYPE(x,t)	do{(x) &=~FORMAT_TYPE_MASK;\
									x|=t;} while(0)

#define FORMAT_SIGNED(x)		((x)&FORMAT_TYPE_SIGN_BIT)
#define FORMAT_FLAG_MASK		0XFFFF0000		//FLAG(����ո���) 15-8������7-0������
#define FORMAT_FLAG_SPACE		0X10000
#define FORMAT_FLAG_ZEROPAD		0X20000
#define FORMAT_FLAG_LEFT		0X40000
#define FORMAT_FLAG_WIDTH		0X100000

#define FORMAT_FLAG(x)			((x)&FORMAT_FLAG_MASK)
#define SET_FORMAT_FLAG(x,f) 	((x)|=f)


#define FORMAT_BASE_MASK		0XFF
#define FORMAT_BASE_O			0X08
#define FORMAT_BASE_X			0X10
#define FORMAT_BASE_D			0X0A
#define FORMAT_BASE_B			0X02

#define FORMAT_BASE(X)			((X)&FORMAT_BASE_MASK)
#define SET_FORMAT_BASE(X,T)	do{(X)&=~FORMAT_BASE_MASK;\
									(X)|=(T);}while(0)

#define do_div(n,base)			({int __res;\
									__res=((unsigned int)n)%(unsigned int)base;\
									n=((unsigned int)n)/(unsigned int)base; \
									__res;})

//�����STD_OUT_ADD��ָ���ĵ�ַ
void __put_char(char * p,int num) {
	while(*p&&(num--)){
		//*(volatile unsigned *) STD_OUT_ADD=*p++;
		* STD_OUT_ADD++=* p++;
		};
}


//�ַ�������������ֱ�ӿ�����������
void * memcpy(void * dest, const void * src,unsigned  int count)
{
	char *temp=(char * )dest , *s=(char * )src;
	while(count --)
		*temp++= *s++;
	return dest;

}

//������ֵ������
char *number(char *str,int num,int base ,unsigned int flags)
{
	int i=0;
	int sign=0;

	if(FORMAT_SIGNED(flags)&&(signed int)num<0)
		{
			sign=1;					//������־
			num=~num+1;				//num+(~num)+1=0  ��ʹ������Ϊ����

		}

	do  {
		numbers[i++]=digits[do_div(num,base)];	//������Ƹ�ʽ��
		}while(num!=0);

	if(FORMAT_BASE(flags)==FORMAT_BASE_O){
		numbers[i++]='o';
		numbers[i++]='0';
	}
	else if(FORMAT_BASE(flags)==FORMAT_BASE_X){
		numbers[i++]='x';
		numbers[i++]='0';
	}
	else if(FORMAT_BASE(flags)==FORMAT_BASE_B){
		numbers[i++]='b';
		numbers[i++]='0';
	}
	if(sign){
	numbers[i++]='-';     //д�븺��
	}

	while(i-->0)
		*str++=numbers[i];	//���������ֵд���ӡ������
	return str;
}

int format_decode(const char *fmt,unsigned int * flags)		//��ʽ���룬��֧�ָ�������/ת���ַ�����
{
 	const char *start=fmt;
 	*flags &=~ FORMAT_TYPE_MASK;
 	*flags |= FORMAT_TYPE_NONE;		//flags����
 	for(;*fmt;++fmt){
 		if(*fmt == '%')
 		break;
 	}

 	if (fmt!=start || !*fmt)		//'%' ǰ�ĳ������ݻ��߽�����ϣ������ʽ�ַ���β��
	return fmt-start;				//���أ�����/�ҵ�%������λ�ã�

	do{
		fmt++;						//ָ��%����һλ����,������ַ������ݣ�
		switch(*fmt){
			case 'l':
				SET_FORMAT_FLAG(*flags,FORMAT_FLAG_WIDTH);
				break;
			default:
				break;
			}
		}while(0);

	SET_FORMAT_BASE(*flags,FORMAT_BASE_D);
	switch(* fmt){
		case 'c':
			SET_FORMAT_TYPE(*flags,FORMAT_TYPE_CHAR);
			break;

		case 's':
			SET_FORMAT_TYPE(*flags,FORMAT_TYPE_STR);
			break;

		case 'o':
			SET_FORMAT_BASE(*flags,FORMAT_BASE_O);
			SET_FORMAT_TYPE(*flags,FORMAT_TYPE_UINT);
			break;

		case 'x':
		case 'X':
			SET_FORMAT_BASE(*flags,FORMAT_BASE_X);
			SET_FORMAT_TYPE(*flags,FORMAT_TYPE_UINT);
			break;

		case 'd':
		case 'i':
			SET_FORMAT_BASE(*flags,FORMAT_BASE_D);
			SET_FORMAT_TYPE(*flags,FORMAT_TYPE_INT);
			break;

		case 'u':
			SET_FORMAT_BASE(*flags,FORMAT_BASE_D);
			SET_FORMAT_TYPE(*flags,FORMAT_TYPE_UINT);
			break;

		default:
			break;

		}
		return ++fmt-start;			//ָ����һ��
}

int  vsnprintf(char *buff, int size ,const char *fmt,va_list args)
{
	int num;
	char *str,*end,*s,c;
	int read;
	unsigned int spec=0;

	str=buff;
	end=str+size;

	if(end < buff){
		end=((void*)-1);
		size=end-buff;
	}						//size����ɸ����ˣ�����β��ʹ��buff[max]='\0'

	while(*fmt){
		const char *old_fmt=fmt;

		read=format_decode(fmt,&spec)	;		//spec��¼��������,������number()�������ã�����ֵΪ%ǰ����
		fmt+=read;								//ָ��%��

		if((FORMAT_TYPE(spec))==FORMAT_TYPE_NONE)		//%ǰ��ֹ�����
		{
			int copy =read;

			if (str<end){
				if(copy>end-str)
					copy=end-str;
				memcpy(str,old_fmt,copy);    //��%ǰ����cp��strָ�򻺳���
			}
			str+=read;
		}

		else if((FORMAT_TYPE(spec))==FORMAT_FLAG_WIDTH){
		}				//δ������ַ�

		else if((FORMAT_TYPE(spec))==FORMAT_TYPE_CHAR){

			c=(unsigned char)va_arg(args,int);		//	��õ�ǰ����num����ʹargsָ����һ��
			if(str<end)
				*str=c;
			++str;
		}

		else if((FORMAT_TYPE(spec))==FORMAT_TYPE_STR){
			s=va_arg(args,char *);
			while(str<end&&s!='\0'){
				*str++=*s++;
			}
		}


		else {
				if((FORMAT_TYPE(spec))==FORMAT_TYPE_INT){
					num= va_arg(args,int);
				}

				else if((FORMAT_TYPE(spec))==FORMAT_TYPE_ULONG){
					num=va_arg(args,unsigned long);
				}

				else if((FORMAT_TYPE(spec))==FORMAT_TYPE_LONG){
					num=va_arg(args, long);
				}

				else if((FORMAT_TYPE(spec))==FORMAT_TYPE_SIZE_T){
					num=va_arg(args,int);
				}

				else if((FORMAT_TYPE(spec))==FORMAT_TYPE_USHORT){
					num=va_arg(args,unsigned short);
				}

				else if((FORMAT_TYPE(spec))==FORMAT_TYPE_SHORT){
					num=va_arg(args,short);
				}

				else{
					num=va_arg(args,unsigned int);
				}

				str=number(str,num,spec&FORMAT_BASE_MASK,spec);
		}
	}
	if(size>0){
		if(str<end)
			* str='\0';
		else
			end[-1]='\0';
	}
	return str-buff;
}


//���յĴ�ӡ����
void printk(const char * fmt, ...)
{
	va_list args;
	unsigned int i;

	va_start (args,fmt);

	i=vsnprintf(print_buff,sizeof(print_buff),fmt,args);
	va_end(args);

	__put_char (print_buff,i);
}
