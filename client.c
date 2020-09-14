#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>         
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

//命令码
#define RE   0  //完善注册时的信息
#define R    1  //注册
#define L    2  //登录
#define GQ   3  //普通用户查询个人信息
#define RQ   4  //root用户查询所有人信息(根据员工号)
#define GU   5  //普通用户修改信息
#define RU   6  //root用户修改所有人的所有信息（根据员工号）
#define GPWD 7  //普通用户修改密码 
#define RPWD 8  //root用户修改密码
#define H    9  //历史登录记录查询

//部门
#define AD  "Administrative Department"  //行政部
#define HRD "Human Resources Department" //人力资源
#define MD  "Market Department"          //市场部
#define TD  "Technology Department"      //技术部
#define CSD "Customer Service Department" //客服部

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
	char data[256];//查询信息
	EMP_t emp; //基本信息
}MSG_t;


#define MSG_LEN sizeof(MSG_t )
#define PERROR(error)  do{perror(error);return -1;}while(0)

/*************函数声明*********************************/
//注册
int do_register(int sockfd,MSG_t *msg);
//完善注册信息
int all_register(int sockfd,MSG_t *msg);
//登录
int do_login(int sockfd,MSG_t *msg);
//普通用户查询信息
int gen_querry(int sockfd,MSG_t *msg);
//普通用户修改信息
int gen_update(int sockfd,MSG_t *msg);
//普通用户修改密码
int gen_update_pwd(int sockfd,MSG_t *msg);
//超级用户查询所有人的信息
int root_querry(int sockfd,MSG_t *msg);
//超级用户修改密码
int root_update_pwd(int sockfd,MSG_t *msg);
//超级用户修改所有人的信息(根据员工号)
int root_update(int sockfd,MSG_t *msg);
//超级用户查询历史登录记录
int do_history(int sockfd,MSG_t *msg);
/****************************************************/

int main(int argc, const char *argv[])
{

	//创建监听套解字
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1){
		PERROR("sockfd");
	}	
	printf("create socket success\n");

	struct sockaddr_in ser_addr;
	int len=sizeof(ser_addr);
	ser_addr.sin_family=AF_INET;
	ser_addr.sin_addr.s_addr=inet_addr("0.0.0.0");
	ser_addr.sin_port=htons(8888);

	//建立链接
	if(-1==connect(sockfd,(struct sockaddr*)&ser_addr,len)){
		PERROR("connect");
	}
	printf("connect success\n");


	int cmd=0;
	MSG_t msg; //消息

	/***************************一级界面***************************************************/
on_while:	
	while(1){
		printf("***************(1级界面)*****************\n");
		printf("*******1.注册   2.登录   0.退出*********\n");
		printf("****************************************\n");
		printf("请输入命令>>:");
		scanf("%d",&cmd);
		getchar();
		switch(cmd){
		case 1:
			msg.cmd=R;
			printf("注测用户名:");	
			fgets(msg.user,sizeof(msg.user),stdin);
			msg.user[strlen(msg.user)-1]='\0';

			printf("注册密码:");
			fgets(msg.pwd,sizeof(msg.pwd),stdin);
			msg.pwd[strlen(msg.pwd)-1]='\0';
			//注册
			do_register(sockfd,&msg);
			break;
		case 2:
			msg.cmd=L;
			printf("请输入你的用户名:");	
			fgets(msg.user,sizeof(msg.user),stdin);
			msg.user[strlen(msg.user)-1]='\0';

			printf("请输入你的密码:");
			fgets(msg.pwd,sizeof(msg.pwd),stdin);
			msg.pwd[strlen(msg.pwd)-1]='\0';
			//登录
			int ret=do_login(sockfd,&msg);
			if(ret==0){
				goto down_while1;
			}else if(ret==1){
				goto down_while2;
			}
			break;	
		case 0:	
			goto END;
			break;
		default:
			printf("请输入有效的命令\n");
			break;	
		}
		printf("##############################################################\n");
	}

	/*****************************普通用户*********************************************/
