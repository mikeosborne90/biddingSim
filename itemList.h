#include <string>

struct itemList{
      std::string itemName;
      int numUnits;
      int unitPrice;
      int maxUnitPrice;
      int currentWinner = 0; //1-4 for client#
      int client1Wins = 0;
      int client2Wins = 0;
      int client3Wins = 0;
      int client4Wins = 0;
      bool isAvailable = true;
      bool soldOut = false;
};
