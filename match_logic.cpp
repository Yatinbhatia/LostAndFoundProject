#include "match_logic.h"
#include <string>
#include <set>
#include <sstream>
#include <algorithm>

std::string toLowerCase(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}

int calculateMatchScore(const std::string& desc1, const std::string& desc2) {
    std::string d1 = toLowerCase(desc1);
    std::string d2 = toLowerCase(desc2);

    std::set<std::string> words1, words2;
    std::istringstream iss1(d1), iss2(d2);
    std::string word;

    while (iss1 >> word) words1.insert(word);
    while (iss2 >> word) words2.insert(word);

    int score = 0;
    for (const std::string& w : words1) {
        if (words2.count(w)) {
            score += 10;  // Each matched word adds 10 to the score
        }
    }

    return score;
}