down_while1:
	while(1){
		printf("****************普通用户(2级界面)**************\n");
		printf("*******1.查询个人信息 2.修改个人信息 **********\n");
		printf("*******3.修改密码     0.退出*******************\n");
		printf("请输入命令>>:");
		scanf("%d",&cmd);
		getchar();
		switch(cmd){
		case 1:
			msg.cmd=GQ;
			gen_querry(sockfd,&msg);
			break;
		case 2:
			msg.cmd=GU;
			gen_update(sockfd,&msg);
			break;
		case 3:
			msg.cmd=GPWD;
			gen_update_pwd(sockfd,&msg);
			break;
		case 0:
			printf("返回一级界面\n");
			printf("#################################################################\n");	
			goto on_while;
			break;
		default:
			break;
		}
		printf("#################################################################\n");	
	}
	/****************************root用户**************************************************/
down_while2:
	while(1){
		printf("**************************root用户(2级界面)*************************\n");
		printf("*******1.查询信息(根据员工号)   2.修改员工信息(根据员工号)**********\n");
		printf("*******3.查询历史登录记录  4.修改密码   0.退出**********************\n");
		printf("请输入命令>>:");
		scanf("%d",&cmd);
		getchar();
		switch(cmd){		
		case 1:
			msg.cmd=RQ;			
			root_querry(sockfd,&msg);
			break;
		case 2:
			msg.cmd=RU;
			root_update(sockfd,&msg);
			break;
		case 3:	
			msg.cmd=H;
			do_history(sockfd,&msg);
			break;
		case 4:
			msg.cmd=RPWD;
			root_update_pwd(sockfd,&msg);
			break;
		case 0:
			printf("返回一级界面\n");
			printf("#################################################################\n");	
			goto on_while;
			break;
		default:
			printf("无效命令，请重新选择\n");
			break;	
		}	
		printf("#################################################################\n");	
	}


END:
	close(sockfd);
	printf("客户端关闭\n");
	return 0;
}
/**************************************函数*******************************************************/

//注册
int do_register(int sockfd,MSG_t *msg){
	char buf[32]={0};

	//发送信息
	if(write(sockfd,msg,MSG_LEN)<0){
		PERROR("register write");
	}

	//接受信息
	if(read(sockfd,buf,sizeof(buf))<0){
		PERROR("register read");
	}

	if(strncmp(buf,"NO",2)==0){
		printf("用户已存在,请重新注册\n");
		return -1;
	}else if(strncmp(buf,"OK",2)==0){
		printf("注册成功\n");
		//调用完善信息的函数
		all_register(sockfd,msg);
	}
	return 0;
}

//完善注册的信息
int all_register(int sockfd,MSG_t *msg){
	msg->cmd=RE;
	int cmd;
	//姓名
	printf("please input your name:");
	fgets(msg->emp.name,sizeof(msg->emp.name),stdin);
	msg->emp.name[strlen(msg->emp.name)-1]='\0';
	//年龄
	printf("please input your age:");
	scanf("%d",&msg->emp.age);
	getchar();
	//性别
	while(1){
		printf("***********************\n");
		printf("****1.男  2.女*********\n");
		printf("please input your sex:");
		scanf("%d",&cmd);
		getchar();
		if(cmd==1){
			strcpy(msg->emp.sex,"man");
			break;
		}else if(cmd==2){
			strcpy(msg->emp.sex,"woman");
			break;
		}else{
			printf("无效命令，请重新选择\n");
		}
	}

	//部门
	while(1){
		printf("*********************************\n");
		printf("***1.行政部 2.人力资源 3.市场部**\n");
		printf("*******4.技术部 5.客服部*********\n ");
		printf("please input your department:");
		scanf("%d",&cmd);
		getchar();
		if(cmd==1){
			strcpy(msg->emp.department,AD);
			break;
		}else if(cmd==2){
			strcpy(msg->emp.department,HRD);
			break;	
		}else if(cmd==3){
			strcpy(msg->emp.department,MD);
			break;
		}else if(cmd==4){
			strcpy(msg->emp.department,TD);
			break;	
		}else if(cmd==5){	
			strcpy(msg->emp.department,CSD);
			break;
		}else {
			printf("无效命令，请重新选择\n");
		}
	}

	//手机号
	printf("please input your phone:");
	fgets(msg->emp.phone,sizeof(msg->emp.phone),stdin);
	msg->emp.phone[strlen(msg->emp.phone)-1]='\0';

	write(sockfd,msg,MSG_LEN);
	printf("信息完善成功\n");
	return 0;
}

