// Yoad Shiran - 302978713


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>

#include "threadpool.h"
int getsizefile(char *path);
int check_get(char *buffer);
int cheak_p(char *path);
char *get_mime_type(char *name);
int sizeofmessage(int size, char*path, DIR *pDir);
int handle(void *newfd);
char *getFileCreationTime(char *path);
int ifhavepath(int flag_path, char *buffer);
int get_path(char *pch, int socket,char *timebuf,char *buf);
int check_port(char *port);
int check_firstline(char *buf, int len, int flag_path);
void respondserver(int newfd ,char * buf, char *firstheader, char *time_date, char *message, int flag302, char* timefile, char *type, char *path, char* filesize);
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		perror("Usage : server <port> <pool-size>\n");
		exit(1);
	}
	if (atoi(argv[1]) < 1 || atoi(argv[2]) < 1 || atoi(argv[3]) < 1)
	{
		perror("Usage : server <port> <pool-size>\n");
		exit(1);
	}
	///  -------------pool-size---------------
	int num_of_pool = atoi (argv[2]);
	threadpool *th = create_threadpool(num_of_pool);
	if (th == NULL)
	{
		exit(-1);
	}
	/// --------------------------------------
	struct sockaddr_in srv;
	int port;
	//int cli_len;
	//1 read requset from soket
	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("problem with create socket in the server");
		destroy_threadpool(th);
		exit(1);
	}
	 /* Initialize socket structure */
   	bzero((char *) &srv, sizeof(srv));
	port = check_port(argv[1]);
	//printf("the port is %d\n", port);

   /* Now bind the host address using bind() call.*/
	srv.sin_family = AF_INET;
	srv.sin_port = htons(port);
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
   if (bind(fd, (struct sockaddr *) &srv, sizeof(srv)) < 0) {
      perror("ERROR on binding");
      close(fd);
      destroy_threadpool(th);
      exit(1);
   }
   /* now listen */
   if (listen(fd,5) < 0)
   {
   	perror("problem is listen");
   	destroy_threadpool(th);
   	exit(1);
   }
   //cli_len = sizeof(cli);
   int *newfd;
   char typeof500[12];
   memset(typeof500,0,sizeof(typeof500));
   strcpy(typeof500,"text/html");
   int maxnumofreq;
   char timebuf[128];
   time_t now = 0;
	now = time(NULL);
	struct sigaction sa;
	memset(&sa,0,sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
   char serererror[256] = "<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H4>500 Internal Server Error</H4>\nSome server side error.\nBODY></HTML>";
   for (maxnumofreq=0; maxnumofreq < atoi(argv[3]); maxnumofreq++)
   {
	   newfd = (int*)malloc(sizeof(int));
	   if (newfd==NULL)
	   {
		   	respondserver(*newfd,NULL,"HTTP/1.0 500 Internal Server Error\r\n",timebuf,serererror,0,NULL,typeof500,NULL,NULL);
		   	perror("error with malloc-newfd!");
		   	exit(0);
	   }
	   *newfd = accept (fd, NULL, NULL);
	   if (newfd < 0)
	   {
	   		respondserver(*newfd,NULL,"HTTP/1.0 500 Internal Server Error\r\n",timebuf,serererror,0,NULL,typeof500,NULL,NULL);
		  	perror ("problem in accept");
		  	exit(1);
	   }
	   if (sigaction(SIGPIPE,&sa,0)==-1)
	   {
	   	perror("sigaction");
	   }
	   // int* ptonewfd = &newfd;
	   dispatch(th, (dispatch_fn)handle,(void*)newfd);
	}
	destroy_threadpool(th);
    close(fd);
    return 0;
}
int handle(void *newfd1)
{
	int *temp = (int*)newfd1;
	int newfd = *temp;
	free(temp);
	char buf[8192];
    memset(buf,0,sizeof(buf));
    int nbytes;
    bzero(buf,8192);
    int totalBytes=0;

	// int n;
  /* If connection is established then start communicating */
	while (1)
    {
	   	if((nbytes = read(newfd, buf, sizeof(buf))) < 0)
    		{
    			perror("read");
    			exit(1);
		}
		if(nbytes <= 0)
			return 1;
		totalBytes += strlen(buf);

		if(strstr(buf, "\r\n") != NULL)
			break;
    }


   // if (nbytes = read(newfd,buf,sizeof(buf)) < 0)
   // {
   // 	perror ("problem with read function in server");
   // 	exit(1);
   // }
   // ********* need to add more buffer with while if /r/n then break
   //printf("%s\n", buf);
    time_t now = 0;
	char timebuf[128];
	now = time(NULL);
	strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
	char *pch = NULL;
	pch = strtok (buf,"\r\n");
	char *temp_path = NULL;
	char *type = NULL;
	char badreq[256] = "\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD> \n<BODY><H4>400 Bad request</H4> \nBad Request.\n</BODY></HTML>";
	char Notsupported[256] = "\n<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD> \n<BODY><H4>501 Not supported</H4> \nMethod is not supported.\n</BODY></HTML>";
	char notfound[256] = "\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H4>404 Not Found</H4> \nFile not found.\n</BODY></HTML>";
	char found[256] = "\n<HTML><HEAD><TITLE>302 Found</TITLE></HEAD>\n<BODY><H4>302 Found</H4>\nDirectories must end with a slash.\n</BODY></HTML>";
	char Forbidden[256] = "\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H4>403 Forbidden</H4>\nAccess denied.\n</BODY></HTML>";
	int sizeofnewbuf = strlen(buf);
	char newbuf[sizeofnewbuf];
	char pathp[sizeofnewbuf];
	int flage=0;
	int flag302 = 0;
	int flag_path = 0;
	int sizeofbuf=0;
	int cheak501=0;
	memset(newbuf,0,sizeof(newbuf));
	memset(pathp,0,sizeof(pathp));
	if (pch == NULL) {
		flage=1;
		//respondserver(newfd,buf,"HTTP/1.0 400 Bad Request\r\n",timebuf,badreq,flag302,NULL,"text/html",NULL,NULL);
	}
	else{
		strcpy(newbuf,pch);
		strcpy(pathp,pch);


	flag_path = ifhavepath(flag_path, newbuf);
	// memset(newbuf,0,sizeof(newbuf));
	// strcpy(newbuf,pch);
	sizeofbuf = strlen(buf);
//	printf("from1 begin path is:%s\n",newbuf);
	// printf("buf is %s\n", buf);
	// printf("pch is %s\n", pch);
	// printf("newbuf is %s\n",newbuf );
	cheak501 = check_get(newbuf);
}
if (flage==1 || check_firstline(buf, sizeofbuf,flag_path) == 1)
	{
		//printf("IN Check check_firstline\n");
		respondserver(newfd,buf,"HTTP/1.0 400 Bad Request\r\n",timebuf,badreq,flag302,NULL,"text/html",NULL,NULL);
	}
	else if (cheak501 == 1)
	{

		respondserver(newfd, buf,"HTTP/1.0 501 Not supported\r\n",timebuf,Notsupported,flag302,NULL,"text/html",NULL,NULL);
	}
	else if(flag_path == 0)
	{
		respondserver(newfd, buf,"HTTP/1.0 404 Not Found\r\n",timebuf,notfound,flag302,NULL,"text/html",NULL,NULL);
	}
	else if (flag_path == 1)
	{
		int whatispath = 0;
		whatispath = get_path(pch, newfd,timebuf,buf);
		//printf("whatispath path %d\n",whatispath);
		// if(buf != NULL)
		// 	printf("from2 begin path is:%s\n",buf);/////////////////////////////////////////////////////////////////
		char *timeofile=NULL;
		char * timeoftype = NULL;
		char *newpath=NULL;
		temp_path = strstr(buf, " ");
		temp_path = strtok(temp_path, " ");
		newpath = strstr(pathp, " ");
		newpath = strtok(newpath, " ");
		// if(temp_path != NULL)
		// 	printf("from3 begin path is:%s\n",temp_path);///////////////////////////////////////////////////////////
		char* cwd = NULL;
	    char buff[5000 + 10];
	    memset(buff,'\0',sizeof(buff));
	    cwd = getcwd( buff, 5000+10 );
			//printf("CWD is :%s\n",cwd );
			printf("temp_path is %s\n", newpath);
	   	int status = cheak_p(pathp );
	    strcat(cwd,temp_path);
	    int input_fd;
	    input_fd = open (cwd, O_RDONLY);
	    if (input_fd == -1)
	    {
            //perror ("open");
            status=1;
	    }
	    // if( cwd != NULL ) {
	    //     printf( "My working directory is %s\n", cwd );
	    // }
	    if (whatispath == 0)
	    {
	    	//table
	    }
		else if (whatispath == 403)
		{
			respondserver(newfd, buf,"HTTP/1.0 404 Not Found\r\n",timebuf,notfound,flag302,NULL,"text/html",NULL,NULL);
		}
		else if (whatispath == 404)
		{
			struct stat statbuf;
			if(stat(cwd, &statbuf) == -1)
			{
		    	respondserver(newfd, buf,"HTTP/1.0 404 Not Found\r\n",timebuf,notfound,flag302,NULL,"text/html",NULL,NULL);
		    }
			else if (status == 1)
			{
				respondserver(newfd, buf,"HTTP/1.0 403 Forbidden\r\n",timebuf,Forbidden,flag302,NULL,"text/html",NULL,NULL);
			}
			else
			{
				timeofile =getFileCreationTime(cwd);
				timeoftype = get_mime_type(cwd);
				printf("\ncwd is %s\n", cwd);
				printf("\ntimeof type is %s\n", timeoftype);
				long int sizeindex = getsizefile(cwd);
				long int temp_sizeindex = sizeindex;
				int count=0;
				while(sizeindex!=0)
				{
					sizeindex = sizeindex/10;
					count++;
				}
				char sizeindex1[count];
				memset(sizeindex1,'\0',sizeof(sizeindex1));

				sprintf(sizeindex1,"%ld",temp_sizeindex);
				printf("size of file is sizeindex%ld\n", sizeindex);
				printf("size of file is sizeindex1%s\n",sizeindex1 );
			    respondserver(newfd, buf,"HTTP/1.0 200 OK\r\n",timebuf,NULL,flag302,timeofile,timeoftype,cwd,sizeindex1);
			}
		}
		else if (whatispath == 302)
		{
			flag302 = 1;
			type = get_mime_type(cwd);
			//printf("buf is %s\n", buf);
			if (strcmp(buf,"favicon.ico") == 0)
			{
				strtok(buf,"favicon.ico");
			}
			//printf(" whatispath --------- > buf is %s\n", buf);
			respondserver(newfd, buf,"HTTP/1.0 302 Found\r\n",timebuf,found,flag302,NULL,type,NULL,NULL);

		}
		else if (whatispath == 200 || whatispath == 201) // 200 is html and 201 is htm
		{
			if (whatispath == 200){strcat(cwd,"index.html");}
			else{strcat(cwd,"index.htm");}
			flag302 = 2; // is mean of found index.html
			timeofile =getFileCreationTime(cwd);
			timeoftype = get_mime_type(cwd);
			int sizeindex = getsizefile(cwd);
			char sizeindex1[sizeindex+1];
			memset(sizeindex1,0,sizeof(sizeindex1));
			sprintf(sizeindex1,"%d",sizeindex);
			respondserver(newfd, buf,"HTTP/1.0 200 OK\r\n",timebuf,NULL,flag302,timeofile,"text/html",cwd,sizeindex1);
		}
	}
	close(newfd);
	return 0;
}
int check_port(char *port)
{
	int i;
	int newport;
	for (i = 0; port[i] != '\0'; ++i)
	{
		if (port[i] <= 47) // ASCII 48 = 0
		{
			printf("DEBUG[1] number in ascii is %d\n", port[i]);
			exit(1);
		}
		else if(port[i] >= 58) // ASCII 57 = 9
		{
			printf("DEBUG[2] number in ascii is %d\n", port[i]);
			exit(1);
		}
	}
	newport = atoi(port);
	if (newport >= 65536 || newport <= 1)
	{
		perror("the number of port is not true");
		exit(1);
	}
	return newport;
}
int check_get(char *buffer)
{

	if (buffer[0] != 'G'){return 1;}
	if (buffer[1] != 'E'){return 1;}
	if (buffer[2] != 'T'){return 1;}
	return 0;
}
int ifhavepath(int flag_path, char *buffer)
{
	char *temp = strstr(buffer," ");
	temp = strtok(temp, " ");
//printf("path in if have path is %s\n", temp);
		if ((temp[0] != '/'))
		{
			flag_path = 1;
			//printf("ooooooooo\n");
			return 0;
		}
		else
		{
			int size = strlen(temp)+1;
			char path[size];
			memset(path,0,sizeof(path));
			char point[2];
			memset(point,0,sizeof(point));
			strcat(point,".");
			strcat(path,point);
			strcat(path,temp);
			struct stat attr;
			memset(&attr,'\0',sizeof(attr));
			stat(path, &attr);

			if(S_ISREG(attr.st_mode) || S_ISDIR(attr.st_mode))
			{
				//perror("have a file");
				return 1;
			}
			//printf("oasdasdadasdsaasdoooooooo\n");
			return 0;
		}

}
	//return 0;
