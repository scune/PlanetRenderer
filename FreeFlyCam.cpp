#include "FreeFlyCam.hpp"

void FreeFlyCam::SetRot(const glm::vec3& rot)
{
  assert(glm::all(glm::epsilonEqual(rot, glm::normalize(rot), 1e-4f)) &&
         "Parameter \"rot\" needs to be normalized!");

  mYaw = glm::atan(rot.y, rot.x);
  mPitch = glm::acos(rot.z);
  mPitch -= mMaxPitch * glm::floor(mPitch / mMaxPitch);

  UpdateLocalRotation();
}

void FreeFlyCam::UpdateLocalBasis()
{
  mForward = mLocalRot * mLocalForward;
  mRight = mLocalRot * mLocalRight;
}

glm::mat4 FreeFlyCam::CalculateViewMatrix()
{
  return glm::lookAt(mPos, mPos + mLocalRot * mLocalForward, mUp);
}
