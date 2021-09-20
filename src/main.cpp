#include "GlobalNamespace/VRControllersInputManager.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "GlobalNamespace/SaberManager.hpp"
#include "GlobalNamespace/CuttingManager.hpp"
#include "GlobalNamespace/NoteCutter.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Physics.hpp"
#include "UnityEngine/RaycastHit.hpp"
#include "UnityEngine/Component.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/PrimitiveType.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "custom-types/shared/register.hpp"
#include "custom-types/shared/types.hpp"

#include "main.hpp"
#include "ctypes.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// Cutting manager to call NoteCutter::Cut()
CuttingManager *cutMan = nullptr;

// Create the saber objects to call NoteCutter::Cut() with
Saber *Lsaber = nullptr;
Saber *Rsaber = nullptr; 

// Create the controller pbjects to get their trigger values in update()
VRController *Lcontrol = nullptr;
VRController *Rcontrol = nullptr;

// Create the trigger value and position vars now so that they don't need to be recreated every frame
float LtriggerVal;
float RtriggerVal;

Vector3 Lpos;
Vector3 Rpos;

// Create vars for getting the hit object from the raycast
RaycastHit rayHit;

Collider *LrayCol = nullptr;
GameObject *LrayGO = nullptr;

Collider *RrayCol = nullptr;
GameObject *RrayGO = nullptr;

// Making bools for these so I don't have to check the trigger like 4 times
bool LtriggerPressed = false;
bool RtriggerPressed = false;

GameObject *obj = nullptr;

DEFINE_TYPE(BangBang, ctypes);
namespace BangBang {
    void ctypes::Awake()
    {
        Array<VRController *>* controlArr = Resources::FindObjectsOfTypeAll<VRController*>();
        Lcontrol = controlArr->get(0);
        Rcontrol = controlArr->get(1);

        Array<Saber *>* saberArr = Resources::FindObjectsOfTypeAll<Saber*>();
        Lsaber = saberArr->get(0);
        Rsaber = saberArr->get(1);

        Array<CuttingManager *>* cutManArr = Resources::FindObjectsOfTypeAll<CuttingManager*>();
        cutMan = cutManArr->get(0);
    }
    void ctypes::Update()
    {
        //Reset everything so they don't get reused
        LrayCol = nullptr;
        LrayGO = nullptr;
        RrayCol = nullptr;
        RrayGO = nullptr;
        LtriggerPressed = false;
        RtriggerPressed = false;
        LtriggerVal = 0.0f;
        RtriggerVal = 0.0f;

        // Get the left trigger's value and store it temporarily
        LtriggerVal = Lcontrol->get_triggerValue();
        RtriggerVal = Rcontrol->get_triggerValue();

        // Getting the positions of the controllers
        Lpos = Lcontrol->get_position();
        Rpos = Rcontrol->get_position();

        if(LtriggerVal == 1.0f)
        {
            LtriggerPressed = true;

            getLogger().info("Left trigger pressed");
            if(Physics::Raycast(Lpos, Lpos.get_forward(), byref(rayHit), 10.0f))
            {
                getLogger().info("Got a hit; getting info");
                
                getLogger().info("Getting collider");
                LrayCol = rayHit.get_collider();

                getLogger().info("Getting gameObject");
                LrayGO = LrayCol->get_gameObject();

                getLogger().info("Left hit- %s", to_utf8(csstrtostr(LrayGO->get_name())).c_str());

                getLogger().info("Calling NoteCutter::Cut");
                cutMan->noteCutter->Cut(Lsaber);
            }
        }

        if(RtriggerVal == 1.0f)
        {
            RtriggerPressed = true;

            getLogger().info("Right trigger pressed");
            if(Physics::Raycast(Rpos, Rpos.get_forward(), byref(rayHit), 10.0f))
            {
                getLogger().info("Got a hit; getting info");
                
                getLogger().info("Getting collider");
                RrayCol = rayHit.get_collider();

                getLogger().info("Getting gameObject");
                RrayGO = RrayCol->get_gameObject();

                getLogger().info("Right hit- %s", to_utf8(csstrtostr(RrayGO->get_name())).c_str());

                getLogger().info("Calling NoteCutter::Cut");
                cutMan->noteCutter->Cut(Rsaber);
            }
        }
    }
}

// Literally just hooking this to make a GO with the custom types component kek
MAKE_HOOK_MATCH(SaberManager_Start, &SaberManager::Start, void, SaberManager* self) {
    SaberManager_Start(self);
    
    getLogger().info("Creating object");
    obj = UnityEngine::Resources::FindObjectsOfTypeAll<GameObject*>()->values[0];
    obj->AddComponent<BangBang::ctypes*>();
}

MAKE_HOOK_MATCH(NoteCutter_Cut, &NoteCutter::Cut, void, NoteCutter* self, Saber* saber)
{
    getLogger().info("Note cutter running");

    if(LtriggerPressed && LrayGO != nullptr)
    {
        getLogger().info("Running left saber part of note cutter");

        Vector3 pos = LrayGO->get_transform()->get_position();

        GameObject *sphere = obj->CreatePrimitive(PrimitiveType::Sphere);
        sphere->get_transform()->get_position() = pos;

        saber->saberBladeTopPos = Vector3(pos.x, pos.y, pos.z + 1.0f);
        saber->saberBladeBottomPos = Vector3(pos.x, pos.y, pos.z - 1.0f);
    }
    else if(RtriggerPressed && RrayGO != nullptr)
    {
        getLogger().info("Running right saber part of note cutter");

        Vector3 pos = RrayGO->get_transform()->get_position();

        GameObject *sphere = obj->CreatePrimitive(PrimitiveType::Sphere);
        sphere->get_transform()->get_position() = pos;

        saber->saberBladeTopPos = Vector3(pos.x, pos.y, pos.z + 1.0f);
        saber->saberBladeBottomPos = Vector3(pos.x, pos.y, pos.z - 1.0f);
    }

    NoteCutter_Cut(self, saber);
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();

    getLogger().info("Installing hooks...");

    INSTALL_HOOK(getLogger(), SaberManager_Start);
    INSTALL_HOOK(getLogger(), NoteCutter_Cut);

    getLogger().info("Installed all hooks!");
}
