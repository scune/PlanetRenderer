#pragma once

#include "Libs.hpp"

class PlayerCam
{
public:
  PlayerCam() = default;

  void Update();

  inline void SetPos(const glm::vec3& pos) noexcept { mPos = pos; }
  void SetRot(const glm::vec3& rot) noexcept;
  inline void SetSpeed(float speed) noexcept { mSpeed = speed; }

  inline const glm::mat4& GetMatrix() const noexcept { return mMat; }
  inline const glm::mat4& GetPrevMatrix() const noexcept { return mPrevMat; }
  inline const glm::vec3& GetPos() const noexcept { return mPos; }
  inline const glm::vec3& GetRot() const noexcept { return mRot; }
  inline float GetNear() const noexcept { return mNear; }
  inline float GetFar() const noexcept { return mFar; }

private:
  inline void ProcessInputEvents();

  inline glm::mat4 CalculateViewMatrix();
  inline void CalculateProjMatrix(float aspectRatio);
  glm::mat4 mMat;
  glm::mat4 mPrevMat;
  glm::mat4 mProjMat;
  const float mFov{90.f};
  const float mNear{0.01f};
  const float mFar{20000.f};

  glm::vec3 mPos{0.f};
  glm::vec3 mPlanetUp{0.f, 0.f, 1.f};
  float mSpeed{1.f};

  inline void GetMouseOffset(float& x_offset, float& y_offset);
  inline void ComputeMouseEvents();
  inline void UpdateRotation();
  glm::vec3 mRot{0.f};
  glm::vec3 mPlanetRot{0.f};
  float mDirYaw{0.f};
  float mDirPitch{0.f};
  float mSensitivity{30.f};

  bool mFirstUpdate{true};
  bool mFirstClick{false};
  VkExtent2D mLastExtent{};
};