//登录
int do_login(int sockfd,MSG_t *msg){

	if(write(sockfd,msg,MSG_LEN)<0){
		PERROR("login write");
	}

	char buf[32]={0};
	if(read(sockfd,buf,sizeof(buf))<0){
		PERROR("login read");
	}

	if(strcmp(buf,"NO")==0){
		printf("登录失败\n");
		return -1;
	}else if(strncmp(buf,"gen",3)==0){
		printf("普通用户登录成功\n");
		return 0;
	}else if(strncmp(buf,"root",4)==0){
		printf("超级用户登录成功\n");
		return 1;
	}
}

//普通用户查询个人信息
int gen_querry(int sockfd,MSG_t *msg){

	if(write(sockfd,msg,MSG_LEN)<0){
		PERROR("gen_querry write");
	}

	MSG_t temp;

	if(read(sockfd,&temp,MSG_LEN)<0){
		PERROR("gen_querry read");
	}

	printf("%s\n",temp.data);

	printf("***************个人信息查询完毕***********\n");
	return 0;
}
//普通用户修改个人信息
int gen_update(int sockfd,MSG_t *msg){
	if(write(sockfd,msg,MSG_LEN)<0){
		PERROR("gen_update write");
	}
	char buf[32]={0};	
	if(read(sockfd,buf,sizeof(buf))<0){
		PERROR("gen_update read");
	}
	if(strcmp(buf,"NO")==0){
		printf("修改信息界面出错\n");	
		return -1;
	}else if(strcmp(buf,"OK")==0);

	int loop=0;
	while(1){		
		printf("------------------(3级界面)----------------\n");
		printf("----------普通用户员工信息修改界面----------\n");
		printf("****1.姓名 2.年龄 3.性别 4.电话 0.退出*****\n");
		printf("-------------------------------------------\n");
		printf("请输入你的选择:");
		scanf("%d",&msg->cmd);
		getchar();
		//清空buf
		memset(buf,0,sizeof(buf));
		switch(msg->cmd){
		case 1:
			printf("请输入新的名字:");
			fgets(msg->emp.name,sizeof(msg->emp.name),stdin);
			msg->emp.name[strlen(msg->emp.name)-1]='\0';

			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));				
			if(strcmp(buf,"OK")==0){
				printf("新姓名:%s 修改成功\n",msg->emp.name);
			}else{
				printf("信息修改失败\n");
				return -1;
			}
			break;
		case 2:	
			printf("请输入新的年龄:");
			scanf("%d",&msg->emp.age);
			getchar();	
			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));				
			if(strcmp(buf,"OK")==0){
				printf("新年龄:%d 修改成功\n",msg->emp.age);
			}else{
				printf("信息修改失败\n");
				return -1;
			}				
			break;
		case 3:
			while(1){
				printf("****1.男  2.女****\n");
				printf("请输入你的选择:");
				scanf("%d",&msg->cmd);
				getchar();
				if(msg->cmd==1){
					strcpy(msg->emp.sex,"man");
					break;
				}else if(msg->cmd==2){
					strcpy(msg->emp.sex,"woman");
					break;
				}else{
					printf("无效选择，请重新选择\n");
				}
			}
			msg->cmd=3;
			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));				
			if(strcmp(buf,"OK")==0){
				printf("性别:%s 修改成功\n",msg->emp.sex);
			}else{
				printf("信息修改失败\n");
				return -1;
			}				
			break;
		case 4:
			printf("请输入新的电话号码:");
			fgets(msg->emp.phone,sizeof(msg->emp.phone),stdin);
			msg->emp.phone[strlen(msg->emp.phone)-1]='\0';

			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));				
			if(strcmp(buf,"OK")==0){
				printf("新手机号:%s 修改成功\n",msg->emp.phone);
			}else{
				printf("信息修改失败\n");
				return -1;
			}		
			break;
		case 0:
			loop=1;
			//发送信息
			printf("返回二级界面\n");
			write(sockfd,msg,MSG_LEN);
			break;
		default:
			printf("无效选项,请重新输入\n");
			break;
		}	
		if(loop==1) break;
	}

	return 0;
}

