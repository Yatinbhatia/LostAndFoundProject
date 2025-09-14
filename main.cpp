#include <iostream>
#include "db_utils.h"

// void seedTestData() {
//     std::vector<std::string> insertQueries = {
//         // LOST ITEMS
//         R"(INSERT INTO items (type, user_name, description, category, location, date_reported)
//            VALUES ('lost', 'Satyam', 'Blue iPhone 13 with sticker', 'mobile', 'birla', '2024-07-08');)",

//         R"(INSERT INTO items (type, user_name, description, category, location, date_reported)
//            VALUES ('lost', 'Priya', 'Black bag containing notebooks', 'bag', 'cse block', '2024-07-08');)",

//         // FOUND ITEMS
//         R"(INSERT INTO items (type, user_name, description, category, location, date_reported, contact_info, current_location)
//            VALUES ('found', 'Aditi', 'Blue iPhone 13 with apple sticker', 'mobile', 'birla', '2024-07-09', '9876543210', 'Admin Block');)",

//         R"(INSERT INTO items (type, user_name, description, category, location, date_reported, contact_info, current_location)
//            VALUES ('found', 'Rahul', 'Old black bag with books', 'bag', 'cse block', '2024-07-09', '9988776655', 'Canteen');)"
//     };

//     std::cout << "📦 Seeding test data into the database...\n";
//     for (const std::string& q : insertQueries) {
//         executeSQL(q);
//     }
//     std::cout << "✅ Test data added successfully!\n";

//     // Optional: Re-run match logic for all new found items
//     std::string matchNewFoundItems = R"(
//         INSERT INTO matches (lost_item_id, found_item_id, match_score)
//         SELECT l.id, f.id, 10 + 
//                CASE WHEN l.description LIKE '%' || f.description || '%' THEN 5 ELSE 0 END
//         FROM items l
//         JOIN items f ON LOWER(l.category) = LOWER(f.category)
//                     AND LOWER(l.location) = LOWER(f.location)
//                     AND l.type = 'lost' AND f.type = 'found'
//                     AND l.is_resolved = 0 AND f.is_resolved = 0
//         WHERE NOT EXISTS (
//             SELECT 1 FROM matches m WHERE m.lost_item_id = l.id AND m.found_item_id = f.id
//         );
//     )";
//     executeSQL(matchNewFoundItems);
// }


int main() {
    initializeDB();
    int choice;
    // Seed the database with test entries once
    //seedTestData();
    deleteOldEntries();

    while (true) {
        std::cout << "\n===== LOST & FOUND SYSTEM =====\n"
                  << "1. Report Lost Item\n"
                  << "2. Report Found Item\n"
                  << "3. View Possible Matches (Admin)\n"
                  << "4. Confirm Match\n"
                  << "5. Search My Matches (by Name)\n"
                  << "6. Search My Matches (by Keyword)\n"
                  << "7. Exit\n"
                  << "Enter your choice: ";
        std::cin >> choice;
        switch (choice) {
            case 1: insertItem("lost"); break;
            case 2: insertItem("found"); break;
            case 3: viewMatches(); break;
            case 4: confirmMatch(); break;
            case 5:
                searchMatchesByName();
                break;
            case 6:
                searchMatchesByKeyword();
                break;
            case 7: sqlite3_close(db); return 0;
            default: std::cout << "Invalid option!\n";
        }
    }
}