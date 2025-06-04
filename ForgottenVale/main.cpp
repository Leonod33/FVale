#include <iostream>      // handles console input and output
#include <string>        // std::string type for storing text
#include <unordered_map> // associative container for room exits
#include <vector>        // stores room and player items
#include <cstdlib>       // rand
#include <ctime>         // time for seeding rand

#include <sstream>       // parsing user input into words

#include <algorithm>     // std::transform used in toLower

// Represents one location in the game world
struct Room {
    std::string name;                        // short title shown to the player
    std::string description;                 // longer text describing the area
    std::unordered_map<std::string, Room*> exits; // directions leading to other rooms
    std::vector<std::string> items;          // items currently lying in the room
    std::vector<std::string> actions;        // special actions allowed here
    std::unordered_map<std::string, std::string> actionResults; // response for each action
};

// Helper to convert a string to lowercase so commands aren't case sensitive
static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

// Capitalize the first letter of a word for nicer inventory output
static std::string capitalize(std::string s) {
    if (!s.empty()) s[0] = static_cast<char>(std::toupper(s[0]));
    return s;
}

// Compute a simple edit distance so commands can tolerate small typos
static int editDistance(const std::string& a, const std::string& b) {
    std::vector<std::vector<int>> dp(a.size() + 1,
                                     std::vector<int>(b.size() + 1));
    for (size_t i = 0; i <= a.size(); ++i) dp[i][0] = static_cast<int>(i);
    for (size_t j = 0; j <= b.size(); ++j) dp[0][j] = static_cast<int>(j);
    for (size_t i = 1; i <= a.size(); ++i) {
        for (size_t j = 1; j <= b.size(); ++j) {
            int cost = a[i - 1] == b[j - 1] ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1,
                                dp[i][j - 1] + 1,
                                dp[i - 1][j - 1] + cost});
        }
    }
    return dp[a.size()][b.size()];
}

// Returns true if the word is within one edit of any given option
static bool fuzzyMatch(const std::string& word,
                       const std::vector<std::string>& options) {
    for (const auto& opt : options) {
        if (editDistance(word, opt) <= 1)
            return true;
    }
    return false;
}

// Find an action matching the word within edit distance 1, or return empty
static std::string matchAction(const std::string& word,
                               const std::vector<std::string>& actions) {
    for (const auto& act : actions) {
        if (editDistance(word, act) <= 1)
            return act;
    }
    return "";
}

// Display the current room description along with items and exits
static void showRoom(const Room* room) {
    std::cout << room->name << "\n" << room->description << "\n";
    if (!room->items.empty()) {
        std::cout << "You see:";
        for (const auto& it : room->items) std::cout << ' ' << it;
        std::cout << "\n";
    }
    if (!room->exits.empty()) {
        std::cout << "Exits:";
        for (const auto& e : room->exits) std::cout << ' ' << e.first;
        std::cout << "\n";
    }
    if (!room->actions.empty()) {
        std::cout << "Actions:";
        for (const auto& a : room->actions) std::cout << ' ' << a;
        std::cout << "\n";
    }
}

// Possible atmospheric events that may occur randomly
static const std::vector<std::string> events = {
    "A raven caws in the distance.",
    "The wind rustles through the trees.",
    "A distant howl echoes across the vale.",
    "Leaves crunch somewhere nearby.",
    "You hear the flap of wings overhead."
};

// 7% chance to display a random atmospheric event
static void maybeAtmosphericEvent() {
    if (std::rand() % 100 < 7) {
        std::cout << events[std::rand() % events.size()] << "\n";
    }
}




