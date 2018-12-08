#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "itemList.h" //Structure for input.txt
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

#define MYPORT 3499    // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define LENGTH 256

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_in my_addr;    // my address information
    struct sockaddr_in their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    std::vector<itemList> biddingListServer; //stores item name, #ofunits, unitPrice, and maxUnitPrice
    std::string itemWithPrice;         //stores item name, and unitPrice

    //Establish connection with incoming client
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(MYPORT);     // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))
        == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while(1) {  // main accept() loop

        //Read initial input.txt and save in itemList struct
        char const *fileName = "biddingList.txt";
        char buffer[LENGTH];
        FILE *fileStream = fopen(fileName, "r");
        if(fileStream == NULL)
        {
            fprintf(stderr, "ERROR: File %s not found. (errno = %d)\n", fileName, errno);
            exit(1);
        }
        bzero(buffer, LENGTH);
        int fsBlockSize;

        biddingListServer.clear();
        itemWithPrice = "";

        while((fsBlockSize = fread(buffer, sizeof(char), LENGTH, fileStream))>0)
        {
            std::stringstream aLine;  //For the line
            aLine<<buffer;               //Bytes of data from the buffer into the string stream object

            //printf("Received: \n%s",aLine.str().c_str()); // Printing what we have stored in the stringstream(commented out for testing)
            itemList tempElement;
            std::string temp;         //For the line to cut up and store into our Struct
            //std::getline(aLine,temp); // Skip the first line, to ignore Description, Units, Price
            while(aLine>>temp){
                tempElement.itemName = temp;          //Store the first part of line, Description as name
                aLine>>temp;                          //Skip over tabs, store new data in Temp(The units now)
                tempElement.numUnits = stoi(temp);    //Store the second part of line, Units
                aLine>>temp;
                tempElement.unitPrice = stoi(temp);            //Set initial unit price to $1 for each item
                aLine>>temp;                          //Skip over tabs, store new data in temp(The price now)
                tempElement.maxUnitPrice = stoi(temp);   //Store the (MAX) price of each unit, don't get next because that starts next line
                biddingListServer.push_back(tempElement);   //Store element into our vector of itemList
            }
            //This forloop shows how to iterate through the vector and access each member of the structure
            //Should make it easier to broadcast itemName, edit the number of units and price
            for(int i = 0; i < biddingListServer.size();i++){ //Print the vector to see that it saves right
                //std::cout << biddingListServer.at(i).itemName << " " << biddingListServer.at(i).numUnits << " " << biddingListServer.at(i).unitPrice<< " " << biddingListServer.at(i).maxUnitPrice << std::endl;
                itemWithPrice.append(biddingListServer.at(i).itemName + " " + std::to_string(biddingListServer.at(i).unitPrice) + "\n");
            }
//for testing
//        for(int i = 0; i < itemWithPrice.length(); i++)
//        {
//            std::cout<<itemWithPrice[i];
//        }

            bzero(buffer, LENGTH);
        }

        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
                             &sin_size)) == -1) {
            perror("accept");
            continue;
        }
        printf("server: got connection from %s\n",
               inet_ntoa(their_addr.sin_addr));
        if (!fork()) { // this is the child process
            char sendBuffer[LENGTH];

            int itemWithPriceLength = itemWithPrice.length();

            if(itemWithPriceLength == 0)
            {
                std::cout<<"ERROR: No items found on server."<<std::endl;
                exit(1);
            }
            bzero(sendBuffer, LENGTH);

            close(sockfd); // child doesn't need the listener


            for(int i = 0; i < itemWithPriceLength; i++)
            {
                sendBuffer[i] = itemWithPrice[i];
            }

            if (send(new_fd, sendBuffer, itemWithPriceLength, 0) == -1)
                perror("send");
            bzero(sendBuffer, itemWithPriceLength);

            int numbytes;
            char buf[LENGTH];

            if ((numbytes=recv(new_fd, buf, LENGTH-1, 0)) == -1) {
                perror("recv");
                exit(1);
            }

            buf[numbytes] = '\0';

            std::stringstream aLine;  //For the line
            aLine<<buf;               //Bytes of data from the buffer into the string stream object

            std::string temp;         //For the line to cut up and store into our Struct
            while(aLine>>temp){
                for(int i = 0; i < biddingListServer.size();i++)
                {
                    if(temp == biddingListServer.at(i).itemName)
                    {
                        aLine>>temp;
                        biddingListServer.at(i).unitPrice = stoi(temp);
                    }
                }
            }

//            for(int i = 0; i < numbytes; i++)
//            {
//                std::cout<<buf[i];
//            }
            std::ofstream out("biddingList.txt", std::ios::out);

            if( !out )
            {
                std::cout << "Couldn't open file."  << std::endl;
                return 1;
            }

            for(int i = 0; i < biddingListServer.size();i++){ //Print the vector to see that it saves right
                out << biddingListServer.at(i).itemName << " " << biddingListServer.at(i).numUnits << " " << biddingListServer.at(i).unitPrice<< " " << biddingListServer.at(i).maxUnitPrice << std::endl;
                std::cout << biddingListServer.at(i).itemName << " " << biddingListServer.at(i).numUnits << " " << biddingListServer.at(i).unitPrice<< " " << biddingListServer.at(i).maxUnitPrice << std::endl;
                itemWithPrice.append(biddingListServer.at(i).itemName + " " + std::to_string(biddingListServer.at(i).unitPrice) + "\n");
            }

            out.close();

            close(new_fd);
            exit(0);
        }

        close(new_fd);  // parent doesn't need this
    }

    return 0;
}