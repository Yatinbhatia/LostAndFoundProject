#include "db_utils.h"
#include "match_logic.h"
#include <iostream>
#include <sstream>
#include <algorithm> 
#include <map>

const std::string ADMIN_PASSWORD = "admin123";

std::string toLower(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

sqlite3* db;

void executeSQL(const std::string& query) {
    char* errMsg;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void initializeDB() {
    int rc = sqlite3_open("lost_found.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        exit(1);
    }

    std::string schema = R"(
         CREATE TABLE IF NOT EXISTS items (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        type TEXT NOT NULL CHECK (type IN ('lost', 'found')),
        user_name TEXT NOT NULL,
        description TEXT NOT NULL,
        category TEXT NOT NULL,
        location TEXT NOT NULL,
        date_reported TEXT NOT NULL,
        contact_info TEXT,
        current_location TEXT,
        is_resolved INTEGER DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS matches (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        lost_item_id INTEGER,
        found_item_id INTEGER,
        match_score INTEGER,
        status TEXT DEFAULT 'pending',
        FOREIGN KEY(lost_item_id) REFERENCES items(id),
        FOREIGN KEY(found_item_id) REFERENCES items(id)
        );
    )";
    executeSQL(schema);
}




void insertItem(const std::string& type) {
    std::string name, desc, category, location, date;
    std::cin.ignore();
    std::cout << "Your name: "; std::getline(std::cin, name);
    std::cout << "Item description: "; std::getline(std::cin, desc);
    std::cout << "Category: "; std::getline(std::cin, category);
    std::cout << "Location: "; std::getline(std::cin, location);
    category = toLower(category);
    location = toLower(location);

    std::cout << "Date reported:(YYYY-MM-DD) "; std::getline(std::cin, date);

    std::stringstream query;

    if (type == "found") {
        std::string contact, currentLoc;
        std::cout << "Your contact info (phone/email): "; std::getline(std::cin, contact);
        std::cout << "Your current location (where the item can be collected): "; std::getline(std::cin, currentLoc);

        query << "INSERT INTO items (type, user_name, description, category, location, date_reported, contact_info, current_location) VALUES ("
              << "'" << type << "', '" << name << "', '" << desc << "', '" << category << "', '" << location << "', '" << date
              << "', '" << contact << "', '" << currentLoc << "');";
        executeSQL(query.str());

        // Case-insensitive match with lost items
        std::string fetchLost = "SELECT id, description FROM items WHERE type='lost' "
                        "AND LOWER(category) = LOWER('" + category + "') "
                        "AND LOWER(location) = LOWER('" + location + "') "
                        "AND is_resolved = 0;";

        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, fetchLost.c_str(), -1, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int lost_id = sqlite3_column_int(stmt, 0);
            std::string lost_desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int score = calculateMatchScore(desc, lost_desc);
            if (score > 0) {
                std::stringstream matchQuery;
                matchQuery << "INSERT INTO matches (lost_item_id, found_item_id, match_score) VALUES ("
                           << lost_id << ", (SELECT MAX(id) FROM items), " << score << ");";
                executeSQL(matchQuery.str());
            }
        }
        sqlite3_finalize(stmt);
    } else {
        // Insert lost item (no contact/current location needed)
        query << "INSERT INTO items (type, user_name, description, category, location, date_reported) VALUES ("
              << "'" << type << "', '" << name << "', '" << desc << "', '" << category << "', '" << location << "', '" << date << "');";
        executeSQL(query.str());
    }

    std::cout << "Item added successfully!\n";
}



//-----------------------for debugging purpose--------------------


// void viewMatches() {
//     std::string password;
//     std::cout << "🔐 Enter admin password to confirm match: ";
//     std::cin >> password;

//     if (password != "admin123") {
//         std::cout << "❌ Access denied. Invalid password.\n";
//         return;
//     }
//     std::string query = R"(
//         SELECT m.id, l.description, f.description, m.match_score,
//                f.user_name, f.contact_info, f.current_location
//         FROM matches m
//         JOIN items l ON m.lost_item_id = l.id
//         JOIN items f ON m.found_item_id = f.id
//         WHERE m.status='pending';
//     )";

//     sqlite3_stmt* stmt;
//     sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

//     std::cout << "\nPossible Matches:\n";
//     bool anyMatch = false;

//     while (sqlite3_step(stmt) == SQLITE_ROW) {
//         anyMatch = true;
//         int id = sqlite3_column_int(stmt, 0);
//         std::string lostDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
//         std::string foundDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
//         int score = sqlite3_column_int(stmt, 3);
//         std::string finderName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
//         std::string contact = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
//         std::string currentLoc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
//         std::string confidence;
//         if (score >= 15) confidence = "🔒 Very Likely Match";
//         else if (score >= 10) confidence = "✅ Likely Match";
//         else if (score >= 5) confidence = "⚠️ Possible Match";
//         else confidence = "❌ Low Confidence Match";


//         std::cout << "✅ Match found for the item!\n"
//                   << "Match ID: " << id
//                   << "\nLost: " << lostDesc
//                   << "\nFound: " << foundDesc
//                   << "\nConfidence: " << confidence
//                   << "\nFound By: " << finderName
//                   << "\nContact: " << contact
//                   << "\nCurrent Location: " << currentLoc
//                   << "\n---\n";
//     }

//     if (!anyMatch) {
//         std::cout << "No matches found.\n";
//     }

//     sqlite3_finalize(stmt);
// }

void viewMatches() {
    std::string password;
    std::cout << "🔐 Enter admin password to view matches: ";
    std::cin >> password;

    if (password != "admin123") {
        std::cout << "❌ Access denied. Invalid password.\n";
        return;
    }

    std::string query = R"(
        SELECT m.id, l.description, l.user_name, f.description, m.match_score,
               f.user_name, f.contact_info, f.current_location
        FROM matches m
        JOIN items l ON m.lost_item_id = l.id
        JOIN items f ON m.found_item_id = f.id
        WHERE m.status='pending';
    )";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

    std::cout << "\nPossible Matches:\n";
    bool anyMatch = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        anyMatch = true;
        int id = sqlite3_column_int(stmt, 0);
        std::string lostDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string lostBy   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string foundDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        int score = sqlite3_column_int(stmt, 4);
        std::string finderName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string contact = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        std::string currentLoc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

        std::string confidence;
        if (score >= 15) confidence = "🔒 Very Likely Match";
        else if (score >= 10) confidence = "✅ Likely Match";
        else if (score >= 5)  confidence = "⚠️ Possible Match";
        else                  confidence = "❌ Low Confidence Match";

        std::cout << "✅ Match found for the item!\n"
                  << "Match ID: " << id
                  << "\nLost: " << lostDesc
                  << "\nLost By: " << lostBy
                  << "\nFound: " << foundDesc
                  << "\nConfidence: " << confidence
                  << "\nFound By: " << finderName
                  << "\nContact: " << contact
                  << "\nCurrent Location: " << currentLoc
                  << "\n---\n";
    }

    if (!anyMatch) {
        std::cout << "No matches found.\n";
    }

    sqlite3_finalize(stmt);
}


