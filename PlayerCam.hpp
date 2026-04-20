#pragma once

#include "CameraBase.hpp"
#include <glm/gtx/quaternion.hpp>

class PlayerCam : public CameraBase
{
public:
  PlayerCam() = default;
  ~PlayerCam() = default;

  void SetRot(const glm::vec3& rot) override;
  glm::vec3 GetRot() const override;

private:
  glm::mat4 CalculateViewMatrix() override;

  void UpdateLocalBasis() override;
  void UpdateLocalRotation() override;
  glm::quat mPlanetRotation{1.f, 0.f, 0.f, 0.f};
  glm::quat mLocalRotation{1.f, 0.f, 0.f, 0.f};
  glm::quat mSurfaceBasis{1.f, 0.f, 0.f, 0.f};

  const glm::vec3 mLocalForward{-1.f, 0.f, 0.f};
  const glm::vec3 mLocalRight{0.f, 1.f, 0.f};
};
