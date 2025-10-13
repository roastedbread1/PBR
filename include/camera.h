#pragma once
 #define GLM_ENABLE_EXPERIMENTAL 
#include  <assert.h>
#include <algorithm>

#include <utils_math.h>
#include <glm/gtx/euler_angles.hpp>

enum CameraPositionerType
{
	CAMERA_POSITIONER_FIRST_PERSON,
	CAMERA_POSITIONER_MOVE_TO,
};

struct Camera;
struct CameraPositioner_FirstPerson;
struct CameraPositioner_MoveTo;

struct FirstPersonMovement
{
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool fastSpeed = false;
};
struct CameraPositioner_FirstPerson
{
	// this is already optimal (i think) because the size is 88 bytes (msvc) so its pretty much correct + 1 alignment
	// so I don't think fiddling with the layout matters that much, of course packing them differently for cache locality is possible
	// but im not going to bother with finding out the access pattern for this
	glm::quat cameraOrientation;   // 16 bytes

	// 12 bytes
	glm::vec3 cameraPosition;      
	glm::vec3 moveSpeed;           
	glm::vec3 up;                  

	// 8 bytes
	FirstPersonMovement movement;  
	glm::vec2 mousePos;            

	// 4-bytes
	float mouseSpeed;
	float acceleration;
	float damping;
	float maxSpeed;
	float fastCoef;
};

struct CameraPositioner_MoveTo
{
	glm::mat4 currentTransform; //64 byte
	
	//12 bytes
	glm::vec3 positionCurrent;
	glm::vec3 positionDesired;
	glm::vec3 anglesCurrent;
	glm::vec3 anglesDesired;
	glm::vec3 dampingEulerAngles;
	
	
	float dampingLinear; // 4 byte
};

struct CameraPositioner
{
	CameraPositionerType type;

	union
	{
		CameraPositioner_FirstPerson firstPerson;
		CameraPositioner_MoveTo moveTo;
	};
};


struct Camera
{
	CameraPositioner positioner;
	glm::mat4 proj;
};

void init_camera(Camera* camera);
void init_camera_first_person(Camera* camera, const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up);
void init_camera_move_to(Camera* camera, const glm::vec3& pos, const glm::vec3& angles);
glm::mat4 get_view_matrix_camera(const Camera* camera);
glm::vec3 get_position_camera(const Camera* camera);
glm::mat4 get_proj_matrix_camera(const Camera* camera);
void set_proj_matrix_camera(Camera* camera, const glm::mat4& proj);


//first person func
void init_first_person(CameraPositioner_FirstPerson* pos);
void init_with_target_first_person(CameraPositioner_FirstPerson* pos, const glm::vec3& cameraPos, const glm::vec3& target, const glm::vec3 & up);
void update_first_person(CameraPositioner_FirstPerson* pos, double deltaSeconds, const glm::vec2& mousePos, bool mousePressed);
void set_position_first_person(CameraPositioner_FirstPerson* pos, const glm::vec3& position);
void set_speed_first_person(CameraPositioner_FirstPerson* pos, const glm::vec3& speed);
void reset_mouse_position_first_person(CameraPositioner_FirstPerson* pos, const glm::vec2& p);
void set_up_vector_first_person(CameraPositioner_FirstPerson* pos, const glm::vec3& up);
void look_at_first_person_(CameraPositioner_FirstPerson* pos, const glm::vec3 cameraPos, const glm::vec3 target, const glm::vec3 up);
glm::mat4 get_view_matrix_first_person(const CameraPositioner_FirstPerson* pos);
glm::vec3 get_position_first_person(const CameraPositioner_FirstPerson* pos);

//move to funcs
void init_move_to(CameraPositioner_MoveTo* pos, const glm::vec3& position, const glm::vec3& angles);
void update_move_to(CameraPositioner_MoveTo* pos, float deltaSeconds, const glm::vec2& mousePos, bool mousePressed);
void set_position_move_to(CameraPositioner_MoveTo* pos, const glm::vec3& position);
void set_angles_move_to(CameraPositioner_MoveTo* pos, float pitch, float pan, float roll);
void set_angles_vec_move_to(CameraPositioner_MoveTo* pos, const glm::vec3& angles);
void set_desired_position_move_to(CameraPositioner_MoveTo* pos, const glm::vec3& position);
void set_desired_angles_move_to(CameraPositioner_MoveTo* pos, float pitch, float pan, float roll);
glm::mat4 get_view_matrix_move_to(const CameraPositioner_MoveTo* pos);
glm::vec3 get_position_move_to(const CameraPositioner_MoveTo* pos);

float clip_angle(float d);
glm::vec3 clip_angles(const glm::vec3& angles);
glm::vec3 angle_delta(const glm::vec3 anglesCurrent, const glm::vec3 anglesDesired);


///============================================================================
///============================================================================


