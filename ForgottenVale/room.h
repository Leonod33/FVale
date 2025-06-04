#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct NPC;

struct Room {
    std::string name;
    std::string description;
    std::unordered_map<std::string, Room*> exits;
    std::unordered_map<std::string, bool> exitLocked;
    std::vector<std::string> items;
    std::vector<std::string> actions;
    std::unordered_map<std::string, std::string> actionResults;
    std::unordered_map<std::string, std::string> pointsOfInterest;
    NPC* npc = nullptr;
};

struct DialogueOption {
    std::string prompt;
    std::string response;
};

struct NPC {
    std::string name;
    std::string greeting;
    std::vector<DialogueOption> options;
};

