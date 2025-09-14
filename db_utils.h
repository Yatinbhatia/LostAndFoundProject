#ifndef DB_UTILS_H
#define DB_UTILS_H

#include "sqlite3.h"
#include <string>

extern sqlite3 *db;

void initializeDB();
void executeSQL(const std::string &query);
void insertItem(const std::string &type);
void viewMatches();
void confirmMatch();
void searchMatchesByName();
void searchMatchesByKeyword();
void deleteOldEntries();

#endif