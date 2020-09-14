#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>         
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sqlite3.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>


//命令码
#define RE   0  //完善注册信息
#define R    1  //注册用户
#define L    2  //登录
#define GQ   3  //普通用户查询
#define RQ   4  //root用户查询所有人的所有信息(根据员工工号)
#define GU   5  //普通用户修改所有人信息
#define RU   6  //root用户修改所有人的所有信息(根据员工工号)
#define GPWD 7  //普通用户修改密码
#define RPWD 8  //root用户修改密码
#define H    9  //历史登录记录

//员工基本信息
typedef struct employee{
	char name[32];//姓名
	int age; //年龄
	char sex[10]; //性别
	int work_id; //工号
	int money; //工资（月）
	char department[32]; //部门
	char phone[32];//电话
}EMP_t;


typedef struct message{
	int cmd;//命令码
	char user[32];//用户名
	char pwd[32];//密码 
	char data[256];//查询的数据
	EMP_t emp; //基本信息
	
}MSG_t;

#define MSG_LEN  sizeof(MSG_t)

//用户服务器或者进程 出错的宏志
#define PERROR(error)  do{perror(error);return -1;}while(0)
//用于数据库出错的宏值
#define FPRINTF(error) do{fprintf(stderr,"%s:%s\n",error,sqlite3_errmsg(db));return-1;}while(0)
#define ERR_MSG(error) do{fprintf(stderr,"%s:%s\n",error,errmsg);return -1;}while(0)

//回收子进程函数
void signal_handle(int p){
	waitpid(p,NULL,WNOHANG);
}

//全局变量
int em_loop=0;  //0代表未进入回调函数  1代表进入回调函数
int vip_loop=0; //0是普通用户 1是超级用户

/*********************函数声明******************************/
//回调函数
int callback(void *prea,int f_num,char **f_value,char **f_name);
//注册
int do_register(sqlite3* db,int connfd,MSG_t* msg);
//完善登录注册的信息
int all_register(sqlite3* db,int connfd,MSG_t* msg);
//登录
int do_login(sqlite3 *db,int connfd,MSG_t* msg);


//普通用户查询个人信息
int gen_querry(sqlite3 *db,int connfd,MSG_t *msg);
//普通用户修改个人信息
int gen_update(sqlite3 *db,int connfd,MSG_t *msg);
//普通用户修改密码
int gen_update_pwd(sqlite3 *db,int connfd,MSG_t *msg);


//root用户查询所有人的所有信息(根据员工工号)
int root_querry(sqlite3 *db,int connfd,MSG_t *msg);
//root用户修改密码
int root_update_pwd(sqlite3 *db,int connfd,MSG_t *msg);
//root用户修改所有人的所有信息(根据员工工号)
int root_update(sqlite3 *db,int connfd,MSG_t *msg);
//root查询历史登录记录
int do_history(sqlite3 *db,int connfd,MSG_t *msg);
/*********************************************************/


