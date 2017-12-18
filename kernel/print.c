//printk函数，需要对输出地址进行def
//即#define STD_OUT_ADD	50000020(ARM9 24X0)
//可选递增方式，const or ++ 通过定义一个值
#include "print.h"
extern char * table_P;
#define STD_OUT_ADD table_P
typedef char * va_list;
#define _INTSIZEOF(n)   ((sizeof(n)+sizeof(int)-1)&~(sizeof(int)-1))	//获取参量n的int型 对齐数，+size(int)-1使得向上取整
#define va_start(ap,v)	(ap = (va_list)&v + _INTSIZEOF(v))				//指向参量表V的高地址从而实现指向参量所在的地址
#define va_arg(ap,t)	(* (t*) ((ap+=_INTSIZEOF(t))-_INTSIZEOF(t)) )	//获得当前*ap，并使得ap指向下一个
#define va_end(ap)		( ap = (va_list)0 )								//NULL

const char * digits="0123456789abcdef"		;	//进制数表
extern char numbers[68];			//数值型数据处理缓存，long8*8=64+符号+0x+'\0'
static char print_buff[1024];			//print_k()输出缓冲区

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
#define FORMAT_FLAG_MASK		0XFFFF0000		//FLAG(对齐空格宽度) 15-8：类型7-0：进制
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

//输出到STD_OUT_ADD所指代的地址
void __put_char(char * p,int num) {
	while(*p&&(num--)){
		//*(volatile unsigned *) STD_OUT_ADD=*p++;
		* STD_OUT_ADD++=* p++;
		};
}


//字符串常量型数据直接拷贝到缓冲区
void * memcpy(void * dest, const void * src,unsigned  int count)
{
	char *temp=(char * )dest , *s=(char * )src;
	while(count --)
		*temp++= *s++;
	return dest;

}

//处理数值型数据
char *number(char *str,int num,int base ,unsigned int flags)
{
	int i=0;
	int sign=0;

	if(FORMAT_SIGNED(flags)&&(signed int)num<0)
		{
			sign=1;					//负数标志
			num=~num+1;				//num+(~num)+1=0  即使负数变为正数

		}

	do  {
		numbers[i++]=digits[do_div(num,base)];	//存入进制格式数
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
	numbers[i++]='-';     //写入负号
	}

	while(i-->0)
		*str++=numbers[i];	//处理完的数值写入打印缓冲区
	return str;
}

int format_decode(const char *fmt,unsigned int * flags)		//格式解码，不支持浮点数和/转义字符解析
{
 	const char *start=fmt;
 	*flags &=~ FORMAT_TYPE_MASK;
 	*flags |= FORMAT_TYPE_NONE;		//flags清零
 	for(;*fmt;++fmt){
 		if(*fmt == '%')
 		break;
 	}

 	if (fmt!=start || !*fmt)		//'%' 前的常量数据或者解析完毕，到达格式字符串尾部
	return fmt-start;				//返回：结束/找到%，返回位置；

	do{
		fmt++;						//指向‘%’后一位数据,处理宽字符型数据；
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
		return ++fmt-start;			//指向下一个
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
	}						//size过大成负数了，函数尾部使得buff[max]='\0'

	while(*fmt){
		const char *old_fmt=fmt;

		read=format_decode(fmt,&spec)	;		//spec记录参数特征,将用于number()函数调用，返回值为%前常量
		fmt+=read;								//指向‘%’

		if((FORMAT_TYPE(spec))==FORMAT_TYPE_NONE)		//%前终止或结束
		{
			int copy =read;

			if (str<end){
				if(copy>end-str)
					copy=end-str;
				memcpy(str,old_fmt,copy);    //将%前数据cp到str指向缓冲区
			}
			str+=read;
		}

		else if((FORMAT_TYPE(spec))==FORMAT_FLAG_WIDTH){
		}				//未处理宽字符

		else if((FORMAT_TYPE(spec))==FORMAT_TYPE_CHAR){

			c=(unsigned char)va_arg(args,int);		//	获得当前参数num，并使args指向下一个
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


//最终的打印函数
void printk(const char * fmt, ...)
{
	va_list args;
	unsigned int i;

	va_start (args,fmt);

	i=vsnprintf(print_buff,sizeof(print_buff),fmt,args);
	va_end(args);

	__put_char (print_buff,i);
}
