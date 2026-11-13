#include "Hooks.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace Hooks {
    uint32_t Hook_hkbClipGenerator::ComputeStartTime(const RE::hkbClipGenerator* a_this, const AnimationMotionRevolution::hkbContext* a_hkbContext) {
        auto* characterClipAnimMotionMap = AnimationMotionRevolution::CharacterClipAnimMotionMap::GetSingleton();
        RE::BSSpinLockGuard lockguard(characterClipAnimMotionMap->lock);

        const RE::hkaAnimation* boundAnimation = AnimationMotionRevolution::GetBoundAnimation(a_this);
        if (boundAnimation) {
            const RE::hkbCharacter* hkbCharacter = a_hkbContext ? a_hkbContext->character : nullptr;
            if (hkbCharacter) {
                AnimationMotionRevolution::AnimMotionData* animMotionData = characterClipAnimMotionMap->Get(hkbCharacter, a_this->name);

                if (animMotionData && animMotionData->animation == boundAnimation) {
                    animMotionData->activeCount++;
                }
                else {
                    animMotionData = nullptr;

                    for (const RE::hkaAnnotationTrack& annotationTrack : boundAnimation->annotationTracks) {
                        for (const RE::hkaAnnotationTrack::Annotation& annotation : annotationTrack.annotations) {
                            auto dataParsed = AnimationMotionRevolution::ParseAnnotation(annotation);

                            if (auto* translation = std::get_if<AnimationMotionRevolution::Translation>(&dataParsed)) {
                                if (animMotionData) {
                                    animMotionData->Add(translation);
                                }
                                else {
                                    animMotionData = characterClipAnimMotionMap->Add(hkbCharacter, a_this->name, AnimationMotionRevolution::AnimMotionData{ boundAnimation, translation });
                                }
                            }
                            else if (auto* rotation = std::get_if<AnimationMotionRevolution::Rotation>(&dataParsed)) {
                                if (animMotionData) {
                                    animMotionData->Add(rotation);
                                }
                                else {
                                    animMotionData = characterClipAnimMotionMap->Add(hkbCharacter, a_this->name, AnimationMotionRevolution::AnimMotionData{ boundAnimation, rotation });
                                }
                            }
                        }

                        if (animMotionData) {
                            animMotionData->SortListsByTime();
                            break;
                        }
                    }
                }
            }
        }

        return _ComputeStartTime(a_this);
    }

    void Hook_hkbClipGenerator::ResetIgnoreStartTime(const RE::hkbClipGenerator* a_this, const AnimationMotionRevolution::hkbContext* a_hkbContext) {
        auto* characterClipAnimMotionMap = AnimationMotionRevolution::CharacterClipAnimMotionMap::GetSingleton();
        RE::BSSpinLockGuard lockguard(characterClipAnimMotionMap->lock);

        const RE::hkaAnimation* boundAnimation = AnimationMotionRevolution::GetBoundAnimation(a_this);
        if (boundAnimation) {
            const RE::hkbCharacter* hkbCharacter = a_hkbContext ? a_hkbContext->character : nullptr;
            if (hkbCharacter) {
                AnimationMotionRevolution::AnimMotionData* animMotionData = characterClipAnimMotionMap->Get(hkbCharacter, a_this->name);

                if (animMotionData && animMotionData->animation == boundAnimation) {
                    if (--animMotionData->activeCount == 0) {
                        characterClipAnimMotionMap->Remove(hkbCharacter, a_this->name);
                    }
                }
            }
        }

        _ResetIgnoreStartTime(a_this);
    }

    void Hook_MotionDataContainer::ProcessTranslationData(AnimationMotionRevolution::MotionDataContainer* a_this, float a_motionTime, RE::NiPoint3& a_translation, const RE::BSFixedString* a_clipName, RE::Character* a_character) {
        auto* characterClipAnimMotionMap = AnimationMotionRevolution::CharacterClipAnimMotionMap::GetSingleton();
        RE::BSSpinLockGuard lockguard(characterClipAnimMotionMap->lock);

        auto* charController = a_character->GetCharController();
        auto* charStateOnGround = reinterpret_cast<RE::bhkCharacterStateOnGround*>(charController->context.stateManager->registeredState[RE::hkpCharacterStateType::kOnGround]);

        RE::hkbCharacter* hkbCharacter = AnimationMotionRevolution::GethkbCharacter(a_character);
        AnimationMotionRevolution::AnimMotionData* animMotionData = characterClipAnimMotionMap->Get(hkbCharacter, *a_clipName);

        if (animMotionData && !animMotionData->translationList.empty()) {
            const auto& customTranslationList = animMotionData->translationList;
            float endMotionTime = customTranslationList.back().time;
            float curMotionTime = std::clamp(a_motionTime, 0.0f, endMotionTime);
            auto segCount = static_cast<uint32_t>(customTranslationList.size());

            for (uint32_t segIndex = 1; segIndex <= segCount; segIndex++) {
                float curSegMotionTime = customTranslationList[segIndex - 1].time;
                if (curMotionTime <= curSegMotionTime) {
                    uint32_t prevSegIndex = segIndex - 1;
                    float segProgress = 1.0f;
                    float prevSegMotionTime = prevSegIndex ? customTranslationList[prevSegIndex - 1].time : 0.0f;

                    float curSegMotionDuration = curSegMotionTime - prevSegMotionTime;
                    if (curSegMotionDuration > std::numeric_limits<float>::epsilon()) {
                        segProgress = (curMotionTime - prevSegMotionTime) / curSegMotionDuration;
                    }

                    const RE::NiPoint3& curSegTranslation = customTranslationList[segIndex - 1].delta;
                    const RE::NiPoint3& prevSegTranslation = prevSegIndex
                        ? customTranslationList[prevSegIndex - 1].delta
                        : RE::NiPoint3{0.0f, 0.0f, 0.0f};

                    a_translation = (curSegTranslation * segProgress + prevSegTranslation * (1.0f - segProgress));
                    charStateOnGround->unk10 = a_translation.z == 0.0f;
                    return;
                }
            }
        }
        else if (a_this->translationSegCount > static_cast<uint32_t>(a_this->IsTranslationDataAligned())) {
            charStateOnGround->unk10 = true;
            _ProcessTranslationData(&a_this->translationDataPtr, a_motionTime, a_translation);
            return;
        }

        charStateOnGround->unk10 = true;
        a_translation = RE::NiPoint3{0.0f, 0.0f, 0.0f};
    }

    void Hook_MotionDataContainer::ProcessRotationData(AnimationMotionRevolution::MotionDataContainer* a_this, float a_motionTime, RE::NiQuaternion& a_rotation, const RE::BSFixedString* a_clipName, RE::Character* a_character) {
        auto* characterClipAnimMotionMap = AnimationMotionRevolution::CharacterClipAnimMotionMap::GetSingleton();
        RE::BSSpinLockGuard lockguard(characterClipAnimMotionMap->lock);

        RE::hkbCharacter* hkbCharacter = AnimationMotionRevolution::GethkbCharacter(a_character);
        AnimationMotionRevolution::AnimMotionData* animMotionData = characterClipAnimMotionMap->Get(hkbCharacter, *a_clipName);

        if (animMotionData && !animMotionData->rotationList.empty()) {
            const auto& customRotationList = animMotionData->rotationList;
            float endMotionTime = customRotationList.back().time;
            float curMotionTime = std::clamp(a_motionTime, 0.0f, endMotionTime);
            auto segCount = static_cast<uint32_t>(customRotationList.size());

            for (uint32_t segIndex = 1; segIndex <= segCount; segIndex++) {
                float curSegMotionTime = customRotationList[segIndex - 1].time;
                if (curMotionTime <= curSegMotionTime) {
                    uint32_t prevSegIndex = segIndex - 1;
                    float segProgress = 1.0f;
                    float prevSegMotionTime = prevSegIndex ? customRotationList[prevSegIndex - 1].time : 0.0f;

                    float curSegMotionDuration = curSegMotionTime - prevSegMotionTime;
                    if (curSegMotionDuration > std::numeric_limits<float>::epsilon()) {
                        segProgress = (curMotionTime - prevSegMotionTime) / curSegMotionDuration;
                    }

                    const RE::NiQuaternion& curSegRotation = customRotationList[segIndex - 1].delta;
                    const RE::NiQuaternion& prevSegRotation = prevSegIndex
                        ? customRotationList[prevSegIndex - 1].delta
                        : RE::NiQuaternion{1.0f, 0.0f, 0.0f, 0.0f};

                    _InterpolateRotation(a_rotation, segProgress, prevSegRotation, curSegRotation);
                    return;
                }
            }
        }
        else if (a_this->rotationSegCount > static_cast<uint32_t>(a_this->IsRotationDataAligned())) {
            _ProcessRotationData(&a_this->rotationDataPtr, a_motionTime, a_rotation);
            return;
        }

        a_rotation = RE::NiQuaternion{1.0f, 0.0f, 0.0f, 0.0f};
    }

    RE::hkpCharacterStateType Hook_bhkCharacterController::GetCharacterState(RE::hkpCharacterContext* a_this) {
        auto* charStateOnGround = reinterpret_cast<RE::bhkCharacterStateOnGround*>(a_this->stateManager->registeredState[RE::hkpCharacterStateType::kOnGround]);
        return charStateOnGround->unk10 ? a_this->currentState : RE::hkpCharacterStateType::kSwimming;
    }

    void Hook_bhkCharacterStateOnGround::SimulateStatePhysics(RE::bhkCharacterStateOnGround* a_this, RE::bhkCharacterController* a_characterController) {
        RE::bhkWorld* world = a_characterController->GetWorldImpl();
        RE::hkVector4 gravity = *_GetGravity(world);

        auto& unk31E = *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(a_characterController) + 0x31E);

        if (a_this->unk10 != 0 &&
            a_characterController->flags.none(RE::CHARACTER_FLAGS::kSupport) &&
            unk31E == 0 &&
            (a_characterController->wantState == a_characterController->context.currentState ||
                a_characterController->wantState == RE::hkpCharacterStateType::kTotal)) {

            RE::hkVector4 position{};
            a_characterController->GetPosition(position, true);

            RE::CFilter collisionFilterInfo{};
            a_characterController->GetCollisionFilterInfo(collisionFilterInfo);

            RE::bhkPickData pickData{};
            pickData.rayInput.from = position;
            pickData.rayInput.to = position - RE::hkVector4{0.0f, 0.0f, 0.5f, 0.0f};
            pickData.rayInput.enableShapeCollectionFilter = false;
            pickData.rayInput.filterInfo = collisionFilterInfo;
            pickData.rayOutput.hitFraction = 1.0f;
            pickData.rayOutput.extraInfo = -1;
            pickData.rayOutput.shapeKeys[0] = RE::HK_INVALID_SHAPE_KEY;
            pickData.rayOutput.rootCollidable = nullptr;

            world->PickObject(pickData);

            if (pickData.rayOutput.rootCollidable) {
                gravity = gravity * 0.2f;
                if (pickData.rayOutput.normal.quad.m128_f32[2] < std::bit_cast<float>(a_characterController->maxSlope)) {
                    a_characterController->flags.set(RE::CHARACTER_FLAGS::kAllowJumpNoContact); // 0x10
                }
            }
            else {
                a_characterController->outVelocity.quad.m128_f32[2] = 0.0f;
                a_characterController->wantState = RE::hkpCharacterStateType::kInAir;
                a_characterController->fallStartHeight = position.quad.m128_f32[2] * 69.99125f;
                _SetWantedState(a_characterController);

                RE::hkpCharacterStateType stateType = a_characterController->context.currentState;
                auto* characterState = static_cast<RE::bhkCharacterState*>(
                    a_characterController->context.stateManager->registeredState[stateType]
                    );
                REL::Relocation<void(RE::bhkCharacterState*, RE::bhkCharacterController*)>(
                    (*reinterpret_cast<uintptr_t**>(characterState))[0x08]
                )(characterState, a_characterController);
                return;
            }
        }

        if (unk31E != 0) {
            RE::hkVector4 curPos{};
            a_characterController->GetPosition(curPos, true);
            const float dx = curPos.quad.m128_f32[0] - a_characterController->initialVelocity.quad.m128_f32[0];
            const float dy = curPos.quad.m128_f32[1] - a_characterController->initialVelocity.quad.m128_f32[1];
            const float dz = curPos.quad.m128_f32[2] - a_characterController->initialVelocity.quad.m128_f32[2];
            if (std::sqrt(dx * dx + dy * dy + dz * dz) > 0.9144f ||
                a_characterController->wantState != RE::hkpCharacterStateType::kTotal) {
                unk31E = 0;
            }
        }

        const float prevOutVelZ = a_characterController->outVelocity.quad.m128_f32[2];
        const bool keepVerticalVel = (a_this->unk10 == 0) || ((a_characterController->flags.underlying() >> 11) & 1) != 0;

        RE::hkVector4 desiredVelocity = a_characterController->velocityMod;
        desiredVelocity.quad.m128_f32[0] = a_characterController->velocityMod.quad.m128_f32[1];
        desiredVelocity.quad.m128_f32[1] = a_characterController->velocityMod.quad.m128_f32[0];
        if (!keepVerticalVel) {
            desiredVelocity.quad.m128_f32[2] = 0.0f;
            desiredVelocity.quad.m128_f32[3] = 0.0f;
        }

        AnimationMotionRevolution::hkpMovementUtilInput movementUtil{};
        movementUtil.forward = a_characterController->forwardVec;
        movementUtil.up = RE::hkVector4{0.0f, 0.0f, 1.0f, 0.0f};
        movementUtil.surfaceNormal = a_characterController->supportNorm;
        movementUtil.currentVelocity = a_characterController->outVelocity;
        movementUtil.desiredVelocity = desiredVelocity;
        movementUtil.surfaceVelocity = a_characterController->surfaceInfo.surfaceVelocity;
        movementUtil.gain = 1.0f;
        movementUtil.maxVelocityDelta = 500.0f;

        _CalculateMovement(movementUtil, a_characterController->outVelocity);

        if (!keepVerticalVel) {
            a_characterController->outVelocity.quad.m128_f32[2] = prevOutVelZ;
            a_characterController->outVelocity = a_characterController->outVelocity + (gravity * a_characterController->stepInfo.deltaTime);
        }

        _ApplyPushVelocity(a_characterController);

        if (a_characterController->wantState != RE::hkpCharacterStateType::kTotal &&
            (a_characterController->wantState != RE::hkpCharacterStateType::kJumping ||
                a_characterController->flags.all(RE::CHARACTER_FLAGS::kCanJump))) {
            _SetWantedState(a_characterController);
        }

        RE::hkVector4 relVel = a_characterController->outVelocity - a_characterController->surfaceInfo.surfaceVelocity;
        float vx = relVel.quad.m128_f32[0];
        float vy = relVel.quad.m128_f32[1];
        float vz = relVel.quad.m128_f32[2];

        if (!keepVerticalVel && vz > 0.001f) {
            const float nx = a_characterController->supportNorm.quad.m128_f32[0];
            const float ny = a_characterController->supportNorm.quad.m128_f32[1];
            const float nz = a_characterController->supportNorm.quad.m128_f32[2];

            if (std::abs(nz) > std::numeric_limits<float>::epsilon()) {
                const float lenSq = vx * vx + vy * vy + vz * vz;
                const float len = std::sqrt(lenSq);
                if (len > 0.0f) {
                    const float invLen = 1.0f / len;
                    const float scale = len / nz;
                    const float cx = (ny * vz - nz * vy) * invLen;
                    const float cy = (nz * vx - nx * vz) * invLen;
                    vx = cy * scale;
                    vy = -cx * scale;
                    vz = 0.0f;
                    relVel = RE::hkVector4{vx, vy, vz, 0.0f};
                }
            }
        }

        a_characterController->outVelocity = relVel + a_characterController->surfaceInfo.surfaceVelocity;
    }
}