int main(int argc, const char *argv[])
{
	//数据库操作

	sqlite3 *db;
	if(sqlite3_open("mylib.db",&db)!=0){
		FPRINTF("sqlite3_open");
	}

	char sql[256]={0};

	//flag 是标志位
	char *errmsg;
	//创建员工信息的表格
	sprintf(sql,"create table if not exists employee(work_id integer primary key autoincrement,flag text,user text,pwd text,name text,age int,sex text,money int,department text,phone text);");
	//数据库操作
	if(0!=sqlite3_exec(db,sql,NULL,NULL,&errmsg)){
		ERR_MSG("main exec1");
	}

	memset(sql,0,sizeof(sql));
	//创建历史登录记录的表格
	sprintf(sql,"create table if not exists  history(id integer primary key autoincrement,flag text,user text,pwd text,time text);");
	if(0!=sqlite3_exec(db,sql,NULL,NULL,&errmsg)){
		ERR_MSG("main exec2");
	}
	
	//创建监听套解字
	int listenfd=socket(AF_INET,SOCK_STREAM,0);
	if(listenfd==-1){
		PERROR("listenfd");
	}
	printf("create success\n");
	struct sockaddr_in  ser_addr,cli_addr;

	int len=sizeof(cli_addr);
	//采用的协议族类型
	ser_addr.sin_family=AF_INET;//IPV4
	ser_addr.sin_addr.s_addr=inet_addr("0.0.0.0");//IP地址
	ser_addr.sin_port=htons(8888);//端口号
	//绑定套接字
	if(-1==bind(listenfd,(struct sockaddr*)&ser_addr,len)){
		PERROR("bind");
	}
	printf("bind success\n");

	//监听100个
	if(-1==listen(listenfd,100)){
		PERROR("listen");
	}
	printf("listening ***************\n");	

	int connfd=0;	
	while(1){
		//接受客户端的请求，并返回通信套接字
		connfd=accept(listenfd,(struct sockaddr*)&cli_addr,&len);
		if(connfd==-1){
			PERROR("connfd");
		}		
		printf("IP:%s port:%d\n",inet_ntoa(cli_addr.sin_addr)
				,ntohs(cli_addr.sin_port));	
		printf("connect success\n");

		pid_t pid=fork();

		if(pid<0){
			perror("fork");
			return -1;
		}else if(pid==0){ //子进程通信
			close(listenfd);//关闭监听套接字
			MSG_t msg;
			memset(&msg,0,MSG_LEN);
			int ret=0;
			while(1){
				ret=read(connfd,&msg,MSG_LEN);
				if(ret<0){
					PERROR("read");
				}else if(ret==0){
					printf("user:%s 的客户端退出\n",msg.user);
					close(connfd);		
					exit(0);
				}
				switch(msg.cmd){
				case R:
					//注册
					do_register(db,connfd,&msg);
					break;
				case RE:
					//完善注册的信息
					all_register(db,connfd,&msg);
					break;
				case L:
					//用户登录
					do_login(db,connfd,&msg);
					break;
				case GQ:
					//普通用户查询个人信息
					gen_querry(db,connfd,&msg);
					break;
				case GU:
					//普通用户修改个人信息
					gen_update(db,connfd,&msg);
					break;
				case GPWD:
					gen_update_pwd(db,connfd,&msg);
					break;					
				case RQ:
					root_querry(db,connfd,&msg);
					break;
				case RU:
					root_update(db,connfd,&msg);
					break;
				case RPWD:
					root_update_pwd(db,connfd,&msg);
					break;			
				case H:
					do_history(db,connfd,&msg);
					break;
				}
				printf("**************************************************************\n");
			}
		}else{  //父进程进行监听新的客户端		
			close(connfd);
			signal(SIGCHLD,signal_handle);
		}
	}

	//关闭数据库
	if(sqlite3_close(db)!=0){
		PERROR("sqlite3_close");
	}
	close(listenfd);
	printf("server close\n");			
	return 0;
}

/***********************函数定义****************************************************/
//回调函数
int callback(void *prea,int f_num,char **f_value,char **f_name){

	em_loop=1;
	int i=0;
	for(i=0;i<f_num;i++){
		printf("%s ",f_name[i]);
	}
	putchar('\n');
	for(i=0;i<f_num;i++){	
		//普通用户
		if(strcmp(f_value[1],"gen")==0){
			vip_loop=0;
			//超级用户
		}else if(strcmp(f_value[1],"root")==0){
			vip_loop=1;
		}		
		printf("%s ",f_value[i]);
	}
	putchar('\n');

	return 0;
}

//注册
int do_register(sqlite3 *db,int connfd,MSG_t *msg){

	char sql[256]={0};
	char *errmsg=NULL;

	sprintf(sql,"select *from employee where user='%s';",msg->user);
	if(sqlite3_exec(db,sql,callback,NULL,&errmsg)!=0){
		ERR_MSG("do_register 1");
	}

	if(em_loop==1){
		write(connfd,"NO",2);
		//重置emloop
		em_loop=0;
		printf("帐号存在，请重新注册\n");
		return -1;
	}else if(em_loop==0){
		write(connfd,"OK",2);
	}

	//清空 插入部分值
	memset(sql,0,sizeof(sql));
	//gen是普通用户  root是超级用户，超级用户需要在数据库里更改
	sprintf(sql,"insert into employee(flag,user,pwd) values('%s','%s','%s');","gen",msg->user,msg->pwd);	
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
		ERR_MSG("do_register2");
	}
	//重置em_loop
	printf("%s  注册成功\n",msg->user);
	em_loop=0;
	return 0;
}
//完善信息
int all_register(sqlite3* db,int connfd,MSG_t *msg){

	char sql[256]={0};
	char *errmsg=NULL;
	sprintf(sql,"update employee set name='%s',age=%d,sex='%s',money=%d,department='%s',phone='%s' where user='%s';",msg->emp.name,msg->emp.age,msg->emp.sex,0,msg->emp.department,msg->emp.phone,msg->user);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
		ERR_MSG("all_register");
	}

	printf("信息完善成功\n");
	return 0;
}

