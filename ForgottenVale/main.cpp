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

// --- Vault word puzzle ---
static bool chestPuzzleSolved = false; // ensures the chest puzzle is solved only once

// --- Dynamic weather ---
static const std::vector<std::string> weatherStates = {
    "clear skies stretched like blue silk",
    "a low mist curling about the roots",
    "a light drizzle speckling the leaves",
    "a steady rain whispering against the earth",
    "brooding overcast clouds"
};

static std::string currentWeather = weatherStates[0];

// Forward declaration so showRoom can call it
static void maybeChangeWeather();
// Forward declaration for the word puzzle in the vault
static void solveChalicePuzzle(Room* room);

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
    "A lone raven caws in the distance, its cry a thread of melancholy.",
    "The wind rustles through the trees, stirring a sigh from the branches.",
    "From far off, a mournful howl sweeps across the vale.",
    "Somewhere nearby, leaves crunch as though under unseen feet.",
    "Above, the heavy beat of wings passes and fades into the clouds."
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

// Word puzzle presented by the chest in the vault. The player must
// unscramble the letters to obtain the treasure within.
static void solveChalicePuzzle(Room* room) {
    std::cout << "The chest's lid displays a whirl of letters: C A L I C E H.\n";
    std::cout << "Arrange them into a single word and speak it, or type 'leave' to step back." << "\n";
    std::string guess;
    while (true) {
        std::cout << CLR_CYAN << "> " << CLR_RESET;
        std::getline(std::cin, guess);
        guess = toLower(guess);
        if (guess == "chalice") {
            std::cout << "The letters flare and drift aside. The chest creaks open, revealing a golden chalice." << "\n";
            room->items.push_back("golden chalice");
            chestPuzzleSolved = true;
            break;
        } else if (guess == "leave" || guess == "exit") {
            std::cout << "You step away and the letters settle into stillness." << "\n";
            break;
        } else {
            std::cout << "Nothing stirs. Perhaps another arrangement?" << "\n";
        }
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
    Room glade{"Forest Glade", "You stand within a quiet glade, encircled by ancient oaks whose interlaced branches weave a living roof. Shafts of pale light drift through the leaves, and the air smells of earth and old rain."};
    Room river{"Crystal River", "A gentle river murmurs here, its waters clear as glass and cold as mountain snow, winding over pebbles polished by ages. A faint mist hangs above, catching the light like spun silver."};
    Room cave{"Shadowy Cave", "The cave mouth gapes like a wound in the hillside, breathing damp, mineral-laden air upon you. Shadows cling to the interior, promising both shelter and secrets."};
    Room meadow{"Sunny Meadow", "Grasses sway in a wide meadow alive with insects and drifting seeds, the hum of life blending with the distant warble of birds. Patches of wild colour nod in the breeze."};
    Room hill{"Grassy Hill", "From this rise the surrounding forest rolls away in waves of green, fading to blue at the horizon. A cool wind brushes your face, carrying the scent of pine and distant rain."};
    Room ruins{"Ancient Ruins", "Crumbling stones speak of a forgotten settlement swallowed by time. Ivy threads through shattered walls, and the ground is littered with pottery shards and ghostly memories."};
    Room tower{"Abandoned Tower", "A lonely tower leans towards the clouds, its stones scarred by weather and war. The door above is barred, and a draught whistles through the cracks like distant whispering."};
    Room vault{"Hidden Vault", "A secret chamber lies here, its air thick with dust and the faint scent of old metal. Shelves sag with tarnished relics, and the silence is deep as a grave."};
    Room sanctum{"Ancient Sanctum", "Stones arch above a chamber steeped in silence. The air is cool and still, and every step awakens echoes that seem to belong to someone else."};

    NPC hermit;
    hermit.name = "hermit";
    hermit.greeting = "An old hermit smiles faintly, his beard a tangle of silver threads.";
    hermit.options = {
        {"Who are you?", "Merely a wanderer who lends an ear to the murmurs of the vale."},
        {"Know anything about the tower?", "The tower's upper room is said to cradle treasure, yet the door above remains stubbornly locked."},
        {"Farewell", "The hermit nods and returns to his thoughts, eyes misty with distant memories."}
    };

    NPC traveller;
    traveller.name = "traveller";
    traveller.greeting = "A weary traveller doffs his cap, dust clinging to his cloak.";
    traveller.options = {
        {"Any news?", "Only whispers of pale spirits drifting near the old ruins."},
        {"Seen any treasure?", "Rumour speaks of bright riches locked away in the leaning tower."},
        {"Farewell", "He wishes you safe roads and softer nights than his own."}
    };

    NPC ranger;
    ranger.name = "ranger";
    ranger.greeting = "A stern ranger watches the vale, his cloak mottled like the forest floor.";
    ranger.options = {
        {"How may I reach the sanctum?", "Fashion yourself a torch from branch and cloth; only its light will reveal the tunnel's secret. Search deep within the cave and the ornate key will be yours."},
        {"Farewell", "He returns to his silent vigil, gaze sweeping the horizon."}
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
    sanctum.items.push_back("ancient crown");

    // Points of interest in each room
    glade.pointsOfInterest["oak"] = "The ancient oak is etched with weathered runes whose meaning has long since faded.";
    glade.pointsOfInterest["altar"] = "A moss-covered altar hints at long-lost worship, the stone warmed by countless seasons.";
    glade.pointsOfInterest["brook"] = "A narrow brook trickles between the roots, its water singing over tiny stones.";

    river.pointsOfInterest["bridge"] = "Remnants of a wooden bridge jut from the banks, slick with algae and age.";
    river.pointsOfInterest["stones"] = "Flat stones form a crossing for the nimble, each wobbling in the current.";
    river.pointsOfInterest["fish"] = "Silver fish dart just beneath the surface, flashing like living coins.";

    cave.pointsOfInterest["markings"] = "Faded symbols spiral across the damp rock, their patterns too ancient to decipher.";
    cave.pointsOfInterest["stalactites"] = "Sharp formations drip slowly from above, each drop echoing in the hush.";
    cave.pointsOfInterest["tunnel"] = "A narrow tunnel disappears into darkness, exhaling a chill breath.";

    meadow.pointsOfInterest["flowers"] = "Wild blooms colour the meadow like a tapestry, petals nodding in lazy conversation.";
    meadow.pointsOfInterest["log"] = "A fallen log hosts colonies of bright fungi, their caps glistening after recent rain.";
    meadow.pointsOfInterest["bees"] = "Bees flit busily from flower to flower, droning a drowsy melody.";

    hill.pointsOfInterest["cairn"] = "A small cairn marks some forgotten traveller, stones carefully balanced by patient hands.";
    hill.pointsOfInterest["mountains"] = "Distant peaks loom, veiled by mist, their snow caps mere smudges on the sky.";
    hill.pointsOfInterest["vale"] = "The vale stretches out in quiet majesty, ridges folding into one another like sleeping beasts.";

    ruins.pointsOfInterest["statue"] = "A headless statue watches over the rubble, its remaining features softened by moss.";
    ruins.pointsOfInterest["archway"] = "A collapsed arch frames the grey sky, each stone blackened by age.";
    ruins.pointsOfInterest["fire"] = "A small hearth where someone recently camped, ashes still warm to the touch.";

    ruins.npc = &hermit;
    meadow.npc = &traveller;
    hill.npc = &ranger;

    tower.pointsOfInterest["stairs"] = "Crumbling stairs spiral upwards and stop, as though the builders lost heart.";
    tower.pointsOfInterest["door"] = "A heavy wooden door bars the way up, its iron bands flecked with rust.";
    tower.pointsOfInterest["ivy"] = "Thick ivy clings stubbornly to the stone, leaves whispering in the breeze.";

    vault.pointsOfInterest["chest"] = "An iron-bound chest rests against the far wall, letters swirling faintly upon its lid.";
    vault.pointsOfInterest["mural"] = "A faded mural depicts a forgotten coronation, colours leeched by time.";
    vault.pointsOfInterest["bones"] = "Old bones lie scattered across the floor, brittle as autumn twigs.";

    sanctum.pointsOfInterest["pedestal"] = "Upon the stone pedestal rests a final treasure, bathed in a shaft of dusty light.";

    // Special actions for each room
    glade.actions = {"rest"};
    glade.actionResults["rest"] = "You rest for a moment, listening to the whispering leaves and the far-off chatter of unseen birds.";

    river.actions = {"drink"};
    river.actionResults["drink"] = "You cup your hands and drink the cool river water, feeling new strength seep into your limbs.";

    cave.actions = {"search"};
    cave.actionResults["search"] = "You find strange markings on the damp walls, their curves shining faintly with moisture.";

    meadow.actions = {"gather"};
    meadow.actionResults["gather"] = "You gather a handful of colourful wildflowers, their petals brushing your fingers like silk.";

    hill.actions = {"climb"};
    hill.actionResults["climb"] = "From the hilltop you glimpse the entire vale, a tapestry of greens and greys stretching to the horizon.";

    ruins.actions = {"search"};
    ruins.actionResults["search"] = "You sift through the rubble but find nothing of value, only the cold touch of forgotten stones.";

    tower.actions = {"climb", "unlock door"};
    tower.actionResults["climb"] = "You climb the crumbling stairs, but they end abruptly against a ceiling of splintered beams.";

    vault.actions = {"unlock door", "decipher"};
    vault.actionResults["decipher"] = "You ponder the shifting letters on the chest.";


    // Descriptions the player can read when examining items
    std::unordered_map<std::string, std::string> itemDesc;
    itemDesc["flower"] = "A delicate wildflower whose petals hold a faint, pleasant scent.";
    itemDesc["stone"] = "A smooth river stone, cool and reassuring to the touch.";
    itemDesc["rusty key"] = "The iron is pitted yet sturdy; perhaps it unlocks something ancient.";
    itemDesc["herbs"] = "Bundles of fragrant healing herbs, tied with a faded thread.";
    itemDesc["branch"] = "A sturdy branch, dry and ready to burn; its sap long since gone.";
    itemDesc["cloth"] = "A strip of cloth torn from some old garment, frayed at the edges.";
    itemDesc["torch"] = "A makeshift torch of branch and cloth, its flame small but steadfast.";
    itemDesc["ornate key"] = "Intricately worked and surprisingly bright, the key bears motifs of entwined vines.";
    itemDesc["map"] = "A faded map of the surrounding lands, edges softened by many foldings.";
    itemDesc["ancient coin"] = "Time-worn currency from a forgotten era, its symbols almost erased.";
    itemDesc["silver sword"] = "Still sharp despite years of neglect; it catches the light with a wicked gleam.";
    itemDesc["golden chalice"] = "Jewelled and heavy, it glitters despite the dust.";
    itemDesc["ancient crown"] = "Wrought of silver and set with dull gems that hint at former splendour.";


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
    std::cout << "Type 'help' for guidance, or 'exit' to take your leave." << "\n\n";
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
                    std::cout << "You inhale the flower's faint fragrance, a reminder of gentler places." << "\n";
                } else if (target == "branch") {
                    std::cout << "You swing the branch as though fighting unseen foes, feeling slightly foolish yet warmed by the exertion." << "\n";
                } else if (target == "rusty key") {
                    std::cout << "The old key feels cold in your hand, edges rounded by countless turns." << "\n";
                } else if (target == "herbs") {
                    std::cout << "Chewing the herbs leaves a pleasant taste and lifts your spirits for a spell." << "\n";
                } else if (target == "cloth") {
                    std::cout << "You fold the cloth neatly, its threads rough against your fingers." << "\n";
                } else if (target == "torch") {
                    std::cout << "The torch crackles softly, casting flickering light that chases shadows to the edges." << "\n";
                } else if (target == "ornate key") {
                    std::cout << "The ornate key glints with promise, engravings shimmering in the firelight." << "\n";
                } else if (target == "ancient coin") {
                    std::cout << "You flip the ancient coin; it lands head up and rings with a thin, sweet note." << "\n";
                } else if (target == "silver sword") {
                    std::cout << "You practice a few cautious swings with the sword, its weight surprisingly well balanced." << "\n";
                } else if (target == "golden chalice") {
                    std::cout << "You admire your reflection in the chalice's gleam, distorted as though seen through still water." << "\n";
                } else if (target == "ancient crown") {
                    std::cout << "You briefly crown yourself, feeling rather grand until the dust tickles your brow." << "\n";
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
                    } else if (action == "decipher" && current == &vault) {
                        if (!chestPuzzleSolved) {
                            solveChalicePuzzle(current);
                        } else {
                            std::cout << "The chest already lies open, its letters dormant." << "\n";
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
            if (action == "decipher" && current == &vault) {
                if (!chestPuzzleSolved) {
                    solveChalicePuzzle(current);
                } else {
                    std::cout << "The chest already lies open, its letters dormant." << "\n";
                }
            } else {
                auto r = current->actionResults.find(action);
                if (r != current->actionResults.end())
                    std::cout << r->second << "\n";
                else
                    std::cout << "You " << action << ".\n";
            }
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
