#include "PlayerCam.hpp"

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

  mSurfaceBasis = upAlignment * mSurfaceBasis;
  mSurfaceBasis = glm::normalize(mSurfaceBasis);

  mPlanetRotation = mSurfaceBasis * mLocalRotation;

  return glm::lookAt(mPos, mPos + mPlanetRotation * mLocalForward, mUp);
}

void PlayerCam::UpdateLocalRotation()
{
  mYaw -= (mYaw >= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mYaw += (mYaw <= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mPitch = glm::clamp(mPitch, glm::radians(-89.9f), glm::radians(89.9f));

  glm::quat yawQ = glm::angleAxis(mYaw, glm::vec3(0.f, 0.f, -1.f));
  glm::quat pitchQ = glm::angleAxis(mPitch, glm::vec3(0.f, -1.f, 0.f));
  mLocalRotation = yawQ * pitchQ;
}