//普通用户修改密码
int gen_update_pwd(int sockfd,MSG_t *msg){

	char buf[32]={0};
	printf("新的密码:");
	fgets(msg->pwd,sizeof(msg->pwd),stdin);
	msg->pwd[strlen(msg->pwd)-1]='\0';

	if(write(sockfd,msg,MSG_LEN)<0){
		PERROR("gen_update_pwd write");
	}

	//接收信息
	read(sockfd,buf,sizeof(buf));				
	if(strcmp(buf,"OK")==0){
		printf("新密码:************ 修改成功,请妥善保管\n");
	}else{
		printf("信息修改失败\n");
		return -1;
	}	
	return 0;
}

//root用户查询所有人的信息
int root_querry(int sockfd,MSG_t *msg){

	memset(msg->data,0,sizeof(msg->data));
	printf("请输入想要查找的员工工号:");
	scanf("%d",&msg->emp.work_id);
	getchar();
	//发送信息
	if(write(sockfd,msg,MSG_LEN)<0){
		PERROR("root_querry  write");
	}
	//接收信息	
	if(read(sockfd,msg,MSG_LEN)<0){
		PERROR("root_querry read");	
	}

	if(strncmp(msg->data,"NO",2)==0){
		printf("员工工号输入有误\n");
		return -1;
	}

	printf("工号:%d\n",msg->emp.work_id);
	printf("%s\n",msg->data);
	return 0;
}

//超级用户查询登录记录
int do_history(int sockfd,MSG_t *msg){
	//发送信息
	if(write(sockfd,msg,MSG_LEN)<0){
		PERROR("root_querry  write");
	}
	while(1){
		//接收信息	
		if(read(sockfd,msg,MSG_LEN)<0){
			PERROR("root_querry read");	
		}

		if(strncmp(msg->data,"OK",2)==0){
			printf("退出\n");
			break;
		}

		printf("%s",msg->data);
		printf("....................................\n");		
	}

	return 0;
}
//超级用户修改密码
int root_update_pwd(int sockfd,MSG_t *msg){

	char buf[32]={0};
	printf("新的密码:");
	fgets(msg->pwd,sizeof(msg->pwd),stdin);
	msg->pwd[strlen(msg->pwd)-1]='\0';

	if(write(sockfd,msg,MSG_LEN)<0){
		PERROR("root_update_pwd write");
	}

	//接收信息
	read(sockfd,buf,sizeof(buf));				
	if(strcmp(buf,"OK")==0){
		printf("新密码:************ 修改成功,请妥善保管\n");
	}else{
		printf("信息修改失败\n");
		return -1;
	}	
	return 0;
}

