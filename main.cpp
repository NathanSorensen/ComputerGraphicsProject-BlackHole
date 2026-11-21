#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>

#include "BlackHoleVisualSim.hpp"

bool parseArgs(int argc, char* argv[], bool defaultMode = false) {
    if (argc <= 1) return defaultMode;

    std::string arg = argv[1];
    std::transform(arg.begin(), arg.end(), arg.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (arg == "true" || arg == "1") return true;
    if (arg == "false" || arg == "0") return false;

    std::cerr << "[Warning] Unknown argument for \"2D Mode\": '" << argv[1]
        << "'. Expected 'true' or 'false'. Defaulting to "
        << (defaultMode ? "2D" : "3D") << " Mode.\n";
    return defaultMode;
}

int main(int argc, char* argv[]) {
    bool is2DMode = parseArgs(argc, argv);
    std::cout << "Starting in " << (is2DMode ? "2D Mode" : "3D Mode") << "\n";

    try {
        nanogui::init();
        {
            nanogui::ref<BlackHoleVisualSim> app = new BlackHoleVisualSim(is2DMode);
            app->drawAll();
            app->setVisible(true);
            nanogui::mainloop(16);
        }
        nanogui::shutdown();
    }
    catch (const std::exception& e) {
        std::cerr << "[Fatal Error]: " << e.what() << "\n";
        return -1;
    }

    return 0;
}
