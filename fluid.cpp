#include "fluid.h"
#include "engine/RenderContext.h"

FluidSimulator::FluidSimulator()
{
	mName = "fluid";
	mPanel = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("panel");
	mPanel->drawCallback = [&settings = mSettings, &profile = mProfile](auto gui) {
		 ImGui::Text("cpu: %.3f ms, gpu: %.3f ms", profile->getCPUTime(), profile->getGPUTime());
		 
		 ImGui::SliderInt("diffusion", &settings.diffusionIteraionCount,0,100);
		 ImGui::SliderFloat("viscosity", &settings.viscosity, 0.0001f, 1.0f, "%.5f");
		 ImGui::SliderInt("pressure", &settings.pressureIterationCount, 0, 100);
		 ImGui::SliderFloat("force intensity", &settings.force, 0.0f, 500.0f);
		 ImGui::SliderInt("force count", &settings.count,1,100);

		 ImGui::SliderFloat("barrier radius", &settings.radius, 1.0f, 300.0f);

		 settings.reset = ImGui::Button("clear");
		 ImGui::DragFloat4("value", settings.clearValue.data(),10.0f,-500.0f,500.0f,"%.1f");

		 return true;
	};
	mShow = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("fluid");

	mImage = mShow->createChild<ImGuiOverlay::ImGuiImage>();
	mImage->drawCallback = [parent = (ImGuiOverlay::ImGuiWindow*)mShow, &settings = mSettings](auto ui) {
		if (ImGui::IsItemHovered(0))
		{
			parent->flags |= ImGuiWindowFlags_NoMove;
			ImGuiIO& io = ImGui::GetIO();
			static bool inPressed = false;
			if (ImGui::IsMouseDown(0) && !inPressed)
			{
				inPressed = true;
			}
			else if (ImGui::IsMouseReleased(0))
			{
				inPressed = false;
			}

			if (inPressed)
			{
				auto pos = ImGui::GetItemRectMin();
				settings.barrierPos = { io.MousePos.x - pos.x, io.MousePos.y - pos.y };
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

	UINT w = 640;
	UINT h = 360;
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
		for (auto& q: mQuads)
			q.setRect({0,0,(LONG)w ,(LONG)h});
	}
}

void FluidSimulator::setup()
{
}

void FluidSimulator::compile(const RenderGraph::Inputs& inputs)
{
	write(inputs[0]->getRenderTarget(0),IT_NOUSE);
}

void FluidSimulator::execute()
{
	mProfile->begin();
	static auto last = ::GetTickCount();
	auto cur = ::GetTickCount();
	auto delta = cur - last;
	if (delta <= 0)
	{
		mProfile->end();
		return;
	}
	mDeltaTime = (float)delta / 1000.0f;
	last = cur;

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


	if (mSettings.reset)
	{
		//advect.setResource("V",mInitial->getShaderResource());

		cmdlist->clearRenderTarget(dst->getView(),mSettings.clearValue);
	}
	else
	{
		advect.setResource("V",src->getView()->getTexture()->getShaderResource());
		advect.setResource("Barrier", barrier->getView()->getTexture()->getShaderResource());

		advect.setVariable("texelSize", mTexelSize);
		advect.setVariable("deltaTime", mDeltaTime);
		advect.setVariable("force", mSettings.force);
		advect.setVariable("count", mSettings.count);

		

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

	//float alpha = (mTexelSize[0] * mTexelSize[1]) /(mViscosity * mDeltaTime);
	float alpha = 1.0f / (mSettings.viscosity * mDeltaTime);
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
	div.setVariable("halfTexelSize", half);
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

	if (mSettings.reset)
	{
		cmdlist->transitionBarrier(dstP->getView()->getTexture(), D3D12_RESOURCE_STATE_RENDER_TARGET, 0, true);
		cmdlist->clearRenderTarget(dstP->getView(),{});
	}

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
	subgrad.setVariable("halfTexelSize", half);
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
	visual.setVariable("add",0.5f);
	visual.setVariable("scale", 0.5f);
	visual.setVariable("clampMin", -1.0f);
	visual.setVariable("clampMax", 1.0f);

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
