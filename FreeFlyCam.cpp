#include "FreeFlyCam.hpp"

void FreeFlyCam::SetRot(const glm::vec3& rot)
{
  assert(glm::all(glm::epsilonEqual(rot, glm::normalize(rot), 1e-4f)) &&
         "Parameter \"rot\" needs to be normalized!");

  mYaw = glm::atan(rot.y, rot.x);
  mPitch = glm::acos(rot.z);
  COUT("After Yaw: " << mYaw);
  COUT("After Pitch: " << mPitch);
  COUT_VEC3(rot);
  UpdateLocalRotation();
  COUT_VEC3(mRotation);
}

void FreeFlyCam::UpdateLocalBasis()
{
  mForward = mRotation;
  mRight = glm::cross(mRotation, glm::vec3(0.f, 0.f, 1.f));
}

glm::mat4 FreeFlyCam::CalculateViewMatrix()
{
  return glm::lookAt(mPos, mPos + mRotation, mUp);
}

void FreeFlyCam::UpdateLocalRotation()
{
  mYaw -= (mYaw >= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mYaw += (mYaw <= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mPitch = glm::clamp(mPitch, glm::radians(1.f), glm::radians(179.f));

  mRotation.x = glm::cos(mYaw) * glm::sin(mPitch);
  mRotation.y = glm::sin(mYaw) * glm::sin(mPitch);
  mRotation.z = glm::cos(mPitch);
  mRotation = glm::normalize(mRotation);
}
