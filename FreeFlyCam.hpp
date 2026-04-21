#pragma once

#include "CameraBase.hpp"

class FreeFlyCam : public CameraBase
{
public:
  FreeFlyCam() = default;
  ~FreeFlyCam() = default;

  void SetRot(const glm::vec3& rot) override;
  glm::vec3 GetRot() const override
  {
    return glm::normalize(mLocalRot * mLocalForward);
  }

private:
  glm::mat4 CalculateViewMatrix() override;

  void UpdateLocalBasis() override;
};
