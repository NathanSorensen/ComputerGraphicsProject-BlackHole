#include "GuiPanel.hpp"
#include <array>
#include <cstdio>
#include <string>

static void addGridSlider(nanogui::Widget* parent, const std::string& label,
    float minVal, float maxVal, float& targetVar,
    const char* format = "%.2f", float stepSize = 0.0f) {
    new nanogui::Label(parent, label, "sans");

    auto* row = new nanogui::Widget(parent);
    row->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
        nanogui::Alignment::Middle, 0, 4));

    auto* slider = new nanogui::Slider(row);
    slider->setRange({ minVal, maxVal });
    slider->setValue(targetVar);
    slider->setFixedWidth(95);

    auto* textBox = new nanogui::TextBox(row);
    textBox->setFixedSize(nanogui::Vector2i(54, 20));
    std::array<char, 32> buf{};
    std::snprintf(buf.data(), buf.size(), format, targetVar);
    textBox->setValue(buf.data());

    slider->setCallback([&targetVar, slider, textBox, format, stepSize](float value) {
        if (stepSize > 0.0f) {
            value = std::round(value / stepSize) * stepSize;
            slider->setValue(value); // Snaps the slider thumb visually to the step
        }
        targetVar = value;

        std::array<char, 32> b{};
        std::snprintf(b.data(), b.size(), format, value);
        textBox->setValue(b.data());
        });
}

nanogui::Window* GuiPanel::build(nanogui::Screen* screen, SimParams& params, bool is2DMode) {
    auto* win = new nanogui::Window(screen, "Black Hole Controls (G to Hide)");
    win->setPosition(nanogui::Vector2i(12, 12));
    win->setLayout(new nanogui::GroupLayout(8, 6, 8, 10));

    // --- Section 1: Rays & Speed ---
    auto* simGrid = new nanogui::Widget(win);
    simGrid->setLayout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
        nanogui::Alignment::Middle, 2, 4));

    // Shared in both 2D and 3D modes
    addGridSlider(simGrid, "Cam Speed", 1.0f, 30.0f, params.cameraSpeed, "%.1f", 0.5f);

    // 2D-exclusive ray controls
    if (is2DMode) {
        addGridSlider(simGrid, "Sim Speed", 0.5f, 20.0f, params.timeScale, "%.1f", 0.5f);
        addGridSlider(simGrid, "Beam Spread", 1.0f, 12.0f, params.raySpreadY, "%.1f", 0.5f);
        addGridSlider(simGrid, "Launch Angle", -45.0f, 45.0f, params.rayAngleDeg, "%.0f", 1.0f);

        new nanogui::Label(simGrid, "Ray Count", "sans");
        auto* countRow = new nanogui::Widget(simGrid);
        countRow->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
            nanogui::Alignment::Middle, 0, 4));
        auto* countSlider = new nanogui::Slider(countRow);
        countSlider->setRange({ 1.0f, 80.0f });
        countSlider->setValue(static_cast<float>(params.rayCount));
        countSlider->setFixedWidth(95);

        auto* countBox = new nanogui::TextBox(countRow);
        countBox->setFixedSize(nanogui::Vector2i(54, 20));
        countBox->setValue(std::to_string(params.rayCount));

        countSlider->setCallback([&params, countBox](float val) {
            params.rayCount = static_cast<int>(val);
            countBox->setValue(std::to_string(params.rayCount));
            params.resetRequested = true;
            });
    }

    // --- Section 2: Black Hole 1 (Orange) ---
    new nanogui::Label(win, "Black Hole 1 (Orange)", "sans-bold");
    auto* bh1Grid = new nanogui::Widget(win);
    bh1Grid->setLayout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
        nanogui::Alignment::Middle, 2, 4));
    addGridSlider(bh1Grid, "BH1 Radius", 0.2f, 3.5f, params.bh1Rs);
    addGridSlider(bh1Grid, "BH1 Pos X", -8.0f, 8.0f, params.bh1X, "%.1f");
    addGridSlider(bh1Grid, "BH1 Pos Y", -8.0f, 8.0f, params.bh1Y, "%.1f");

    // --- Section 3: Black Hole 2 (Cyan) & Binary Orbit ---
    new nanogui::Label(win, "Black Hole 2 (Cyan)", "sans-bold");
    auto* enableCheck = new nanogui::CheckBox(win, "Enable Second Black Hole");
    enableCheck->setChecked(params.enableSecondBH);
    enableCheck->setCallback([&params](bool checked) {
        params.enableSecondBH = checked;
        });

    auto* bh2Grid = new nanogui::Widget(win);
    bh2Grid->setLayout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
        nanogui::Alignment::Middle, 2, 4));
    addGridSlider(bh2Grid, "BH2 Radius", 0.2f, 3.5f, params.bh2Rs);
    addGridSlider(bh2Grid, "BH2 Pos X", -8.0f, 8.0f, params.bh2X, "%.1f");
    addGridSlider(bh2Grid, "BH2 Pos Y", -8.0f, 8.0f, params.bh2Y, "%.1f");

    auto* orbitCheck = new nanogui::CheckBox(win, "Orbiting Binary Mode");
    orbitCheck->setChecked(params.binaryOrbit);
    orbitCheck->setCallback([&params](bool checked) {
        params.binaryOrbit = checked;
        });

    auto* orbitGrid = new nanogui::Widget(win);
    orbitGrid->setLayout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
        nanogui::Alignment::Middle, 2, 4));
    addGridSlider(orbitGrid, "Orbit Dist", 0.0f, 24.0f, params.binarySeparation, "%.1f", 0.5f);
    addGridSlider(orbitGrid, "Orbit Speed", -2.0f, 2.0f, params.binarySpeed, "%.2f", 0.01f);

    if (!is2DMode) {
        new nanogui::Label(win, "3D Render Layers", "sans-bold");

        auto* rtCheck = new nanogui::CheckBox(win, "Enable GPU Ray Tracing");
        rtCheck->setChecked(params.enableRayTracing);
        rtCheck->setCallback([&params](bool checked) {
            params.enableRayTracing = checked;
            });

        auto* discCheck = new nanogui::CheckBox(win, "Show Accretion Disc");
        discCheck->setChecked(params.showAccretionDisc);
        discCheck->setCallback([&params](bool checked) {
            params.showAccretionDisc = checked;
            });

        auto* gridCheck = new nanogui::CheckBox(win, "Show Spacetime Grid");
        gridCheck->setChecked(params.showSpacetimeGrid);
        gridCheck->setCallback([&params](bool checked) {
            params.showSpacetimeGrid = checked;
            });
    }


    // --- Section 4: Action Buttons ---
    auto* btnRow = new nanogui::Widget(win);
    btnRow->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
        nanogui::Alignment::Middle, 0, 6));

    auto* pauseBtn = new nanogui::Button(btnRow, "Pause (Space)");
    pauseBtn->setFlags(nanogui::Button::ToggleButton);
    pauseBtn->setChangeCallback([&params](bool state) {
        params.paused = state;
        });

    if (is2DMode) {
        auto* resetBtn = new nanogui::Button(btnRow, "Respawn (R)");
        resetBtn->setCallback([&params]() {
            params.resetRequested = true;
            });
    }

    return win;
}