//登录
int do_login(sqlite3 *db,int connfd,MSG_t *msg){
	char sql[256]={0};
	char *errmsg=NULL;
	char temp[32]={0};
	sprintf(sql,"select *from employee where user='%s' and pwd='%s';",msg->user,msg->pwd);
	if(sqlite3_exec(db,sql,callback,NULL,&errmsg)!=0){
		ERR_MSG("do_login");
	}
	
	if(vip_loop==0&&em_loop==1){
		write(connfd,"gen",3);
		strcpy(temp,"gen");
		printf("**********普通用户登录*************\n");
	}else if(vip_loop==1&&em_loop==1){			
		strcpy(temp,"root");
		write(connfd,"root",4);
		printf("**********超级用户登录*************\n");
	}else{
		write(connfd,"NO",2);
		printf("登录失败，帐号或密码输入有误\n");
		em_loop=0;
		vip_loop=0;
		return -1;
	}


	/*****************************添加history的数据库的信息***************************/
	memset(sql,0,sizeof(sql));
	//获取登录的时间
	time_t t;
	t=time(NULL);
	struct tm *ti=localtime(&t);
	char time_buf[64]={0};
	sprintf(time_buf,"%dday %d:%d",ti->tm_mday,ti->tm_hour,ti->tm_min);

	sprintf(sql,"insert into history(flag,user,pwd,time) values('%s','%s','%s','%s');",temp,msg->user,msg->pwd,time_buf);

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
		ERR_MSG("history");
	}
	/*****************************添加history的数据库的信息***************************/
	
	//重置条件变量
	em_loop=0;
	vip_loop=0;
	return 0;
}
//普通用户查询个人信息
int gen_querry(sqlite3 *db,int connfd,MSG_t *msg){
	char *errmsg=NULL;
	char sql[256]={0};


	sprintf(sql,"select *from employee where user='%s';",msg->user);

	char **result; //记录查找到所有的数据 
	int nrow;//成功返回的字段数目(多少行数据)
	int ncolumn;//每条记录包含的字段数目(多少列数据)

	if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg)!=0){
		ERR_MSG("gen_querry");
	}

	printf("nrow=%d,ncolumn=%d\n",nrow,ncolumn);

	//获取信息
	int i=0,k=0;
	memset(msg->data,0,sizeof(msg->data));
	for(i=0;i<ncolumn;i++){
		for(k=0;k<nrow+1;k++){
			printf("%s  ",*(result+(ncolumn*k+i))    );
			strcat(msg->data,*(result+(ncolumn*k+i))  );
			if(k==0){
				strcat(msg->data," :");
			}else if(k==1){
				strcat(msg->data,"\n");
			}
		}
		putchar('\n');
	}

	//发送查询到的数据
	write(connfd,msg,MSG_LEN);
	printf("*************信息查询，发送完毕**********");
	return 0;
}
//普通用户修改个人信息
int gen_update(sqlite3 *db,int connfd,MSG_t *msg){
	char sql[256]={0};	
	char *errmsg=NULL;
	//进入用户修改信息函数 向客户端发送OK
	write(connfd,"OK",2);

	int ret=0;
	while(1){
		//清空sql字符串
		memset(sql,0,sizeof(sql));
		ret=read(connfd,msg,MSG_LEN);
		if(ret<0){
			PERROR("gen_update read");	
		}else if(ret==0){
			printf("客户端关闭\n");
			return -1;
		}		
		//修改普通用户信息
		switch(msg->cmd){			
		case 1:			
			sprintf(sql,"update employee set name='%s' where user='%s';",msg->emp.name,msg->user);
			if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
				ERR_MSG("gen_update name");
			}
			//向客户端发送修改成功的信息
			write(connfd,"OK",2);
			printf("新姓名:%s 修改成功\n",msg->emp.name);
			break;
		case 2:
			sprintf(sql,"update employee set age=%d where user='%s';",msg->emp.age,msg->user);
			if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
				ERR_MSG("gen_update age");
			}
			//向客户端发送修改成功的信息
			write(connfd,"OK",2);
			printf("新年龄:%d 修改成功\n",msg->emp.age);					
			break;
		case 3:			
			sprintf(sql,"update employee set sex='%s' where user='%s';",msg->emp.sex,msg->user);
			if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
				ERR_MSG("gen_update sex");
			}			
			//向客户端发送修改成功的信息
			write(connfd,"OK",2);
			printf("性别:%s 修改成功\n",msg->emp.sex);
			
			break;
		case 4:
			sprintf(sql,"update employee set phone='%s' where user='%s';",msg->emp.phone,msg->user);
			if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
				ERR_MSG("gen_update phone");
			}
			//向客户端发送修改成功的信息
			write(connfd,"OK",2);
			printf("新手机号:%s 修改成功\n",msg->emp.phone);					
			break;		   
		}
		if(msg->cmd==0){
			break;
		}
		printf("---------------------------------------------------\n");
	}

	return 0;
}

