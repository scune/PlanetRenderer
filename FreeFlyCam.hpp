#pragma once

#include "Libs.hpp"

class FreeFlyCam
{
public:
  FreeFlyCam() = default;

  void Update();

  inline void SetPos(const glm::vec3& pos) noexcept { mPos = pos; }
  inline void SetRot(const glm::vec3& rot) noexcept
  {
    assert(glm::all(glm::epsilonEqual(rot, glm::normalize(rot), 1e-4f)) &&
           "Parameter \"rot\" needs to be normalized!");

    mRot = rot;
    mLastDir = rot;
    mDirPitch = std::asin(rot.z);
    mDirYaw = std::asin(rot.x / std::cos(mDirPitch));
    mDirPitch = glm::degrees(mDirPitch);
    mDirYaw = glm::degrees(mDirYaw);
  }
  inline void SetUp(const glm::vec3& up) noexcept { mUp = up; }
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
  glm::vec3 mUp{0.f, 0.f, 1.f};
  const float mFov{90.f};
  const float mNear{0.01f};
  const float mFar{20000.f};

  glm::vec3 mPos{0.f};
  float mSpeed{1.f};

  inline void GetMouseOffset(float& x_offset, float& y_offset);
  inline void ComputeMouseEvents();
  inline void UpdateRotation();
  glm::vec3 mRot{0.f};
  glm::vec3 mLastDir{0.f};
  float mDirYaw{0.f};
  float mDirPitch{0.f};
  float mSensitivity{30.f};

  bool mFirstUpdate{true};
  bool mFirstClick{false};
  VkExtent2D mLastExtent{};
};
