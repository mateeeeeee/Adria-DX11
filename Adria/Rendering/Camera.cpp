#include <algorithm>
#include "Camera.h"
#include "Core/Input.h"
#include "Math/Constants.h"

using namespace DirectX;

namespace adria
{

	Camera::Camera(CameraParameters const& desc) : position{ desc.position }, right_vector{ 1.0f,0.0f,0.0f }, up_vector{ 0.0f,1.0f,0.0f },
		look_vector{ desc.look_at }, aspect_ratio{ desc.aspect_ratio }, fov{ desc.fov }, near_plane{ desc.near_plane }, far_plane{ desc.far_plane },
		speed{ desc.speed }, sensitivity{ desc.sensitivity }
	{
		SetView();
		SetLens(fov, aspect_ratio, near_plane, far_plane);
	}

	Float Camera::Near() const
	{
		return near_plane;
	}
	Float Camera::Far() const
	{
		return far_plane;
	}
	Float Camera::Fov() const
	{
		return fov;
	}
	Float Camera::AspectRatio() const
	{
		return aspect_ratio;
	}

	void Camera::Tick(Float dt)
	{
		if (!enabled) return;
		Input& input = g_Input;
		if (input.GetKey(KeyCode::Space)) return;

		Float speed_factor = 1.0f;

		if (input.GetKey(KeyCode::ShiftLeft)) speed_factor *= 5.0f;
		if (input.GetKey(KeyCode::CtrlLeft))  speed_factor *= 0.2f;

		if (input.GetKey(KeyCode::W)) Walk(speed_factor * dt);
		if (input.GetKey(KeyCode::S)) Walk(-speed_factor * dt);
		if (input.GetKey(KeyCode::A)) Strafe(-speed_factor * dt);
		if (input.GetKey(KeyCode::D)) Strafe(speed_factor * dt);
		if (input.GetKey(KeyCode::Q)) Jump(speed_factor * dt);
		if (input.GetKey(KeyCode::E)) Jump(-speed_factor * dt);
		if (input.GetKey(KeyCode::MouseRight))
		{
			Float dx = input.GetMouseDeltaX();
			Float dy = input.GetMouseDeltaY();
			Pitch((Sint64)dy);
			Yaw((Sint64)dx);
		}
		UpdateViewMatrix();
	}
	void Camera::Zoom(Sint32 increment)
	{
		fov -= XMConvertToRadians(increment * 1.0f);
		fov = std::clamp(fov, 0.00005f, pi_div_2<Float>);
		SetLens(fov, aspect_ratio, near_plane, far_plane);
	}
	void Camera::OnResize(Uint32 w, Uint32 h)
	{
		SetAspectRatio(static_cast<Float>(w) / h);
	}

	void Camera::SetAspectRatio(Float ar)
	{
		aspect_ratio = ar;
		SetLens(fov, aspect_ratio, near_plane, far_plane);
	}
	void Camera::SetFov(Float _fov)
	{
		fov = _fov;
		SetLens(fov, aspect_ratio, near_plane, far_plane);
	}
	void Camera::SetNearAndFar(Float n, Float f)
	{
		near_plane = n;
		far_plane = f;
		SetLens(fov, aspect_ratio, near_plane, far_plane);
	}
	void Camera::SetPosition(Vector3 const& pos)
	{
		position = pos;
	}

	Matrix Camera::View() const
	{
		return view_matrix;
	}
	Matrix Camera::Proj() const
	{
		return projection_matrix;
	}
	Matrix Camera::ViewProj() const
	{
		return view_matrix * projection_matrix;
	}
	BoundingFrustum Camera::Frustum() const
	{
		BoundingFrustum frustum(Proj());
		frustum.Transform(frustum, view_matrix.Invert());
		return frustum;
	}

	void Camera::UpdateViewMatrix()
	{
		look_vector.Normalize();
		up_vector = look_vector.Cross(right_vector);
		up_vector.Normalize();
		right_vector = up_vector.Cross(look_vector);
		SetView();
	}

	void Camera::Strafe(Float dt)
	{
		position += dt * speed * right_vector;
	}
	void Camera::Walk(Float dt)
	{
		position += dt * speed * look_vector;
	}
	void Camera::Jump(Float dt)
	{
		position += dt * speed * up_vector;
	}
	void Camera::Pitch(Sint64 dy)
	{
		Matrix R = Matrix::CreateFromAxisAngle(right_vector, sensitivity * XMConvertToRadians((Float)dy));
		up_vector = Vector3::TransformNormal(up_vector, R);
		look_vector = Vector3::TransformNormal(look_vector, R);
	}
	void Camera::Yaw(Sint64 dx)
	{
		Matrix R = Matrix::CreateRotationY(sensitivity * XMConvertToRadians((Float)dx));

		right_vector = Vector3::TransformNormal(right_vector, R);
		up_vector = Vector3::TransformNormal(up_vector, R);
		look_vector = Vector3::TransformNormal(look_vector, R);
	}
	void Camera::SetLens(Float fov, Float aspect, Float zn, Float zf)
	{
		projection_matrix = XMMatrixPerspectiveFovLH(fov, aspect, zn, zf);
	}
	void Camera::SetView()
	{
		view_matrix = XMMatrixLookToLH(position, look_vector, up_vector);
	}
}