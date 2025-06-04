#include <iostream>      // handles console input and output
#include <string>        // std::string type for storing text
#include <unordered_map> // associative container for room exits
#include <algorithm>     // std::transform used in toLower

// Represents one location in the game world
struct Room {
    std::string name;                        // short title shown to the player
    std::string description;                 // longer text describing the area
    std::unordered_map<std::string, Room*> exits; // directions leading to other rooms
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

    // Connect rooms so the player can move between them
    glade.exits["north"] = &river;
    river.exits["south"] = &glade;
    glade.exits["east"] = &cave;
    cave.exits["west"] = &glade;

    Room* current = &glade; // The player's current location

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
            std::cout << "Available commands: look, go [direction], help, exit\n";
        }
        else if (input == "look") {                    // re-describe current room
            std::cout << current->name << "\n" << current->description << "\n";
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