inline void init_camera(Camera* camera)
{
	camera->proj = glm::mat4(1.0f);
}

inline void init_camera_first_person(Camera* camera, const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up)
{
	init_camera(camera);
	camera->positioner.type = CAMERA_POSITIONER_FIRST_PERSON;
	init_with_target_first_person(&camera->positioner.firstPerson, pos, target, up);
}

inline void init_camera_move_to(Camera* camera, const glm::vec3& pos, const glm::vec3& angles)
{
	init_camera(camera);
	camera->positioner.type = CAMERA_POSITIONER_MOVE_TO;
	init_move_to(&camera->positioner.moveTo, pos, angles);
}

inline glm::mat4 get_view_matrix_camera(const Camera* camera)
{
	switch (camera->positioner.type)
	{
	case CAMERA_POSITIONER_FIRST_PERSON:
		return get_view_matrix_first_person(&camera->positioner.firstPerson);
	case CAMERA_POSITIONER_MOVE_TO:
			return get_view_matrix_move_to(&camera->positioner.moveTo);
	default:
		return glm::mat4(1.0f);
	}
}

inline glm::vec3 get_position_camera(const Camera* camera)
{
	switch (camera->positioner.type)
	{
	case CAMERA_POSITIONER_FIRST_PERSON:
		return get_position_first_person(&camera->positioner.firstPerson);
	case CAMERA_POSITIONER_MOVE_TO:
		return get_position_move_to(&camera->positioner.moveTo);
	default:
		return glm::vec3(0.0f);
	}
}

inline glm::mat4 get_proj_matrix_camera(const Camera* camera)
{
	return camera->proj;
}

inline void set_proj_matrix_camera(Camera* camera, const glm::mat4& proj)
{
	camera->proj = proj;
}


inline void init_first_person(CameraPositioner_FirstPerson* pos)
{
	pos->mousePos = glm::vec2(0);
	pos->cameraPosition = glm::vec3(0.0f, 10.0f, 10.f);
	pos->cameraOrientation = glm::quat(glm::vec3(0));
	pos->moveSpeed = glm::vec3(0.0f);
	pos->up = glm::vec3(0.0f, 0.0f, 1.0f);
	pos->mouseSpeed = 4.0f;
	pos->acceleration = 150.0f;
	pos->damping = 0.2f;
	pos->maxSpeed = 10.0f;
	pos->fastCoef = 10.0f;

	FirstPersonMovement movement = {};
	pos->movement = movement;
}

inline void init_with_target_first_person(CameraPositioner_FirstPerson* pos, const glm::vec3& cameraPos, const glm::vec3& target, const glm::vec3& up)
{
	init_first_person(pos);
	pos->cameraPosition = cameraPos;
	pos->cameraOrientation = glm::lookAt(cameraPos, target, up);
	pos->up = up;
}

inline  void update_first_person(CameraPositioner_FirstPerson* pos, double deltaSeconds, const glm::vec2& mousePos, bool mousePressed)
{
	if (mousePressed)
	{
		const glm::vec2 delta = mousePos - pos->mousePos;
		const glm::quat deltaQuat = glm::quat(glm::vec3(-pos->mouseSpeed * delta.y, pos->mouseSpeed * delta.x, 0.0f));
		pos->cameraOrientation = deltaQuat * pos->cameraOrientation;
		pos->cameraOrientation = glm::normalize(pos->cameraOrientation);
		set_up_vector_first_person(pos, pos->up);
	}

	pos->mousePos = mousePos;

	const glm::mat4 v = glm::mat4_cast(pos->cameraOrientation);
	const glm::vec3 forward = -glm::vec3(v[0][2], v[1][2], v[2][2]);
	const glm::vec3 right = glm::vec3(v[0][0], v[1][0], v[2][0]);
	const glm::vec3 up = glm::cross(right, forward);

	glm::vec3 accel(0.0f);

	if (pos->movement.forward)	{ accel += forward; }
	if (pos->movement.backward) { accel -= forward;	}
	if (pos->movement.left)		{ accel -= right ;	}
	if (pos->movement.right)	{ accel += right;	}
	if (pos->movement.up)		{ accel += up;		}
	if (pos->movement.down)		{ accel -= up;		}

	if (pos->movement.fastSpeed) { accel *= pos->fastCoef; }

	if (accel == glm::vec3(0))
	{
		pos->moveSpeed -= pos->moveSpeed * std::min((1.0f / pos->damping) * static_cast<float>(deltaSeconds), 1.0f);
	}
	else
	{
		pos->moveSpeed += accel * pos->acceleration * static_cast<float>(deltaSeconds);
		const float maxSpeed = pos->movement.fastSpeed ? pos->maxSpeed * pos->fastCoef : pos->maxSpeed;

		if (glm::length(pos->moveSpeed) > maxSpeed)
		{
			pos->moveSpeed = glm::normalize(pos->moveSpeed) * maxSpeed;
		}

		pos->cameraPosition += pos->moveSpeed * static_cast<float>(deltaSeconds);
	}

}

