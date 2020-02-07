#include "engine/RenderGraph.h"
#include "engine/Quad.h"
#include "engine/ImguiOverlay.h"
#include <chrono> 

class FluidSimulator final: public RenderGraph::RenderPass
{
	enum
	{
		Velocity_SRC,
		Velocity_DST,

		Pressure_SRC,
		Pressure_DST,

		Divergence,
		Visualizaion,
		Barrier,

		NUM_RTS
	};
public:
	FluidSimulator();
	void setup();
	void compile(const RenderGraph::Inputs& inputs);
	void execute();

private:
	void initField();
	void advect();
	void diffuse(UINT iterCount);
	void divergence();
	void pressure(  UINT iterCount);
	void visualize();
	void barrier();
private:
	std::array<Quad, 8> mQuads;
	std::array<ResourceHandle::Ptr, NUM_RTS> mRenderTargets;
	ImGuiOverlay::ImGuiObject* mShow;
	ImGuiOverlay::ImGuiObject* mImage;
	ImGuiOverlay::ImGuiObject* mPanel;
	Renderer::Profile::Ref mProfile;

	float mDeltaTime;
	std::chrono::high_resolution_clock::time_point mTimePoint;

	Vector2 mTexelSize;
	struct Settings
	{
		int diffusionIteraionCount = 10;
		int pressureIterationCount = 50;
		float viscosity = 0.01f;
		float intensity = 100.0f;
		bool reset = true;
		Color clearValue = {};
		Vector2 barrierPos = {300,150};
		float radius = 10.0f;
		Vector2 forcePos = {};
		Vector2 forceDir = {};
		Vector2 color = {};
		bool velocityVisualization = false;
	} mSettings;

};