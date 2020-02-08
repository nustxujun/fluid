#include "fluid.h"
#include "engine/RenderContext.h"

FluidSimulator::FluidSimulator()
{
	mName = "fluid";
	mPanel = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("panel");
	mPanel->drawCallback = [&settings = mSettings, &profile = mProfile](auto gui) {
		 ImGui::Text("cpu: %.3f ms, gpu: %.3f ms", profile->getCPUTime(), profile->getGPUTime());
		 
		 ImGui::SliderInt("diffusion", &settings.diffusionIteraionCount,0,100);
		 ImGui::SliderFloat("viscosity", &settings.viscosity, 0.0001f, 100.0f, "%.5f");
		 ImGui::SliderInt("pressure", &settings.pressureIterationCount, 0, 100);
		 ImGui::SliderFloat("intensity", &settings.intensity, 0.0f, 500.0f);

		 ImGui::SliderFloat("barrier radius", &settings.radius, 1.0f, 300.0f);

		 settings.reset = settings.reset || ImGui::Button("clear");
		 ImGui::DragFloat4("value", settings.clearValue.data(),10.0f,-500.0f,500.0f,"%.1f");
		 ImGui::Checkbox("velocity",&settings.velocityVisualization );
		 return true;
	};
	mShow = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("fluid");

	mImage = mShow->createChild<ImGuiOverlay::ImGuiImage>();
	mImage->drawCallback = [parent = (ImGuiOverlay::ImGuiWindow*)mShow, &settings = mSettings](auto ui) {
		if (ImGui::IsItemHovered(0))
		{
			parent->flags |= ImGuiWindowFlags_NoMove;
			ImGuiIO& io = ImGui::GetIO();
			const int btncount = 3;
			static bool inPressed[btncount] = {};

			for (int i = 0 ; i < btncount; ++i)
				inPressed[i] = ImGui::IsMouseDown(i);

			auto pos = ImGui::GetItemRectMin();
			pos = { io.MousePos.x - pos.x, io.MousePos.y - pos.y };

			auto toVec = [](float x, float y, float len)
			{
				if (x != 0 || y != 0)
				{
					float c = std::sqrt(std::pow(x, 2) + std::pow(y, 2));
					c = len / c;
					x *= c;
					y *= c;
				}
				return Vector2{x,y};
			};
			if (inPressed[2])
			{
				settings.barrierPos = {  pos.x,  pos.y };
			}
			if (inPressed[0])
			{
				static ImVec2 lastpos = pos;

				settings.forcePos = { pos.x, pos.y };

				settings.forceDir =  toVec(io.MouseDelta.x, io.MouseDelta.y, 50.0f);
				lastpos = pos;
				settings.color = {50,0};
			}
			else if (inPressed[1])
			{
				settings.forcePos = { pos.x, pos.y };
				settings.forceDir = toVec( settings.barrierPos[0] - pos.x, settings.barrierPos[1] - pos.y,10.0f);
				settings.color = {0,50 };
			}
			else
			{
				settings.forcePos = { };
				settings.forceDir = {};
			}
			
		}
		else
		{
			parent->flags &= ~ImGuiWindowFlags_NoMove;
		}

		return true;
	};

	auto renderer = Renderer::getSingleton();
	mProfile = renderer->createProfile();

	UINT w = 1024;
	UINT h = 576;
	mTexelSize = { 1.0f / w, 1.0f / w };
	for (UINT i = 0; i < 2; ++i)
		mRenderTargets[i] = ResourceHandle::create(Renderer::VT_RENDERTARGET, (UINT)w, (UINT)h, DXGI_FORMAT_R16G16B16A16_FLOAT);
	for (UINT i = 2; i < 5; ++i)
		mRenderTargets[i] = ResourceHandle::create(Renderer::VT_RENDERTARGET, (UINT)w, (UINT)h, DXGI_FORMAT_R16_FLOAT);

	mRenderTargets[Visualizaion] = ResourceHandle::create(Renderer::VT_RENDERTARGET, (UINT)w, (UINT)h, DXGI_FORMAT_R8G8B8A8_UNORM);
	mRenderTargets[Barrier] = ResourceHandle::create(Renderer::VT_RENDERTARGET, (UINT)w, (UINT)h, DXGI_FORMAT_R8G8B8A8_UNORM);

	{
		auto rs = Renderer::RenderState::Default;
		rs.setRenderTargetFormat({ DXGI_FORMAT_R16G16B16A16_FLOAT });
		// advect
		mQuads[0].init("advect.hlsl", rs);
		// diffusion
		mQuads[1].init("jacobi.hlsl", rs);

		rs.setRenderTargetFormat({ DXGI_FORMAT_R16_FLOAT });
		// divergence
		mQuads[2].init("divergence.hlsl",rs);
		// pressure
		mQuads[3].init("jacobi.hlsl", rs);
		rs.setRenderTargetFormat({ DXGI_FORMAT_R16G16B16A16_FLOAT });
		mQuads[4].init("substract_gradient.hlsl", rs);
		// visualize
		rs.setRenderTargetFormat({ DXGI_FORMAT_R8G8B8A8_UNORM });
		mQuads[5].init("visualize.hlsl",rs); 
		//barrier
		rs.setRenderTargetFormat({ DXGI_FORMAT_R8G8B8A8_UNORM });
		mQuads[6].init("draw_barrier.hlsl",rs);
		// init field
		rs.setRenderTargetFormat({ DXGI_FORMAT_R16G16B16A16_FLOAT });
		mQuads[7].init("init_field.hlsl", rs);


		for (auto& q: mQuads)
			q.setRect({0,0,(LONG)w ,(LONG)h});
	}

	mTimePoint = std::chrono::high_resolution_clock::now();
	mDeltaTime = 0.01f;
}

