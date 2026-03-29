#include <openssl/evp.h>
#include <openssl/ssl.h>



#include "menu.h"
#include "SDK/entity/CWeaponServices.h"
#include "SDK/weapon/C_EconEntity.h"
#include "SDK/CEconItemAttributeManager.h"
#include "update_offsets.h"

void UpdateActiveMenuDef(const uintptr_t localPlayer)
{
    // Implementation for HUD update signal
}

void SetMeshMask(uintptr_t entity, uint64_t mask) {
    if (!entity) return;
    uintptr_t gameSceneNode = mem.Read<uintptr_t>(entity + Offsets::m_pGameSceneNode);
    if (!gameSceneNode) return;

    // CS2 uses m_pDirtyModelData + m_DrityMeshGroupMask to signal a mesh update
    // Setting mask to 2 usually forces the game to refresh the model and materials
    mem.Write<uint64_t>(gameSceneNode + Offsets::m_modelState + Offsets::m_MeshGroupMask, mask);
    
    uintptr_t pDirtyModelData = mem.Read<uintptr_t>(entity + Offsets::m_pDirtyModelData);
    if (pDirtyModelData) {
        mem.Write<uint32_t>(pDirtyModelData + Offsets::m_DrityMeshGroupMask, 1);
    }
}

void Radar()
{
    for (uint8_t i = 1; i < 64; i++)
    {
        const uintptr_t Controller = GetEntityByHandle(i);
        if(!Controller)
			continue;
        const uintptr_t Pawn = GetEntityByHandle(mem.Read<uint16_t>(Controller + 0x6B4));
		if (!Pawn)
            continue;

		const uintptr_t entitySpotted = Pawn + 0x2700;
        mem.Write<bool>(entitySpotted + 0x8, true);
    }
}

class CBaseEntity
{
public:
	char pad0[0x330];//0x000
	uintptr_t sceneNode; //0x330
};

