#include "particle.h"
#include "engine/RenderContext.h"

ParticlePass::ParticlePass()
{
	mName = "particle";

	mSize = 512;
	mCount = mSize * mSize;

	mPanel = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("particle panel");
	mPanel->drawCallback = [&visible = mVisible](auto gui) {
		ImGui::Checkbox("particles", &visible);
		return true;
	};
	{
		auto settings = Renderer::RenderState::Default;
		settings.setRenderTargetFormat({ DXGI_FORMAT_R32G32B32A32_FLOAT });
		mQuad.init("particle_pos.hlsl",settings);
	}
	auto renderer = Renderer::getSingleton();
	for (auto& p: mParticles)
	{
		p = renderer->createTexture(mSize, mSize, 1, DXGI_FORMAT_R32G32B32A32_FLOAT,1,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		p->createShaderResource();
		p->createRenderTargetView(nullptr);
	}



	std::vector<Vector4> initdata(mCount);
	for (auto& p : initdata)
	{
		p = { float(rand() % mSize) / mSize * 2.0f - 1,float(rand() % mSize) / mSize * 2.0f - 1, 0, 0};
	}
	renderer->updateTexture(mParticles[0],0,initdata.data(),sizeof(Vector4) * initdata.size(), false);

	auto vs = renderer->compileShaderFromFile("particle.hlsl", "vs", SM_VS);
	auto ps = renderer->compileShaderFromFile("particle.hlsl", "ps", SM_PS);
	std::vector<Renderer::Shader::Ptr> shaders = { vs, ps };
	vs->enable32BitsConstants(true);
	vs->registerStaticSampler({
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0,0,
		D3D12_COMPARISON_FUNC_NEVER,
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
		0,
		D3D12_FLOAT32_MAX,
		0,0,
		D3D12_SHADER_VISIBILITY_ALL
		});

	Renderer::RenderState rs = Renderer::RenderState::Default;
	rs.setPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);

	mPSO = renderer->createPipelineState(shaders, rs);

}

void ParticlePass::setup()
{
}

void ParticlePass::compile(const RenderGraph::Inputs& inputs)
{
	read(inputs[0]->getRenderTarget(1));
	write(inputs[0]->getRenderTarget(0));
	write(inputs[0]->getRenderTarget(1), IT_NOUSE, 1);
}

void ParticlePass::execute()
{
	if (!mVisible)
		return;
	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();
	cmdlist->setPipelineState(mPSO);
	auto size = renderer->getSize();
	cmdlist->setViewport({ 0,0,(float)size[0],(float)size[1],0,1.0f });
	cmdlist->setScissorRect({ 0,0,size[0],size[1] });
	mPSO->setVSResource("P", mParticles[0]->getShaderResource());
	mPSO->setVSVariable("size", &mSize);
	cmdlist->setPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	cmdlist->drawInstanced(mCount);

	mQuad.setResource("P", mParticles[0]->getShaderResource());
	mQuad.setResource("V", mShaderResources[0]->getView()->getShaderResource());
	

	cmdlist->transitionBarrier(mParticles[1],D3D12_RESOURCE_STATE_RENDER_TARGET,0,true);
	cmdlist->setRenderTarget(mParticles[1]);
	mQuad.setRect({0,0,(LONG)mSize, (LONG)mSize});
	RenderContext::getSingleton()->renderScreen(&mQuad);

	std::swap(mParticles[0], mParticles[1]);
}
