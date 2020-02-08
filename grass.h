#pragma once
#include "engine/RenderGraph.h"
#include "engine/Quad.h"
#include "engine/ImguiOverlay.h"

class GrassPass final : public RenderGraph::RenderPass
{
public:
	GrassPass();
	void setup();
	void compile(const RenderGraph::Inputs& inputs);
	void execute();

private:
	UINT mNumGrass = 100;
	ImGuiOverlay::ImGuiObject* mPanel;
	Renderer::PipelineState::Ref mPSO;
	bool mVisible = false;
};