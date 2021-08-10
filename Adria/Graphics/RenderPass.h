#pragma once
#include <variant>
#include <vector>
#include <d3d11.h>
#include "../Core/Definitions.h" 

namespace adria
{
	
	using rtv_t = ID3D11RenderTargetView*;
	using dsv_t = ID3D11DepthStencilView*;
	
	enum class LoadOp
	{
		eLoad,
		eClear,
		eDontCare
	};

	struct rtv_attachment_desc_t
	{
		rtv_t view;
		LoadOp load_op;
		std::array<f32,4> clear_color;
	};

	struct dsv_attachment_desc_t
	{
		dsv_t view;
		LoadOp load_op;
		f32 clear_depth;
		u8 clear_stencil;
	};

	
	struct render_pass_desc_t
	{
		std::vector<rtv_attachment_desc_t> rtv_attachments;
		std::optional<dsv_attachment_desc_t> dsv_attachment;
		u32 width, height;
	};


	
	class RenderPass
	{
		
	public:
		RenderPass() = default;

		RenderPass(render_pass_desc_t const& desc);

		void Begin(ID3D11DeviceContext* context);

		void End(ID3D11DeviceContext* context);

	private:
		u32 width, height;
		std::vector<rtv_t> render_targets;
		std::unordered_map<u64, std::array<f32,4>> clear_values;
		dsv_t depth_target = nullptr;
		bool depth_clear = false;
		f32 clear_depth = 0.0f;
		u8 clear_stencil = 0;
		
	};

}

