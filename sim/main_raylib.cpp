#include "../src/qubit_simulator.hpp"
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstring>

using namespace qbits;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ═══════════════════════════════════════════════════════════════
//  COLORS (Dark Theme)
// ═══════════════════════════════════════════════════════════════

const Color BG        = {12, 12, 18, 255};
const Color PANEL_BG  = {22, 22, 30, 255};
const Color PANEL_BD  = {40, 40, 55, 255};
const Color WIRE_COL  = {180, 180, 180, 255};
const Color GATE_BG   = {60, 70, 90, 255};
const Color GATE_HOV  = {80, 95, 120, 255};
const Color GATE_SEL  = {70, 130, 220, 255};
const Color GATE_DIS  = {40, 40, 45, 255};
const Color TXT_PRI   = {240, 240, 240, 255};
const Color TXT_SEC   = {160, 160, 160, 255};
const Color TXT_ACC   = {80, 200, 255, 255};
const Color RED_VEC   = {255, 60, 60, 255};
const Color ENT_RED   = {255, 50, 50, 200};
const Color ENT_GRN   = {80, 200, 80, 100};
const Color SPHERE_WF = {100, 180, 255, 50};   // ← more transparent

// ═══════════════════════════════════════════════════════════════
//  ENUMS & DATA
// ═══════════════════════════════════════════════════════════════

enum class GateType { H, X, Y, Z, S, T, RX, RY, RZ, CNOT };
enum class AppMode { Q1, Q2 };
enum class DialogState { NONE, ENTER_ANGLE, SELECT_QUBIT, SELECT_CNOT_CONTROL, SELECT_CNOT_TARGET };

struct CircuitGate {
    GateType type;
    int target = 0;
    int control = -1;
    float param = 0.0f;
};

struct AngleDialog {
    bool active = false;
    GateType gate;
    float angle = (float)M_PI / 2.0f;
    char radText[32] = "1.5708";
    char degText[32] = "90.00";
    char piText[32] = "0.50";
    int activeField = 0;
};

struct AppState {
    AppMode mode = AppMode::Q1;
    std::vector<CircuitGate> circuit;
    int currentStep = -1;
    int selectedGateIdx = -1;

    Qubit q1;
    TwoQubit q2;

    float animT = 0.0f;
    bool animating = false;
    Vector3 blochStart = {0.0f, 0.0f, 1.0f};
    Vector3 blochEnd = {0.0f, 0.0f, 1.0f};
    Vector3 blochStart2 = {0.0f, 0.0f, 1.0f};
    Vector3 blochEnd2 = {0.0f, 0.0f, 1.0f};

    AngleDialog angleDlg;
    DialogState dialogState = DialogState::NONE;
    GateType pendingGate;
    float pendingAngle = 0.0f;
    int cnotControl = -1;