inline void set_position_first_person(CameraPositioner_FirstPerson* pos, const glm::vec3& position)
{
	pos->cameraPosition = position;
}

inline void set_speed_first_person(CameraPositioner_FirstPerson* pos, const glm::vec3& speed)
{
	pos->moveSpeed = speed;
}

inline void reset_mouse_position_first_person(CameraPositioner_FirstPerson* pos, const glm::vec2& p)
{
	pos->mousePos = p;
}

inline void set_up_vector_first_person(CameraPositioner_FirstPerson* pos, const glm::vec3& up)
{
	const glm::mat4 view = get_view_matrix_first_person(pos);
	const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
	pos->cameraOrientation = glm::lookAt(pos->cameraPosition, pos->cameraPosition + dir, up);
}


inline void look_at_first_person_(CameraPositioner_FirstPerson* pos, const glm::vec3 cameraPos, const glm::vec3 target, const glm::vec3 up)
{
	pos->cameraPosition = cameraPos;
	pos->cameraOrientation = glm::lookAt(cameraPos, target, up);
}

inline glm::mat4 get_view_matrix_first_person(const CameraPositioner_FirstPerson* pos)
{
	const glm::mat4 t = glm::translate(glm::mat4(1.0f), -pos->cameraPosition);
	const glm::mat4 r = glm::mat4_cast(pos->cameraOrientation);
	return r * t;
}

inline glm::vec3 get_position_first_person(const CameraPositioner_FirstPerson* pos)
{
	return pos->cameraPosition;
}


inline void init_move_to(CameraPositioner_MoveTo* pos, const glm::vec3& position, const glm::vec3& angles)
{
	pos->positionCurrent = position;
	pos->positionDesired = position;
	pos->anglesCurrent = angles;
	pos->anglesDesired = angles;
	pos->currentTransform = glm::mat4(1.0f);
	pos->dampingLinear = 10.0f;
	pos->dampingEulerAngles = glm::vec3(5.0f, 5.0f, 5.0f);
}

inline void update_move_to(CameraPositioner_MoveTo* pos, float deltaSeconds, const glm::vec2& mousePos, bool mousePressed)
{
	pos->positionCurrent += pos->dampingLinear * deltaSeconds * (pos->positionDesired - pos->positionCurrent);

	pos->anglesCurrent = clip_angles(pos->anglesCurrent);
	pos->anglesDesired = clip_angles(pos->anglesDesired);

	pos->anglesCurrent -= angle_delta(pos->anglesCurrent, pos->anglesDesired) * pos->dampingEulerAngles * deltaSeconds;

	pos->anglesCurrent = clip_angles(pos->anglesCurrent);

	const glm::vec3 a = glm::radians(pos->anglesCurrent);
	pos->currentTransform = glm::translate(glm::yawPitchRoll(a.y, a.x, a.z), -pos->positionCurrent);

}

inline void set_position_move_to(CameraPositioner_MoveTo* pos, const glm::vec3& position)
{
	pos->positionCurrent = position;
}

inline void set_angles_move_to(CameraPositioner_MoveTo* pos, float pitch, float pan, float roll)
{
	pos->anglesCurrent = glm::vec3(pitch, pan, roll);
}

inline void set_angles_vec_move_to(CameraPositioner_MoveTo* pos, const glm::vec3& angles)
{
	pos->anglesCurrent = angles;
}

inline void set_desired_position_move_to(CameraPositioner_MoveTo* pos, const glm::vec3& position)
{
	pos->positionDesired = position;
}

inline void set_desired_angles_move_to(CameraPositioner_MoveTo* pos, float pitch, float pan, float roll)
{
	pos->anglesDesired = glm::vec3(pitch, pan, roll);
}

inline glm::mat4 get_view_matrix_move_to(const CameraPositioner_MoveTo* pos)
{
	return pos->currentTransform;
}

inline glm::vec3 get_position_move_to(const CameraPositioner_MoveTo* pos)
{
	return pos->positionCurrent;
}

inline float clip_angle(float d)
{
	if (d < -180.0f) { return d + 360.0f; }
	if (d > +180.0f) { return d - 360.0f; }

	return d;
}

inline glm::vec3 clip_angles(const glm::vec3& angles)
{
	return glm::vec3(
		std::fmod(angles.x, 360.0f),
		std::fmod(angles.y, 360.0f),
		std::fmod(angles.z, 360.0f)
	);
}

inline glm::vec3 angle_delta(const glm::vec3 anglesCurrent, const glm::vec3 anglesDesired)
{
	const glm::vec3 d = clip_angles(anglesCurrent) - clip_angles(anglesDesired);
	return glm::vec3(clip_angle(d.x), clip_angle(d.y), clip_angle(d.z));
}