
#include "engine/Renderer.h"
#include "engine/DefaultFrame.h"
#include "engine/Pipeline.h"
#include "fluid.h"

class FluidPipeline final :public DefaultPipeline
{
	FluidSimulator mFluidPass;
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
		graph.begin() >> mFluidPass >> mGui >> mPresent;

		graph.setup();
		graph.compile();
		graph.execute();
	}
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
	}
};

int main()
{
	{
		Frame frame;
		try {
			frame.init();
			frame.update();
		}
		catch (...)
		{
			frame.rendercmd.invalid();
		}
	}

	return 0;
}