//普通用户修改密码
int gen_update_pwd(sqlite3 *db,int connfd,MSG_t *msg){
	char sql[256]={0};
	char *errmsg=NULL;

	sprintf(sql,"update employee set pwd='%s' where user='%s';",msg->pwd,msg->user);

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
		ERR_MSG("gen_update_pwd");
	}
	
	//向客户端发送修改成功的信息
	write(connfd,"OK",2);
	printf("新密码:%s 修改成功\n",msg->pwd);		
	return 0;
}

//root用户查询所有人的所有信息
int root_querry(sqlite3 *db,int connfd,MSG_t *msg){	
	char sql[256]={0};
	char *errmsg=NULL;
	char **result; //记录查找到所有的数据 
	int nrow;//(多少行数据)
	int ncolumn;//(多少列数据)

	sprintf(sql,"select *from employee where work_id=%d;",msg->emp.work_id);
	if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg)!=0){
		ERR_MSG("gen_querry");
	}
	//清空data保存数据的字符串
	memset(msg->data,0,sizeof(msg->data));
	printf("nrow=%d ncolumn=%d\n",nrow,ncolumn);
	if(nrow==0){
		strcpy(msg->data,"NO");
		write(connfd,msg,MSG_LEN);
		printf("没有这个工号\n");
		return -1;
	}
	//获取信息
	int i=0,k=0;
	for(i=0;i<ncolumn;i++){
		for(k=0;k<nrow+1;k++){
			printf("%s  ",*(result+(ncolumn*k+i))    );
			strcat(msg->data,*(result+(ncolumn*k+i))  );
			if(k==0){
				strcat(msg->data," :");
			}else if(k==1){
				strcat(msg->data,"\n");
			}
		}
		putchar('\n');
	}
	//发送查询到的数据
	write(connfd,msg,MSG_LEN);
	printf("------------信息查询成功,发送完毕-----------\n");
	return 0;
}

//root查询历史登录记录
int do_history(sqlite3 *db,int connfd,MSG_t *msg){

	char sql[256]={0};
	char *errmsg=NULL;
	char **result; //记录查找到所有的数据 
	int nrow;//(多少行数据)
	int ncolumn;//(多少列数据)

	sprintf(sql,"select *from history;");
	if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg)!=0){
		ERR_MSG("do_history");
	}

	//获取信息
	int i=0,k=0;
	for(i=0;i<nrow+1;i++){
		memset(msg->data,0,sizeof(msg->data));
		for(k=0;k<ncolumn;k++){
			printf("%s  ",*result);
			strcat(msg->data,*result++);
			strcat(msg->data,"  ");		
		}
		strcat(msg->data,"\n");
		write(connfd,msg,MSG_LEN);
		putchar('\n');
	}

	//查询完毕后 发送一个NO,代表遍历结束
	memset(msg->data,0,sizeof(msg->data));
	strcpy(msg->data,"OK");
	write(connfd,msg,MSG_LEN);

	return 0;
}