//--------------------for debugging purpose------------------

// void viewMatches() {
//     std::string query = R"(
//         SELECT m.id, m.lost_item_id, m.found_item_id, m.match_score,
//                l.description, l.user_name,
//                f.description, f.user_name, f.contact_info, f.current_location
//         FROM matches m
//         JOIN items l ON m.lost_item_id = l.id
//         JOIN items f ON m.found_item_id = f.id
//         WHERE m.status='pending';
//     )";

//     sqlite3_stmt* stmt;
//     sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

//     std::map<int, int> maxScorePerLost;
//     std::vector<std::tuple<int, int, int, int, std::string, std::string, std::string, std::string, std::string, std::string>> allMatches;

//     // Gather all matches and find max scores per lost item
//     while (sqlite3_step(stmt) == SQLITE_ROW) {
//         int matchID = sqlite3_column_int(stmt, 0);
//         int lostID = sqlite3_column_int(stmt, 1);
//         int foundID = sqlite3_column_int(stmt, 2);
//         int score = sqlite3_column_int(stmt, 3);

//         std::string lostDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
//         std::string lostBy = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
//         std::string foundDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
//         std::string foundBy = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
//         std::string contact = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
//         std::string currentLoc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));

//         allMatches.emplace_back(matchID, lostID, foundID, score, lostDesc, lostBy, foundDesc, foundBy, contact, currentLoc);

//         if (maxScorePerLost.find(lostID) == maxScorePerLost.end() || maxScorePerLost[lostID] < score) {
//             maxScorePerLost[lostID] = score;
//         }
//     }
//     sqlite3_finalize(stmt);

//     std::cout << "\nPossible Matches:\n";
//     bool anyMatch = false;

//     for (const auto& [matchID, lostID, foundID, score, lostDesc, lostBy, foundDesc, foundBy, contact, currentLoc] : allMatches) {
//         if (score == maxScorePerLost[lostID]) {
//             anyMatch = true;
//             std::string confidence;
//             if (score >= 15) confidence = "🔒 Very Likely Match";
//             else if (score >= 10) confidence = "✅ Likely Match";
//             else if (score >= 5) confidence = "⚠️ Possible Match";
//             else confidence = "❌ Low Confidence Match";

