//Bidding Server reads input.txt -> stores in biddingListServer<itemList> Vector
//Sends list to client
//Receives bid from client and updates vector
//Ends session after SESSION_TIMER in seconds or all items sold out
//Server resets prices to $1 and decrements # of units by 1
//Repeat until all items are gone.

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
#include "itemList.h"                                        //Structure for input.txt
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <csignal>                                           //Used for timer
#include <iomanip>

#define MYPORT 3499                                          //the port users will be connecting to

#define BACKLOG 10                                           //how many pending connections queue will hold

#define LENGTH 256

#define SESSION_TIMER 30                                     //in seconds

std::sig_atomic_t volatile done = 0;
void sessionOver(int) { done = 1; }                          //Timer Function

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(void)
{
    std::signal(SIGALRM, sessionOver);
    alarm(SESSION_TIMER);                                     //timer for session

    int sockfd, new_fd;                                       // listen on sock_fd, new connection on new_fd
    struct sockaddr_in my_addr;                               // my address information
    struct sockaddr_in their_addr;                            // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;

    std::vector<itemList> biddingListServer;                  //stores item name, #ofunits, unitPrice, and maxUnitPrice
    std::string itemWithPrice;                                //stores item name, and unitPrice

    //Read initial input.txt and save in itemList struct
    char const *fileName = "input.txt";
    char buffer[LENGTH];
    FILE *fileStream = fopen(fileName, "r");
    if (fileStream == NULL) {
        fprintf(stderr, "ERROR: File %s not found. (errno = %d)\n", fileName, errno);
        exit(1);
    }
    bzero(buffer, LENGTH);
    int fsBlockSize;

    itemWithPrice = "";

    bool skipRecv = false;

    while ((fsBlockSize = fread(buffer, sizeof(char), LENGTH, fileStream)) > 0) {
        std::stringstream aLine;                                //For the line
        aLine << buffer;                                        //Bytes of data from the buffer into the string stream object

        itemList tempElement;
        std::string temp;                                       //For the line to cut up and store into our Struct
        std::getline(aLine,temp);                               //Skip the first line, to ignore Description, Units, Price
        while (aLine >> temp) {
            tempElement.itemName = temp;                        //Store the first part of line, Description as name
            aLine >> temp;                                      //Skip over tabs, store new data in Temp(The units now)
            tempElement.numUnits = stoi(temp);                  //Store the second part of line, Units
            tempElement.unitPrice = 1;                          //Set initial unit price to $1 for each item
            aLine >> temp;                                      //Skip over tabs, store new data in temp(The price now)
            tempElement.maxUnitPrice = stoi(temp);              //Store the (MAX) price of each unit, don't get next because that starts next line
            biddingListServer.push_back(tempElement);           //Store element into our vector of itemList
        }
        //This forloop shows how to iterate through the vector and access each member of the structure
        //Should make it easier to broadcast itemName, edit the number of units and price
        for (int i = 0; i < biddingListServer.size(); i++) { //Print the vector to see that it saves right
            itemWithPrice.append(
                    biddingListServer.at(i).itemName + " " + std::to_string(biddingListServer.at(i).unitPrice)
                    + " " + std::to_string(biddingListServer.at(i).currentWinner) + "\n");
        }

        bzero(buffer, LENGTH);
    }

    //Establish connection with incoming client
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    my_addr.sin_family = AF_INET;                               //host byte order
    my_addr.sin_port = htons(MYPORT);                           //short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY;                       //automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8);                       //zero the rest of the struct

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))
        == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;                           //reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while(1) {                                                 // main accept() loop

            sin_size = sizeof(struct sockaddr_in);
            if ((new_fd = accept(sockfd, (struct sockaddr *) &their_addr,
                                 &sin_size)) == -1) {
                perror("accept");
                continue;
            }
            printf("server: got connection from %s\n",
                   inet_ntoa(their_addr.sin_addr));
            if (!fork()) {                                    //this is the child process

                char sendBuffer[LENGTH];

                int itemWithPriceLength = itemWithPrice.length();

                if (itemWithPriceLength == 0) {
                    std::cout << "No items available." << std::endl;
                    exit(1);
                }

                bzero(sendBuffer, LENGTH);

                close(sockfd);                               //child doesn't need the listener

                //converts string to character array
                for (int i = 0; i < itemWithPriceLength; i++) {
                    sendBuffer[i] = itemWithPrice[i];
                }

                if (send(new_fd, sendBuffer, itemWithPriceLength, 0) == -1)
                    perror("send");

                bzero(sendBuffer, itemWithPriceLength);

            close(new_fd);

            exit(0);
            }
            else                                            //I am a parent
            {
                skipRecv = false;

                bool allItemsGone = true;
                for (int i = 0; i < biddingListServer.size(); i++) {
                    if (biddingListServer.at(i).isAvailable) {
                        allItemsGone = false;
                    }
                }

                if (done == 1 || allItemsGone)             //represents end of bidding session
                {
                    //save biddingResults to text file
                    std::ofstream out("biddingResults.txt", std::ios::out);

                    if( !out )
                    {
                        std::cout << "Couldn't open file."  << std::endl;
                        return 1;
                    }

                    out << "Name" << std::setw(6) << "C1" << std::setw(5) << "C2" << std::setw(5) << "C3"  //text file
                              << std::setw(5) << "C4" << std::endl;
                    out << "-----------------------------" << std::endl;

                    std::cout << "Name" << std::setw(6) << "C1" << std::setw(5) << "C2" << std::setw(5) << "C3" //terminal
                              << std::setw(5) << "C4" << std::endl;
                    std::cout << "-----------------------------" << std::endl;

                    for (int i = 0; i < biddingListServer.size(); i++)  //reset prices to $1
                    {

                        if (biddingListServer.at(i).isAvailable) {
                            //decrement units by 1 at end of session
                            biddingListServer.at(i).numUnits -= 1;

                            //increase client wins by 1
                            if (biddingListServer.at(i).currentWinner == 1) {
                                biddingListServer.at(i).client1Wins += 1;
                            } else if (biddingListServer.at(i).currentWinner == 2) {
                                biddingListServer.at(i).client2Wins += 1;
                            } else if (biddingListServer.at(i).currentWinner == 3) {
                                biddingListServer.at(i).client3Wins += 1;
                            } else if (biddingListServer.at(i).currentWinner == 4) {
                                biddingListServer.at(i).client4Wins += 1;
                            }
                        }

                        if (biddingListServer.at(i).numUnits <= 0) {
                            biddingListServer.at(i).isAvailable = false;
                            biddingListServer.at(i).soldOut = true;
                        } else {
                            biddingListServer.at(i).isAvailable = true;
                            biddingListServer.at(i).unitPrice = 1;
                            biddingListServer.at(i).currentWinner = 0;
                            skipRecv = true;
                        }

                        if (biddingListServer.at(i).itemName == "Item10")     //for formatting item 10
                        {
                            out << biddingListServer.at(i).itemName << std::setw(4)
                                      << biddingListServer.at(i).client1Wins << std::setw(5)
                                      << biddingListServer.at(i).client2Wins << std::setw(5)
                                      << biddingListServer.at(i).client3Wins << std::setw(5)
                                      << biddingListServer.at(i).client4Wins << std::endl;
                            std::cout << biddingListServer.at(i).itemName << std::setw(4)
                                      << biddingListServer.at(i).client1Wins << std::setw(5)
                                      << biddingListServer.at(i).client2Wins << std::setw(5)
                                      << biddingListServer.at(i).client3Wins << std::setw(5)
                                      << biddingListServer.at(i).client4Wins << std::endl;
                        } else {
                            out << biddingListServer.at(i).itemName << std::setw(5)
                                      << biddingListServer.at(i).client1Wins << std::setw(5)
                                      << biddingListServer.at(i).client2Wins << std::setw(5)
                                      << biddingListServer.at(i).client3Wins << std::setw(5)
                                      << biddingListServer.at(i).client4Wins << std::endl;
                            std::cout << biddingListServer.at(i).itemName << std::setw(5)
                                      << biddingListServer.at(i).client1Wins << std::setw(5)
                                      << biddingListServer.at(i).client2Wins << std::setw(5)
                                      << biddingListServer.at(i).client3Wins << std::setw(5)
                                      << biddingListServer.at(i).client4Wins << std::endl;
                        }
                    }

                    out.close();

                    done = 0;
                    alarm(5);                                      //5 second buffer timer to show results in terminal

                    while (!done)
                    {
                        //buffer after session ends
                    }

                    done = 0;
                    alarm(SESSION_TIMER);                          //1 minute timer for session
                }
            }

                    if(skipRecv)                                   //special case right after starting new session
                    {
                        skipRecv = false;                          //prevents receiving old bids
                    }

                    else
                    {
                        int numbytes;
                        char buf[LENGTH];

                        if ((numbytes = recv(new_fd, buf, LENGTH - 1, 0)) == -1) {
                            perror("recv");
                            exit(1);
                        }

                        buf[numbytes] = '\0';

                        std::stringstream aLine;                   //For the line
                        aLine << buf;                              //Bytes of data from the buffer into the string stream object

                        std::string temp;                          //For the line to cut up and store into our Struct
                        while (aLine >> temp) {
                            for (int i = 0; i < biddingListServer.size(); i++) {
                                if (temp == biddingListServer.at(i).itemName) {
                                    aLine >> temp;
                                    biddingListServer.at(i).unitPrice = stoi(temp);
                                    aLine >> temp;
                                    biddingListServer.at(i).currentWinner = stoi(temp);
                                }
                            }
                        }
                    }

                itemWithPrice = "";

                std::cout << "Name" << std::setw(7) << "Units" << std::setw(6) << "Price" << std::setw(5) << "Max"
                          << std::setw(7) << "HiBid" << std::endl;
                std::cout << "-----------------------------" << std::endl;

                for (int i = 0; i < biddingListServer.size(); i++) { //Print the vector to see that it saves right

                    if (biddingListServer.at(i).soldOut == false)    //if item is sold out skip item
                    {

                        if (biddingListServer.at(i).unitPrice < biddingListServer.at(i).maxUnitPrice &&
                            biddingListServer.at(i).isAvailable == true) {

                            if (biddingListServer.at(i).itemName == "Item10")    //Just for formatting cout for item10
                            {
                                std::cout << biddingListServer.at(i).itemName << std::setw(4)
                                          << biddingListServer.at(i).numUnits << std::setw(5) <<
                                          biddingListServer.at(i).unitPrice << std::setw(7)
                                          << biddingListServer.at(i).maxUnitPrice << std::setw(5)
                                          << biddingListServer.at(i).currentWinner << std::endl;
                            } else {
                                std::cout << biddingListServer.at(i).itemName << std::setw(5)
                                          << biddingListServer.at(i).numUnits << std::setw(5) <<
                                          biddingListServer.at(i).unitPrice << std::setw(7)
                                          << biddingListServer.at(i).maxUnitPrice << std::setw(5)
                                          << biddingListServer.at(i).currentWinner << std::endl;
                            }

                            itemWithPrice.append(
                                    biddingListServer.at(i).itemName + " " +
                                    std::to_string(biddingListServer.at(i).unitPrice)
                                    + " " + std::to_string(biddingListServer.at(i).currentWinner) + "\n");

                        } else if (biddingListServer.at(i).unitPrice == biddingListServer.at(i).maxUnitPrice &&
                                   biddingListServer.at(i).isAvailable == true) {

                            biddingListServer.at(i).isAvailable = false;

                            //increase client wins by 1
                            if (biddingListServer.at(i).currentWinner == 1) {
                                biddingListServer.at(i).client1Wins += 1;
                            } else if (biddingListServer.at(i).currentWinner == 2) {
                                biddingListServer.at(i).client2Wins += 1;
                            } else if (biddingListServer.at(i).currentWinner == 3) {
                                biddingListServer.at(i).client3Wins += 1;
                            } else if (biddingListServer.at(i).currentWinner == 4) {
                                biddingListServer.at(i).client4Wins += 1;
                            }
                            biddingListServer.at(i).numUnits -= 1; //decrement item by 1

                            std::cout << biddingListServer.at(i).itemName << " Max Price Reached!, Winner: "
                                      << biddingListServer.at(i).currentWinner << std::endl;
                        } else {
                            std::cout << biddingListServer.at(i).itemName << " Max Price Reached!, Winner: "
                                      << biddingListServer.at(i).currentWinner << std::endl;
                        }
                    }
                }

            close(new_fd);                                                        //parent doesn't need this
    }

    return 0;                                                                     //never reached with infinite loop
}