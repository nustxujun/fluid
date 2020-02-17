
#include "engine/Renderer.h"
#include "engine/DefaultFrame.h"
#include "engine/Pipeline.h"
#include "fluid.h"

#include "grass.h"
#include "particle.h"

class FluidPipeline final :public DefaultPipeline
{
	FluidSimulator mFluidPass;
	GrassPass mGrassPass;
	ParticlePass mPartilePass;
public:
	FluidPipeline()
	{
		mRenderSettingsWnd->visible = false;
		mVisualizationWnd->visible = false;
		mDebugInfo->visible = false;
		mProfileWindow->visible = false;
	}
	void update() override
	{


		RenderGraph graph;
		graph.begin() >> mFluidPass >> mGrassPass >> mPartilePass >> mGui >> mPresent;

		graph.setup();
		graph.compile();
		graph.execute();
	}

private:
};


class Frame final: public DefaultFrame
{
public:
	void init()
	{
		DefaultFrame::init(false);

		Renderer::getSingleton()->addSearchPath("shaders/");
		pipeline.reset();
		pipeline = decltype(pipeline)(new FluidPipeline);
		Renderer::getSingleton()->setVSync(true);
	}
};

int main()
{
	{
		Frame frame;
		frame.init();
		frame.update();
	
	}

	return 0;
}