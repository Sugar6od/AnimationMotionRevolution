#include "AnimationMotionRevolution.h"
#include <charconv>
#include <cmath>
#include <numbers>

namespace AnimationMotionRevolution {
    std::variant<std::monostate, Translation, Rotation> ParseAnnotation(const RE::hkaAnnotationTrack::Annotation& a_annotation) {
        constexpr std::string_view animmotionPrefix = "animmotion ";
        constexpr std::string_view animrotationPrefix = "animrotation ";

        std::string_view text{a_annotation.text.c_str()};

        if (text.starts_with(animmotionPrefix)) {
            std::string_view start = text.substr(animmotionPrefix.size());
            std::string_view end = start.substr(start.find(' ') + 1);

            RE::NiPoint3 translation{};
            std::from_chars(start.data(), end.data(), translation.x);

            start = end;
            end = start.substr(start.find(' ') + 1);
            std::from_chars(start.data(), end.data(), translation.y);

            start = end;
            end = start.substr(start.size());
            std::from_chars(start.data(), end.data(), translation.z);

            return Translation{a_annotation.time, translation};
        }
        else if (text.starts_with(animrotationPrefix)) {
            std::string_view start = text.substr(animrotationPrefix.size());
            std::string_view end = start.substr(start.size());

            float yawDegrees = 0.0f;
            std::from_chars(start.data(), end.data(), yawDegrees);

            const float halfYaw = yawDegrees * (std::numbers::pi_v<float> / 360.0f);
            RE::NiQuaternion rotation{std::cos(halfYaw), 0.0f, 0.0f, std::sin(halfYaw)};

            return Rotation{a_annotation.time, rotation};
        }

        return {};
    }

    const RE::hkaAnimation* GetBoundAnimation(const RE::hkbClipGenerator* a_this) noexcept {
        return a_this->binding ? a_this->binding->animation.get() : nullptr;
    }

    RE::hkbCharacter* GethkbCharacter(RE::Character* a_character) noexcept {
        RE::BSAnimationGraphManagerPtr animGraph;
        a_character->GetAnimationGraphManager(animGraph);
        return &animGraph->graphs[animGraph->activeGraph]->characterInstance;
    }
}