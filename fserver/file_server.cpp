
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>

struct file_head_t
{
	unsigned int file_path_len;
	char file_path[128];
};

int write_file(char *filename, char *buffer, int length, char isappend) 
{
	FILE *fp = NULL;
	if (isappend) {
		fp = fopen(filename, "ab+");
	} else {
		fp = fopen(filename, "wb+");
	}
	if (fp == NULL) {
		printf("fopen failed\n");
		return -1;
	}

	int size = fwrite(buffer, 1, length, fp);
	if (size != length) {
		printf("fread failed count : %d\n", size);
		goto _exit;
	}

_exit:
	fclose(fp);
	return size;
}

int main () 
{
    zmq::context_t context (1);  // 创建zmq的上下文
    zmq::socket_t socket (context, ZMQ_PULL); // 创建ZMQ_PULL类型的套接字
	socket.setsockopt(ZMQ_RCVTIMEO, 0);
    socket.bind("tcp://*:8080");   // 设置为tcp类型的通信方式，且绑定5555端口

	zmq::pollitem_t items [] = {
        { socket, 0, ZMQ_POLLIN, 0 }
    };
    
    while (true) 
    {
		int rc = zmq::poll (&items[0], 1, -1);
		if (items[0].revents & ZMQ_POLLIN)
		{
			zmq::message_t file_pkt;
			socket.recv(&file_pkt);
			
			file_head_t fh;
			memcpy(&fh.file_path_len, file_pkt.data(), sizeof(fh.file_path_len));
	        fh.file_path_len = ntohl(fh.file_path_len);
	        std::cout << "received file path len " << fh.file_path_len << std::endl;
			
			socket.recv(&file_pkt);
			memcpy(&fh.file_path, file_pkt.data(), fh.file_path_len);
			std::cout << "received file path " << fh.file_path << std::endl;
		    
			while (1)
			{
				bool more = file_pkt.more();
				if (more)
				{
					socket.recv(&file_pkt);
					write_file((char*)fh.file_path, (char*)file_pkt.data(), file_pkt.size(), 1);
				}
				else
				{
					break;
				}
			}
			
			std::cout << "received file content of file " << fh.file_path << std::endl;
		}
    }
    
    socket.close();
    context.close();
    
    return 0;
}