    Camera3D camera = {
        {3.5f, 2.5f, 3.5f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f, CAMERA_PERSPECTIVE
    };

    Font monoFont;

    Rectangle topBar = {0.0f, 0.0f, 1200.0f, 50.0f};
    Rectangle circuitPanel = {0.0f, 50.0f, 336.0f, 670.0f};
    Rectangle vizPanel = {336.0f, 50.0f, 504.0f, 670.0f};
    Rectangle statePanel = {840.0f, 50.0f, 360.0f, 670.0f};
    Rectangle palettePanel = {0.0f, 720.0f, 1200.0f, 80.0f};
};

// ═══════════════════════════════════════════════════════════════
//  HELPERS
// ═══════════════════════════════════════════════════════════════

const char* GateName(GateType t) {
    switch(t) {
        case GateType::H: return "H";
        case GateType::X: return "X";
        case GateType::Y: return "Y";
        case GateType::Z: return "Z";
        case GateType::S: return "S";
        case GateType::T: return "T";
        case GateType::RX: return "RX";
        case GateType::RY: return "RY";
        case GateType::RZ: return "RZ";
        case GateType::CNOT: return "C";
    }
    return "?";
}

bool GateNeedsAngle(GateType t) {
    return t == GateType::RX || t == GateType::RY || t == GateType::RZ;
}

bool GateIs2QubitOnly(GateType t) {
    return t == GateType::CNOT;
}

float GateWidth(GateType t) {
    return GateNeedsAngle(t) ? 70.0f : 40.0f;
}

float Smoothstep(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

// ═══════════════════════════════════════════════════════════════
//  QUANTUM RECOMPUTATION (with proper reduced density matrices)
// ═══════════════════════════════════════════════════════════════

float GetEntanglement(const TwoQubit& reg);

void RecomputeState(AppState& app) {
    app.q1 = Qubit();
    app.q2 = TwoQubit();

    for (int i = 0; i <= app.currentStep && i < (int)app.circuit.size(); ++i) {
        const auto& g = app.circuit[i];
        if (app.mode == AppMode::Q1) {
            switch(g.type) {
                case GateType::H: app.q1.H(); break;
                case GateType::X: app.q1.X(); break;
                case GateType::Y: app.q1.Y(); break;
                case GateType::Z: app.q1.Z(); break;
                case GateType::S: app.q1.S(); break;
                case GateType::T: app.q1.T(); break;
                case GateType::RX: app.q1.RX(g.param); break;
                case GateType::RY: app.q1.RY(g.param); break;
                case GateType::RZ: app.q1.RZ(g.param); break;
                default: break;
            }
        } else {
            switch(g.type) {
                case GateType::H: app.q2.applySingle(Qubit::Gates::H, g.target); break;
                case GateType::X: app.q2.applySingle(Qubit::Gates::X, g.target); break;
                case GateType::Y: app.q2.applySingle(Qubit::Gates::Y, g.target); break;
                case GateType::Z: app.q2.applySingle(Qubit::Gates::Z, g.target); break;
                case GateType::S: app.q2.applySingle(Qubit::Gates::S, g.target); break;
                case GateType::T: app.q2.applySingle(Qubit::Gates::T, g.target); break;
                case GateType::RX: {
                    std::array<Complex, 4> rx = {
                        Complex(std::cos(g.param/2), 0.0), Complex(0.0, -std::sin(g.param/2)),
                        Complex(0.0, -std::sin(g.param/2)), Complex(std::cos(g.param/2), 0.0)
                    };
                    app.q2.applySingle(rx, g.target);
                    break;
                }
                case GateType::RY: {
                    std::array<Complex, 4> ry = {
                        Complex(std::cos(g.param/2), 0.0), Complex(-std::sin(g.param/2), 0.0),
                        Complex(std::sin(g.param/2), 0.0), Complex(std::cos(g.param/2), 0.0)
                    };
                    app.q2.applySingle(ry, g.target);
                    break;
                }
                case GateType::RZ: {
                    std::array<Complex, 4> rz = {
                        Complex(std::cos(g.param/2), -std::sin(g.param/2)), Complex(0.0, 0.0),
                        Complex(0.0, 0.0), Complex(std::cos(g.param/2), std::sin(g.param/2))
                    };
                    app.q2.applySingle(rz, g.target);
                    break;
                }
                case GateType::CNOT: app.q2.CNOT(g.control, g.target); break;
                default: break;
            }
        }
    }

    if (app.mode == AppMode::Q1) {
        auto [bx, by, bz] = app.q1.blochSphere();
        app.blochEnd = {(float)bx, (float)by, (float)bz};
    } else {
        // Full reduced density matrix Bloch vectors
        auto a = app.q2.amp(0);  // |00⟩
        auto b = app.q2.amp(1);  // |01⟩
        auto c = app.q2.amp(2);  // |10⟩
        auto d = app.q2.amp(3);  // |11⟩

        // Qubit 0 reduced: trace out qubit 1
        Complex rho0_off = a * std::conj(c) + b * std::conj(d);
        float x0 = 2.0f * (float)rho0_off.real();
        float y0 = 2.0f * (float)rho0_off.imag();
        float z0 = (float)(std::norm(a) + std::norm(b) - std::norm(c) - std::norm(d));
        
        float ent = GetEntanglement(app.q2);
        float scale = 1.0f - ent; // Shrink to zero when entanglement hits max

        app.blochEnd2 = {x0 * scale, y0 * scale, z0 * scale};

        // Qubit 1 reduced: trace out qubit 0
        Complex rho1_off = a * std::conj(b) + c * std::conj(d);
        float x1 = 2.0f * (float)rho1_off.real();
        float y1 = 2.0f * (float)rho1_off.imag();
        float z1 = (float)(std::norm(a) + std::norm(c) - std::norm(b) - std::norm(d));

        ent = GetEntanglement(app.q2);
        scale = 1.0f - ent; // Shrink to zero when entanglement hits max

        app.blochEnd = {x1 * scale, y1 * scale, z1 * scale};
    }
}

float GetEntanglement(const TwoQubit& reg) {
    auto a = reg.amp(0), b = reg.amp(1);
    auto c = reg.amp(2), d = reg.amp(3);
    return 2.0f * (float)std::abs(a * d - b * c);
}

// ═══════════════════════════════════════════════════════════════
//  LAYOUT
// ═══════════════════════════════════════════════════════════════

void RecalcLayout(AppState& app) {
    int W = GetScreenWidth();
    int H = GetScreenHeight();

    app.topBar = {0.0f, 0.0f, (float)W, 50.0f};
    app.circuitPanel = {0.0f, 50.0f, (float)W * 0.28f, (float)(H - 130)};
    app.vizPanel = {(float)W * 0.28f, 50.0f, (float)W * 0.42f, (float)(H - 130)};
    app.statePanel = {(float)W * 0.70f, 50.0f, (float)W * 0.30f, (float)(H - 130)};
    app.palettePanel = {0.0f, (float)(H - 80), (float)W, 80.0f};
}

// ═══════════════════════════════════════════════════════════════
//  DRAWING: PANELS
// ═══════════════════════════════════════════════════════════════

void DrawPanel(Rectangle r, const char* title) {
    DrawRectangleRec(r, PANEL_BG);
    DrawRectangleLinesEx(r, 1, PANEL_BD);
    if (title) {
        DrawText(title, (int)r.x + 10, (int)r.y + 8, 18, TXT_ACC);
    }
}

// ═══════════════════════════════════════════════════════════════
//  DRAWING: TOP BAR
// ═══════════════════════════════════════════════════════════════

void DrawTopBar(AppState& app) {
    Rectangle r = app.topBar;
    DrawRectangleRec(r, Color{18, 18, 26, 255});
    DrawLine(0, 50, (int)r.width, 50, PANEL_BD);

    DrawText("qBITS LIVE", 15, 15, 24, TXT_PRI);

    Rectangle modeBtn = {200, 10, 100, 30};
    DrawRectangleRec(modeBtn, app.mode == AppMode::Q1 ? GATE_SEL : GATE_BG);
    DrawRectangleLinesEx(modeBtn, 1, PANEL_BD);
    DrawText(app.mode == AppMode::Q1 ? "1-Qubit" : "2-Qubit", 
             (int)modeBtn.x + 10, (int)modeBtn.y + 7, 16, TXT_PRI);
    if (CheckCollisionPointRec(GetMousePosition(), modeBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        app.mode = (app.mode == AppMode::Q1) ? AppMode::Q2 : AppMode::Q1;
        app.circuit.clear();
        app.currentStep = -1;
        app.selectedGateIdx = -1;
        RecomputeState(app);
        app.blochStart = app.blochEnd;
        app.blochStart2 = app.blochEnd2;
    }

    Rectangle resetBtn = {320, 10, 70, 30};
    DrawRectangleRec(resetBtn, GATE_BG);
    DrawRectangleLinesEx(resetBtn, 1, PANEL_BD);
    DrawText("Reset", (int)resetBtn.x + 12, (int)resetBtn.y + 7, 16, TXT_PRI);
    if (CheckCollisionPointRec(GetMousePosition(), resetBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        app.circuit.clear();
        app.currentStep = -1;
        app.selectedGateIdx = -1;
        RecomputeState(app);
        app.blochStart = app.blochEnd;
        app.blochStart2 = app.blochEnd2;
    }

    Rectangle delBtn = {400, 10, 50, 30};
    bool canDel = app.selectedGateIdx >= 0;
    DrawRectangleRec(delBtn, canDel ? Color{200, 60, 60, 255} : GATE_DIS);
    DrawRectangleLinesEx(delBtn, 1, PANEL_BD);
    DrawText("Del", (int)delBtn.x + 10, (int)delBtn.y + 7, 16, canDel ? TXT_PRI : TXT_SEC);
    if (canDel && CheckCollisionPointRec(GetMousePosition(), delBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        app.circuit.erase(app.circuit.begin() + app.selectedGateIdx);
        app.selectedGateIdx = -1;
        if (app.currentStep >= (int)app.circuit.size()) app.currentStep = (int)app.circuit.size() - 1;
        RecomputeState(app);
    }

    float sx = 500;
    Rectangle backBtn = {sx, 10, 40, 30};
    Rectangle fwdBtn = {sx + 45, 10, 40, 30};
    Rectangle playBtn = {sx + 90, 10, 40, 30};

    DrawRectangleRec(backBtn, GATE_BG); DrawText("<", (int)backBtn.x + 15, (int)backBtn.y + 6, 18, TXT_PRI);
    DrawRectangleRec(fwdBtn, GATE_BG); DrawText(">", (int)fwdBtn.x + 15, (int)fwdBtn.y + 6, 18, TXT_PRI);
    DrawRectangleRec(playBtn, GATE_BG); DrawText(">>", (int)playBtn.x + 8, (int)playBtn.y + 6, 16, TXT_PRI);

    if (CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (app.currentStep > -1) { app.currentStep--; RecomputeState(app); app.animating = true; app.animT = 0; }
    }
    if (CheckCollisionPointRec(GetMousePosition(), fwdBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (app.currentStep < (int)app.circuit.size() - 1) { app.currentStep++; RecomputeState(app); app.animating = true; app.animT = 0; }
    }

    DrawText(TextFormat("Step: %d / %d", app.currentStep + 1, (int)app.circuit.size()), 
             (int)sx + 145, 17, 16, TXT_SEC);
}

// ═══════════════════════════════════════════════════════════════
//  DRAWING: CIRCUIT PANEL
// ═══════════════════════════════════════════════════════════════

void DrawCircuitPanel(AppState& app) {
    DrawPanel(app.circuitPanel, "CIRCUIT");
    Rectangle r = app.circuitPanel;

    int numWires = (app.mode == AppMode::Q1) ? 1 : 2;
    float wireY[2];
    float wireSpacing = (r.height - 60) / (numWires + 1);
    for (int w = 0; w < numWires; ++w) {
        wireY[w] = r.y + 40 + wireSpacing * (w + 1);
        DrawLine((int)r.x + 10, (int)wireY[w], (int)(r.x + r.width - 10), (int)wireY[w], WIRE_COL);
        DrawText(TextFormat("q%d", w), (int)r.x + 15, (int)wireY[w] - 25, 14, TXT_SEC);
    }

    float x = r.x + 50;
    for (int i = 0; i < (int)app.circuit.size(); ++i) {
        const auto& g = app.circuit[i];
        float gw = GateWidth(g.type);
        float gy = wireY[g.target];

        if (g.type == GateType::CNOT && numWires == 2) {
            float cy = wireY[g.control];
            float ty = wireY[g.target];
            DrawLine((int)(x + gw/2), (int)cy, (int)(x + gw/2), (int)ty, WIRE_COL);
            DrawCircle((int)(x + gw/2), (int)cy, 6, TXT_PRI);
        }

        Rectangle gateRect = {x, gy - 15, gw, 30};
        bool isSelected = (i == app.selectedGateIdx);
        bool isActive = (i <= app.currentStep);
        Color bg = isSelected ? GATE_SEL : (isActive ? GATE_BG : GATE_DIS);
        DrawRectangleRec(gateRect, bg);
        DrawRectangleLinesEx(gateRect, 1, isSelected ? TXT_ACC : PANEL_BD);

        const char* name = GateName(g.type);
        int tw = MeasureText(name, 16);
        DrawText(name, (int)(gateRect.x + gateRect.width/2 - tw/2), (int)(gateRect.y + 7), 16, isActive ? TXT_PRI : TXT_SEC);

        if (GateNeedsAngle(g.type)) {
            char angleStr[16];
            snprintf(angleStr, 16, "%.2f", g.param);
            int aw = MeasureText(angleStr, 10);
            DrawText(angleStr, (int)(gateRect.x + gateRect.width/2 - aw/2), (int)(gateRect.y + 22), 10, TXT_SEC);
        }

        if (CheckCollisionPointRec(GetMousePosition(), gateRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)
            && app.dialogState == DialogState::NONE) {
            app.selectedGateIdx = i;
        }

        x += gw + 15;
    }

    if (app.dialogState == DialogState::SELECT_QUBIT || app.dialogState == DialogState::SELECT_CNOT_CONTROL) {
        DrawText("Click a wire...", (int)r.x + 10, (int)r.y + (int)r.height - 30, 16, YELLOW);
    } else if (app.dialogState == DialogState::SELECT_CNOT_TARGET) {
        DrawText("Click target wire...", (int)r.x + 10, (int)r.y + (int)r.height - 30, 16, YELLOW);
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && app.dialogState != DialogState::NONE) {
        for (int w = 0; w < numWires; ++w) {
            Rectangle wireRect = {r.x, wireY[w] - 20, r.width, 40};
            if (CheckCollisionPointRec(GetMousePosition(), wireRect)) {
                if (app.dialogState == DialogState::SELECT_QUBIT) {
                    app.circuit.push_back({app.pendingGate, w, -1, app.pendingAngle});
                    app.currentStep = (int)app.circuit.size() - 1;
                    app.dialogState = DialogState::NONE;
                    RecomputeState(app);
                    app.animating = true; app.animT = 0;
                } else if (app.dialogState == DialogState::SELECT_CNOT_CONTROL) {
                    app.cnotControl = w;
                    app.dialogState = DialogState::SELECT_CNOT_TARGET;
                } else if (app.dialogState == DialogState::SELECT_CNOT_TARGET && w != app.cnotControl) {
                    app.circuit.push_back({GateType::CNOT, w, app.cnotControl, 0});
                    app.currentStep = (int)app.circuit.size() - 1;
                    app.dialogState = DialogState::NONE;
                    RecomputeState(app);
                    app.animating = true; app.animT = 0;
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  DRAWING: 3D BLOCH SPHERE
// ═══════════════════════════════════════════════════════════════

void DrawBlochSphere(Vector3 center, float radius, Vector3 arrow, Color arrowCol) {
    // State vector tip
    Vector3 tip = Vector3{
        center.x + arrow.x * radius,
        center.y + arrow.y * radius,
        center.z + arrow.z * radius
    };

    // === DRAW ARROW FIRST (so it shows through wireframe) ===
    Vector3 dir = Vector3Subtract(tip, center);
    float len = Vector3Length(dir);
    if (len > 0.01f) {
        Vector3 unitDir = Vector3Scale(dir, 1.0f / len);
        Vector3 shaftEnd = Vector3Subtract(tip, Vector3Scale(unitDir, radius * 0.15f));

        // Thin shaft
        DrawCylinderEx(center, shaftEnd, radius * 0.015f, radius * 0.015f, 12, arrowCol);
        
        // Arrow head cone
        DrawCylinderEx(shaftEnd, tip, radius * 0.05f, 0.0f, 16, arrowCol);
    }

    // Origin point
    DrawSphere(center, radius * 0.02f, WHITE);

    // === WIREFRAME ONLY ===
    DrawSphereWires(center, radius, 16, 16, SPHERE_WF);

    // Thick Equatorial Ring
    for (int i = 0; i < 48; ++i) {
        float t1 = (float)i / 48.0f * 2.0f * PI;
        float t2 = (float)(i + 1) / 48.0f * 2.0f * PI;
        Vector3 a = Vector3{center.x + radius*cosf(t1), center.y, center.z + radius*sinf(t1)};
        Vector3 b = Vector3{center.x + radius*cosf(t2), center.y, center.z + radius*sinf(t2)};
        DrawCylinderEx(a, b, radius*0.008f, radius*0.008f, 6, Color{150, 150, 150, 120});
    }

    // Axes
    Vector3 xEnd = {center.x + radius*1.2f, center.y, center.z};
    DrawCylinderEx(Vector3{center.x - radius*1.2f, center.y, center.z}, xEnd, radius*0.005f, radius*0.005f, 8, Color{255, 80, 80, 180});
    
    Vector3 yEnd = {center.x, center.y + radius*1.2f, center.z};
    DrawCylinderEx(Vector3{center.x, center.y - radius*1.2f, center.z}, yEnd, radius*0.005f, radius*0.005f, 8, Color{80, 255, 80, 180});
    
    Vector3 zEnd = {center.x, center.y, center.z + radius*1.2f};
    DrawCylinderEx(Vector3{center.x, center.y, center.z - radius*1.2f}, zEnd, radius*0.005f, radius*0.005f, 8, Color{80, 80, 255, 180});
}

void Draw3DViz(AppState& app) {
    DrawPanel(app.vizPanel, nullptr);
    Rectangle r = app.vizPanel;

    if (app.animating) {
        app.animT += GetFrameTime() * 4.0f;
        if (app.animT >= 1.0f) { app.animT = 1.0f; app.animating = false; }
    }

    float t = app.animating ? Smoothstep(app.animT) : 1.0f;
    Vector3 curr = Vector3{
        app.blochStart.x + (app.blochEnd.x - app.blochStart.x) * t,
        app.blochStart.y + (app.blochEnd.y - app.blochStart.y) * t,
        app.blochStart.z + (app.blochEnd.z - app.blochStart.z) * t
    };
    Vector3 curr2 = Vector3{
        app.blochStart2.x + (app.blochEnd2.x - app.blochStart2.x) * t,
        app.blochStart2.y + (app.blochEnd2.y - app.blochStart2.y) * t,
        app.blochStart2.z + (app.blochEnd2.z - app.blochStart2.z) * t
    };

    if (!app.animating && t >= 1.0f) {
        app.blochStart = app.blochEnd;
        app.blochStart2 = app.blochEnd2;
    }

    BeginMode3D(app.camera);

    if (app.mode == AppMode::Q1) {
        Vector3 center = Vector3{0.0f, 0.0f, 0.0f};
        Vector3 renderCurr = {curr.x, curr.z, -curr.y};
        DrawBlochSphere(center, 1.0f, renderCurr, RED_VEC);
    } else {
        Vector3 left = Vector3{-1.2f, 0.0f, 0.0f};
        Vector3 right = Vector3{1.2f, 0.0f, 0.0f};

        Vector3 renderCurr  = {curr.x, curr.z, -curr.y};
        Vector3 renderCurr2 = {curr2.x, curr2.z, -curr2.y};

        DrawBlochSphere(left, 0.7f, renderCurr2, RED_VEC);
        DrawBlochSphere(right, 0.7f, renderCurr, Color{255, 150, 50, 255});

        float ent = GetEntanglement(app.q2);
        Color lineCol = ent > 0.01f ? ENT_RED : ENT_GRN;
        DrawLine3D(left, right, lineCol);
    }

    EndMode3D();

    // After 3D projection is over, draw the axis labels
    auto drawLabels = [&](Vector3 center, float r) {
        Vector2 posX = GetWorldToScreen(Vector3{center.x + r*1.3f, center.y, center.z}, app.camera);
        // Remember physics Z mapped to Raylib Y
        Vector2 posY = GetWorldToScreen(Vector3{center.x, center.y + r*1.3f, center.z}, app.camera);
        // Remember physics Y mapped to Raylib -Z, but labels are X, Z, Y conceptually, so we just use the raw coordinates here. 
        // Let's just use the canonical X Y Z strings that match our visual coordinates
        Vector2 posZ = GetWorldToScreen(Vector3{center.x, center.y, center.z + r*1.3f}, app.camera);
        
        DrawText("X", (int)posX.x - 5, (int)posX.y - 5, 16, Color{255, 120, 120, 255});
        DrawText("Z", (int)posY.x - 5, (int)posY.y - 5, 16, Color{120, 255, 120, 255}); // Mapped Z (Physics Z -> Raylib Y)
        DrawText("Y", (int)posZ.x - 5, (int)posZ.y - 5, 16, Color{120, 120, 255, 255}); // Mapped Y (Physics Y -> Raylib Z)
    };

    if (app.mode == AppMode::Q1) {
        drawLabels(Vector3{0.0f, 0.0f, 0.0f}, 1.0f);
    } else {
        drawLabels(Vector3{-1.2f, 0.0f, 0.0f}, 0.7f);
        drawLabels(Vector3{1.2f, 0.0f, 0.0f}, 0.7f);
    }

    if (app.mode == AppMode::Q2) {
        float ent = GetEntanglement(app.q2);
        const char* entStr = ent > 0.01f ? "ENTANGLED" : "SEPARABLE";
        Color entC = ent > 0.01f ? ENT_RED : ENT_GRN;
        int tw = MeasureText(entStr, 20);
        DrawText(entStr, (int)(r.x + r.width/2 - tw/2), (int)(r.y + r.height - 40), 20, entC);
    }

    DrawText("(left-click drag to rotate)", (int)(r.x + 10), (int)(r.y + r.height - 20), 12, TXT_SEC);
}

// ═══════════════════════════════════════════════════════════════
//  DRAWING: STATE PANEL
// ═══════════════════════════════════════════════════════════════

void DrawStatePanel(AppState& app) {
    DrawPanel(app.statePanel, "STATE");
    Rectangle r = app.statePanel;
    int y = (int)r.y + 35;
    int x = (int)r.x + 12;

    if (app.mode == AppMode::Q1) {
        auto a = app.q1.alpha();
        auto b = app.q1.beta();
        
        if (app.monoFont.texture.id > 0) {
            DrawTextEx(app.monoFont, TextFormat("a = %.3f %+.3fi", a.real(), a.imag()), Vector2{(float)x, (float)y}, 16, 0, TXT_PRI); y += 22;
            DrawTextEx(app.monoFont, TextFormat("b = %.3f %+.3fi", b.real(), b.imag()), Vector2{(float)x, (float)y}, 16, 0, TXT_PRI); y += 30;
        } else {
            DrawText(TextFormat("a = %.3f %+.3fi", a.real(), a.imag()), x, y, 14, TXT_PRI); y += 22;
            DrawText(TextFormat("b = %.3f %+.3fi", b.real(), b.imag()), x, y, 14, TXT_PRI); y += 30;
        }

        DrawText("Probabilities:", x, y, 14, TXT_ACC); y += 20;
        float p0 = app.q1.prob0(), p1 = app.q1.prob1();
        int bw = (int)r.width - 30;
        int n0 = (int)(p0 * bw);
        DrawRectangle(x, y, n0, 12, Color{100, 200, 100, 200});
        DrawRectangle(x + n0, y, bw - n0, 12, Color{40, 40, 50, 255});
        DrawText(TextFormat("|0> %.1f%%", p0*100), x + bw + 5, y, 12, TXT_SEC);
        y += 18;
        int n1 = (int)(p1 * bw);
        DrawRectangle(x, y, n1, 12, Color{200, 100, 100, 200});
        DrawRectangle(x + n1, y, bw - n1, 12, Color{40, 40, 50, 255});
        DrawText(TextFormat("|1> %.1f%%", p1*100), x + bw + 5, y, 12, TXT_SEC);
        y += 30;

        auto [bx, by, bz] = app.q1.blochSphere();
        DrawText("Bloch Sphere:", x, y, 14, TXT_ACC); y += 20;
        if (app.monoFont.texture.id > 0) {
            DrawTextEx(app.monoFont, TextFormat("x = %+.3f", bx), Vector2{(float)x, (float)y}, 16, 0, TXT_PRI); y += 18;
            DrawTextEx(app.monoFont, TextFormat("y = %+.3f", by), Vector2{(float)x, (float)y}, 16, 0, TXT_PRI); y += 18;
            DrawTextEx(app.monoFont, TextFormat("z = %+.3f", bz), Vector2{(float)x, (float)y}, 16, 0, TXT_PRI); y += 18;
        } else {
            DrawText(TextFormat("x = %+.3f", bx), x, y, 14, TXT_PRI); y += 18;
            DrawText(TextFormat("y = %+.3f", by), x, y, 14, TXT_PRI); y += 18;
            DrawText(TextFormat("z = %+.3f", bz), x, y, 14, TXT_PRI); y += 18;
        }
    } else {
        DrawText("Probabilities:", x, y, 14, TXT_ACC); y += 20;
        float probs[4];
        const char* labels[] = {"|00>", "|01>", "|10>", "|11>"};
        for (int i = 0; i < 4; ++i) {
            probs[i] = app.q2.prob(i);
            int bw = (int)r.width - 50;
            int ni = (int)(probs[i] * bw);
            DrawRectangle(x, y, ni, 10, Color{100, 180, 255, 180});
            DrawRectangle(x + ni, y, bw - ni, 10, Color{40, 40, 50, 255});
            DrawText(TextFormat("%s %.1f%%", labels[i], probs[i]*100), x + bw + 5, y, 11, TXT_SEC);
            y += 14;
        }
        y += 10;

        float ent = GetEntanglement(app.q2);
        DrawText("Entanglement:", x, y, 14, TXT_ACC); y += 20;
        DrawText(TextFormat("Strength: %.4f", ent), x, y, 14, ent > 0.01f ? ENT_RED : ENT_GRN);
    }
}

// ═══════════════════════════════════════════════════════════════
//  DRAWING: GATE PALETTE
// ═══════════════════════════════════════════════════════════════

void DrawGatePalette(AppState& app) {
    DrawPanel(app.palettePanel, nullptr);
    Rectangle r = app.palettePanel;

    GateType gates[] = {GateType::H, GateType::X, GateType::Y, GateType::Z, 
                        GateType::S, GateType::T, GateType::CNOT,
                        GateType::RX, GateType::RY, GateType::RZ};
    int numGates = 10;
    float btnW = 60, btnH = 40;
    float spacing = 15;
    float startX = r.x + 20;
    float startY = r.y + 20;

    for (int i = 0; i < numGates; ++i) {
        GateType gt = gates[i];
        Rectangle btn = {startX + i * (btnW + spacing), startY, btnW, btnH};

        bool enabled = true;
        if (app.mode == AppMode::Q1 && GateIs2QubitOnly(gt)) enabled = false;

        bool hovered = CheckCollisionPointRec(GetMousePosition(), btn);
        Color bg = enabled ? (hovered ? GATE_HOV : GATE_BG) : GATE_DIS;
        DrawRectangleRec(btn, bg);
        DrawRectangleLinesEx(btn, 1, PANEL_BD);

        const char* name = GateName(gt);
        int tw = MeasureText(name, 18);
        DrawText(name, (int)(btn.x + btnW/2 - tw/2), (int)(btn.y + 10), 18, enabled ? TXT_PRI : TXT_SEC);

        if (enabled && hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) 
            && app.dialogState == DialogState::NONE) {
            if (GateNeedsAngle(gt)) {
                app.angleDlg.active = true;
                app.angleDlg.gate = gt;
                app.angleDlg.angle = (float)M_PI / 2.0f;
                snprintf(app.angleDlg.radText, 32, "%.4f", app.angleDlg.angle);
                snprintf(app.angleDlg.degText, 32, "%.2f", app.angleDlg.angle * 180.0f / (float)M_PI);
                snprintf(app.angleDlg.piText, 32, "%.4f", app.angleDlg.angle / (float)M_PI);
                app.dialogState = DialogState::ENTER_ANGLE;
            } else if (app.mode == AppMode::Q2 && !GateIs2QubitOnly(gt)) {
                app.pendingGate = gt;
                app.pendingAngle = 0;
                app.dialogState = DialogState::SELECT_QUBIT;
            } else if (GateIs2QubitOnly(gt)) {
                app.dialogState = DialogState::SELECT_CNOT_CONTROL;
                app.cnotControl = -1;
            } else {
                app.circuit.push_back({gt, 0, -1, 0});
                app.currentStep = (int)app.circuit.size() - 1;
                RecomputeState(app);
                app.animating = true; app.animT = 0;
            }
        }
    }

    DrawText("Click gate to append to circuit", (int)(r.x + 20), (int)(r.y + 65), 12, TXT_SEC);
}

// ═══════════════════════════════════════════════════════════════
//  DRAWING: ANGLE DIALOG
// ═══════════════════════════════════════════════════════════════

void DrawAngleDialog(AppState& app) {
    if (app.dialogState != DialogState::ENTER_ANGLE) return;

    int W = GetScreenWidth(), H = GetScreenHeight();

    DrawRectangle(0, 0, W, H, Color{0, 0, 0, 180});

    Rectangle box = {(float)W/2 - 200, (float)H/2 - 160, 400, 320};
    DrawRectangleRec(box, PANEL_BG);
    DrawRectangleLinesEx(box, 2, TXT_ACC);

    DrawText("ENTER ANGLE", (int)box.x + 20, (int)box.y + 20, 24, TXT_PRI);
    DrawText(TextFormat("Gate: %s", GateName(app.angleDlg.gate)), 
             (int)box.x + 20, (int)box.y + 55, 18, TXT_SEC);

    const char* labels[] = {"Radians:", "Degrees:", "x pi:"};
    char* texts[] = {app.angleDlg.radText, app.angleDlg.degText, app.angleDlg.piText};
    float y = box.y + 90;

    for (int i = 0; i < 3; ++i) {
        DrawText(labels[i], (int)box.x + 20, (int)y, 14, TXT_SEC);
        Rectangle field = {box.x + 100, y, 200, 28};
        bool active = (app.angleDlg.activeField == i);
        DrawRectangleRec(field, active ? Color{40, 50, 70, 255} : Color{30, 30, 40, 255});
        DrawRectangleLinesEx(field, 1, active ? TXT_ACC : PANEL_BD);
        DrawText(texts[i], (int)field.x + 8, (int)field.y + 6, 14, TXT_PRI);

        if (CheckCollisionPointRec(GetMousePosition(), field) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            app.angleDlg.activeField = i;
        }
        y += 40;
    }

    int field = app.angleDlg.activeField;
    char* text = texts[field];
    int len = (int)strlen(text);

    int ch = GetCharPressed();
    while (ch > 0) {
        if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-') {
            if (len < 30) { text[len] = (char)ch; text[len+1] = '\0'; }
        }
        ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && len > 0) text[len-1] = '\0';
    if (IsKeyPressed(KEY_TAB)) app.angleDlg.activeField = (field + 1) % 3;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_TAB)) {
        float val = (float)atof(text);
        if (field == 0) {
            app.angleDlg.angle = val;
            snprintf(app.angleDlg.degText, 32, "%.2f", val * 180.0f / (float)M_PI);
            snprintf(app.angleDlg.piText, 32, "%.4f", val / (float)M_PI);
        } else if (field == 1) {
            app.angleDlg.angle = val * (float)M_PI / 180.0f;
            snprintf(app.angleDlg.radText, 32, "%.4f", app.angleDlg.angle);
            snprintf(app.angleDlg.piText, 32, "%.4f", app.angleDlg.angle / (float)M_PI);
        } else {
            app.angleDlg.angle = val * (float)M_PI;
            snprintf(app.angleDlg.radText, 32, "%.4f", app.angleDlg.angle);
            snprintf(app.angleDlg.degText, 32, "%.2f", app.angleDlg.angle * 180.0f / (float)M_PI);
        }
    }

    const char* presets[] = {"pi/4", "pi/2", "pi", "2pi"};
    float pvals[] = {(float)M_PI/4, (float)M_PI/2, (float)M_PI, 2*(float)M_PI};
    float px = box.x + 20;
    for (int i = 0; i < 4; ++i) {
        Rectangle pbtn = {px, box.y + 220, 70, 28};
        DrawRectangleRec(pbtn, GATE_BG);
        DrawRectangleLinesEx(pbtn, 1, PANEL_BD);
        int tw = MeasureText(presets[i], 14);
        DrawText(presets[i], (int)(pbtn.x + 35 - tw/2), (int)pbtn.y + 6, 14, TXT_PRI);
        if (CheckCollisionPointRec(GetMousePosition(), pbtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            app.angleDlg.angle = pvals[i];
            snprintf(app.angleDlg.radText, 32, "%.4f", pvals[i]);
            snprintf(app.angleDlg.degText, 32, "%.2f", pvals[i] * 180.0f / (float)M_PI);
            snprintf(app.angleDlg.piText, 32, "%.4f", pvals[i] / (float)M_PI);
        }
        px += 85;
    }

    Rectangle applyBtn = {box.x + 200, box.y + 270, 80, 30};
    Rectangle cancelBtn = {box.x + 290, box.y + 270, 80, 30};
    DrawRectangleRec(applyBtn, Color{60, 160, 80, 255});
    DrawRectangleRec(cancelBtn, Color{160, 60, 60, 255});
    DrawText("Apply", (int)applyBtn.x + 18, (int)applyBtn.y + 7, 16, TXT_PRI);
    DrawText("Cancel", (int)cancelBtn.x + 12, (int)cancelBtn.y + 7, 16, TXT_PRI);

    if (CheckCollisionPointRec(GetMousePosition(), applyBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        app.pendingGate = app.angleDlg.gate;
        app.pendingAngle = app.angleDlg.angle;
        app.dialogState = DialogState::NONE;
        if (app.mode == AppMode::Q2) {
            app.dialogState = DialogState::SELECT_QUBIT;
        } else {
            app.circuit.push_back({app.pendingGate, 0, -1, app.pendingAngle});
            app.currentStep = (int)app.circuit.size() - 1;
            RecomputeState(app);
            app.animating = true; app.animT = 0;
        }
    }
    if (CheckCollisionPointRec(GetMousePosition(), cancelBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        app.dialogState = DialogState::NONE;
    }
    if (IsKeyPressed(KEY_ESCAPE)) app.dialogState = DialogState::NONE;
}

// ═══════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1200, 800, "qBITS Live — Quantum Circuit Visualizer");
    SetTargetFPS(60);

    AppState app;
    app.monoFont = LoadFontEx("C:/Windows/Fonts/consola.ttf", 20, 0, 250);
    
    RecomputeState(app);
    app.blochStart = app.blochEnd;

    while (!WindowShouldClose()) {
        RecalcLayout(app);

        // Manual camera orbit with left mouse in 3D panel
        Vector2 mouse = GetMousePosition();
        Rectangle viz = app.vizPanel;
        bool inViz = (mouse.x >= viz.x && mouse.x <= viz.x + viz.width &&
                      mouse.y >= viz.y && mouse.y <= viz.y + viz.height);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && inViz && app.dialogState == DialogState::NONE) {
            Vector2 delta = GetMouseDelta();
            float yaw = -delta.x * 0.005f;
            float pitch = -delta.y * 0.005f;

            Vector3 forward = Vector3Subtract(app.camera.target, app.camera.position);
            float dist = Vector3Length(forward);
            forward = Vector3Normalize(forward);

            float cosY = cosf(yaw), sinY = sinf(yaw);
            Vector3 newForward = {
                forward.x * cosY - forward.z * sinY,
                forward.y,
                forward.x * sinY + forward.z * cosY
            };

            float cosP = cosf(pitch), sinP = sinf(pitch);
            Vector3 right = Vector3CrossProduct(newForward, app.camera.up);
            right = Vector3Normalize(right);

            Vector3 finalForward = {
                newForward.x * cosP + right.x * sinP,
                newForward.y * cosP + right.y * sinP,
                newForward.z * cosP + right.z * sinP
            };

            app.camera.position = Vector3Subtract(app.camera.target, 
                                                  Vector3Scale(finalForward, dist));
        }

        if (IsKeyPressed(KEY_DELETE) && app.selectedGateIdx >= 0) {
            app.circuit.erase(app.circuit.begin() + app.selectedGateIdx);
            app.selectedGateIdx = -1;
            if (app.currentStep >= (int)app.circuit.size()) app.currentStep = (int)app.circuit.size() - 1;
            RecomputeState(app);
        }

        BeginDrawing();
        ClearBackground(BG);

        DrawTopBar(app);
        DrawCircuitPanel(app);
        Draw3DViz(app);
        DrawStatePanel(app);
        DrawGatePalette(app);
        DrawAngleDialog(app);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}