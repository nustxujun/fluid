#include "grass.h"

GrassPass::GrassPass()
{
	mName = "grass";

	mPanel = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("grass panel");
	mPanel->drawCallback = [&visible = mVisible](auto gui) {
		ImGui::Checkbox("grass", &visible);
		return true;
	};

	auto renderer = Renderer::getSingleton();
	auto vs = renderer->compileShaderFromFile("grass.hlsl", "vs", SM_VS);
	auto gs = renderer->compileShaderFromFile("grass.hlsl", "gs", SM_GS);
	auto ps = renderer->compileShaderFromFile("grass.hlsl", "ps", SM_PS);
	std::vector<Renderer::Shader::Ptr> shaders = { vs,gs, ps };
	vs->enable32BitsConstants(true);
	vs->registerStaticSampler({
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
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
	gs->enable32BitsConstants(true);

	Renderer::RenderState rs = Renderer::RenderState::Default;
	rs.setPrimitiveType (D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);

	mPSO = renderer->createPipelineState(shaders,rs);

}

void GrassPass::setup()
{
}

void GrassPass::compile(const RenderGraph::Inputs& inputs)
{
	read(inputs[0]->getRenderTarget(1));
	write(inputs[0]->getRenderTarget(0));
	write(inputs[0]->getRenderTarget(1),IT_NOUSE,1);

}

void GrassPass::execute()
{
	if (!mVisible)
		return ;
	auto renderer = Renderer::getSingleton();
	auto cmdlist = renderer->getCommandList();

	cmdlist->setPipelineState(mPSO);
	auto size = renderer->getSize();
	cmdlist->setViewport({0,0,(float)size[0],(float)size[1],0,1.0f});
	cmdlist->setScissorRect({0,0,size[0],size[1]});
	mPSO->setResource(Renderer::Shader::ST_GEOMETRY,"V", mShaderResources[0]->getView()->getShaderResource());



	mPSO->setVSVariable("numInstance", &mNumGrass);
	cmdlist->setPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	cmdlist->drawInstanced(mNumGrass);
	

}