void FluidSimulator::setup()
{
}

void FluidSimulator::compile(const RenderGraph::Inputs& inputs)
{
	write(inputs[0]->getRenderTarget(0),IT_NOUSE,0);
	write(mRenderTargets[Velocity_SRC], IT_NOUSE,1);
}

void FluidSimulator::execute()
{
	mProfile->begin();
	
	auto point = std::chrono::high_resolution_clock::now();



	mDeltaTime = (float)(point - mTimePoint).count() / 1000000000.0;

	mTimePoint = point;

	initField();

	barrier();

	// advection
	advect();

	// diffusion
	diffuse((UINT)mSettings.diffusionIteraionCount);

	// pressure
	divergence();
	pressure((UINT)mSettings.pressureIterationCount);


	visualize();

	auto src = mRenderTargets[Visualizaion];


	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();
	cmdlist->transitionBarrier(src->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,0, true);
	((ImGuiOverlay::ImGuiImage*)mImage)->texture = src->getView()->getTexture();
	mProfile->end();

}

void FluidSimulator::initField()
{
	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();
	if ((mSettings.forcePos[0] != 0.0f || mSettings.forcePos[1] != 0.0f))
	{
		auto& src = mRenderTargets[Velocity_SRC];
		auto& dst = mRenderTargets[Velocity_DST];

		auto& init = mQuads[7];
		init.setVariable("force", mSettings.forceDir);
		init.setVariable("pos", mSettings.forcePos);
		init.setVariable("intensity", mSettings.intensity * 10.0f);
		init.setVariable("deltaTime", (float)mDeltaTime);
		init.setVariable("color", mSettings.color);




		cmdlist->transitionBarrier(src->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmdlist->transitionBarrier(dst->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET, 0, true);
		cmdlist->setRenderTarget(dst->getView());
		init.setResource("field", src->getView()->getTexture()->getShaderResource());
		RenderContext::getSingleton()->renderScreen(&init);

		std::swap(src, dst);
	}
	else if (mSettings.reset)
	{
		auto& srcV = mRenderTargets[Velocity_SRC];
		auto& srcP = mRenderTargets[Pressure_SRC];
		cmdlist->transitionBarrier(srcV->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdlist->transitionBarrier(srcP->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET, 0, true);
		cmdlist->clearRenderTarget(srcV->getView(), mSettings.clearValue);
		cmdlist->clearRenderTarget(srcP->getView(), {});
		mSettings.reset = false;
	}


}

void FluidSimulator::advect( )
{
	auto& src = mRenderTargets[Velocity_SRC];
	auto& dst = mRenderTargets[Velocity_DST];
	auto& barrier = mRenderTargets[Barrier];
	auto& advect = mQuads[0];



	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();

	cmdlist->transitionBarrier(src->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdlist->transitionBarrier(barrier->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	cmdlist->transitionBarrier(dst->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET, 0, true);



	{
		advect.setResource("V",src->getView()->getTexture()->getShaderResource());
		advect.setResource("Barrier", barrier->getView()->getTexture()->getShaderResource());

		advect.setVariable("texelSize", mTexelSize);
		advect.setVariable("deltaTime", (float)mDeltaTime);
		advect.setVariable("intensity", mSettings.intensity);


		cmdlist->setRenderTarget(dst->getView());

		RenderContext::getSingleton()->renderScreen(&advect);
	}



	std::swap(src, dst);
}

void FluidSimulator::diffuse(UINT iterCount)
{
	if (iterCount == 0)
		return;
	auto& src = mRenderTargets[0];
	auto& dst = mRenderTargets[1];

	auto& jacobi = mQuads[1];

	float alpha = 1.0f / std::max (float(mSettings.viscosity * mDeltaTime), FLT_MIN);
	float beta = 4 + alpha;

	jacobi.setVariable("alpha", alpha);
	jacobi.setVariable("invBeta", 1.0f / beta);

	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();
	for (UINT i = 0; i < iterCount; ++i)
	{
		cmdlist->transitionBarrier(src->getView()->getTexture(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmdlist->transitionBarrier(dst->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET,0,true);

		
		jacobi.setResource("X",src->getView()->getTexture()->getShaderResource());
		jacobi.setResource("B", src->getView()->getTexture()->getShaderResource());

		cmdlist->setRenderTarget(dst->getView());
		RenderContext::getSingleton()->renderScreen(&jacobi);
		std::swap(src, dst);
	}

}

void FluidSimulator::divergence()
{
	auto vel = mRenderTargets[Velocity_SRC];
	auto& desc = vel->getView()->getTexture()->getDesc();
	auto rt = mRenderTargets[Divergence];
	auto barrier = mRenderTargets[Barrier];
	auto& div = mQuads[2];

	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();
	cmdlist->transitionBarrier(rt->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdlist->transitionBarrier(barrier->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdlist->transitionBarrier(vel->getView()->getTexture(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,0,true);
	cmdlist->setRenderTarget(rt->getView());

	Vector2 half = { mTexelSize[0] * 0.5f, mTexelSize[1] * 0.5f };
	//div.setVariable("halfTexelSize", half);
	div.setResource("V", vel->getView()->getTexture()->getShaderResource());
	div.setResource("Barrier", barrier->getView()->getTexture()->getShaderResource());

	RenderContext::getSingleton()->renderScreen(&div);
}

void FluidSimulator::pressure(UINT iterCount)
{
	auto& srcV = mRenderTargets[0];
	auto& dstV = mRenderTargets[1];
	auto& srcP = mRenderTargets[2];
	auto& dstP = mRenderTargets[3];

	auto div = mRenderTargets[Divergence];

	auto& jacobi = mQuads[3];

	//float alpha = -(mTexelSize[0] * mTexelSize[1]) ;
	float alpha = -1.0f;
	float beta = 4.0f ;

	jacobi.setVariable("alpha", alpha);
	jacobi.setVariable("invBeta", 1.0f / beta);


	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();
	cmdlist->transitionBarrier(div->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	jacobi.setResource("B", div->getView()->getTexture()->getShaderResource());


	for (UINT i = 0; i < iterCount; ++i)
	{
		cmdlist->transitionBarrier(srcP->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmdlist->transitionBarrier(dstP->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET, 0, true);


		jacobi.setResource("X", srcP->getView()->getTexture()->getShaderResource());

		cmdlist->setRenderTarget(dstP->getView());
		RenderContext::getSingleton()->renderScreen(&jacobi);
		std::swap(srcP, dstP);
	}

	auto& subgrad = mQuads[4];

	cmdlist->transitionBarrier(srcV->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdlist->transitionBarrier(srcP->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdlist->transitionBarrier(dstV->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET,0,true);

	cmdlist->setRenderTarget(dstV->getView());
	subgrad.setResource("V", srcV->getView()->getTexture()->getShaderResource());
	subgrad.setResource("P", srcP->getView()->getTexture()->getShaderResource());
	Vector2 half = { mTexelSize[0] * 0.5f, mTexelSize[1] * 0.5f };
	//subgrad.setVariable("halfTexelSize", half);
	RenderContext::getSingleton()->renderScreen(&subgrad);
	std::swap(srcV, dstV);
}

void FluidSimulator::visualize()
{
	auto& visual = mQuads[5];
	auto& field = mRenderTargets[Velocity_SRC];
	auto& barrier = mRenderTargets[Barrier];
	auto& visualrt = mRenderTargets[Visualizaion];

	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();
	cmdlist->transitionBarrier(visualrt->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdlist->transitionBarrier(barrier->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdlist->transitionBarrier(field->getView()->getTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,0,true);
	cmdlist->setRenderTarget(visualrt->getView());
	visual.setVariable("velocity", mSettings.velocityVisualization);


	visual.setResource("field", field->getView()->getTexture()->getShaderResource());
	visual.setResource("barrier", barrier->getView()->getTexture()->getShaderResource());

	RenderContext::getSingleton()->renderScreen(&visual);

}

void FluidSimulator::barrier()
{
	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();
	auto & b = mRenderTargets[Barrier];
	auto& db = mQuads[6];
	cmdlist->transitionBarrier(b->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET, 0, true);
	cmdlist->setRenderTarget(b->getView());
	//if (mSettings.reset)
	//{
	//	cmdlist->clearRenderTarget(b->getView(),{});
	//}
	db.setVariable("radiusSQ", mSettings.radius * mSettings.radius);
	db.setVariable("pos", mSettings.barrierPos);
	db.setVariable("texelSize", mTexelSize);
	RenderContext::getSingleton()->renderScreen(&db);

}
