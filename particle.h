#pragma once
#include "engine/RenderGraph.h"
#include "engine/Quad.h"
#include "engine/ImguiOverlay.h"

class ParticlePass final : public RenderGraph::RenderPass
{
public:
	ParticlePass();
	void setup();
	void compile(const RenderGraph::Inputs& inputs);
	void execute();

private:
	UINT mSize = 128;
	UINT mCount ;
	Renderer::PipelineState::Ref mPSO;
	std::array<Renderer::Resource::Ref ,2> mParticles;
	Quad mQuad; 
};