void su()
{
    INPUT input = { 0 };
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VK_SPACE;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void sd()
{
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VK_SPACE;
    SendInput(1, &input, sizeof(INPUT));
}

int main()
{
    mem.Write<uint16_t>(Sigs::RegenerateWeaponSkins + 0x52, static_cast<uint16_t>(Offsets::m_AttributeManager + Offsets::m_Item + Offsets::m_AttributeList + Offsets::m_Attributes));

    skindb->Dump();
    Updater::UpdateOffsets();
    
    InitMenu();
    
    configManager->Setup();
    configManager->AutoLoad();

    std::cout << "SkinChanger Started\n";

    while (true)
    {
        Sleep(5);

        const uintptr_t localController = GetLocalController();
        const uintptr_t inventoryServices = mem.Read<uintptr_t>(localController + Offsets::m_pInventoryServices);
        const uintptr_t localPlayer = GetLocalPlayer();
        const uintptr_t pWeaponServices = mem.Read<uintptr_t>(localPlayer + Offsets::m_pWeaponServices);

        mem.Write<uint16_t>(inventoryServices + Offsets::m_unMusicID, skinManager->MusicKit.id);

        // --- Music Kit Update Logic ---
        static uint16_t lastMusicID = 0;
        if (skinManager->MusicKit.id != lastMusicID || ForceUpdate) {
            mem.Write<uint16_t>(inventoryServices + Offsets::m_unMusicID, skinManager->MusicKit.id);
            lastMusicID = skinManager->MusicKit.id;
        }

        UpdateActiveMenuDef(localPlayer);
        OnFrame();

        bool ShouldUpdate = false;

        const std::vector<uintptr_t> weapons = GetWeapons(localPlayer);
    /*
    //clean up & hud update

    for (const uintptr_t& weapon : weapons)
    {
        const uintptr_t item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
        
        if (mem.Read<uint32_t>(weapon + Offsets::m_nFallbackPaintKit) == -1)
            continue;

        mem.Write<uint32_t>(weapon + Offsets::m_nFallbackPaintKit, -1);

        //UpdateHud(weapon);

        econItemAttributeManager.Remove(item);
        continue;
    }
    */
        for (const uintptr_t& weapon : weapons)
        {
            const uintptr_t item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
            
            if(ForceUpdate)
                mem.Write<uint32_t>(item + Offsets::m_iItemIDHigh, NULL);

            if (mem.Read<uint32_t>(item + Offsets::m_iItemIDHigh) == -1)
                continue;

            mem.Write<uint32_t>(item + Offsets::m_iItemIDHigh, -1);

            SkinInfo_t skin = GetSkin(item);
            if (!skin.Paint && skin.weaponType != WeaponsEnum::CtKnife && skin.weaponType != WeaponsEnum::Tknife) 
                continue;

            // --- Knife Changer Support ---
            if (skin.weaponType == WeaponsEnum::CtKnife || skin.weaponType == WeaponsEnum::Tknife) {
                if (skinManager->Knife.defIndex != 0) {
                    // Overwrite item definition index for the knife
                    mem.Write<uint16_t>(item + Offsets::m_iItemDefinitionIndex, skinManager->Knife.defIndex);
                    // Set quality to 3 (Star)
                    mem.Write<int>(item + Offsets::m_iEntityQuality, 3);
                }
            }

            mem.Write<uint32_t>(weapon + Offsets::m_nFallbackPaintKit, skin.Paint);

            const uint64_t mask = skin.bUsesOldModel + 1;

            const uintptr_t hudWeapon = GetHudWeapon(weapon);
            SetMeshMask(weapon, mask);
            SetMeshMask(hudWeapon, mask);

            //mem.Write<ItemCustomName_t>(item + Offsets::m_szCustomNameOverride, ItemCustomName_t("this is a custom name"));

            econItemAttributeManager.Create(item, skin);

            ShouldUpdate = true;
        }

        // --- Knife & Viewmodel Update ---
        uintptr_t pViewModelServices = mem.Read<uintptr_t>(localPlayer + Offsets::m_pViewModelServices);
        if (pViewModelServices) {
            uint32_t hViewModel = mem.Read<uint32_t>(pViewModelServices + Offsets::m_hViewModel);
            if (hViewModel != 0xFFFFFFFF) {
                uintptr_t viewModel = GetEntityByHandle(hViewModel & 0x7FFF);
                if (viewModel) {
                     uint32_t activeWeaponHandle = mem.Read<uint32_t>(pWeaponServices + Offsets::m_hActiveWeapon);
                     uintptr_t activeWeapon = GetEntityByHandle(activeWeaponHandle & 0x7FFF);
                     if (activeWeapon) {
                         uintptr_t item = activeWeapon + Offsets::m_AttributeManager + Offsets::m_Item;
                         uint16_t activeDef = mem.Read<uint16_t>(item + Offsets::m_iItemDefinitionIndex);
                         
                         // CT: 42, T: 59, or custom knives
                         if (activeDef == 42 || activeDef == 59 || (activeDef >= 500 && activeDef <= 526)) {
                             if (skinManager->Knife.defIndex != 0) {
                                 mem.Write<uint16_t>(viewModel + Offsets::m_iItemDefinitionIndex, skinManager->Knife.defIndex);
                                 mem.Write<int>(viewModel + Offsets::m_iEntityQuality, 3);
                                 
                                 SkinInfo_t knifeSkin = skinManager->GetKnife();
                                 econItemAttributeManager.Create(viewModel, knifeSkin);
                             }
                         }
                     }
                }
            }
        }

        // --- Glove Changer Logic ---
        if (skinManager->Gloves.defIndex != 0)
        {
             const uintptr_t localPlayerPawn = GetLocalPlayer(); 
             if (localPlayerPawn) {
                 const uintptr_t econGloves = localPlayerPawn + Offsets::m_EconGloves;
                 
                 static uint16_t lastTargetDef = 0;
                 static uint32_t lastTargetPaint = 0;
                 static bool firstRun = true;

                 if (firstRun || skinManager->Gloves.defIndex != lastTargetDef || skinManager->Gloves.Paint != lastTargetPaint || ForceUpdate) {
                     std::cout << "[GloveUpdate] TargetDef: " << skinManager->Gloves.defIndex << " | TargetPaint: " << skinManager->Gloves.Paint << std::endl;
                     
                     mem.Write<uint16_t>(econGloves + Offsets::m_iItemDefinitionIndex, skinManager->Gloves.defIndex);
                     mem.Write<uint32_t>(econGloves + Offsets::m_iItemIDHigh, 0xFFFFFFFF);
                     mem.Write<uint32_t>(econGloves + Offsets::m_iAccountID, 0);
                     mem.Write<bool>(econGloves + Offsets::m_bInitialized, true);
                     mem.Write<int>(econGloves + Offsets::m_iEntityQuality, 3);

                     SkinInfo_t gloveSkin;
                     gloveSkin.Paint = skinManager->Gloves.Paint;
                     gloveSkin.defIndex = skinManager->Gloves.defIndex;
                     gloveSkin.weaponType = WeaponsEnum::none;
                     econItemAttributeManager.Create(econGloves, gloveSkin);
                     mem.Write<bool>(localPlayerPawn + Offsets::m_bNeedToReApplyGloves, true);

                     lastTargetDef = skinManager->Gloves.defIndex;
                     lastTargetPaint = skinManager->Gloves.Paint;
                     firstRun = false;
                 }
             }
        }

        if (ShouldUpdate || ForceUpdate)
            UpdateWeapons(weapons);

        ForceUpdate = false;
        
        if (ShouldUpdate) {
            configManager->AutoSave();
        }
    }
    
    return 0;
}