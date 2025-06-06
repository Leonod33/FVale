#include <iostream>      // handles console input and output
#include <string>        // std::string type for storing text
#include <unordered_map> // associative container for room exits
#include <unordered_set> // storing visited rooms
#include <vector>        // stores room and player items
#include <cstdlib>       // rand
#include <ctime>         // time for seeding rand

#include <sstream>       // parsing user input into words

#include <algorithm>     // std::transform used in toLower

#include "room.h"       // Room structure definition

// ------------ Visual helpers ------------
#ifdef _WIN32
static const char* CLEAR_COMMAND = "cls";
#else
static const char* CLEAR_COMMAND = "clear";
#endif

static const std::string CLR_RESET   = "\033[0m";
static const std::string CLR_BOLD    = "\033[1m";
static const std::string CLR_CYAN    = "\033[36m";
static const std::string CLR_GREEN   = "\033[32m";
static const std::string CLR_YELLOW  = "\033[33m";
static const std::string CLR_MAGENTA = "\033[35m";
static const std::string CLR_BLUE    = "\033[34m";

static void clearScreen() {
    std::system(CLEAR_COMMAND);
}

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
static std::unordered_set<const Room*> visitedRooms;

// --- Quest tracking ---
static bool torchQuestActive = false;
static bool torchQuestComplete = false;

// --- Dynamic weather ---
static const std::vector<std::string> weatherStates = {
    "clear skies",
    "low mist",
    "light drizzle",
    "steady rain",
    "overcast clouds"
};

static std::string currentWeather = weatherStates[0];

// Forward declaration so showRoom can call it
static void maybeChangeWeather();

// Display the current room description along with items and exits
static void showRoom(const Room* room) {
    maybeChangeWeather();
    if (visitedRooms.insert(room).second) {
        std::cout << CLR_BOLD << CLR_CYAN << room->name << CLR_RESET
                  << "\n\n" << room->description << "\n\n";
    } else {
        std::cout << "You return to " << CLR_BOLD << CLR_CYAN << room->name
                  << CLR_RESET << ".\n\n";
    }
    std::cout << CLR_BLUE << "Weather: " << currentWeather << CLR_RESET << "\n";
    if (!room->items.empty()) {
        std::cout << CLR_GREEN << "You see:";
        for (const auto& it : room->items) std::cout << ' ' << it;
        std::cout << CLR_RESET << "\n";
    }
    if (!room->pointsOfInterest.empty()) {
        std::cout << CLR_YELLOW << "Notable:";
        for (const auto& p : room->pointsOfInterest) std::cout << ' ' << p.first;
        std::cout << CLR_RESET << "\n";
    }
    if (room->npc) {
        std::cout << CLR_MAGENTA << "Someone is here: " << room->npc->name
                  << CLR_RESET << "\n";
    }
    if (!room->exits.empty()) {
        std::cout << CLR_CYAN << "Exits:";
        for (const auto& e : room->exits) std::cout << ' ' << e.first;
        std::cout << CLR_RESET << "\n";
    }
    if (!room->actions.empty()) {
        std::cout << CLR_YELLOW << "Actions:";
        for (const auto& a : room->actions) std::cout << ' ' << a;
        std::cout << CLR_RESET << "\n";
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
        std::cout << '\n' << events[std::rand() % events.size()] << "\n";
    }
}

// 10% chance to change the weather each time the room is shown
static void maybeChangeWeather() {
    if (std::rand() % 100 < 10) {
        currentWeather = weatherStates[std::rand() % weatherStates.size()];
        std::cout << CLR_BLUE << "The weather shifts: " << currentWeather
                  << "." << CLR_RESET << "\n";
    }
}

// Simple NPC conversation loop
static void talkTo(NPC* npc) {
    if (!npc) return;
    std::cout << CLR_MAGENTA << npc->greeting << CLR_RESET << "\n";
    while (true) {
        for (size_t i = 0; i < npc->options.size(); ++i) {
            std::cout << i + 1 << ". " << npc->options[i].prompt << "\n";
        }
        std::cout << CLR_CYAN << "> " << CLR_RESET;
        std::string choice;
        std::getline(std::cin, choice);
        choice = toLower(choice);
        int index = -1;
        try {
            index = std::stoi(choice) - 1;
        } catch (...) {
            // not a number
        }
        if (index >= 0 && static_cast<size_t>(index) < npc->options.size()) {
            std::cout << npc->options[index].response << "\n";
            if (npc->name == "ranger" && index == 0) {
                torchQuestActive = true;
            }
            if (toLower(npc->options[index].prompt).find("farewell") != std::string::npos)
                break;
        } else {
            std::cout << "He doesn't seem to understand." << "\n";
        }
    }
}