int check_firstline(char *buffer, int len, int flag_path)
{
	//printf("JJJJJJJJJJJJJJJJJJJJJJJJJJ\n");
	//printf("1 the buf is %s\n", buffer);
	int i = 0;
	int count = 0;
	int j = 0;
	for (i = 0; i < len; ++i)
	{
		if ((buffer[i] == ' ')) // 8 is " " in ascii
		{
			if ((buffer[i+1] == '/'))
			{
				flag_path = 1;
			}
			count++;
			j = i;
		}
	}
	j++;
	if (buffer[j] != 'H'){return 1;}
	if (buffer[j+1] != 'T'){return 1;}
	if (buffer[j+2] != 'T'){return 1;}
	if (buffer[j+3] != 'P'){return 1;}
	if (buffer[j+4] != '/'){return 1;}
	if (buffer[j+5] != '1'){return 1;}
	if (buffer[j+6] != '.'){return 1;}
	if (buffer[j+7] == '1' || buffer[j+7] == '0')
		{
			if (count!=2)
			{
				return 1;
			}
			return 0;
		}
	return 1;
}
void respondserver(int newfd ,char *buf, char *firstheader, char *time_date, char *message, int flag302, char* timefile, char *type, char *path, char *filesize)
{
	printf("buf respondserver %s\n", buf);
	int size =0;
	char sizestring[5] = "";
	memset(sizestring,0,strlen(sizestring));
	char *path302 = NULL;
	size+= strlen(firstheader);
	size+= strlen("\nServer: webserver/1.0\r\n");
	strcat (time_date, "\r\n");
	size+= strlen("Date: ");
	size+= strlen(time_date);

	if (flag302 == 1)
	{
		printf("path302 is %s\n", path302 );
		printf("buf flag302 is %s\n", buf);
		size+= strlen("Location: ");
		path302 = strstr(buf, "/");
		printf("3 --- > path302 is %s\n", path302 );
		path302 = strtok(path302, " ");
		printf("2 -- > path302 is %s\n", path302 );
		size+= strlen(path302+3); // 1 is for / and more 2 for \n
	}
	size+= strlen("Content-Type: text/html\r\n\n");
	if (type!=NULL)
	{
		size+= strlen(type);
	}
	size+= strlen("Content-Length: 999\r\n");
	if (message!=NULL)
	{
		strcat (message, "\r\n");
		size+= strlen(message);
	}
	if (timefile!=NULL)
	{
		size+= strlen("Last-Modified: ");
		size+= strlen(timefile);
		size+= strlen("\n");
	}
	size+= strlen("Connection: close\r\n");
	char print[size];
	memset(print,0,size);
	strcat (print, firstheader);
	strcat (print, "Server: webserver/1.0\r\n");
	strcat (print, "Date: ");
	strcat (print, time_date);
	if (flag302 == 1)
	{
		strcat(print,"Location: ");
		strcat(print,path302);
		strcat(print, "/\n"); // add \ to respond path
	}
	if (type != NULL){
		strcat (print,"Content-Type: ");
		strcat (print,type);
		strcat (print,"\n");
	}

	if (message!=NULL)
	{
		strcat (print,"Content-Length: ");
		sprintf(sizestring,"%d",(int)strlen(message));
		strcat (print,sizestring);
		strcat (print,"\r\n");
	}
	if (filesize!=NULL)
	{
		strcat (print,"Content-Length: ");
		strcat(print,filesize);
		strcat (print,"\n");
	}

	if (timefile!=NULL)
	{
		strcat(print, "Last-Modified: ");
		strcat(print,timefile);
	}
	strcat (print,"Connection: close\n\n");
	if (message!=NULL)
	{
		strcat (print,message);
	}
	write (newfd,print,strlen(print));
	//printf("path is %s\n", path);
	if (path!=NULL)
	{

			char buffer[8192];
			//memset(buffer,0,sizeof(buffer));
			//ssize_t ret_in, ret_out;
			int input_fd;
		    input_fd = open (path, O_RDONLY);
		    if (input_fd == -1)
		    {
	            perror ("open");
		    }
		    //Copy process
		int nBytes=1;
    	//read(fp,buffer,sizeof(buffer));


    	while(1)
    	{
    		memset(buffer,0,sizeof(buffer));
    		nBytes =  read(input_fd,buffer,sizeof(buffer));
    		if(nBytes<0)
    		{
    			//perror("error with reading\n");
    			//printf("wait for another ask\n");
    			return;
    		}
    		//printf("nBytes is : %d\n",nBytes );
    		int temp = write(newfd,buffer,nBytes);
    		if(temp<0)
    		{
    			//perror("error with writing\n");
    			//printf("wait for another ask\n");
    			return;
    		}

    		if(nBytes==0)
    		{
    			break;
    		}



    	}
    	//close(fp);


		  //  close (input_fd);
	}
	strcat (print, "\r\n\r\n");
	//printf("-------HEADERS--------------\n%s",print);
	memset(sizestring,0,strlen(sizestring));
	path302 = NULL;
}
int get_path(char *pch, int socket, char *timebuf, char *buf)
{
	int flag302 =0;
	DIR *pDir_temp = NULL;
	struct dirent *pDirent = NULL;
	char foundindex[strlen(pch)+15]; // +15 becuase later i will add index.html and cheak if have a file
	memset(foundindex,'\0',sizeof(foundindex));
	char *lolo = NULL;
	//printf("lolo sapir pch is %s\n", pch);
	lolo = strstr(pch, " ");
	lolo = strtok(lolo, " ");
	//printf("lolo sapir here --- >%s \n", lolo);
	char temp_path[strlen(buf)];
	memset(temp_path,'\0',sizeof(temp_path));
	temp_path[0] = '.';
	strcat(temp_path,lolo);
	//printf("temp path is -- %s %d\n",temp_path, strlen(temp_path));

	int sizepath=0;

	if (temp_path!=NULL)
	{
		sizepath = strlen(temp_path);
	}
	struct stat attr;
	memset(&attr,'\0',sizeof(attr));
	char *timeofile = NULL;
	//char *timetype = NULL;
	if (get_mime_type(temp_path) != NULL)
	{
		//perror("have a file");
		return 404;
	}
    stat(temp_path, &attr);
	if(S_ISREG(attr.st_mode))
	{
		//perror("have a file");
		return 404;
	}
	snprintf(foundindex, sizeof(foundindex), "%sindex.html", temp_path);
	if (stat(foundindex, &attr) >= 0)
	{
		return 200;
	}
	memset(foundindex,'\0',sizeof(foundindex));
	snprintf(foundindex, sizeof(foundindex), "%sindex.htm", temp_path);
	if (stat(foundindex, &attr) >= 0)
	{
		return 201;
	}
	DIR *pDir = NULL;
	memset(&attr,0,sizeof(attr));
	stat(temp_path, &attr);
	if (S_ISDIR(attr.st_mode))
	{
		printf(" path: %s\n", temp_path);///////////////////////////////////////////////////////////////////
		if ((pDir = opendir(temp_path)) == NULL )
		{
			if (pDir == NULL)
			{
				perror("No have dircotory\n");
				return 404;
			}
		}

		if (temp_path[sizepath-1] != '/')
		{
			return 302;
		}
	}
	if (temp_path[sizepath-1] == '/')
	{
			memset(&attr,0,sizeof(attr));
			printf(" ***path: %s\n", temp_path);
   			int size = 0;
   			char sizeb[10];
   			int sizeofullpath = 0;
   			int sizeofbody = 0;
   			sizeofullpath+=strlen("<HTML>\n<HEAD><TITLE>Index of \n</TITLE></HEAD>\n<BODY>\n<H4>Index of </H4>\n");
   			sizeofullpath+=strlen("<table CELLSPACING=8>\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n");
   			sizeofullpath+=strlen(temp_path);
   			sizeofullpath+=strlen(temp_path);
			sizeofullpath+=strlen("\n</table>\r\n\n<HR>\r\n\n<ADDRESS>webserver/1.0</ADDRESS>\r\n\n</BODY>\r\n\n</HTML>\r\n");
			char *fullpath = NULL;
			fullpath = calloc(sizeofullpath,sizeof(char));
			if (fullpath== NULL)
			{
				perror("malloc in get_path(fullpath)");
			}
			memset(fullpath,0,sizeof(sizeofullpath));
			strcat(fullpath, "<HTML>\n");
			strcat(fullpath, "<HEAD><TITLE>Index of ");
			strcat(fullpath,temp_path); // add
			strcat(fullpath,"\n</TITLE></HEAD>\n");
			strcat(fullpath,"<BODY>\n");
			strcat(fullpath,"<H4>Index of ");
			strcat(fullpath,temp_path); // add
			strcat(fullpath, "</H4>\n");
			strcat(fullpath,"<table CELLSPACING=8>\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n");

			//memset(pDirent,0,sizeof(pDirent));
			pDir_temp = opendir(temp_path);
			while ((pDirent = readdir(pDir_temp)) != NULL)
			{
					sizeofbody+=strlen("<tr>\n<td><A HREF=\"");
					sizeofbody+=strlen("\"> </A></td><td>");
					sizeofbody+=strlen(ctime(&attr.st_mtime));
					sizeofbody+=strlen("</td> <td> </td>\n</tr>\n ");
					stat(pDirent->d_name, &attr);

					sizeofbody+=strlen(pDirent->d_name);
					sizeofbody+=strlen(pDirent->d_name);
					sizeofbody+=attr.st_size;
					fullpath = (char*)realloc(fullpath,sizeofbody*5);
		            strcat(fullpath, "<tr>\n<td><A HREF=\"");
		            strcat(fullpath,pDirent->d_name);
		          if (S_ISDIR(attr.st_mode))
		            	strcat(fullpath,"\"> ");
		          else  if (S_ISREG(attr.st_mode))
		            	strcat(fullpath,"\"> ");
							else
								strcat(fullpath,"\"> ");
		            strcat(fullpath,pDirent->d_name);
		            strcat(fullpath,"</A></td><td>");
		            strcat(fullpath,ctime(&attr.st_mtime));
		            size = (intptr_t)attr.st_size;
		            if (size>0 && S_ISREG(attr.st_mode))
		            {
		            	sprintf(sizeb,"%d",size);
		            	strcat(fullpath,"</td>");
		           		strcat(fullpath,"<td>");
		           		strcat(fullpath,sizeb);
		            }
		            strcat(fullpath,"</td>\n</tr>\n");
		    }
		    	closedir(pDir_temp);
		    	strcat(fullpath,"\n</table>\r\n");
		        strcat(fullpath,"\n<HR>\r\n" );
		        strcat(fullpath, "\n<ADDRESS>webserver/1.0</ADDRESS>\r\n");
		        strcat(fullpath,"\n</BODY>\r\n" );
		        strcat(fullpath, "\n</HTML>\r\n");
		    	timeofile = getFileCreationTime(temp_path);

	        	respondserver(socket, buf,"\nHTTP/1.0 200 OK\r\n",timebuf,fullpath,0,timeofile,"text/html",NULL,NULL);
			   	write(socket,fullpath, strlen(fullpath));
			  	memset(fullpath,0,sizeof(sizeofullpath));
			   	memset(&attr,'\0',sizeof(attr));
					memset(&pDirent,'\0',sizeof(pDirent));
					memset(temp_path,'\0',sizeof(temp_path));
					sizeofullpath = 0;
   				sizeofbody = 0;
					lolo = NULL;
					pDir = NULL;
					timeofile=NULL;
					sizepath=0;
					free(fullpath);
					closedir(pDir);
	}

	//else
	return 0;
}
char *get_mime_type(char *name)
{
	name++;
//	printf("name is %s",name);
	char *ext = strrchr(name, '.');
	if (!ext)
{
	//printf("alalalalalal\n");
		return NULL;
	}
	if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
	if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
	if (strcmp(ext, ".gif") == 0) return "image/gif";
	if (strcmp(ext, ".png") == 0) return "image/png";
	if (strcmp(ext, ".css") == 0) return "text/css";
	if (strcmp(ext, ".au") == 0) return "audio/basic";
	if (strcmp(ext, ".wav") == 0) return "audio/wav";
	if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
	if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
	if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
	if (strcmp(ext, ".txt") == 0) return "text/html";
//	printf("\n*****************\n");
	return NULL;
}
char *getFileCreationTime(char *path)
{
    struct stat attr;
    memset(&attr,0,sizeof(attr));
    if (stat(path, &attr)<0){perror("stat getFileCreationTime");}
    return ctime(&attr.st_mtime);
}
// int sizeofmessage(int size, char* path, DIR *pDir)
// {
// 	size+= strlen("<HTML>\n<HEAD><TITLE>Index of \n</TITLE></HEAD>\n<BODY>\n<H4>Index of </H4>\n<table CELLSPACING=8>\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n");
// 	size+= strlen(path);
// 	size+= strlen("<tr>\n<td><A HREF=\"\"> </A></td><td></td><td></td>\n</tr>\n");
// 	size+= strlen("\n</table>\r\n\n<HR>\r\n\n<ADDRESS>webserver/1.0</ADDRESS>\r\n\n</BODY>\r\n\n</HTML>\r\n");
// //	pDir = opendir(path);
// 	if (pDir == NULL)
// 	{
// 		printf("No have dircotory\n");
// 		return 404;
// 	}
// 	struct dirent *pDirent;
// 	memset(&pDirent,0,sizeof(pDirent));
// 	while ((pDirent = readdir(pDir)) != NULL)
// 	{
// 			size+= strlen(pDirent->d_name);
// 			size+=20; //10 for time and 10 for size
// 	}
// 	memset(&pDirent,0,sizeof(pDirent));
// 	return size;
// }
int cheak_p(char *path)
{
  char temp[4000];
  memset(temp, '\0', sizeof(temp));
  strcpy(temp, path);
  char temp1[4000];
  memset(temp1, '\0', sizeof(temp1));
  struct stat statop;
  int i;
  for(i = 1; i < strlen(temp); i++)
  {
    if( temp[i] == '/')
    {
      temp[i] = '\0';
      strcpy(temp1, temp);
      if(stat(temp1, &statop)>=0)
      {
        if(!(statop.st_mode & S_IROTH))
          return 1;
      }
      temp[i] = '/';
      memset(temp1, '\0', sizeof(temp1));
    }
  }
  return 0;
}
int getsizefile(char *path)
{
	struct stat st;
	stat(path, &st);
	int size = st.st_size;
	return size;

}
