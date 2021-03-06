/********************************************************
  gcc 2014510056_server.c -o 2014510056_server -------->  ./2014510056_server  

	/send <USER_ID> <MSG>

	/makegroup <CLIENT_ID1> <CLIENT_ID2> <CLIENT_ID3>...

	/sendgroup <GROUP_ID> <message>

	/joingroup <GROUP_ID>
*********************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<errno.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<time.h>
#include<ctype.h>

//storing message information
struct message{
	int source;
	int dest;
	char msg[1024];
};

//storing group information
struct group{
	int admin;
	int no_of_clients;
	int members[10];
	int group_id;
};

//storing temporary group information
struct grp_create_req{
	int g_id;
	int members[10];
	int processed[10];
	int admin;
	int no_of_clients;
	int no_of_rplys;
	int dec_count;
};

int main(int argc,char *argv[]){
	struct group *grps[10];
	struct grp_create_req *grp_reqs[10];
	int group_count=1,req=0;
	int serv_fd,port,client_port,sock_activity,client_socket;
	int max_sd,total_clients = 5,sd,online_clients=0;
	char buf[1024];
	int opt = 1; 
	struct sockaddr_in serv_addr;
	
	//socket descriptors set
	fd_set readfds;
	
	int clients[5],i;
	//initialized all clients with 0
	for(i=0;i<total_clients;i++){
		clients[i] = -1;
	}
	for(i=0;i<10;i++){
		grps[i] = NULL;
		grp_reqs[i] = NULL;
	}
	//creating socket
	serv_fd = socket(AF_INET,SOCK_STREAM,0);
	if(serv_fd < 0){
		perror("[-]ERROR: While Creating Server Socket..!\n");
		exit(0);
	}
	printf("[+]Succussfully Server Socket is created.. \n");
	//set server socket to allow multiple connections
	if( setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR,(char *)&opt,sizeof(opt)) < 0 ) { 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	}
	
	bzero(&serv_addr,sizeof(serv_addr));
	//assigning PORT and IP
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	port = 3205;
	serv_addr.sin_port = htons(port);
	//binding to particular port
	int bind_val = bind(serv_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
	if(bind_val < 0){
		printf("[-]ERROR: While binding to port %d\n",port);
		exit(0);
	}
	printf("[+]Server Socket successfully binded...\n");
	//sever listening
	if(listen(serv_fd,5) != 0){
		printf("[-]ERROR: while listen...\n"); 
		exit(0);
	}
	printf("[+]Server listening..\n");
	socklen_t len = sizeof(serv_addr);
	printf("[+]Waiting for connection...\n");
	//infinite loop for processing multiple client requests
	while(1){
		struct message *msg = NULL;
		//clearing readfds set
		FD_ZERO(&readfds);
		//adding serv_fd to readfds set
		FD_SET(serv_fd,&readfds);
		max_sd = serv_fd;
		for(i=0;i<total_clients;i++){
			//socket descriptor
			sd = clients[i];
			//cheching for socket decriptor valid or not 
			if(sd > 0){
				FD_SET(sd,&readfds);
			}
			if(sd > max_sd){
				max_sd = sd;
			}
		}
		//wait for an activity on one of the sockets
		sock_activity = select(max_sd+1,&readfds,NULL,NULL,NULL);
		if(sock_activity < 0){
			perror("[-]ERROR:in Select..!\n");
			exit(0);
		}
		//if something happens on server socket
		if(FD_ISSET(serv_fd,&readfds)){
			//accepting client request
			client_socket = accept(serv_fd,(struct sockaddr*)&serv_addr,&len);
			printf("socket desrcipoter %d\n",client_socket);
			if(client_socket < 0){
				perror("[-]ERROR: While accepting client requests..!\n");
				exit(0);
			}
			if(online_clients < total_clients){
				printf("[+]Connection accepted from %s:%d\n",inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port));
				//printf("[+]Succussfully connected to server...\n");
				bzero(buf,sizeof(buf));
				snprintf(buf, sizeof(buf), "[+]Succussfully connected to server...\nWelcome to Chart Application...\nYour unique id is %05d\n",client_socket);
				send(client_socket,buf,sizeof(buf),0);
				online_clients++;
				printf("online = %d  total = %d\n",online_clients,total_clients);
				for(i=0;i<total_clients;i++){
					if(clients[i] == -1){
						clients[i] = client_socket;
						break;
					}
				}
			}
			else{
				send(client_socket,"[-]Connection Limit Exceeded !!\n",strlen("[-]Connection Limit Exceeded !!\n"),0);
			}
		}
		for(i=0;i<total_clients;i++){
			sd = clients[i];
			if(FD_ISSET(sd,&readfds)){
				bzero(buf,sizeof(buf));
				//reading from client
				int read_bytes = read(sd,buf,1024);
				char buf2[1024];
				bzero(buf2,sizeof(buf2));
				strcpy(buf2,buf);
				printf("from client%s\n",buf);
				char* start_token = '\0';
				if(strlen(buf2) > 0){
					start_token = strtok(buf2, " "); 
					if(start_token[strlen(start_token)-1] == '\n'){
						start_token[strlen(start_token)-1] = '\0';
					}
				}
				if(read_bytes <= 0){
					getpeername(sd ,(struct sockaddr*)&serv_addr ,&len);
					printf("[-]Host Address { %s:%d } is Disconnected from Server..!\n",inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port)); 
					
					for(int j=0;j<total_clients;j++){
						if(clients[j] != -1 && clients[j] != sd){
							char msg[256];
							sprintf(msg,"%05d is going to exit..!\n",sd);
							//printf("%s",msg);
							send(clients[j],msg,sizeof(msg),0);
						}
					}
					printf("%05d is going to exit..!\n",sd);
					for(int j=0;j<10;j++){
						if(grps[j] != NULL){
							for(int k=0;k<grps[j]->no_of_clients;k++){
								if(grps[j]->members[k] == sd){
									printf("%05d is quiting from Group %05d..\n",sd,grps[j]->group_id);
									grps[j]->members[k] = -1;
									//send(sd,rply,sizeof(rply),0);
									break;
								}
							}
						}
					}
					close(sd);
					clients[i] = -1;
					online_clients--;
				}
				else if(strcmp(start_token,"/send") == 0){
					msg = (struct message*)malloc(sizeof(struct message));
					msg->source = sd;
					char* token = strtok(NULL," "); 
					//token = strtok(NULL," ");
					int start_index = strlen(start_token) + strlen(token)+2,check=0;
					char rply[1024];
					bzero(rply,sizeof(rply));
					if(token != NULL){
						msg->dest = atoi(token);
						for(int j=0;j<total_clients;j++){
							if(clients[j] == msg->dest){
								check =1;
								break;
							}
						}
					}
					else{
						sprintf(rply,"Invalid Format..!\n");
						printf("%s",rply);
						send(sd,rply,sizeof(rply),0);
					}
					if(check == 0){
						sprintf(rply,"Destination Not Exists..!\n");
						printf("%s",rply);
						send(sd,rply,sizeof(rply),0);
					}
					strcpy(msg->msg,buf+start_index);

					bzero(buf,sizeof(buf));
					sprintf(buf,"%05d: ",msg->source);
					strcat(buf,msg->msg);
					printf("sending : %s\n",buf);
					for(int j=0;j<total_clients;j++){
						if(clients[j] == msg->dest){
							send(clients[j],buf,sizeof(buf),0);
							char reply[256];
							sprintf(reply,"Message sent to %05d\n",msg->dest);
							printf("%s",rply);
							send(sd,reply,sizeof(reply),0);
						}
					}
					free(msg);
				}
				else if(strcmp(start_token,"/makegroup") == 0){
					struct group *new_group = (struct group*)malloc(sizeof(struct group));
					new_group->no_of_clients = 0;
					new_group->admin = sd;
					new_group->group_id = group_count;
					char grp_clients[1024];
					bzero(grp_clients,sizeof(grp_clients));
					strcpy(grp_clients,buf+10);
					char* token = strtok(grp_clients, " "); 
					char reply[256],create = 0;
					while(token != NULL){
						int id = atoi(token);
						int check = 0;
						for(int j=0;j<total_clients;j++){
							if(id == clients[j] && id != sd){
								new_group->members[new_group->no_of_clients++] = id;
								bzero(reply,sizeof(reply));
								sprintf(reply,"You were added to group with id %05d\n",new_group->group_id);
								printf("%05d added to group %05d\n",id,new_group->group_id);
								send(id,reply,sizeof(reply),0);
								check = 1;
								create++;
								break;
							}
						}
						if(check == 0 && id != sd){
							bzero(reply,sizeof(reply));
							sprintf(reply,"No client existed with id %s..!",token);
							printf("%s",reply);
							send(sd,reply,sizeof(reply),0);
						}
							
						token = strtok(NULL, " "); 
					}
					/*
					for(int j=0;j<new_group->no_of_clients;j++){
						bzero(reply,sizeof(reply));
						sprintf(reply,"You were added to group with id %05d\n",new_group->group_id);
						send(new_group->members[j],reply,sizeof(reply),0);
					}*/
					if(create == 0){
						bzero(reply,sizeof(reply));
						sprintf(reply,"Group %05d is not created..!\n",new_group->group_id);
						printf("%s",reply);
						send(sd,reply,sizeof(reply),0);
					}
					else{
						bzero(reply,sizeof(reply));
						sprintf(reply,"Group %05d created..!\n",new_group->group_id);
						printf("%s",reply);
						new_group->members[new_group->no_of_clients++] = sd;
						send(sd,reply,sizeof(reply),0);
						grps[group_count++] = new_group;
					}
				}
				else if((strcmp(start_token,"/joingroup") == 0)){
					char* token = strtok(buf, " "); 
					token = strtok(NULL," ");
					int grp_id=0,check=0;
					char rply[256];
					while(token != NULL){
						if(check != 0){
							check = -1;
							send(sd,"Invalid format..!",strlen("Invalid format..!"),0);
							printf("Invalid format..!\n");
							break;
						}
						grp_id = atoi(token);
						check++;
						token = strtok(NULL, " "); 
					}
					if(grp_id > group_count || grp_id == 0){
						bzero(rply,sizeof(rply));
						sprintf(rply,"Invalid group id..!\n");
						printf("Invalid group id..!\n");
						send(sd,rply,sizeof(rply),0);
					}
					else if(check == 1){				
						for(int j=0;j<10;j++){
							if(grps[j] != NULL){
								if(grp_id == grps[j]->group_id){
									for(int k=0;k<grps[j]->no_of_clients;k++){
										if(grps[j]->members[k] == sd){
											bzero(rply,sizeof(rply));
											sprintf(rply,"You are already in group %05d\n",grp_id);
											printf("%05d  already in group %05d\n",sd,grp_id);
											send(sd,rply,sizeof(rply),0);
											check = 0;
											break;
										}
									}
								}
							}
						}
								
						if(check == 1){
							int temp = 0;
							for(int l=0;l<10;l++){
								if(grp_reqs[l] != NULL && grp_reqs[l]->g_id == grp_id){
									for(int k=0;k<grp_reqs[l]->no_of_clients;k++){
										if(grp_reqs[l]->members[k] == sd){
											bzero(rply,sizeof(rply));
											if(grp_reqs[l]->processed[k] == -8888){
												sprintf(rply,"already declained..!\n");
												send(sd,rply,sizeof(rply),0);
												
											}
											else if(grp_reqs[l]->processed[k] == -8989){
												sprintf(rply,"waitig for others to accept\n");
												printf("waitig for others to accept\n");
												send(sd,rply,sizeof(rply),0);
												
											}
											else{
												grp_reqs[l]->no_of_rplys++;
												grp_reqs[l]->processed[k] = -8989;
												//grp_req[l]->members[k] = -1;
												
												sprintf(rply,"You are request for addig in group %05d is processed and Waiting for others..!\n",grp_id);
												printf("%05d %s",sd,rply);
												send(sd,rply,sizeof(rply),0);
												bzero(rply,sizeof(rply));
												sprintf(rply,"%05d has accepted your request for joining the group..!\n",sd);
												send(grp_reqs[l]->admin,rply,sizeof(rply),0);
												
											}
											check =0;
											break;
										}
									}
									//printf("rply %d clie %d\n",grp_reqs[l]->no_of_rplys,grp_reqs[l]->no_of_clients);
					//grp_reqs[l]->no_of_rplys + grp_reqs[l]->dec_count == grp_reqs[l]->no_of_clients && grp_reqs[l]->no_of_rplys != 0
									if(grp_reqs[l]->no_of_rplys + grp_reqs[l]->dec_count == grp_reqs[l]->no_of_clients){
										for(int k=0;k<10;k++){
											if(grps[k] == NULL){
												struct group *add = (struct group*)malloc(sizeof(struct group*));
												grps[k] = add;
												grps[k]->admin = grp_reqs[l]->admin;
												grps[k]->no_of_clients = grp_reqs[l]->no_of_clients;
												grps[k]->group_id = grp_reqs[l]->g_id;
												for(int m=0;m<grps[k]->no_of_clients;m++){
													if(grp_reqs[l]->members[m] != -1){
														grps[k]->members[m] = grp_reqs[l]->members[m];
														bzero(rply,sizeof(rply));
														sprintf(rply,"You were added to the group %05d\n",grps[k]->group_id);
														printf("%05d %s",grps[k]->members[m],rply);
														send(grps[k]->members[m],rply,sizeof(rply),0);
													}
												}
												//admin
												bzero(rply,sizeof(rply));
												sprintf(rply,"Group %05d is created..!\n",grp_reqs[l]->g_id);
												printf("%s",rply);
												grps[k]->members[grps[k]->no_of_clients++] = grp_reqs[l]->admin;
												send(grp_reqs[l]->admin,rply,sizeof(rply),0);
												break;
											}
										}	
									}
								}
								
							}
						}
						if(check == 1){
								bzero(rply,sizeof(rply));
								sprintf(rply,"You donn't have any request to join the group %05d\n",grp_id);
								send(sd,rply,sizeof(rply),0);
						}
					}
				}
				else if(strcmp(start_token,"/sendgroup") == 0){
					char temp[1024];
					strncpy(temp,buf,strlen(buf));
					char* token = strtok(NULL, " ");
					if(token!= NULL){
						int grp_id = atoi(token);
						int start_index = strlen(start_token) + strlen(token) + 2;
						printf("grp%s\n",token);
						char msg[1024];
						bzero(msg,sizeof(msg));
						strcpy(msg,buf+start_index);
						bzero(buf,sizeof(buf));
						sprintf(buf,"%05d:(group) ",grp_id);
						strcat(buf,msg);
						//strcat(buf,msg);
						int check = 0;
						char rply[1024];
						for(int j=0;j<group_count;j++){
							if(grps[j] != NULL && grp_id == grps[j]->group_id){
								for(int k = 0;k<grps[j]->no_of_clients;k++){
									if(sd == grps[j]->members[k]){
										check = 1;
										break;
									}
								}
								if(check == 1){
									for(int k = 0;k<grps[j]->no_of_clients;k++){
										printf("sending....!\n");
										send(grps[j]->members[k],buf,sizeof(buf),0);
									}
									bzero(rply,sizeof(rply));
									sprintf(rply,"Message delivered to all active members in group %05d..!\n",grp_id);
									printf("%s",rply);
									send(sd,rply,sizeof(rply),0);
								}
								else{
									send(sd,"You are not in the group\n",strlen("You are not in the group\n"),0);
									check = 1;
								}
								break;
							}
						}
						if(check == 0){
							bzero(rply,sizeof(rply));
							//printf("no grp\n");
							sprintf(rply,"No group is existed with id %s\n",token);
							printf("%s",rply);
							send(sd,rply,sizeof(rply),0);
						}
					
					}
					else{
						printf("Invalide format..!\n");
						send(sd,"Invalide format..!\n",strlen("Invalide format..!\n"),0);
					}
				}
				else{
					//checking for non exit condition
					printf("Invalid Request..!\n");
					send(sd,"nvalid Request..!\n",strlen("nvalid Request..!\n"),0);
					//bzero(buf,sizeof(buf));
				}
			}
		}
	}
}
