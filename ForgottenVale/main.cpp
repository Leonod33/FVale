#include <iostream>      // handles console input and output
#include <string>        // std::string type for storing text
#include <unordered_map> // associative container for room exits
#include <vector>        // stores room and player items
#include <algorithm>     // std::transform used in toLower

// Represents one location in the game world
struct Room {
    std::string name;                        // short title shown to the player
    std::string description;                 // longer text describing the area
    std::unordered_map<std::string, Room*> exits; // directions leading to other rooms
    std::vector<std::string> items;          // items currently lying in the room
};

// Helper to convert a string to lowercase so commands aren't case sensitive
static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

int main() {
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
    std::cout << current->description << "\n";

    while (true) { // repeat until the player types "exit"
        std::cout << "\n> ";        // simple command prompt
        std::getline(std::cin, input); // read a full line of input
        input = toLower(input);        // make command comparisons easier

        if (input == "help") {                         // show available commands
            std::cout << "Available commands: look, go [direction], take [item], inventory, help, exit\n";
        }
        else if (input == "look") {                    // re-describe current room
            std::cout << current->name << "\n" << current->description << "\n";
            if (!current->items.empty()) {
                std::cout << "You see:";
                for (const auto& it : current->items) {
                    std::cout << ' ' << it;
                }
                std::cout << '\n';
            }
            if (!current->exits.empty()) {
                std::cout << "Exits:";
                for (const auto& e : current->exits) {
                    std::cout << ' ' << e.first;
                }
                std::cout << '\n';
            }
        }
        else if (input.rfind("go ", 0) == 0) {         // move if the direction exists
            std::string dir = input.substr(3);          // get direction after "go"
            auto it = current->exits.find(dir);
            if (it != current->exits.end()) {
                current = it->second;
                std::cout << "You move " << dir << ".\n";
                std::cout << current->description << "\n";
            } else {
                std::cout << "You can't go that way.\n";
            }
        }
        else if (input.rfind("take ", 0) == 0) {       // attempt to pick up an item
            std::string item = input.substr(5);
            auto it = std::find(current->items.begin(), current->items.end(), item);
            if (it != current->items.end()) {
                inventory.push_back(*it);
                current->items.erase(it);
                std::cout << "You take the " << item << ".\n";
            } else {
                std::cout << "There is no " << item << " here.\n";
            }
        }
        else if (input == "inventory" || input == "inv") { // list carried items
            if (inventory.empty()) {
                std::cout << "You are carrying nothing.\n";
            } else {
                std::cout << "You are carrying:";
                for (const auto& it : inventory) {
                    std::cout << ' ' << it;
                }
                std::cout << '\n';
            }
        }
        else if (input == "exit") {                    // leave the game
            std::cout << "Farewell, wanderer...\n";
            break;
        }
        else {                                          // command wasn't recognized
            std::cout << "Unknown command. Try 'help'.\n";
        }
    }

    return 0; // program completed successfully
}
