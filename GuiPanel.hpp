#pragma once

#include <GL/glew.h>
#include <nanogui/nanogui.h>
#include "Simulation.hpp"

class GuiPanel {
public:
    static nanogui::Window* build(nanogui::Screen* screen, SimParams& params, bool is2DMode);
};
