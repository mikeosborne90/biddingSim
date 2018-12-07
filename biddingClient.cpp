/*
** client.c -- a stream socket client demo
*/

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

#define PORT 3499 // the port client will be connecting to

#define MAXDATASIZE 256 // max number of bytes we can get at once

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in _addr; // connector's address information

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
    std::vector<itemList> biddingList;
    itemList tempElement;
    std::string temp;         //For the line to cut up and store into our Struct
    std::getline(aLine,temp); // Skip the first line, to ignore Description, Units, Price
    while(aLine>>temp){
      tempElement.itemName = temp;          //Store the first part of line, Description as name
      aLine>>temp;                          //Skip over tabs, store new data in Temp(The units now)
      tempElement.numUnits = stoi(temp);    //Store the second part of line, Units
      aLine>>temp;                          //Skip over tabs, store new data in temp(The price now)
      tempElement.unitPrice = stoi(temp);   //Store the price of each unit, don't get next because that starts next line
      biddingList.push_back(tempElement);   //Store element into our vector of itemList
    }
    //This forloop shows how to iterate through the vector and access each member of the structure
    //Should make it easier to broadcast itemName, edit the number of units and price
    for(int i = 0; i < biddingList.size();i++){ //Print the vector to see that it saves right
      std::cout << biddingList.at(i).itemName << " " << biddingList.at(i).numUnits << " " << biddingList.at(i).unitPrice << std::endl;
    }
/*
    int myPrice[1000];
    int myUnits[1000];
    int j = 0;
    int counter1 = 0;
    int counter2 = 0;

    for(int i = 0; i <= numbytes; i++)
    {
        if(i != 0) //To prevent checking out of array bound at i = 0
        {
            if ((buf[i]=='0'||buf[i]=='1'||buf[i]=='2'||buf[i]=='3'||buf[i]=='4'||buf[i]=='5'||buf[i]=='6'||buf[i]=='7'||buf[i]=='8'||buf[i]=='9')&&buf[i-1]=='\t')
            {
                char value[10];
                int x = 0;
                int y = i;
                while (buf[y] != '\t')
                {
                    value[x] = buf[y];
                    x++;
                    y++;
                }

                if(j % 2 == 0)
                {
                    myUnits[counter1] = atoi(value);
                    counter1++;
                }
                else
                {
                    myPrice[counter2] = atoi(value);
                    counter2++;
                }

                for (int a = 0; a < 10; a++) //zero out value
                {
                    value[a] = 0;
                }
                j++;
            }
        }

    }

    for(int i = 0; i < (counter1); i++)
    {
        printf("%i,", myUnits[i]);
        printf(" %i", myPrice[i]);
        printf("\n");
    }
*/
    close(sockfd);

    return 0;
}
