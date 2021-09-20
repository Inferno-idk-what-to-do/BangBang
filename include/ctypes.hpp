#pragma once

#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/types.hpp"
#include "UnityEngine/MonoBehaviour.hpp"

// Thanks Raemien for teaching me custom types:)
DECLARE_CLASS_CODEGEN(BangBang, ctypes, UnityEngine::MonoBehaviour,
    DECLARE_INSTANCE_METHOD(void, Awake);
    DECLARE_INSTANCE_METHOD(void, Update);
)
