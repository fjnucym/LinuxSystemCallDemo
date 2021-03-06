#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <strings.h>
#include<sstream>

using namespace std;

#define PRIVATE_CHAT 0
#define GROUP_CHAT 1

string to_string_(int a);
typedef struct chat {
	int chat_type;		// 聊天类型 私聊|群聊
	int sender_id;		// [重要]发送方id	双方必须互相指定
	int receiver_id;	// [重要]接收方id	双方必须互相指定
	char message[100];	// 信息
}CHAT;

int socket_fd;
struct sockaddr_in s_addr;	// 网络套接字结构体
int len;
pthread_t read_pthread;
pthread_t write_pthread;
CHAT send_buf;				// 发送消息结构体
CHAT rcv_buf;				// 接收消息结构体
char buf[50];
int send_id;				// 输入send_id时 暂存send_id

/// <summary>
/// 客户端 读 线程函数
/// </summary>
/// <param name="p">传入该线程使用的参数</param>
/// <returns></returns>
void* client_read(void* p) {
	while (1) {
		int res = read(socket_fd, &rcv_buf, sizeof(rcv_buf));
		char buf[16];
		string name;
		if (res > 0) {
			if (rcv_buf.sender_id == send_id) {
				name = "自己";
			}
			else {
				sprintf(buf, "客户端%d", rcv_buf.sender_id);
				name += string(buf);
			}
		}
		else {
			cout << "服务器关闭...退出" << endl;
			exit(-1);
		}

		cout << name << ":" << rcv_buf.message << endl;
		bzero(rcv_buf.message, sizeof(rcv_buf.message));
	}
}

/// <summary>
/// 客户端 写 线程函数
/// </summary>
/// <param name="p">传入该线程使用的参数</param>
/// <returns></returns>
void* client_write(void* p) {
	while (1) {
		cout << "输入聊天类型（私聊：0，群聊：1）" << endl;
		cin >> send_buf.chat_type;
		if (send_buf.chat_type == GROUP_CHAT) {
			// 群聊
			cout << "输入自己的id：";
			cin >> send_buf.sender_id;
			send_id = send_buf.sender_id;
			// [重要] 客户端自报信息，否则服务器无法进行记录，也就无法为其开启转发线程
			write(socket_fd, &send_buf, sizeof(send_buf));
			while (1) {
				cin >> send_buf.message;
				write(socket_fd, &send_buf, sizeof(send_buf));
				bzero(send_buf.message, sizeof(send_buf.message));
			}
		}
		else if (send_buf.chat_type == PRIVATE_CHAT) {
			// 私聊
			cout << "输入自己的id：";
			cin >> send_buf.sender_id;
			send_id = send_buf.sender_id;
			cout << "输入接收方的id：";
			cin >> send_buf.receiver_id;
			// [重要] 客户端自报信息，否则服务器无法进行记录，也就无法为其开启转发线程
			write(socket_fd, &send_buf, sizeof(send_buf));
			while (1) {
				cin >> send_buf.message;
				write(socket_fd, &send_buf, sizeof(send_buf));
				bzero(send_buf.message, sizeof(send_buf.message));
			}
		}
		else {
			cout << "聊天类型输入不正确" << endl;
		}
	}
}

int main() {

	/*
	* 搭建网络通路
	* 参数1：协议族
	* 参数2：协议类型
	* 参数3：因为第二个参数已经确定了协议类型，默认0
	*/
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_fd < 0) {
		perror("socket error");
	}
	else {
		// 协议族
		s_addr.sin_family = AF_INET;
		// 设置端口号 9000以上，除此之外还有大小端问题，可以通过htons转换函数解决
		s_addr.sin_port = htons(10086);
		// 设置IP有Linux操作系统默认提供
			// 回环地址 127.0.0.1
		s_addr.sin_addr.s_addr = inet_addr("192.168.150.199");// 注意建议使用 s_addr.sin_addr.s_addr 有两层addr
		len = sizeof(s_addr);
		// connect记得要使用if语句验证的方式
		if (connect(socket_fd, (struct sockaddr*)&s_addr, len)) {
			perror("connect error");
			exit(-1);
		}

		// 客户端开启读、写线程向服务发送信息
		if (pthread_create(&write_pthread, NULL, client_write, NULL) < 0) {
			perror("pthread create error");
		}

		if (pthread_create(&read_pthread, NULL, client_read, NULL) < 0) {
			perror("pthrea create error");
		}

		// [重要]主线程死循环等待
		while (1);
	}
	return 0;
}

string to_string_(int a)
{
	ostringstream ostr;
	ostr << a;
	string astr = ostr.str();
	//cout << astr <<endl;
	return astr;
}