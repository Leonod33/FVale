#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct Room {
    std::string name;
    std::string description;
    std::unordered_map<std::string, Room*> exits;
    std::vector<std::string> items;
    std::vector<std::string> actions;
    std::unordered_map<std::string, std::string> actionResults;
};

