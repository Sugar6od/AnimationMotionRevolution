#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace AnimationMotionRevolution {
    template <typename T>
    struct Motion {
        float time;
        T delta;
    };

    using Translation = Motion<RE::NiPoint3>;
    using Rotation = Motion<RE::NiQuaternion>;

    struct hkaAnimationBinding : public RE::hkReferencedObject {
        RE::hkStringPtr originalSkeletonName;      // 10
        RE::hkRefPtr<RE::hkaAnimation> animation;  // 18
    };

    struct hkbContext {
        RE::hkbCharacter* character;               // 00
    };

    struct hkpMovementUtilInput {
        RE::hkVector4 forward;                     // 00
        RE::hkVector4 up;                          // 10
        RE::hkVector4 surfaceNormal;               // 20
        RE::hkVector4 currentVelocity;             // 30
        RE::hkVector4 desiredVelocity;             // 40
        RE::hkVector4 surfaceVelocity;             // 50
        float         gain;                        // 60
        float         maxVelocityDelta;            // 64
        uint64_t      pad68;                       // 68
    };

    struct MotionDataContainer {
        uintptr_t translationDataPtr;              // 00
        uint32_t  translationSegCount;             // 08
        uint32_t  pad0C;                           // 0C
        uintptr_t rotationDataPtr;                 // 10
        uint32_t  rotationSegCount;                // 18
        uint32_t  pad1C;                           // 1C

        [[nodiscard]] bool IsTranslationDataAligned() const noexcept {
            return (translationDataPtr & 1) == 0;
        }

        [[nodiscard]] bool IsRotationDataAligned() const noexcept {
            return (rotationDataPtr & 1) == 0;
        }
    };

    class AnimMotionData {
    public:
        AnimMotionData() = default;

        AnimMotionData(const RE::hkaAnimation* a_animation, const Translation* a_translation) :
            animation{ a_animation }, translationList{ *a_translation } {
        }

        AnimMotionData(const RE::hkaAnimation* a_animation, const Rotation* a_rotation) :
            animation{ a_animation }, rotationList{ *a_rotation } {
        }

        void Add(const Translation* a_translation) {
            translationList.push_back(*a_translation);
        }

        void Add(const Rotation* a_rotation) {
            rotationList.push_back(*a_rotation);
        }

        void SortListsByTime() {
            if (!translationList.empty()) {
                std::sort(translationList.begin(), translationList.end(),
                    [](const Translation& a_lhs, const Translation& a_rhs) -> bool {
                        return a_lhs.time < a_rhs.time;
                    });
            }

            if (!rotationList.empty()) {
                std::sort(rotationList.begin(), rotationList.end(),
                    [](const Rotation& a_lhs, const Rotation& a_rhs) -> bool {
                        return a_lhs.time < a_rhs.time;
                    });
            }
        }

        const RE::hkaAnimation* animation = nullptr;
        std::vector<Translation> translationList;
        std::vector<Rotation> rotationList;
        int32_t activeCount = 1;
    };

    class CharacterClipAnimMotionMap {
    public:
        static CharacterClipAnimMotionMap* GetSingleton() {
            static CharacterClipAnimMotionMap singleton;
            return &singleton;
        }

        template <typename StringT>
        AnimMotionData* Add(const RE::hkbCharacter* a_hkbCharacter, const StringT& a_clipName, const AnimMotionData& a_animMotionData) {
            const char* clipNameStr = a_clipName.c_str();
            if (!clipNameStr) return nullptr;
            try {
                auto& charMap = data[a_hkbCharacter];
                auto [it, _] = charMap.insert_or_assign(clipNameStr, a_animMotionData);
                return &it->second;
            }
            catch (...) {
                data.clear();
                return nullptr;
            }
        }

        template <typename StringT>
        AnimMotionData* Get(const RE::hkbCharacter* a_hkbCharacter, const StringT& a_clipName) {
            const char* clipNameStr = a_clipName.c_str();
            if (!a_hkbCharacter || !clipNameStr) return nullptr;
            if (auto charIt = data.find(a_hkbCharacter); charIt != data.end()) {
                if (auto clipIt = charIt->second.find(std::string_view{ clipNameStr }); clipIt != charIt->second.end()) {
                    return &clipIt->second;
                }
            }
            return nullptr;
        }

        template <typename StringT>
        void Remove(const RE::hkbCharacter* a_hkbCharacter, const StringT& a_clipName) {
            const char* clipNameStr = a_clipName.c_str();
            if (!a_hkbCharacter || !clipNameStr) return;
            if (auto charIt = data.find(a_hkbCharacter); charIt != data.end()) {
                if (auto clipIt = charIt->second.find(std::string_view{ clipNameStr }); clipIt != charIt->second.end()) {
                    if (charIt->second.size() == 1) {
                        data.erase(charIt);
                    }
                    else {
                        charIt->second.erase(clipIt);
                    }
                }
            }
        }

        mutable RE::BSSpinLock lock;

    private:
        std::map<const RE::hkbCharacter*, std::map<std::string, AnimMotionData, std::less<>>> data;
    };

    std::variant<std::monostate, Translation, Rotation> ParseAnnotation(const RE::hkaAnnotationTrack::Annotation& a_annotation);
    const RE::hkaAnimation* GetBoundAnimation(const RE::hkbClipGenerator* a_this) noexcept;
    RE::hkbCharacter* GethkbCharacter(RE::Character* a_character) noexcept;
}