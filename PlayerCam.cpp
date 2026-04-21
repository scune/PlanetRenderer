#include "PlayerCam.hpp"

void PlayerCam::SetRot(const glm::vec3& rot)
{
  assert(glm::all(glm::epsilonEqual(rot, glm::normalize(rot), 1e-4f)) &&
         "Parameter \"rot\" needs to be normalized!");

  glm::vec3 oldPlanetUp = mUp;
  glm::vec3 newPlanetUp = glm::normalize(mPos);
  glm::quat upAlignment = glm::rotation(newPlanetUp, oldPlanetUp);

  mLocalRotation = upAlignment * rot;

  glm::vec3 localRot = mLocalRotation * mLocalForward;
  mYaw = glm::atan(localRot.y, localRot.x);
  mPitch = glm::acos(localRot.z);

  UpdateLocalRotation();

  COUT("Rot should be:");
  COUT_VEC3(rot);
  COUT("Rot is now:");
  upAlignment = glm::rotation(oldPlanetUp, newPlanetUp);
  localRot = upAlignment * mLocalRotation * mLocalForward;
  COUT_VEC3(localRot);
}

glm::vec3 PlayerCam::GetRot() const { return mPlanetRotation * mLocalForward; }

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

  mPlanetRotation = mSurfaceBasis * mLocalRotation;

  return glm::lookAt(mPos, mPos + mPlanetRotation * mLocalForward, mUp);
}

void PlayerCam::UpdateLocalRotation()
{
  mYaw -= (mYaw >= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mYaw += (mYaw <= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mPitch = glm::clamp(mPitch, glm::radians(-89.9f), glm::radians(89.9f));

  glm::quat yawQ = glm::angleAxis(mYaw, glm::vec3(0.f, 0.f, 1.f));
  glm::quat pitchQ = glm::angleAxis(mPitch, glm::vec3(1.f, 0.f, 0.f));
  mLocalRotation = yawQ * pitchQ;
}
