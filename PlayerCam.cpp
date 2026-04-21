#include "PlayerCam.hpp"

void PlayerCam::SetRot(const glm::vec3& rot)
{
  assert(glm::all(glm::epsilonEqual(rot, glm::normalize(rot), 1e-4f)) &&
         "Parameter \"rot\" needs to be normalized!");

  glm::vec3 oldPlanetUp = mUp;
  glm::vec3 newPlanetUp = glm::normalize(mPos);
  glm::quat upAlignment = glm::rotation(newPlanetUp, oldPlanetUp);

  glm::vec3 localRot = upAlignment * rot;
  mYaw = glm::atan(localRot.y, localRot.x);
  mPitch = glm::acos(localRot.z);
  mPitch -= glm::radians(89.f) * glm::floor(mPitch / glm::radians(89.f));

  UpdateLocalRotation();
}

glm::vec3 PlayerCam::GetRot() const
{
  return glm::normalize(mPlanetRotation * mLocalForward);
}

void PlayerCam::UpdateLocalBasis()
{
  mForward = mPlanetRotation * mLocalForward;
  mRight = mPlanetRotation * mLocalRight;
}

glm::mat4 PlayerCam::CalculateViewMatrix()
{
  glm::vec3 oldPlanetUp = mUp;
  mUp = glm::normalize(mPos);

  glm::quat upAlignment = glm::rotation(oldPlanetUp, mUp);
  if (upAlignment != glm::quat_identity<float, glm::packed_highp>())
  {
    mSurfaceBasis = upAlignment * mSurfaceBasis;
    mSurfaceBasis = glm::normalize(mSurfaceBasis);
  }

  mPlanetRotation = mSurfaceBasis * mLocalRot;

  return glm::lookAt(mPos, mPos + mPlanetRotation * mLocalForward, mUp);
}