//             std::cout << "✅ Match found for the item!\n"
//                       << "Lost: " << lostDesc
//                       << "\nLost By: " << lostBy
//                       << "\nFound: " << foundDesc
//                       << "\nFound By: " << foundBy
//                       << "\nConfidence: " << confidence
//                       << "\nContact: " << contact
//                       << "\nCurrent Location: " << currentLoc
//                       << "\n---\n";
//         }
//     }

//     if (!anyMatch) {
//         std::cout << "No matches found.\n";
//     }
// }



// void confirmMatch() {
//     std::string password;
//     std::cout << "🔐 Enter admin password to confirm match: ";
//     std::cin >> password;

//     if (password != "admin123") {
//         std::cout << "❌ Access denied. Invalid password.\n";
//         return;
//     }

    
//     int matchID;
//     std::cout << "Enter Match ID to confirm: ";
//     std::cin >> matchID;

//     int lostID = -1, foundID = -1;
//     std::string fetchQuery = "SELECT lost_item_id, found_item_id FROM matches WHERE id=" + std::to_string(matchID) + ";";

//     sqlite3_stmt* stmt;
//     if (sqlite3_prepare_v2(db, fetchQuery.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
//         if (sqlite3_step(stmt) == SQLITE_ROW) {
//             lostID = sqlite3_column_int(stmt, 0);
//             foundID = sqlite3_column_int(stmt, 1);
//         }
//     }
//     sqlite3_finalize(stmt);

//     if (lostID == -1 || foundID == -1) {
//         std::cerr << "❌ Invalid Match ID or match not found.\n";
//         return;
//     }
//     std::cout << "✅ Match confirmed! The lost item has been returned to its owner.\n";


//     // Confirm deletion
//     std::string confirm;
//     std::cout << "Are you sure you want to delete both items and match? (yes/no): ";
//     std::cin >> confirm;
//     if (confirm != "yes") {
//         std::cout << "❌ Operation cancelled.\n";
//         return;
//     }

//     // Optional: show what will be deleted
//     std::cout << "🗑 Deleting:\n";
//     std::cout << "- Lost Item ID: " << lostID << "\n";
//     std::cout << "- Found Item ID: " << foundID << "\n";
//     std::cout << "- Match ID: " << matchID << "\n";

//     // Delete items
//     std::string deleteItems = "DELETE FROM items WHERE id = " + std::to_string(lostID) +
//                               " OR id = " + std::to_string(foundID) + ";";
//     executeSQL(deleteItems);

//     // Delete match
//     std::string deleteMatch = "DELETE FROM matches WHERE id = " + std::to_string(matchID) + ";";
//     executeSQL(deleteMatch);

//     std::cout << "🎉 Thank you! The match has been resolved and both entries have been cleared.\n";

// }

void confirmMatch() {
    int matchID;
    std::cout << "Enter Match ID to confirm and delete: ";
    std::cin >> matchID;

    int lostID = -1, foundID = -1;
    std::string fetchQuery = "SELECT lost_item_id, found_item_id FROM matches WHERE id=" + std::to_string(matchID) + ";";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, fetchQuery.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            lostID = sqlite3_column_int(stmt, 0);
            foundID = sqlite3_column_int(stmt, 1);
        }
    }
    sqlite3_finalize(stmt);

    if (lostID == -1 || foundID == -1) {
        std::cerr << "❌ Invalid Match ID or match not found.\n";
        return;
    }

    // 🔐 Ask user to verify with last 4 letters of their name
    std::string storedName, nameCheck;
    std::string getNameQuery = "SELECT user_name FROM items WHERE id=" + std::to_string(lostID) + ";";
    if (sqlite3_prepare_v2(db, getNameQuery.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            storedName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        }
    }
    sqlite3_finalize(stmt);

    std::cout << "🔐 To confirm, enter the last 4 letters of your name: ";
    std::cin >> nameCheck;

    // Convert both to lowercase for case-insensitive comparison
    std::transform(nameCheck.begin(), nameCheck.end(), nameCheck.begin(), ::tolower);
    std::transform(storedName.begin(), storedName.end(), storedName.begin(), ::tolower);

    if (storedName.length() < 4 || storedName.substr(storedName.length() - 4) != nameCheck) {
        std::cout << "❌ Verification failed. You are not authorized to delete this match.\n";
        return;
    }

    std::cout << "✅ Match confirmed! The lost item has been returned to its owner.\n";

    std::string confirm;
    std::cout << "Are you sure you want to delete both items and match? (yes/no): ";
    std::cin >> confirm;
    if (confirm != "yes") {
        std::cout << "❌ Operation cancelled.\n";
        return;
    }

    std::cout << "🗑 Deleting:\n";
    std::cout << "- Lost Item ID: " << lostID << "\n";
    std::cout << "- Found Item ID: " << foundID << "\n";
    std::cout << "- Match ID: " << matchID << "\n";

    // Delete lost & found items
    std::string deleteItems = "DELETE FROM items WHERE id = " + std::to_string(lostID) +
                              " OR id = " + std::to_string(foundID) + ";";
    executeSQL(deleteItems);

    // Delete the match
    std::string deleteMatch = "DELETE FROM matches WHERE id = " + std::to_string(matchID) + ";";
    executeSQL(deleteMatch);

    std::cout << "🎉 Thank you! The match has been resolved and both entries have been cleared.\n";
}


