#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <string>
#include <fcntl.h>

#include "message.h"
#include "server.h"


#define BUFLEN 356

using namespace std;

Server_State_T server_state;
string cmd_string[] = {"CMD_LS", "CMD_SEND","CMD_GET","CMD_REMOVE"};
int rot13 ( char *inbuf, char *outbuf ) ;

int main(int argc, char *argv[])
{
    unsigned short udp_port = 0;
        if ((argc != 1) && (argc != 3))
        {
                cout << "Usage: " << argv[0];
                cout << " -p <udp_port>" << endl;
                return 1;
        }
        else
        {
                system("clear");
                //process input arguments
                for (int i = 1; i < argc; i++)
                {                              
                        if (strcmp(argv[i], "-p") == 0)
                                udp_port = (unsigned short) atoi(argv[++i]);
                }
        }

        int     sk ;            // socket descriptor
        sockaddr_in remote ;         // socket address for remote
        sockaddr_in local ;          // socket address for us
        char        buf[BUFLEN] ;        // buffer from remote
        char        retbuf[BUFLEN] ;     // buffer to remote
        socklen_t     rlen = sizeof(remote) ; // length of remote address
        socklen_t     len = sizeof(local) ;    // length of local address
        int     moredata = 1 ;       // keep processing or quit
        int     mesglen ;       // actual length of message
        // create the socket
        sk = socket(AF_INET,SOCK_DGRAM,0) ;
        // set up the socket
        local.sin_family = AF_INET ;          // internet family
        local.sin_addr.s_addr = INADDR_ANY ; // wild card machine address

        if(udp_port == 0) local.sin_port = 0 ;                  // let system choose the port
        else local.sin_port = udp_port;





        // bind the name (address) to a port
        bind(sk,(struct sockaddr *)&local,sizeof(local)) ;
        // get the port name and print it out
        getsockname(sk,(struct sockaddr *)&local,&len) ;
//      cout << "socket has port " << local.sin_port << "\n" ;




        cout << "Command UDP port: " << local.sin_port << endl;

        string dir = string("./backup/");
        Cmd_T cmd;



    char filename[FILE_NAME_LEN];
        uint32_t size;
        uint16_t port;

    while(true)
    {
        usleep(100);
        CMD_MSG_tag command;
        switch(server_state)
        {
            case WAITING:
            {
                cout << "Waiting..." << endl;
                while (moredata){

                        mesglen = recvfrom(sk, (char *)&command, sizeof(command), 0, (struct sockaddr *)&remote, &rlen);

                        switch(command.cmd){
                                case CMD_LS:
                                {
                                        server_state = PROCESS_LS;
                                        break;
                                }
                                case CMD_SEND:
                                {
                                        server_state = PROCESS_SEND;
                                        break;
                                }

                                case CMD_GET:
                                {
                                        break;
                                }

                                case CMD_REMOVE:
                                {
                                        server_state = PROCESS_REMOVE;
                                        break;
								}
								case SHUTDOWN: 
								{
									exit(0);
								}
								default:
								{
									server_state = WAITING;
									break;
								}
                        }
                        break;
                }


                break;
            }
            case PROCESS_LS:
            {
                cout << "[CMD RECEIVED]: CMD_LS" << endl;
                vector<string> files = vector<string>();
                getdir(dir,files);
                size = files.size(); //size = 0 if empty

                CMD_MSG_tag responseMsg; //build the response message
                responseMsg.cmd = CMD_LS;
                responseMsg.size = size;

                sendto(sk, (char *)&responseMsg, sizeof(responseMsg), 0, (struct sockaddr *)&remote, sizeof(remote)); //send the command received and number of files back to the client

                if (size == 0) cout << " - server backup folder is empty" << endl;
                else{ //we have files, so send their names

                for(int i = 0; i < size; i++){ //loop through each file and send the file name
                        cout << " - " << files[i] << endl;

                        DATA_MSG_tag fileNames;

                        for (int k = 0; k < DATA_BUF_LEN; k++) fileNames.data[k] = '\0';
                        for (int j = 0; j < files[i].length(); j++) fileNames.data[j] = files[i][j]; //copy all characters from the file name to the data buffer

                        sendto(sk, (char *)&fileNames, sizeof(fileNames), 0, (struct sockaddr *)&remote, sizeof(remote)); //send the file name to the client

                }//end for

                }//end else



                server_state = WAITING;
                break;
            } //end PROCESS_LS

            case PROCESS_SEND:
            {
                cout << "[CMD RECEIVED]:  CMD_SEND" << endl;
                CMD_MSG_tag sendCmdResponse; //used to send port number to client

                //int theFileOpen;
				int theFileExists; //if the file exists or not
                string newFileLoc = "backup/";
                newFileLoc.append(command.filename);
                //theFileOpen = open(newFileLoc.c_str(), O_WRONLY);
				theFileExists = open(newFileLoc.c_str(), O_WRONLY);

				cout << " file already exists :"<< theFileExists << endl;					

                ofstream theFile(newFileLoc.c_str(), ios::out | ios::binary);

				if (theFileExists == 4){
                        cout << " file already exists " << endl;					
				}

                if(theFile == NULL){ //if theFile is NULL, the file could not be opened, so we send port 0 back to client.
                        sendCmdResponse.port = 0;
                        sendto(sk, (char *)&sendCmdResponse, sizeof(sendCmdResponse), 0, (struct sockaddr *)&remote, sizeof(remote));
                        cout << " - open file " << command.filename << " error." << endl;
                } //end if
 					///handles the fact that there is an existing connection
				else if (theFileExists > 0) {
                        cout << "Filename: " << command.filename << endl;
                        cout << "Filesize: " << command.size << endl;

                        //initialize needed variables and set up/bind TCP connection
                        int tcpSocket, tcpSocket2; //TCP socket descriptor
                        sockaddr_in tcpLocal;
                        sockaddr_in tcpRemote;
                        char buf[BUFLEN];
                        char retbuf[BUFLEN];
                        socklen_t tcpRlen = sizeof(tcpRemote);
                        socklen_t tcpLen = sizeof(tcpLocal);
                        tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
                        tcpLocal.sin_family = AF_INET;
                        tcpLocal.sin_addr.s_addr = INADDR_ANY;
                        tcpLocal.sin_port = 0;
                        bind(tcpSocket, (struct sockaddr *)&tcpLocal, sizeof(tcpLocal));
                        getsockname(tcpSocket, (struct sockaddr *)&tcpLocal, &tcpLen);


                        //send TCP port info to client
                        sendCmdResponse.cmd = CMD_SEND;
                        sendCmdResponse.port = tcpLocal.sin_port;
                        sendto(sk, (char *)&sendCmdResponse, sizeof(sendCmdResponse), 0, (struct sockaddr *)&remote, sizeof(remote));

                        //listen for incoming TCP connection
                        listen(tcpSocket, 1);
                        tcpSocket2 = accept(tcpSocket, (struct sockaddr *)0, 0);
                        //close(tcpSocket);

                        //file transfer is finished, so send the ack
                        //sendCmdResponse.cmd = CMD_ACK;
                        sendCmdResponse.error = 2;

                        sendto(sk, (char *)&sendCmdResponse, sizeof(sendCmdResponse), 0, (struct sockaddr *)&remote, sizeof(remote));
                        cout << " - send overwrite command" << endl;

						// while (accept(tcpSocket, (struct sockaddr *)0, 0)){

						// }


				}
                else{//if file is good, open the port and transfer the file
                        cout << "Filename: " << command.filename << endl;
                        cout << "Filesize: " << command.size << endl;

                        //initialize needed variables and set up/bind TCP connection
                        int tcpSocket, tcpSocket2; //TCP socket descriptor
                        sockaddr_in tcpLocal;
                        sockaddr_in tcpRemote;
                        char buf[BUFLEN];
                        char retbuf[BUFLEN];
                        socklen_t tcpRlen = sizeof(tcpRemote);
                        socklen_t tcpLen = sizeof(tcpLocal);
                        tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
                        tcpLocal.sin_family = AF_INET;
                        tcpLocal.sin_addr.s_addr = INADDR_ANY;
                        tcpLocal.sin_port = 0;
                        bind(tcpSocket, (struct sockaddr *)&tcpLocal, sizeof(tcpLocal));
                        getsockname(tcpSocket, (struct sockaddr *)&tcpLocal, &tcpLen);
                        cout << " - listen @: " << tcpLocal.sin_port << endl;

                        //send TCP port info to client
                        sendCmdResponse.cmd = CMD_SEND;
                        sendCmdResponse.port = tcpLocal.sin_port;
                        sendto(sk, (char *)&sendCmdResponse, sizeof(sendCmdResponse), 0, (struct sockaddr *)&remote, sizeof(remote));

                        //listen for incoming TCP connection
                        listen(tcpSocket, 1);
                        tcpSocket2 = accept(tcpSocket, (struct sockaddr *)0, 0);
                        close(tcpSocket);

                        if(tcpSocket2 == -1) cout << "accept failed!\n";
                        else{
                                cout << " - connected with client..." << endl;

                        DATA_MSG_tag fileData;

                        while(1){
                                int bytes_read = read(tcpSocket2, fileData.data, sizeof(fileData.data));
                                if (bytes_read == 0) break;    
								cout << " - total bytes read: " << bytes_read << endl;

                        theFile.write(fileData.data, sizeof(fileData.data));

                        while(bytes_read > 0){
                                int bytes_written = write(tcpSocket2, fileData.data, bytes_read);
                                bytes_read -= bytes_written;
                                }
                        }



                        }//send else

                        //file transfer is finished, so send the ack
                        sendCmdResponse.cmd = CMD_ACK;
                        sendCmdResponse.error = 0;

                        sendto(sk, (char *)&sendCmdResponse, sizeof(sendCmdResponse), 0, (struct sockaddr *)&remote, sizeof(remote));
                        cout << " - send acknowledgement" << endl;

                } //end else
                theFile.close();
                server_state = WAITING;
                break;
            } //end PROCESS_SEND

            case PROCESS_REMOVE:
            {
                cout << "[CMD RECEIVED]: CMD_REMOVE" << endl;

                CMD_MSG_tag responseMsg; //instantiate a response to client
                string fileLocation= "backup/";
                fileLocation.append(command.filename);
                int removed = remove(fileLocation.c_str()); //remove the file from the directory

                if(removed == 0){ //if remove was successful, set flags and send reply
                        cout << " - File: " << command.filename << " deleted." << endl;
                        responseMsg.cmd = CMD_ACK;
                        responseMsg.error = 0;

                        sendto(sk, (char *)&responseMsg, sizeof(responseMsg), 0, (struct sockaddr *)&remote, sizeof(remote));
                }
                else{ //else set the error flag and send reply
                        cout << " - file doesn't exist" << endl;
                        responseMsg.error = 1;

                        sendto(sk, (char *)&responseMsg, sizeof(responseMsg), 0, (struct sockaddr *)&remote, sizeof(remote));
                }


                        server_state = WAITING;
                break;
            }
        }
    }
    close(sk);
    return 0;
}

//this function is used to get all the filenames from the
//backup directory
int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << " - error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    int j=0;
    while ((dirp = readdir(dp)) != NULL) {
        //do not list the file "." and ".."
        if((string(dirp->d_name)!= ".") && (string(dirp->d_name)!=".."))
                files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

int rot13 ( char *inbuf, char *outbuf ) {
    int idx ;
    if( inbuf[0]=='.' ) return 0 ;
    idx=0 ;
    while( inbuf[idx]!='\0' ) {
        if( isalpha(inbuf[idx]) ) {
        if( (inbuf[idx]&31)<=13 )
             outbuf[idx] = inbuf[idx]+13 ;
        else
             outbuf[idx] = inbuf[idx]-13 ;
        } else
        outbuf[idx] = inbuf[idx] ;
        idx++ ;
    }
    outbuf[idx] = '\0';
    return 1 ;
}