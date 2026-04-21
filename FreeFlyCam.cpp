#include "FreeFlyCam.hpp"

void FreeFlyCam::SetRot(const glm::vec3& rot)
{
  assert(glm::all(glm::epsilonEqual(rot, glm::normalize(rot), 1e-4f)) &&
         "Parameter \"rot\" needs to be normalized!");

  mYaw = glm::atan(rot.y, rot.x);
  mPitch = glm::acos(rot.z);
  mPitch -= glm::radians(89.f) * glm::floor(mPitch / glm::radians(89.f));

  UpdateLocalRotation();
}

void FreeFlyCam::UpdateLocalBasis()
{
  mForward = GetRot();
  mRight = glm::cross(mForward, glm::vec3(0.f, 0.f, 1.f));
}

glm::mat4 FreeFlyCam::CalculateViewMatrix()
{
  return glm::lookAt(mPos, mPos + GetRot(), mUp);
}

void FreeFlyCam::UpdateLocalRotation()
{
  mYaw -= (mYaw >= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mYaw += (mYaw <= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mPitch = glm::clamp(mPitch, glm::radians(-89.f), glm::radians(89.f));

  glm::quat yawQ = glm::angleAxis(mYaw, glm::vec3(0.f, 0.f, 1.f));
  glm::quat pitchQ = glm::angleAxis(mPitch, glm::vec3(0.f, 1.f, 0.f));
  mRotation = yawQ * pitchQ;
}
