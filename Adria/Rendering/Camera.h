#pragma once
#include <utility>

namespace adria
{
	struct CameraParameters
	{
		Float aspect_ratio;
		Float near_plane;
		Float far_plane;
		Float fov;
		Float speed;
		Float sensitivity;
		Vector3 position;
		Vector3 look_at;
	};

	class Camera
	{
	public:
		Camera() = default;
		explicit Camera(CameraParameters const&);

		Matrix View() const;
		Matrix Proj() const;
		Matrix ViewProj() const;
		BoundingFrustum Frustum() const;

		Vector3 Position() const
		{
			return position;
		}
		Vector3 Up() const
		{
			return up_vector;
		}
		Vector3 Right() const
		{
			return right_vector;
		}
		Vector3 Forward() const
		{
			return look_vector;
		}

		Float Near() const;
		Float Far() const;
		Float Fov() const;
		Float AspectRatio() const;

		void SetPosition(Vector3 const& pos);
		void SetNearAndFar(Float n, Float f);
		void SetAspectRatio(Float ar);
		void SetFov(Float fov);

		void Zoom(Sint32 increment);
		void OnResize(Uint32 w, Uint32 h);
		void Tick(Float dt);
		void Enable(Bool _enabled) { enabled = _enabled; }
	private:

		Vector3 position;
		Vector3 right_vector;
		Vector3 up_vector;
		Vector3 look_vector;
		Matrix view_matrix;
		Matrix projection_matrix;

		Float aspect_ratio;
		Float fov;
		Float near_plane, far_plane;
		Float speed;
		Float sensitivity;
		Bool  enabled;

	private:
		void UpdateViewMatrix();
		void Strafe(Float dt);
		void Walk(Float dt);
		void Jump(Float dt);
		void Pitch(Sint64 dy);
		void Yaw(Sint64 dx);
		void SetLens(Float fov, Float aspect, Float zn, Float zf);
		void SetView();
	};

}