int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    // -------- Set up the rooms --------
    // Define each location with a name and a description
    Room glade{"Forest Glade", "You are standing in a forgotten glade, trees whispering above."};
    Room river{"Crystal River", "A gentle river flows here, sparkling in the light."};
    Room cave{"Shadowy Cave", "A dark cave mouth yawns before you."};
    Room meadow{"Sunny Meadow", "Open grass stretches out, dotted with wildflowers."};
    Room hill{"Grassy Hill", "A low hill provides a view of the surrounding forest."};
    Room ruins{"Ancient Ruins", "Crumbling stone walls hint at stories long forgotten."};
    Room tower{"Abandoned Tower", "The remains of a stone tower rise against the sky."};

    // Place a few simple items in the world
    glade.items.push_back("flower");
    river.items.push_back("stone");
    cave.items.push_back("rusty key");
    meadow.items.push_back("herbs");
    hill.items.push_back("map");
    ruins.items.push_back("ancient coin");
    tower.items.push_back("silver sword");

    // Special actions for each room
    glade.actions = {"rest"};
    glade.actionResults["rest"] = "You rest for a moment, listening to the whispering leaves.";

    river.actions = {"drink"};
    river.actionResults["drink"] = "You drink the cool river water.";

    cave.actions = {"search"};
    cave.actionResults["search"] = "You find strange markings on the damp walls.";

    meadow.actions = {"gather"};
    meadow.actionResults["gather"] = "You gather a handful of colorful wildflowers.";

    hill.actions = {"climb"};
    hill.actionResults["climb"] = "From the hilltop you glimpse the entire vale.";

    ruins.actions = {"search"};
    ruins.actionResults["search"] = "You sift through the rubble but find nothing of value.";

    tower.actions = {"climb"};
    tower.actionResults["climb"] = "You climb the crumbling stairs, but they lead nowhere.";


    // Descriptions the player can read when examining items
    std::unordered_map<std::string, std::string> itemDesc;
    itemDesc["flower"] = "A delicate wildflower with a pleasant scent.";
    itemDesc["stone"] = "A smooth river stone.";
    itemDesc["rusty key"] = "Perhaps it unlocks something ancient.";
    itemDesc["herbs"] = "Bundles of fragrant healing herbs.";
    itemDesc["map"] = "A faded map of the surrounding lands.";
    itemDesc["ancient coin"] = "Time-worn currency from a forgotten era.";
    itemDesc["silver sword"] = "Still sharp despite years of neglect.";


    // Connect rooms so the player can move between them
    glade.exits["north"] = &river;
    river.exits["south"] = &glade;
    glade.exits["east"] = &cave;
    cave.exits["west"] = &glade;
    glade.exits["south"] = &meadow;
    meadow.exits["north"] = &glade;
    glade.exits["west"] = &hill;
    hill.exits["east"] = &glade;
    river.exits["east"] = &tower;
    tower.exits["west"] = &river;
    meadow.exits["east"] = &ruins;
    ruins.exits["west"] = &meadow;

    Room* current = &glade;                // The player's current location
    std::vector<std::string> inventory;    // items the player has collected

    std::string input; // holds the player's typed command
    std::cout << "Welcome to Whispers of the Forgotten Vale.\n";
    std::cout << "Type 'help' for commands, 'exit' to quit.\n";
    // Show the description of the starting room

    showRoom(current);


    while (true) { // repeat until the player types "exit"
        std::cout << "\n> ";        // simple command prompt
        std::getline(std::cin, input); // read a full line of input
        input = toLower(input);        // make command comparisons easier


        // Split the command into individual words and drop filler like 'the'
        std::istringstream iss(input);
        std::vector<std::string> words;
        std::string word;
        while (iss >> word) {
            if (word == "the" || word == "a" || word == "an" || word == "at")
                continue;
            words.push_back(word);
        }
        if (words.empty())
            continue;

        // Word groups used to recognise commands and tolerate slight typos
        const std::vector<std::string> lookWords = {"look", "examine", "inspect"};
        const std::vector<std::string> goWords = {"go", "move", "walk"};
        const std::vector<std::string> takeWords = {"take", "get", "pickup", "pick", "grab"};
        const std::vector<std::string> dropWords = {"drop", "leave"};
        const std::vector<std::string> useWords = {"use", "do"};
        const std::vector<std::string> invWords = {"inventory", "inv", "i"};
        const std::vector<std::string> helpWords = {"help", "?"};
        const std::vector<std::string> exitWords = {"exit", "quit"};

        if (fuzzyMatch(words[0], helpWords)) {          // show available commands

            std::cout << "Available commands: look [item], go [direction], take [item], drop [item], [action], inventory, help, exit\n";
            std::cout << "Type an action listed in the room to perform it." << "\n";

        }
        else if (fuzzyMatch(words[0], lookWords)) {    // look around or at an item
            if (words.size() == 1) {
                showRoom(current);
            } else {
                std::string item;
                for (size_t i = 1; i < words.size(); ++i) {
                    if (i > 1) item += ' ';
                    item += words[i];
                }
                auto it = std::find(inventory.begin(), inventory.end(), item);
                if (it != inventory.end()) {
                    auto d = itemDesc.find(item);
                    if (d != itemDesc.end())
                        std::cout << d->second << "\n";
                    else
                        std::cout << "It's just a " << item << ".\n";
                } else {
                    std::cout << "You don't have a " << item << ".\n";
                }
            }
        }
        else if (fuzzyMatch(words[0], goWords) && words.size() >= 2) { // move if the direction exists
            std::string dir = words[1];

            auto it = current->exits.find(dir);
            if (it != current->exits.end()) {
                current = it->second;
                std::cout << "You move " << dir << ".\n";

                showRoom(current);

            } else {
                std::cout << "You can't go that way.\n";
            }
        }

        else if (fuzzyMatch(words[0], takeWords) && words.size() >= 2) { // attempt to pick up an item
            std::string item;
            for (size_t i = 1; i < words.size(); ++i) {
                if (i > 1) item += ' ';
                item += words[i];
            }

            auto it = std::find(current->items.begin(), current->items.end(), item);
            if (it != current->items.end()) {
                inventory.push_back(*it);
                current->items.erase(it);
                std::cout << "You take the " << item << ".\n";
            } else {
                std::cout << "There is no " << item << " here.\n";
            }
        }

        else if (fuzzyMatch(words[0], dropWords) && words.size() >= 2) { // drop an item
            std::string item;
            for (size_t i = 1; i < words.size(); ++i) {
                if (i > 1) item += ' ';
                item += words[i];
            }

            auto it = std::find(inventory.begin(), inventory.end(), item);
            if (it != inventory.end()) {
                inventory.erase(it);
                current->items.push_back(item);
                std::cout << "You drop the " << item << ".\n";
            } else {
                std::cout << "You don't have a " << item << ".\n";
            }
        }

        else if (fuzzyMatch(words[0], useWords) && words.size() >= 2) { // perform a room action
            std::string action;
            for (size_t i = 1; i < words.size(); ++i) {
                if (i > 1) action += ' ';
                action += words[i];
            }

            auto it = std::find(current->actions.begin(), current->actions.end(), action);
            if (it != current->actions.end()) {
                auto r = current->actionResults.find(action);
                if (r != current->actionResults.end())
                    std::cout << r->second << "\n";
                else
                    std::cout << "You " << action << ".\n";
            } else {
                std::cout << "You can't " << action << " here.\n";
            }
        }

        else if (!matchAction(words[0], current->actions).empty()) { // action without 'use'
            std::string action = matchAction(words[0], current->actions);
            auto r = current->actionResults.find(action);
            if (r != current->actionResults.end())
                std::cout << r->second << "\n";
            else
                std::cout << "You " << action << ".\n";
        }


        else if (fuzzyMatch(words[0], invWords)) {     // list carried items
            if (inventory.empty()) {
                std::cout << "Your inventory is empty.\n";
            } else {
                std::cout << "You are carrying ";
                for (size_t i = 0; i < inventory.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << capitalize(inventory[i]);
                }
                std::cout << ".\n";
            }
        }
        else if (fuzzyMatch(words[0], exitWords)) {    // leave the game

            std::cout << "Farewell, wanderer...\n";
            break;
        }
        else {                                          // command wasn't recognized
            std::cout << "Unknown command. Try 'help'.\n";
        }

        maybeAtmosphericEvent();
    }

    return 0; // program completed successfully
}
