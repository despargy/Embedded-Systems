/*
        Embedded Systems
  Main Project HMMY Sept 2018
  Despina-Ekaterini Argiropoulos        8491
        SERVER-TCP Pthreads
   RPI code modified by D.E Argiropoulos
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#define N 10
#define MaxMes 10
const char f[7] = {"from: "};
const char *userlist[N] = { "user1", "user2", "user3", "user4", "user5", "user6", "user7", "user8", "user9", "user10" };
char msg[N][MaxMes][256];
int from[N][MaxMes];
int num_of_msg[N] = { 0 };
//struct timeval startwtime, endwtime;
double total_t;
double diff1,diff2;

void store_to_file(){
  FILE *fp;
  fp = fopen("timeResults.txt","a+");
  int x;
  x = fprintf(fp,"%lf\t",seq_time);	//WALLTIME
  x = fprintf(fp,"%lf\t",diff1);	//CPU
  x = fprintf(fp,"%lf\t",diff2);	//SYSTEM
  x = fprintf(fp, "%lf\n", total_t);	//TOTAL
  fclose(fp);
}

void error(char *msg)
{
  perror(msg);
  exit(1);
}

struct param{
   int newsockfd;
   struct sockaddr_in cli_addr;
};

void func(struct param *clientPar){

  char buffer[256], username[10], clientname[10];
  int n, i, delid, clid;
  clock_t start_t, end_t;
  struct rusage buf2,buf3;
  //get time
   start_t = clock();
   getrusage(RUSAGE_SELF, &buf2); // POINT B
  //read clientname
    sleep(1);
    bzero(clientname,10);
    n = read((*clientPar).newsockfd,clientname,9);
    if (n < 0) error("ERROR reading from socket the clientname");
  //match clientname to clid
    clientname[n-1] = '\0';
    i = 0;
    while( (strcmp(clientname, userlist[i]) != 0) && ( i != (N-1) ) )  i++;
    if (strcmp(clientname, userlist[i]) == 0) clid = i;
    else error("ERROR in clientname");
  //check if there is any message for this client
    if( num_of_msg[clid] == 0 ){
      n = write((*clientPar).newsockfd,"There is no message for you",28);
      if (n < 0) error("ERROR writing to socket");      
    }else{
      bzero(buffer,256);
      for(i = 0; i < num_of_msg[clid]; i++){
        strcat(buffer,f);
        strcat(buffer, userlist[from[clid][i]] );
        strcat(buffer, "  ");
        strcat(buffer, msg[clid][i]); 
      }
      n = write((*clientPar).newsockfd,buffer,256);
      if (n < 0) error("ERROR writing to socket");      
      for( i = 0; i < num_of_msg[clid]; i++){
        bzero(msg[clid][i],256);
        from[clid][i] = 0;
      }
      num_of_msg[clid] = 0;
    }    
  //read message from client
    sleep(1);
    bzero(buffer,256);
    n = read((*clientPar).newsockfd,buffer,255);
    if (n < 0) error("ERROR reading from socket the msg");
//    printf("Here is the message: %s\n",buffer);
  //read the delivery address
    sleep(1);
    n = read((*clientPar).newsockfd,username,9);
    if (n < 0) error("ERROR reading from socket the address");
    username[n-1] = '\0';
//    printf("Here is the address: %s\n",username);
  //match username to delid
    i = 0;
    while( (strcmp(username, userlist[i]) != 0) && ( i != (N-1) ) )  i++;
    if (strcmp(username, userlist[i]) == 0) delid = i;
    else error("ERROR in username");
  //store message, from who, asc number of messages
    if(num_of_msg[delid] != MaxMes ){
      strcpy(msg[delid][num_of_msg[delid]],buffer);
      num_of_msg[delid]++;
      from[delid][num_of_msg[delid]] = clid;
//    printf("delid=%d\t msg=%s\n",delid,msg[delid][num_of_msg[delid]-1]);
//    printf("num=%d\t from=%s\n",num_of_msg[delid],userlist[clid]);
      n = write((*clientPar).newsockfd,"Server got your message",25);
    }else{ 
      n = write((*clientPar).newsockfd,"Overflow messages for this client",35);
    }
    if (n < 0) error("ERROR writing to socket");
//end of time counting
    end_t = clock();
    getrusage(RUSAGE_SELF, &buf3); //POINT C
//calc time duraction
    total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
    diff1=   ((double) buf3.ru_utime.tv_sec
            + (double) buf3.ru_utime.tv_usec / (double) CLOCKS_PER_SEC)-
            ((double) buf2.ru_utime.tv_sec
            + (double) buf2.ru_utime.tv_usec / (double) CLOCKS_PER_SEC);
    diff2=   ((double) buf3.ru_stime.tv_sec
            + (double) buf3.ru_stime.tv_usec / (double) CLOCKS_PER_SEC)-
            ((double) buf2.ru_stime.tv_sec
            + (double) buf2.ru_stime.tv_usec / (double) CLOCKS_PER_SEC);

    store_to_file();
    pthread_cancel(pthread_self());
    return(NULL);
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, clilen;
  struct sockaddr_in serv_addr, cli_addr;
  pthread_t thread_id;
  struct param cliPar;
  if (argc < 2) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0) 
    error("ERROR on binding");
  listen(sockfd,5);
//init file data
  FILE *fp;
  fp = fopen("timeResults.txt","w+");
  int x;
  x = fprintf(fp,"WALL TIME\t");
  x = fprintf(fp,"CPU TIME\t");
  x = fprintf(fp,"SYSTEM TIME\t");
  x = fprintf(fp,"TOTAL TIME\n");
  fclose(fp);
  while(1){
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
      error("ERROR on accept");
    cliPar.newsockfd = newsockfd;
    cliPar.cli_addr = cli_addr;
    pthread_create(&thread_id, NULL, (void *) &func, (void *) &cliPar);
  }

  return 0;
 
}