//root用户修改密码
int root_update_pwd(sqlite3 *db,int connfd,MSG_t *msg){	

	char sql[256]={0};
	char *errmsg=NULL;

	sprintf(sql,"update employee set pwd='%s' where user='%s';",msg->pwd,msg->user);

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
		ERR_MSG("gen_update_pwd");
	}
	
	//向客户端发送修改成功的信息
	write(connfd,"OK",2);
	printf("新密码:%s 修改成功\n",msg->pwd);		
	return 0;

}

//root用户修改所有人的所有信息(根据员工工号)
int root_update(sqlite3 *db,int connfd,MSG_t *msg){	
	char sql[256]={0};
	char *errmsg=NULL;
	char **result; //记录查找到所有的数据 
	int nrow;//(多少行数据)
	int ncolumn;//(多少列数据)
	
	sprintf(sql,"select *from employee where work_id=%d;",msg->emp.work_id);
	if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg)!=0){
		ERR_MSG("gen_querry");
	}
	
	if(nrow==0){
		//无此员工工号
		write(connfd,"NO",2);
		printf("查询出错,请核实员工工号是否无误\n");
		return -1;
	}

	//查找到此员工工号
	write(connfd,"OK",2);
	
	while(1){
		memset(sql,0,sizeof(sql));
		//接收传送来的信息
		if(read(connfd,msg,MSG_LEN)<0){
			PERROR("root_update read");
			return -1;
		}

		switch(msg->cmd){
			case 1:
				sprintf(sql,"update employee set name='%s' where work_id=%d;",msg->emp.name,msg->emp.work_id);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
					ERR_MSG("root_update name");
				}
				//向客户端发送修改成功的信息
				write(connfd,"OK",2);
				printf("工号:%d 新姓名:%s 修改成功\n",msg->emp.work_id,msg->emp.name);		
				break;
			case 2:
				sprintf(sql,"update employee set age=%d where work_id=%d;",msg->emp.age,msg->emp.work_id);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
					ERR_MSG("root_update age");
				}
				//向客户端发送修改成功的信息
				write(connfd,"OK",2);
				printf("工号:%d 新年龄:%d 修改成功\n",msg->emp.work_id,msg->emp.age);	
				break;
			case 3:
				sprintf(sql,"update employee set sex='%s' where work_id=%d;",msg->emp.sex,msg->emp.work_id);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
					ERR_MSG("root_update sex");
				}
				//向客户端发送修改成功的信息
				write(connfd,"OK",2);
				printf("工号:%d 新性别:%s 修改成功\n",msg->emp.work_id,msg->emp.sex);	
				break;
			case 4:	
				sprintf(sql,"update employee set phone='%s' where work_id=%d;",msg->emp.phone,msg->emp.work_id);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
					ERR_MSG("root_update phone");
				}
				//向客户端发送修改成功的信息
				write(connfd,"OK",2);			
				printf("工号:%d 新手机号:%s 修改成功\n",msg->emp.work_id,msg->emp.phone);	
				break;
			case 5:
				sprintf(sql,"update employee set money=%d where work_id=%d;",msg->emp.money,msg->emp.work_id);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
					ERR_MSG("root_update money");
				}
				//向客户端发送修改成功的信息
				write(connfd,"OK",2);
				printf("工号:%d 新的工资(月薪):%d 修改成功\n",msg->emp.work_id,msg->emp.money);		
				break;
			case 6:
				sprintf(sql,"update employee set department='%s' where work_id=%d;",msg->emp.department,msg->emp.work_id);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
					ERR_MSG("root_update department");
				}
				//向客户端发送修改成功的信息
				write(connfd,"OK",2);			
				printf("工号:%d 新的部门:%s 修改成功\n",msg->emp.work_id,msg->emp.department);		
				break;
		}

		if(msg->cmd==0){
			break;
		}	
	}

	printf("root用户退出修改信息界面\n");
		return 0;
}

