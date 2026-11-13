#include "Hooks.h"

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);
    REL::GetTrampoline().create(512);

    Hooks::Install();

    return true;
}