//超级用户修改所有人的信息(根据员工号)
int root_update(int sockfd,MSG_t *msg){	

	char buf[32]={0};
	printf("请输入你想要修改的员工工号:");
	scanf("%d",&msg->emp.work_id);

	if(write(sockfd,msg,MSG_LEN)<0){
		PERROR("root_update write");
	}

	if(read(sockfd,buf,sizeof(buf))<0){
		PERROR("root_update read");
	}

	//对比传来的信息，如果没有此工号，则退出，有就可以修改
	if(strcmp(buf,"NO")==0){
		printf("无此员工，请核实员工工号是否正确\n");
		return -1;
	}else if(strcmp(buf,"OK")==0);

	int loop=0;

	while(1){
		printf("------------------(3级界面)----------------\n");
		printf("----------root用户员工信息修改界面---------\n");
		printf("---------------员工工号:%d-----------------\n",msg->emp.work_id);
		printf("********1.姓名 2.年龄 3.性别 4.电话********\n");
		printf("***********5.月薪  6.部门   0退出**********\n ");
		printf("-------------------------------------------\n");
		printf("请输入你的选择:");
		scanf("%d",&msg->cmd);
		getchar();
		memset(buf,0,sizeof(buf));
		//选择
		switch(msg->cmd){
		case 1:
			printf("工号:%d 新的姓名:",msg->emp.work_id);
			fgets(msg->emp.name,sizeof(msg->emp.name),stdin);
			msg->emp.name[strlen(msg->emp.name)-1]='\0';

			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));				
			if(strcmp(buf,"OK")==0){
				printf("工号:%d   新姓名:%s 修改成功\n",msg->emp.work_id,msg->emp.name);
			}else{
				printf("信息修改失败\n");
				return -1;
			}
			break;
		case 2:
			printf("工号:%d 新的年龄:",msg->emp.work_id);
			scanf("%d",&msg->emp.age);
			getchar();

			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));				
			if(strcmp(buf,"OK")==0){
				printf("工号:%d   新姓名:%s 修改成功\n",msg->emp.work_id,msg->emp.name);
			}else{
				printf("信息修改失败\n");
				return -1;
			}
			break;
		
		case 3:
			while(1){
				printf("*****工号:%d******\n",msg->emp.work_id);
				printf("****1.男  2.女****\n");
				printf("请输入你的选择:");
				scanf("%d",&msg->cmd);
				getchar();
				if(msg->cmd==1){
					strcpy(msg->emp.sex,"man");
					break;
				}else if(msg->cmd==2){
					strcpy(msg->emp.sex,"woman");
					break;
				}else{
					printf("无效选择，请重新选择\n");
				}
			}
			msg->cmd=3;
			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));				
			if(strcmp(buf,"OK")==0){
				printf("工号:%d  性别:%s 修改成功\n",msg->emp.work_id,msg->emp.sex);
			}else{
				printf("信息修改失败\n");
				return -1;
			}
			break;
		case 4:
			printf("员工:%d 新的手机号:",msg->emp.work_id);
			fgets(msg->emp.phone,sizeof(msg->emp.phone),stdin);
			msg->emp.phone[strlen(msg->emp.phone)-1]='\0';

			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));				
			if(strcmp(buf,"OK")==0){
				printf("员工:%d 手机号:%s 修改成功\n",msg->emp.work_id,msg->emp.phone);
			}else{
				printf("信息修改失败\n");
				return -1;
			}			
			break;

		case 5:		
			printf("员工:%d 新的工资(月薪):",msg->emp.work_id);
			scanf("%d",&msg->emp.money);
			getchar();

			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));				
			if(strcmp(buf,"OK")==0){
				printf("员工:%d 新的工资(月薪):%d 修改成功\n",msg->emp.work_id,msg->emp.money);
			}else{
				printf("信息修改失败\n");
				return -1;
			}			
			break;
		case 6:
			while(1){
			printf("************工号:%d**************\n",msg->emp.work_id);
			printf("***1.行政部 2.人力资源 3.市场部**\n");
			printf("*******4.技术部 5.客服部*********\n ");
			printf("请选择工号:%d 的部门:",msg->emp.work_id);
			scanf("%d",&msg->cmd);	
			getchar();
			if(msg->cmd==1){
				strcpy(msg->emp.department,AD);
				break;
			}else if(msg->cmd==2){
				strcpy(msg->emp.department,HRD);
				break;	
			}else if(msg->cmd==3){
				strcpy(msg->emp.department,MD);
				break;
			}else if(msg->cmd==4){
				strcpy(msg->emp.department,TD);
				break;	
			}else if(msg->cmd==5){	
				strcpy(msg->emp.department,CSD);
				break;
			}else {
				printf("无效命令，请重新选择\n");
			}
			}

			msg->cmd=6;
			//发送信息
			write(sockfd,msg,MSG_LEN);
			//接收信息
			read(sockfd,buf,sizeof(buf));
			if(strcmp(buf,"OK")==0){
				printf("员工:%d 新的部门:%s 修改成功\n",msg->emp.work_id,msg->emp.department);
			}else{			
				printf("信息修改失败\n");
				return -1;
			}
			
			break;
		case 0:
			loop=1;
			//发送信息
			write(sockfd,msg,MSG_LEN);		  	 
			break;
		default:
			printf("无效命令，请重新选择\n");
			break;
		}		
		if(loop==1){
			break;
		}
	}
	printf("root 用户退出修改信息界面\n");
	return 0;
}

