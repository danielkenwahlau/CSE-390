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
#include <netdb.h>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>
 
#include "message.h"
#include "client.h"
 
#define BUFLEN 356
 
using namespace std;
 
int main(int argc, char *argv[])
{
    unsigned short udp_port = 0;
    const char* server_host = "127.0.0.1";
        if ((argc != 3) && (argc != 5))
        {
                cout << "Usage: " << argv[0];
                cout << " -s <server_host> -p <udp_port>" << endl;
                return 1;
        }
        else
        {
  	system("clear");
		for (int i = 1; i < argc; i++)
		{				
			if (strcmp(argv[i], "-p") == 0)
				udp_port = (unsigned short) atoi(argv[++i]);
			else if (strcmp(argv[i], "-s") == 0)
			{
				server_host = argv[++i];
				if (argc == 3)
				{
				    cout << "Usage: " << argv[0];
		            cout << " [-s <server_host>] -p <udp_port>" << endl;
		            return 1;
				}
		    }
	        else
	        {
	            cout << "Usage: " << argv[0];
		        cout << " [-s <server_host>] -p <udp_port>" << endl;
		        return 1;
	        }
		}
        }
               
       
 
        int         sk ;        // socket descriptor
        sockaddr_in     remote ;    // socket address for remote side
        char         buf[BUFLEN] ;    // buffer for response from remote
        hostent         *hp ;        // address of remote host
        int         mesglen ;    // actual length of the message
        socklen_t rlen = sizeof(remote);
        int moredata = 1;
 
 
        // create the socket
        sk = socket(AF_INET,SOCK_DGRAM,0) ;
        // designate the addressing family
        remote.sin_family = AF_INET ;
        // get the address of the remote host and store
        hp = gethostbyname(server_host) ;
        memcpy(&remote.sin_addr,hp->h_addr,hp->h_length) ;
        remote.sin_port = udp_port ;
       
        Client_State_T client_state = WAITING;
        string in_cmd;
        uint32_t size;
        uint16_t port;
       
        while(true)
        {
            usleep(100);
           
 
            switch(client_state)
            {
                case WAITING:
                {
                    cout<<"$ ";
                    cin>>in_cmd;
                   
                    if(in_cmd == "ls")
                    {
                        client_state = PROCESS_LS;
                    }
                    else if(in_cmd == "send")
                    {
                        client_state = PROCESS_SEND;
                    }
                    else if(in_cmd == "remove" )
                    {
                        client_state = PROCESS_REMOVE;
                    }            
					else if(in_cmd == "shutdown")
					{
						client_state = SHUTDOWN;
					}
					else if(in_cmd == "quit")
					{
						client_state = QUIT;
					}					
                    else
                    {
                        cout<<" - wrong command."<<endl;
                        client_state = WAITING;
                    }
                    break;
                }
                case PROCESS_LS:
                {                              
                        CMD_MSG_tag msgToServer;
                        msgToServer.cmd = CMD_LS;
 
                        CMD_MSG_tag servResponse;
                        int numFiles;
 
                        sendto(sk, (char*)&msgToServer, sizeof(msgToServer), 0, (struct sockaddr *)&remote, sizeof(remote));
 
 
 
                        mesglen = recvfrom(sk, (char *)&servResponse, sizeof(servResponse), 0, (struct sockaddr *)&remote, (socklen_t*) &rlen);
 
                        if(servResponse.cmd != CMD_LS) cout << " - command response error." << endl;
                        numFiles = servResponse.size;
                        if (numFiles == 0) cout << " - server backup folder empty" << endl;
                        else{
                                cout << "Number of files: " << numFiles<< endl;
                                for(int i = 0; i < numFiles; i++){ //loop through number of files and get the file names
                                        DATA_MSG_tag receivedFile;
                                        mesglen = recvfrom(sk, (char *)&receivedFile, sizeof(receivedFile), 0, (struct sockaddr *)&remote, (socklen_t*) &rlen);
                                       
                                        cout << " - " << receivedFile.data << endl;
 
                                } //end for    
                        } //end else
 
                        client_state = WAITING;
                        break;
                }
                case PROCESS_SEND:
                {
                        cin>>in_cmd; //get the second input - filename
 
                        CMD_MSG_tag sendCmd;
                        CMD_MSG_tag sendCmdReply;
 
                        int theFile;
                        theFile = open(in_cmd.c_str(), O_RDONLY);
 
 
                        sendCmd.cmd = CMD_SEND;
                        strcpy(sendCmd.filename, in_cmd.c_str());
 
                        struct stat fileStat;
                        stat(sendCmd.filename, &fileStat);
                        sendCmd.size = fileStat.st_size;
 
 
 
                        sendto(sk, (char *)&sendCmd, sizeof(sendCmd), 0, (struct sockaddr *)&remote, sizeof(remote));
 
 
                    //receive server reply
                        mesglen = recvfrom(sk, (char *)&sendCmdReply, sizeof(sendCmdReply), 0, (struct sockaddr *)&remote, (socklen_t*) &rlen);
 
                        if(sendCmdReply.cmd != CMD_SEND || sendCmdReply.port == 0) cout << " - error or incorrect response from server.";
                        else{ //we have the port, so establish TCP connection and send file.
 
                                cout << "TCP Port: " << sendCmdReply.port << endl;
 
                                int tcpSocket;
                                sockaddr_in tcpRemote;
                                hostent *tcpHp;
                                socklen_t tcpMsgLen;
 
                                tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
                                tcpRemote.sin_family = AF_INET;
                                tcpHp = gethostbyname(server_host);
                                memcpy(&tcpRemote.sin_addr, tcpHp->h_addr, tcpHp->h_length);
                                tcpRemote.sin_port = sendCmdReply.port;
 
                                DATA_MSG_tag fileData;
 
                                if(connect(tcpSocket, (struct sockaddr *)&tcpRemote, sizeof(tcpRemote)) < 0){
                                        cout << "connection error!\n";
                                        close(tcpSocket);
                                }
 
                                else{
                                        while(1){
                                                int bytes_read = read(theFile, fileData.data, sizeof(fileData.data));
                                                if(bytes_read == 0) break;
 
                                                write(tcpSocket, fileData.data, bytes_read);
                                        }
                               
                                close(tcpSocket);
                        } //end else
 
                        //receive ACK from server that file is sent or there's error
                        mesglen = recvfrom(sk, (char *)&sendCmdReply, sizeof(sendCmdReply), 0, (struct sockaddr *)&remote, (socklen_t*) &rlen);
 
                        if(sendCmdReply.cmd == CMD_ACK && sendCmdReply.error == 0) cout << "file transmission is completed." << endl;
						else if (sendCmdReply.error == 2){
							string response;
							cout << "file exists. overwrite? (y/n): ";
							cin >> response;
						} 
                        else cout << " - file transmission is failed" << endl;
                        client_state = WAITING;
                        close(theFile);
                        break;
                        }//end else
                } //end case
 
                case PROCESS_REMOVE:
                {
                    cin>>in_cmd; //get the second input - filename
                   
                    CMD_MSG_tag removeCmd;
                    CMD_MSG_tag removeCmdReply;
 
                    removeCmd.cmd = CMD_REMOVE;
                    strcpy(removeCmd.filename, in_cmd.c_str());
 
                    //send remove command
                    sendto(sk, (char *)&removeCmd, sizeof(removeCmd), 0, (struct sockaddr *)&remote, sizeof(remote));
 
                    //receive server reply
                    mesglen = recvfrom(sk, (char *)&removeCmdReply, sizeof(removeCmdReply), 0, (struct sockaddr *)&remote, (socklen_t*) &rlen);
 
                    if(removeCmdReply.error) cout << " - file doesn't exist" << endl;
                    else if (removeCmdReply.cmd == CMD_ACK && removeCmdReply.error == 0) cout << "File deleted." << endl;
                                //your code
                               
                    client_state = WAITING;
                    break;
                }
			case SHUTDOWN:
	        {	          
				CMD_MSG_tag msgToServer;
				msgToServer.cmd = SHUTDOWN;
				sendto(sk, (char*)&msgToServer, sizeof(msgToServer), 0, (struct sockaddr *)&remote, sizeof(remote));
				client_state = WAITING; 
	            break;	            
	        }
	        case QUIT:
	        {
					exit(0);
	        }
	        default:
	        {
	        	client_state = WAITING;
	            break;
	        }    				
            }
        }
    close(sk);
    return 0;
}