int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    // -------- Set up the rooms --------
    // Define each location with a name and a description
    Room glade{"Forest Glade", "You stand within a quiet glade, encircled by ancient oaks whose branches weave a living roof."};
    Room river{"Crystal River", "A gentle river murmurs here, its waters clear as glass and cold as mountain snow."};
    Room cave{"Shadowy Cave", "The cave mouth gapes like a wound in the hillside, breathing damp air upon you."};
    Room meadow{"Sunny Meadow", "Grasses sway in a meadow alive with insects and drifting seeds."};
    Room hill{"Grassy Hill", "From this rise the surrounding forest rolls away in waves of green."};
    Room ruins{"Ancient Ruins", "Crumbling stones speak of a forgotten settlement swallowed by time."};
    Room tower{"Abandoned Tower", "A lonely tower leans towards the clouds, its door barred above."};
    Room vault{"Hidden Vault", "A secret chamber filled with dust and riches long unseen."};
    Room sanctum{"Ancient Sanctum", "Stones arch above a chamber steeped in silence."};

    NPC hermit;
    hermit.name = "hermit";
    hermit.greeting = "An old hermit smiles faintly.";
    hermit.options = {
        {"Who are you?", "Just a wanderer who listens to the vale."},
        {"Know anything about the tower?", "Its upper room hides treasure behind a locked door."},
        {"Farewell", "The hermit nods and returns to his thoughts."}
    };

    NPC traveller;
    traveller.name = "traveller";
    traveller.greeting = "A weary traveller doffs his cap.";
    traveller.options = {
        {"Any news?", "Only whispers of ghosts near the ruins."},
        {"Seen any treasure?", "Rumour speaks of riches locked in the tower."},
        {"Farewell", "He wishes you safe roads."}
    };

    NPC ranger;
    ranger.name = "ranger";
    ranger.greeting = "A stern ranger watches the vale.";
    ranger.options = {
        {"How may I reach the sanctum?", "Craft a torch by combining a branch and cloth, then search the cave's tunnel. The ornate key awaits."},
        {"Farewell", "He returns to his silent vigil."}
    };

    // Place a few simple items in the world
    glade.items.push_back("flower");
    glade.items.push_back("branch");
    river.items.push_back("stone");
    cave.items.push_back("rusty key");
    meadow.items.push_back("herbs");
    hill.items.push_back("map");
    ruins.items.push_back("ancient coin");
    ruins.items.push_back("cloth");
    tower.items.push_back("silver sword");
    vault.items.push_back("golden chalice");
    sanctum.items.push_back("ancient crown");

    // Points of interest in each room
    glade.pointsOfInterest["oak"] = "The ancient oak is etched with weathered runes.";
    glade.pointsOfInterest["altar"] = "A moss-covered altar hints at long-lost worship.";
    glade.pointsOfInterest["brook"] = "A narrow brook trickles between the roots.";

    river.pointsOfInterest["bridge"] = "Remnants of a wooden bridge jut from the banks.";
    river.pointsOfInterest["stones"] = "Flat stones form a crossing for the nimble.";
    river.pointsOfInterest["fish"] = "Silver fish dart just beneath the surface.";

    cave.pointsOfInterest["markings"] = "Faded symbols spiral across the damp rock.";
    cave.pointsOfInterest["stalactites"] = "Sharp formations drip slowly from above.";
    cave.pointsOfInterest["tunnel"] = "A narrow tunnel disappears into darkness.";

    meadow.pointsOfInterest["flowers"] = "Wild blooms colour the meadow like a tapestry.";
    meadow.pointsOfInterest["log"] = "A fallen log hosts colonies of bright fungi.";
    meadow.pointsOfInterest["bees"] = "Bees flit busily from flower to flower.";

    hill.pointsOfInterest["cairn"] = "A small cairn marks some forgotten traveller.";
    hill.pointsOfInterest["mountains"] = "Distant peaks loom, veiled by mist.";
    hill.pointsOfInterest["vale"] = "The vale stretches out in quiet majesty.";

    ruins.pointsOfInterest["statue"] = "A headless statue watches over the rubble.";
    ruins.pointsOfInterest["archway"] = "A collapsed arch frames the grey sky.";
    ruins.pointsOfInterest["fire"] = "A small hearth where someone recently camped.";

    ruins.npc = &hermit;
    meadow.npc = &traveller;
    hill.npc = &ranger;

    tower.pointsOfInterest["stairs"] = "Crumbling stairs spiral upwards and stop.";
    tower.pointsOfInterest["door"] = "A heavy wooden door bars the way up.";
    tower.pointsOfInterest["ivy"] = "Thick ivy clings stubbornly to the stone.";

    vault.pointsOfInterest["chest"] = "An iron-bound chest rests against the far wall.";
    vault.pointsOfInterest["mural"] = "A faded mural depicts a forgotten coronation.";
    vault.pointsOfInterest["bones"] = "Old bones lie scattered across the floor.";

    sanctum.pointsOfInterest["pedestal"] = "Upon the stone pedestal rests a final treasure.";

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

    tower.actions = {"climb", "unlock door"};
    tower.actionResults["climb"] = "You climb the crumbling stairs, but they lead nowhere.";

    vault.actions = {"unlock door"};


    // Descriptions the player can read when examining items
    std::unordered_map<std::string, std::string> itemDesc;
    itemDesc["flower"] = "A delicate wildflower with a pleasant scent.";
    itemDesc["stone"] = "A smooth river stone.";
    itemDesc["rusty key"] = "Perhaps it unlocks something ancient.";
    itemDesc["herbs"] = "Bundles of fragrant healing herbs.";
    itemDesc["branch"] = "A sturdy branch, dry and ready to burn.";
    itemDesc["cloth"] = "A strip of cloth torn from some old garment.";
    itemDesc["torch"] = "A makeshift torch of branch and cloth.";
    itemDesc["ornate key"] = "Intricately worked and surprisingly bright.";
    itemDesc["map"] = "A faded map of the surrounding lands.";
    itemDesc["ancient coin"] = "Time-worn currency from a forgotten era.";
    itemDesc["silver sword"] = "Still sharp despite years of neglect.";
    itemDesc["golden chalice"] = "Jeweled and heavy, it glitters despite the dust.";
    itemDesc["ancient crown"] = "Wrought of silver and set with dull gems.";


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
    tower.exits["up"] = &vault;
    tower.exitLocked["up"] = true;
    vault.exits["down"] = &tower;
    vault.exits["east"] = &sanctum;
    vault.exitLocked["east"] = true;
    sanctum.exits["west"] = &vault;
    meadow.exits["east"] = &ruins;
    ruins.exits["west"] = &meadow;

    Room* current = &glade;                // The player's current location
    std::vector<std::string> inventory;    // items the player has collected

    auto printMap = [&]() {
        std::vector<std::string> map = {
            "                 [Sanctum]",
            "                     |",
            "                  [Vault]",
            "                     |",
            "                 [Tower]",
            "                     |",
            "                [River]",
            "                     |",
            "     [Hill]--[Glade]--[Cave]",
            "                     |",
            "                [Meadow]--[Ruins]"
        };

        std::vector<std::pair<const Room*, std::string>> names = {
            {&glade, "Glade"}, {&river, "River"}, {&cave, "Cave"},
            {&meadow, "Meadow"}, {&hill, "Hill"}, {&ruins, "Ruins"},
            {&tower, "Tower"}, {&vault, "Vault"}, {&sanctum, "Sanctum"}
        };

        for (auto& n : names) {
            if (n.first == current) {
                std::string token = "[" + n.second + "]";
                std::string repl  = "[" + n.second + "*]";
                for (auto& line : map) {
                    size_t pos = line.find(token);
                    if (pos != std::string::npos) {
                        line.replace(pos, token.size(), repl);
                    }
                }
            }
        }

        for (const auto& line : map) std::cout << line << "\n";
    };

    std::string input; // holds the player's typed command
    clearScreen();
    std::cout << CLR_BOLD << "Welcome to Whispers of the Forgotten Vale." << CLR_RESET << "\n";
    std::cout << "Type 'help' for commands, 'exit' to quit." << "\n\n";
    showRoom(current);


    while (true) { // repeat until the player types "exit"
        std::cout << "\n" << CLR_CYAN << "> " << CLR_RESET;        // simple command prompt
        std::getline(std::cin, input); // read a full line of input
        input = toLower(input);        // make command comparisons easier


        // Split the command into individual words and drop filler like 'the'
        std::istringstream iss(input);
        std::vector<std::string> words;
        std::string word;
        while (iss >> word) {
            if (word == "the" || word == "a" || word == "an" || word == "at" ||
                word == "to" || word == "with" || word == "on" || word == "in" ||
                word == "into" || word == "from" || word == "off")
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
        const std::vector<std::string> useWords = {"use", "do", "open"};
        const std::vector<std::string> combineWords = {"combine", "craft"};
        const std::vector<std::string> invWords = {"inventory", "inv", "i"};
        const std::vector<std::string> talkWords = {"talk", "speak", "chat"};
        const std::vector<std::string> helpWords = {"help", "?"};
        const std::vector<std::string> exitWords = {"exit", "quit"};

        if (fuzzyMatch(words[0], helpWords)) {          // show available commands

            std::cout << "Available commands: look [item], go [direction], take [item], drop [item], combine [a] [b], [action], talk, inventory, help, exit\n";
            std::cout << "Type an action listed in the room to perform it." << "\n";

        }
        else if (fuzzyMatch(words[0], lookWords)) {    // look around or at an item
            if (words.size() == 1) {
                clearScreen();
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
                    auto p = current->pointsOfInterest.find(item);
                    if (p != current->pointsOfInterest.end()) {
                        std::cout << p->second << "\n";
                    } else {
                        std::cout << "You cannot see a " << item << " here." << "\n";
                    }
                }
            }
        }
        else if (fuzzyMatch(words[0], talkWords)) {   // converse with NPC
            if (current->npc) {
                if (words.size() >= 2) {
                    std::string target;
                    for (size_t i = 1; i < words.size(); ++i) {
                        if (i > 1) target += ' ';
                        target += words[i];
                    }
                    if (toLower(current->npc->name) == target) {
                        talkTo(current->npc);
                        clearScreen();
                        showRoom(current);
                    } else {
                        std::cout << "There is no " << target << " here." << "\n";
                    }
                } else {
                    talkTo(current->npc);
                    clearScreen();
                    showRoom(current);
                }
            } else {
                std::cout << "There is no one here to talk to." << "\n";
            }
        }
        else if (fuzzyMatch(words[0], goWords) && words.size() >= 2) { // move if the direction exists
            std::string dir = words[1];

            auto it = current->exits.find(dir);
            if (it != current->exits.end()) {
                auto lock = current->exitLocked.find(dir);
                if (lock != current->exitLocked.end() && lock->second) {
                    std::cout << "The way is locked." << "\n";
                } else {
                    current = it->second;
                    std::cout << "You move " << dir << ".\n";
                    clearScreen();
                    showRoom(current);
                }
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

        else if (fuzzyMatch(words[0], combineWords) && words.size() >= 3) {
            std::string first = words[1];
            std::string second = words[2];

            auto it1 = std::find(inventory.begin(), inventory.end(), first);
            auto it2 = std::find(inventory.begin(), inventory.end(), second);
            if (it1 != inventory.end() && it2 != inventory.end()) {
                if ((first == "branch" && second == "cloth") ||
                    (first == "cloth" && second == "branch")) {
                    inventory.erase(std::remove(inventory.begin(), inventory.end(), first), inventory.end());
                    inventory.erase(std::remove(inventory.begin(), inventory.end(), second), inventory.end());
                    inventory.push_back("torch");
                    std::cout << "You craft a torch." << "\n";
                } else {
                    std::cout << "Those items refuse to join." << "\n";
                }
            } else {
                std::cout << "You lack the materials." << "\n";
            }
        }

        else if (fuzzyMatch(words[0], useWords) && words.size() >= 2) {
            std::string target;
            for (size_t i = 1; i < words.size(); ++i) {
                if (i > 1) target += ' ';
                target += words[i];
            }

            auto invIt = std::find(inventory.begin(), inventory.end(), target);
            if (invIt != inventory.end()) {
                if (target == "map") {
                    printMap();
                } else if (target == "stone") {
                    std::vector<std::string> jokes = {
                        "You attempt to juggle the stone, but it immediately drops on your foot.",
                        "You proudly present the stone to the air as if it were a rare gem.",
                        "You balance the stone on your head for a moment before it tumbles off."
                    };
                    std::cout << jokes[std::rand() % jokes.size()] << "\n";
                } else if (target == "flower") {
                    std::cout << "You inhale the sweet scent of the flower." << "\n";
                } else if (target == "branch") {
                    std::cout << "You swing the branch as though fighting unseen foes." << "\n";
                } else if (target == "rusty key") {
                    std::cout << "The old key feels cold in your hand." << "\n";
                } else if (target == "herbs") {
                    std::cout << "Chewing the herbs leaves a pleasant taste and lifts your spirits." << "\n";
                } else if (target == "cloth") {
                    std::cout << "You fold the cloth neatly." << "\n";
                } else if (target == "torch") {
                    std::cout << "The torch crackles softly, casting flickering light." << "\n";
                } else if (target == "ornate key") {
                    std::cout << "The ornate key glints with promise." << "\n";
                } else if (target == "ancient coin") {
                    std::cout << "You flip the ancient coin. It lands head up." << "\n";
                } else if (target == "silver sword") {
                    std::cout << "You practice a few cautious swings with the sword." << "\n";
                } else if (target == "golden chalice") {
                    std::cout << "You admire your reflection in the chalice's gleam." << "\n";
                } else if (target == "ancient crown") {
                    std::cout << "You briefly crown yourself, feeling rather grand." << "\n";
                } else {
                    std::cout << "You can't think of a use for the " << target << "." << "\n";
                }
            } else {
                std::string action = target;
                auto it = std::find(current->actions.begin(), current->actions.end(), action);
                if (it != current->actions.end()) {
                    if (action == "search" && current == &cave && torchQuestActive && !torchQuestComplete) {
                        if (std::find(inventory.begin(), inventory.end(), "torch") != inventory.end()) {
                            torchQuestComplete = true;
                            inventory.push_back("ornate key");
                            std::cout << "Your torch reveals a hidden niche holding a key." << "\n";
                        } else {
                            std::cout << "It's too dark to see anything." << "\n";
                        }
                    } else if (action == "unlock door" && current == &tower) {
                        auto lock = current->exitLocked.find("up");
                        if (lock != current->exitLocked.end() && !lock->second) {
                            std::cout << "The door is already open." << "\n";
                        } else if (std::find(inventory.begin(), inventory.end(), "rusty key") != inventory.end()) {
                            current->exitLocked["up"] = false;
                            std::cout << "The key turns and the door creaks open." << "\n";
                        } else {
                            std::cout << "You need a key for that." << "\n";
                        }
                    } else if (action == "unlock door" && current == &vault) {
                        auto lock = current->exitLocked.find("east");
                        if (lock != current->exitLocked.end() && !lock->second) {
                            std::cout << "The door is already open." << "\n";
                        } else if (std::find(inventory.begin(), inventory.end(), "ornate key") != inventory.end()) {
                            current->exitLocked["east"] = false;
                            std::cout << "The ornate key clicks and the eastern door swings wide." << "\n";
                        } else {
                            std::cout << "You need a special key." << "\n";
                        }
                    } else {
                        auto r = current->actionResults.find(action);
                        if (r != current->actionResults.end())
                            std::cout << r->second << "\n";
                        else
                            std::cout << "You " << action << ".\n";
                    }
                } else {
                    std::cout << "You can't " << action << " here.\n";
                }
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
        else if ((words[0] == "unlock" || words[0] == "open") && words.size() >= 2 && words[1] == "door" && current == &tower) {
            auto lock = current->exitLocked.find("up");
            if (lock != current->exitLocked.end() && !lock->second) {
                std::cout << "The door is already open." << "\n";
            } else if (std::find(inventory.begin(), inventory.end(), "rusty key") != inventory.end()) {
                current->exitLocked["up"] = false;
                std::cout << "The key turns and the door creaks open." << "\n";
            } else {
                std::cout << "You need a key for that." << "\n";
            }
        }
        else if ((words[0] == "unlock" || words[0] == "open") && words.size() >= 2 && words[1] == "door" && current == &vault) {
            auto lock = current->exitLocked.find("east");
            if (lock != current->exitLocked.end() && !lock->second) {
                std::cout << "The door is already open." << "\n";
            } else if (std::find(inventory.begin(), inventory.end(), "ornate key") != inventory.end()) {
                current->exitLocked["east"] = false;
                std::cout << "The ornate key clicks and the eastern door swings wide." << "\n";
            } else {
                std::cout << "You need a special key." << "\n";
            }
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