void searchMatchesByName() {
    std::string name;
    std::cin.ignore();
    std::cout << "Enter your name to find your matches: ";
    std::getline(std::cin, name);

    std::string query = R"(
        SELECT m.id, l.description, f.description, m.match_score,
               l.user_name, f.user_name, f.contact_info, f.current_location
        FROM matches m
        JOIN items l ON m.lost_item_id = l.id
        JOIN items f ON m.found_item_id = f.id
        WHERE m.status='pending' AND 
              (LOWER(l.user_name) = LOWER(')" + name + R"(') 
               OR LOWER(f.user_name) = LOWER(')" + name + R"('));
    )";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    bool anyMatch = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        anyMatch = true;
        int id = sqlite3_column_int(stmt, 0);
        std::string lostDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string foundDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        int score = sqlite3_column_int(stmt, 3);
        std::string lostBy = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string foundBy = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string contact = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        std::string location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

        std::string confidence;
        if (score >= 15) confidence = "🔒 Very Likely Match";
        else if (score >= 10) confidence = "✅ Likely Match";
        else if (score >= 5) confidence = "⚠️ Possible Match";
        else confidence = "❌ Low Confidence Match";

        std::cout << "\n✅ Match found for the item!"
                  << "\nMatch ID: " << id
                  << "\nLost: " << lostDesc
                  << "\nFound: " << foundDesc
                  << "\nConfidence: " << confidence
                  << "\nLost By: " << lostBy
                  << "\nFound By: " << foundBy
                  << "\nContact: " << contact
                  << "\nCurrent Location: " << location
                  << "\n---\n";
    }

    if (!anyMatch) {
        std::cout << "No matches found for your name.\n";
    }

    sqlite3_finalize(stmt);
}

void searchMatchesByKeyword() {
    std::string keyword;
    std::cin.ignore();
    std::cout << "Enter a keyword to search (e.g., phone, bag, black): ";
    std::getline(std::cin, keyword);

    std::string query = R"(
        SELECT m.id, l.description, f.description, m.match_score,
               l.user_name, f.user_name, f.contact_info, f.current_location
        FROM matches m
        JOIN items l ON m.lost_item_id = l.id
        JOIN items f ON m.found_item_id = f.id
        WHERE m.status='pending' AND 
              (LOWER(l.description) LIKE LOWER('%)" + keyword + R"(%') 
               OR LOWER(f.description) LIKE LOWER('%)" + keyword + R"(%'));
    )";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    bool anyMatch = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        anyMatch = true;
        int id = sqlite3_column_int(stmt, 0);
        std::string lostDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string foundDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        int score = sqlite3_column_int(stmt, 3);
        std::string lostBy = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string foundBy = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string contact = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        std::string location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

        std::string confidence;
        if (score >= 15) confidence = "🔒 Very Likely Match";
        else if (score >= 10) confidence = "✅ Likely Match";
        else if (score >= 5) confidence = "⚠️ Possible Match";
        else confidence = "❌ Low Confidence Match";

        std::cout << "\n✅ Match found for the item!"
                  << "\nMatch ID: " << id
                  << "\nLost: " << lostDesc
                  << "\nFound: " << foundDesc
                  << "\nConfidence: " << confidence
                  << "\nLost By: " << lostBy
                  << "\nFound By: " << foundBy
                  << "\nContact: " << contact
                  << "\nCurrent Location: " << location
                  << "\n---\n";
    }

    if (!anyMatch) {
        std::cout << "No matches found with that keyword.\n";
    }

    sqlite3_finalize(stmt);
}


void deleteOldEntries() {
    std::string deleteOldItems = R"(
        DELETE FROM items
        WHERE date(date_reported) < date('now', '-30 days')
    )";

    std::string deleteOrphanedMatches = R"(
        DELETE FROM matches
        WHERE lost_item_id NOT IN (SELECT id FROM items)
           OR found_item_id NOT IN (SELECT id FROM items)
    )";

    executeSQL(deleteOldItems);
    executeSQL(deleteOrphanedMatches);

    std::cout << "🧹 Old entries (older than 30 days) have been deleted from the database.\n";
}

