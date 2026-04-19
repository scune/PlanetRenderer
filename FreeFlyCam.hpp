#pragma once

#include "CameraBase.hpp"

class FreeFlyCam : public CameraBase
{
public:
  FreeFlyCam() = default;
  ~FreeFlyCam() = default;

  glm::vec3 GetRot() const override { return mRotation; }

private:
  glm::mat4 CalculateViewMatrix() override;

  void UpdateLocalBasis() override;
  void UpdateLocalRotation() override;
  glm::vec3 mRotation{1.f, 0.f, 0.f};
};
