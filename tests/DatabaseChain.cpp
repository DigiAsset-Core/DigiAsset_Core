//
// Created by mctrivia on 27/05/23.
//

#include <cmath>
#include "gtest/gtest.h"
#include "Database.h"

using namespace std;

TEST(DatabaseChain, Constructor) {
    //make sure test database does not exist
    remove("../tests/testFiles/_testNew.db");

    //create a new database

    Database* db = Database::GetInstance("../tests/testFiles/_testNew.db");
    EXPECT_TRUE(true);

    //todo check structure is as expected


}



TEST(DatabaseChain, getPermanentSize) {
    /*
    //make sure test database does not exist

    //open core
    DigiByteCore api = DigiByteCore();
    api.setFileName("config.cfg");
    api.makeConnection();


    //open test db
    Database* db = Database::GetInstance("../tests/testFiles/_testNew.db");
    db->setDigiByteCore(api);
    db->setBeenPrunedNonAssetUTXOHistory(true);
    //todo need to inject exchange rate data
    EXPECT_EQ(db->getPermanentSize("2498f4fde7b1684a6258ef84291dbe46594d7d03981989ae1c654af48c48e75c"), 0);
    unsigned int test = db->getPermanentSize("c57fc42847ebf7b3842fde56ed3ef1897d330413d3325e6b2043b78b5ed7f3fa");

    //todo check structure is as expected

*/
}


//todo
//create new table
//check it exists
//set a value
//destroy object
//open same table
//check value still persists

TEST(DatabaseChain, CleanUp) {
    remove("../tests/testFiles/_testNew.db");
}