#pragma once
#include <lvk/LVK.h>
#include <glm/ext.hpp>
#include <deque>
#include <vector>
#include <implot/implot.h>

struct LinearGraph
{
    const char* name = nullptr;    size_t maxPoints;  
    std::deque<float> graph;
};

void init_graph(LinearGraph* graph, const char* name, size_t maxGraphPoints = 256);
void add_point_graph(LinearGraph* graph, float value);
void render_graph(LinearGraph* graph, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const glm::vec4& color = glm::vec4(1.0));


inline void init_graph(LinearGraph* graph, const char* name, size_t maxGraphPoints)
{
    graph->name = name;
    graph->maxPoints = maxGraphPoints;
    graph->graph.clear();
}

inline void add_point_graph(LinearGraph* graph, float value)
{
    graph->graph.push_back(value);
    if (graph->graph.size() > graph->maxPoints) { graph->graph.erase(graph->graph.begin()); }
}

inline void render_graph(LinearGraph* graph, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const glm::vec4& color)
{
    LVK_PROFILER_FUNCTION();

    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::min();

    for (float f : graph->graph) {
        if (f < minVal)
            minVal = f;
        if (f > maxVal)
            maxVal = f;
    }

    const float range = maxVal - minVal;

    float valX = 0.0;

    std::vector<float> dataX_;
    std::vector<float> dataY_;
    dataX_.reserve(graph->graph.size());
    dataY_.reserve(graph->graph.size());

    for (float f : graph->graph) {
        const float valY = (f - minVal) / range;
        valX += 1.0f / graph->maxPoints;
        dataX_.push_back(valX);
        dataY_.push_back(valY);
    }

    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::Begin(
        graph->name, nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

    if (ImPlot::BeginPlot(graph->name, ImVec2(width, height), ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(color.r, color.g, color.b, color.a));
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0, 0, 0, 0));
        ImPlot::PlotLine("#line", dataX_.data(), dataY_.data(), (int)graph->graph.size(), ImPlotLineFlags_None);
        ImPlot::PopStyleColor(2);
        ImPlot::EndPlot();
    }

    ImGui::End();
}