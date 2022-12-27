#include "AnimationManager.h"
#include "system/platform.h"
#include "Animation3D.h"
#include "Animation2D.h"
#include "AnimatedSprite.h"
#include "graphics/renderer_3d.h"
#include <filesystem>
// No need to include gef::Platform because it is unused in this manager


AsdfAnim::AnimationManager::AnimationManager(gef::Platform& platform) : r_Platform(platform)
{
}

AsdfAnim::AnimationManager::~AnimationManager()
{
}

void AsdfAnim::AnimationManager::Update(float frameTime)
{
    for (auto anim : v_LoadedAnimations2D)
        if(anim->IsActive())
            anim->Update(frameTime);
    for (auto anim : v_LoadedAnimations3D)
        if (anim->IsActive())
            anim->Update(frameTime);
}


void AsdfAnim::AnimationManager::Draw2D(gef::SpriteRenderer* pRenderer2D)
{
    for (auto anim : v_LoadedAnimations2D)
        if (anim->IsActive())
            anim->Render(pRenderer2D);
}

void AsdfAnim::AnimationManager::Draw3D(gef::Renderer3D* pRenderer3D)
{
    for (auto anim : v_LoadedAnimations3D)
        if (anim->IsActive())
            anim->Draw(pRenderer3D);
}

void AsdfAnim::AnimationManager::LoadGef3D(const char* filename)
{
	Animation3D* animation = Animation3D::CreateFromSceneFile(r_Platform, filename);
    v_LoadedAnimations3D.push_back(animation);
    v_AvailableFiles.push_back(&animation->GetFileName());
}

void AsdfAnim::AnimationManager::LoadAllGef3DFromFolder(const char* folderpath, bool recursiveSearch)
{
    std::filesystem::path folder(folderpath);
    if (folder.empty()) folder = std::filesystem::current_path();
    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        const std::string& entryPath(entry.path().string());
        const std::string& entryName(entry.path().filename().string());
        const std::string& entryExt(entry.path().filename().extension().string());

        if (entry.is_directory() && recursiveSearch)
            LoadAllGef3DFromFolder(entryPath.c_str(), recursiveSearch);

        if (!entryExt.compare(".scn") && entryName.find('@') == std::string::npos)  // compare() returns 0 when equal
            LoadGef3D(entryPath.c_str());
    }
}

void AsdfAnim::AnimationManager::LoadDragronbone2D(const char* filename)
{
}

void AsdfAnim::AnimationManager::LoadAllDragonbone2DFromFolder(const char* folderpath, bool recursiveSearch)
{
}

void AsdfAnim::AnimationManager::LoadDragronbone2DJson(const char* filename)
{
    Animation2D* animation = Animation2D::CreateFromJSON(r_Platform, filename);
    v_LoadedAnimations2D.push_back(animation);
    v_AvailableFiles.push_back(&animation->GetFileName());
}

void AsdfAnim::AnimationManager::LoadAllDragonbone2DJsonFromFolder(const char* folderpath, bool recursiveSearch)
{
    std::filesystem::path folder(folderpath);
    if (folder.empty()) folder = std::filesystem::current_path();
    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        const std::string& entryPath(entry.path().string());
        //const std::string& entryName(entry.path().filename().string());

        if (entry.is_directory() && recursiveSearch)
        {
            LoadAllDragonbone2DJsonFromFolder(entryPath.c_str(), recursiveSearch);
            continue;
        }

        size_t commonNameMarker = entryPath.find("_ske.json");
        if (commonNameMarker != std::string::npos)  // compare() returns 0 when equal
        {
            LoadDragronbone2DJson(entryPath.substr(0u, commonNameMarker).c_str());
        }
    }
}

const std::vector<const std::string*>& AsdfAnim::AnimationManager::GetAvailableFileNames()
{
    // This function should be called by the GUI to list all the available animators
    return v_AvailableFiles;
}

const std::vector<AsdfAnim::Animation2D*>& AsdfAnim::AnimationManager::GetAvailable2DDatas()
{
    return v_LoadedAnimations2D;
}

const std::vector<AsdfAnim::Animation3D*>& AsdfAnim::AnimationManager::GetAvailable3DDatas()
{
    return v_LoadedAnimations3D;
}