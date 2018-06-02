/* name: Yuntao Wang 
 * login id: yuntaow
 */


#include "server.h"

int main(int argc, char**argv){
	if (argc != 3){
		printf("corret arugemnt format: [port number] [root directory]\n");
		exit(1);
	}
	int server, connection, clientlen, err_no;
	struct sockaddr_in clientaddr;
	// prepared for getaddrinfo() to hold host address infomatino
	int length = sizeof(clientaddr);
	int thread_num = get_empty_thread();
	/*assigns argument to root directory*/
	strcpy(web_root,argv[2]);
	/*creates a sock address*/
	struct sockaddr_in serv_addr;
	/*creates a sock file descriptor*/
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0){
		perror("Failed to create socket");
		exit(1);
	} 
	/*converts port to int */
	int portnumber = atoi(argv[1]);
	/*basic set-up of serv_addr*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portnumber);
	/*binds the sock_addr to a port*/
	if (bind(server, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) 
	{
		perror("ERROR on binding");
		exit(1);
	}
	// listen to a multiple requests 
	if (listen(server, MAX_REQUEST) == -1) {
		printf("Failed to listen");
		return -1;
	}
	struct thread *args;
	/*all workers*/
	pthread_t multithread[NUM_THREAD];
	/*initialises pthread*/
	if (pthread_mutex_init(&lock, NULL) != 0){
        printf("\n mutex init failed\n");
        exit(1);
    }
    while(1){
    	/*accepts the connection from clients*/
    	if((connection = accept(server, (struct sockaddr *) &clientaddr, (socklen_t *) &length)) == -1){
    		printf("Failed to accept one client");
			continue;
    	}
    	/*sends an error indicating all threads full*/
    	if (thread_list[thread_num] == 1){
    		char req[50] = "ALL threads are working, please try later\n";
			int len = strlen(req);
			send(connection, &req, len, 0);
			close(connection);
    	}
    	else{
    		/*allocates memory for args*/
    		/*args is created for passing arguments to the thread just created*/
    		args = malloc(sizeof(struct thread));
    		args->thread_num = thread_num;
    		args->connection = connection;
    		/*creates new thread*/
    		if (pthread_create(&multithread[thread_num], NULL, &get, args) != 0)
				printf("thread creteError\n");
			/*next thread num is going to be the first empty thread*/
			thread_list[thread_num] = 1;
			/*thread becomes the next empty thread*/
			thread_num = get_empty_thread();
    	}
    }
    /*close socket*/
	if (close(server) == -1)
		printf("Close Error \n");
	return 0;
}

/*gets the first empty thread in the array*/
int get_empty_thread(){
	int thread_num = 1;         // initialise the thread_num to a random thread, just in case there is no emprt thread
	for(int t =1; t< sizeof(thread_list);t++){
		if(thread_list[t] ==0){
			thread_num = t;
			break;
		}
	}
	return thread_num;
}



/*implements the get method for the service*/
void *get(void *a){
	// an easy way to clean up when thread ends
	pthread_detach(pthread_self());
	/*some parameters declaration*/
	int buff_size = 200;
	char recv_buffer[BUFFER_SIZE], response_buffer[BUFFER_SIZE];
	char get[buff_size], file_path[buff_size], type[buff_size], payload_type[buff_size];
	struct thread *args = a;
	int sock = args->connection;
	int thread_num = args->thread_num;
	int recv_length,fd,len;
	/*handles request from client*/
	if((recv_length = recv(sock, recv_buffer, BUFFER_SIZE-1, 0)) == -1){
		printf("Failed to recieve");
		return NULL;
	}
	else{
		/*takes out infomation about the request*/
		sscanf(recv_buffer,  "%s %s %s", get, file_path, type);
	}
	/*checks the request type*/
	if(strcasecmp(get, "get")){
		/*returns errors if the request is other than GET*/
		sprintf(response_buffer, "HTTP/1.0 501 SERVER ONLY SUPPORT GET\n");
		if (send(sock, &response_buffer, BUFFER_SIZE, 0) <0)
			printf("Failed to send 501");
			return NULL;
	}
	/*extracts document type from file, only having the file listed in the sheet*/

	if (strstr(file_path, ".html"))			// checks if a sub string exists in a string
		strcpy(payload_type, "text/html");
	else if (strstr(file_path, ".css"))
		strcpy(payload_type, "text/css");
	else if (strstr(file_path, ".jpg"))
		strcpy(payload_type, "image/jpeg");
	else
		strcpy(payload_type, "text/javascript");
	/*converts the path in the request to real usable path*/
	char real_root[200] = {0};
	char real_path[200] = {0};
	memcpy(real_root, web_root+2, sizeof(web_root));
	memcpy(real_path, file_path+1, sizeof(file_path));
	char* complete = malloc(strlen(real_root) + strlen(real_path) + 2);
	strcpy(complete, real_root);
	strcat(complete, "/");
	strcat(complete, real_path);
	/*opens a file*/

	/*reasons for having stat s
	* stat is used for having [get file] status. It has multiple use cases:
	* 1. get the file size, which can be used for read/write or mmap
	* 2. if the file does not exist, open() usually create a new file.
	* 	 Instead of creating a new one, a 404 error should be return in our case.
	*/
	struct stat s;
	if (stat(complete, &s) < 0){
		sprintf(response_buffer, "HTTP/1.0 404 FILE NOT EXIST\n");
		if((send(sock, &response_buffer, BUFFER_SIZE, 0)) < 0)
			return NULL;
	}
	else{
		/*writes headers into the response buffer*/
		sprintf(response_buffer, "HTTP/1.0 200 OK\r\n");
		sprintf(response_buffer, "%sContent-length: %d\r\n", response_buffer, (int) s.st_size);
		sprintf(response_buffer, "%sContent-type: %s\r\n", response_buffer, payload_type);
		sprintf(response_buffer, "%sconnection: Keep-Alive\r\n\r\n", response_buffer);
		/*used for size clarification in send*/
		int buffer_size = strlen(response_buffer);
		/*sends whatever data is in the response buffer*/
		if((send(sock, &response_buffer, buffer_size, 0)) < 0)
			return NULL;
		/*open() function may have potential bugs,
		* if the file path length is greater than the maximum limit
		* an error of truncating the file path may appear
		*
		* for the second arguemnt, it is a flag parameter, 
		* in this case, it will read and write the file. 
		* write can be responsible for customizing the send data to client.
		* this assignment only require read method
		* 
		* fd: a file descriptor returned by open()
		*/
		if((fd = open(complete, O_RDWR, 0)) <0)
			printf("Failed to open the requested file\n");
	
		/*gets the data from the file descriptor
		* this is an improved way of doing read / write
		* reason: read/write perform system call for every operation
		* mmap perform one system call when you first initialse it.
		*/
		char *buf;
		buf = mmap(0, s.st_size,  PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
		int buck_size = 0;
		int sent = 0;
		/*sends the data from the bucket until all the bytes are sent*/
		while(buck_size < s.st_size){
			if ((sent = send(sock, buf, s.st_size,0)) < 0)
				printf("Failed to send\n");
			buck_size += sent;
		}
		/*close the file*/
		close(fd);
	}
	/*close the socket(file descriptor)*/
	close(sock);
	/*reset the thread to 0, which means it can be used again*/
	thread_list[args->thread_num] = 0;
	return 0;
}