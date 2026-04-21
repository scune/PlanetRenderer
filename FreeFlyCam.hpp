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
    return glm::normalize(mRotation * glm::vec3(1.f, 0.f, 0.f));
  }

private:
  glm::mat4 CalculateViewMatrix() override;

  void UpdateLocalBasis() override;
  void UpdateLocalRotation() override;
  glm::quat mRotation{1.f, 0.f, 0.f, 0.f};
};
