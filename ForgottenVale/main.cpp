#include <iostream>
#include <string>

int main() {
    std::string input;
    std::cout << "Welcome to Whispers of the Forgotten Vale.\n";
    std::cout << "Type 'help' for commands, 'exit' to quit.\n";

    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, input);

        if (input == "help") {
            std::cout << "Available commands: look, go [direction], help, exit\n";
        }
        else if (input == "look") {
            std::cout << "You are standing in a forgotten glade, trees whispering above.\n";
        }
        else if (input == "exit") {
            std::cout << "Farewell, wanderer...\n";
            break;
        }
        else {
            std::cout << "Unknown command. Try 'help'.\n";
        }
    }

    return 0;
}