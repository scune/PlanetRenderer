#include "FreeFlyCam.hpp"

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
  mPitch = glm::clamp(mPitch, glm::radians(-89.f), glm::radians(89.f));

  mRotation.x = glm::sin(mYaw) * glm::cos(mPitch);
  mRotation.y = glm::cos(mYaw) * glm::cos(mPitch);
  mRotation.z = glm::sin(mPitch);
  mRotation = glm::normalize(mRotation);
}
