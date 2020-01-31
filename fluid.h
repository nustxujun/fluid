#include "engine/RenderGraph.h"
#include "engine/Quad.h"
#include "engine/ImguiOverlay.h"
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
	void advect();
	void diffuse(UINT iterCount);
	void divergence();
	void pressure(  UINT iterCount);
	void visualize();
	void barrier();
private:
	std::array<Quad, 7> mQuads;
	std::array<ResourceHandle::Ptr, NUM_RTS> mRenderTargets;
	ImGuiOverlay::ImGuiObject* mShow;
	ImGuiOverlay::ImGuiObject* mImage;
	ImGuiOverlay::ImGuiObject* mPanel;
	Renderer::Profile::Ref mProfile;

	float mDeltaTime = 0.01f;
	Vector2 mTexelSize;
	struct Settings
	{
		int diffusionIteraionCount = 10;
		int pressureIterationCount = 50;
		float viscosity = 0.01f;
		float force = 100.0f;
		int count = 10;
		bool reset = true;
		Color clearValue = {};
		Vector2 barrierPos = {};
		float radius = 10.0f;
	} mSettings;

};