#pragma once
#include "AnimationMotionRevolution.h"
#include <xbyak/xbyak.h>

namespace Hooks {
    class Hook_bhkCharacterStateOnGround {
    public:
        static inline void Install() {
            _SimulateStatePhysics = REL::Relocation{RE::VTABLE_bhkCharacterStateOnGround[0]}.write_vfunc(0x08, SimulateStatePhysics);
        }
    private:
        static void SimulateStatePhysics(RE::bhkCharacterStateOnGround* a_this, RE::bhkCharacterController* a_characterController);
        static inline REL::Relocation<decltype(SimulateStatePhysics)> _SimulateStatePhysics;
        static inline REL::Relocation<void(const AnimationMotionRevolution::hkpMovementUtilInput&, RE::hkVector4&)> _CalculateMovement{REL::ID(81002)};
        static inline REL::Relocation<RE::hkVector4* (RE::bhkWorld*)> _GetGravity{REL::ID(77838)};
        static inline REL::Relocation<void(RE::bhkCharacterController*)> _SetWantedState{REL::ID(78280)};
        static inline REL::Relocation<void(RE::bhkCharacterController*)> _ApplyPushVelocity{REL::ID(78283)};
    };

    class Hook_bhkCharacterController {
    public:
        static inline void Install() {
            REL::GetTrampoline().write_call<5>(REL::ID(78275).address() + 0x7C2, GetCharacterState);
        }
    private:
        static RE::hkpCharacterStateType GetCharacterState(RE::hkpCharacterContext* a_this);
    };

    struct ActivateClipGenerator : Xbyak::CodeGenerator {
        ActivateClipGenerator(uintptr_t a_hookFunc, uintptr_t a_returnAddress) {
            Xbyak::Label hookLabel;
            Xbyak::Label retnLabel;

            mov(rdx, r15);
            call(ptr[rip + hookLabel]);
            jmp(ptr[rip + retnLabel]);

            L(hookLabel), dq(a_hookFunc);
            L(retnLabel), dq(a_returnAddress);
            ready();
        }
    };

    struct DeactivateClipGenerator : Xbyak::CodeGenerator {
        DeactivateClipGenerator(uintptr_t a_hookFunc, uintptr_t a_returnAddress) {
            Xbyak::Label hookLabel;
            Xbyak::Label retnLabel;

            mov(rdx, r14);
            call(ptr[rip + hookLabel]);
            jmp(ptr[rip + retnLabel]);

            L(hookLabel), dq(a_hookFunc);
            L(retnLabel), dq(a_returnAddress);
            ready();
        }
    };

    class Hook_hkbClipGenerator {
    public:
        static inline void Install() {
            _ComputeStartTime = REL::GetTrampoline().write_jmp<5>(REL::ID(59252).address() + 0x66E, REL::GetTrampoline().allocate(ActivateClipGenerator(reinterpret_cast<uintptr_t>(ComputeStartTime), REL::ID(59252).address() + 0x673)));
            _ResetIgnoreStartTime = REL::GetTrampoline().write_jmp<5>(REL::ID(59254).address() + 0x1A, REL::GetTrampoline().allocate(DeactivateClipGenerator(reinterpret_cast<uintptr_t>(ResetIgnoreStartTime), REL::ID(59254).address() + 0x1F)));
        }
    private:
        static uint32_t ComputeStartTime(const RE::hkbClipGenerator* a_this, const AnimationMotionRevolution::hkbContext* a_hkbContext);
        static void ResetIgnoreStartTime(const RE::hkbClipGenerator* a_this, const AnimationMotionRevolution::hkbContext* a_hkbContext);
        static inline REL::Relocation<uint32_t(const RE::hkbClipGenerator*)> _ComputeStartTime;
        static inline REL::Relocation<void(const RE::hkbClipGenerator*)> _ResetIgnoreStartTime;
    };

    struct ProcessMotionData : Xbyak::CodeGenerator {
        ProcessMotionData(uintptr_t a_hookFunc) {
            Xbyak::Label hookLabel;

            sub(rsp, 0x28);
            mov(ptr[rsp + 0x20], r13);
            mov(r9, rbx);
            sub(r9, 0x14);
            call(ptr[rip + hookLabel]);
            add(rsp, 0x28);
            ret();

            L(hookLabel), dq(a_hookFunc);
            ready();
        }
    };

    class Hook_MotionDataContainer {
    public:
        static inline void Install() {
            REL::GetTrampoline().write_call<5>(REL::ID(32703).address() + 0x298, REL::GetTrampoline().allocate(ProcessMotionData(reinterpret_cast<uintptr_t>(ProcessTranslationData))));
            REL::GetTrampoline().write_call<5>(REL::ID(32703).address() + 0x2AA, REL::GetTrampoline().allocate(ProcessMotionData(reinterpret_cast<uintptr_t>(ProcessTranslationData))));
            REL::GetTrampoline().write_call<5>(REL::ID(32703).address() + 0x35C, REL::GetTrampoline().allocate(ProcessMotionData(reinterpret_cast<uintptr_t>(ProcessRotationData))));
            REL::GetTrampoline().write_call<5>(REL::ID(32703).address() + 0x36D, REL::GetTrampoline().allocate(ProcessMotionData(reinterpret_cast<uintptr_t>(ProcessRotationData))));
        }
    private:
        static void ProcessTranslationData(AnimationMotionRevolution::MotionDataContainer* a_this, float a_motionTime, RE::NiPoint3& a_translation, const RE::BSFixedString* a_clipName, RE::Character* a_character);
        static void ProcessRotationData(AnimationMotionRevolution::MotionDataContainer* a_this, float a_motionTime, RE::NiQuaternion& a_rotation, const RE::BSFixedString* a_clipName, RE::Character* a_character);
        static inline REL::Relocation<void(uintptr_t*, float, RE::NiPoint3&)> _ProcessTranslationData{REL::ID(32582)};
        static inline REL::Relocation<void(uintptr_t*, float, RE::NiQuaternion&)> _ProcessRotationData{REL::ID(32583)};
        static inline REL::Relocation<void(RE::NiQuaternion&, float, const RE::NiQuaternion&, const RE::NiQuaternion&)> _InterpolateRotation{REL::ID(70836)};
    };

    inline void Install() {
        Hook_bhkCharacterStateOnGround::Install();
        Hook_bhkCharacterController::Install();
        Hook_hkbClipGenerator::Install();
        Hook_MotionDataContainer::Install();
        REX::INFO("Hooks Installed");
    }
}