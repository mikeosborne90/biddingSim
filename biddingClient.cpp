#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "itemList.h" //Structure for input.txt
#include <iostream>
#include <vector>
#include <time.h>
#include <sys/time.h>

#define PORT 3499 // the port client will be connecting to

#define MAXDATASIZE 256 // max number of bytes we can get at once

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in _addr; // connector's address information
    timeval t1;
    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);          //random number generated in microseconds not seconds
    std::string itemWithPrice = "";         //stores item name, and unitPrice

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    if ((he=gethostbyname(argv[1])) == NULL) {  // get the host info
        herror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    _addr.sin_family = AF_INET;    // host byte order
    _addr.sin_port = htons(PORT);  // short, network byte order
    _addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(_addr.sin_zero), '\0', 8);  // zero the rest of the struct

    if (connect(sockfd, (struct sockaddr *)&_addr,
                sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    if ((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    std::stringstream aLine;  //For the line
    aLine<<buf;               //Bytes of data from the buffer into the string stream object

    //printf("Received: \n%s",aLine.str().c_str()); // Printing what we have stored in the stringstream(commented out for testing)
    std::vector<itemList> biddingListClient;
    biddingListClient.clear();
    itemList tempElement;
    std::string temp;         //For the line to cut up and store into our Struct
    while(aLine>>temp){
        tempElement.itemName = temp;          //Store the first part of line, Description as name
        aLine>>temp;                          //Skip over tabs, store new data in Temp(The units now)
        tempElement.unitPrice = stoi(temp);   //Store the price of each unit, don't get next because that starts next line
        biddingListClient.push_back(tempElement);   //Store element into our vector of itemList
    }
    //This forloop shows how to iterate through the vector and access each member of the structure
    //Should make it easier to broadcast itemName, edit the number of units and price
    for(int i = 0; i < biddingListClient.size();i++){ //Print the vector to see that it saves right
        std::cout << biddingListClient.at(i).itemName << " " << biddingListClient.at(i).unitPrice << std::endl;
    }

    int randomItem = rand() % 10; //Need to change once item list gets reduced in size
    int notBidChance = rand() % 100;
    char sendBuffer[MAXDATASIZE];

    if(notBidChance >= 30)                                       //70% chance to bid
    {
        for (int i = 0; i < biddingListClient.size(); i++)
        {

            if (randomItem == i)                            //randomly add one item to list
            {                                               //increase unitPrice by $1
                itemWithPrice.append(
                        biddingListClient.at(i).itemName + " " +
                        std::to_string(biddingListClient.at(i).unitPrice + 1) +
                        "\n");
                std::cout<<"Bid on "<<biddingListClient.at(i).itemName<<" "<<biddingListClient.at(i).unitPrice + 1<<std::endl;
            }
        }
    }
    else {                                                      //30% chance to not bid
        std::cout << "Did not bid this round!" << std::endl;
    }

    for(int i = 0; i < itemWithPrice.length(); i++)
    {
        sendBuffer[i] = itemWithPrice[i];
    }
//Trying to send back to server
    if(numbytes == 0)
    {
        std::cout<<"ERROR: No items found on for client."<<std::endl;
        exit(1);
    }

    if (send(sockfd, sendBuffer, itemWithPrice.length(), 0) == -1)
        perror("send");

    close(sockfd);

    return 0;
}