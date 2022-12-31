#include "Animation3D.h"
#include "system/platform.h"
#include "graphics/scene.h"
#include "graphics/mesh.h"
#include "graphics/skinned_mesh_instance.h"
#include "graphics/renderer_3d.h"
#include "animation/skeleton.h"
#include "animation/animation.h"
#include "system/string_id.h"
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

AsdfAnim::Animation3D::Animation3D() : p_Scene(nullptr), p_Mesh(nullptr), p_MeshInstance(nullptr), p_CurrentAnimation(nullptr), m_BlendInfo{},
m_Activeplayer(&m_AnimationPlayers[0]), m_BodyVelociy(0.f), p_BlendTree(nullptr), p_Ragdoll(nullptr), m_NeedsPhysicsUpdate(false)
{
}

AsdfAnim::Animation3D::~Animation3D()
{
    if (p_BlendTree)    delete p_BlendTree, p_BlendTree = nullptr;
    if (p_Ragdoll)      delete p_Ragdoll, p_Ragdoll = nullptr;
}

AsdfAnim::Animation3D* AsdfAnim::Animation3D::CreateFromSceneFile(gef::Platform& platform, const char* filepath)
{
    Animation3D* animation = new Animation3D();
    animation->LoadScene(platform, filepath);
    animation->SetType(AnimationType::Animation_Type_3D);
    return animation;
}

void AsdfAnim::Animation3D::LoadScene(gef::Platform& platform, const char* filepath)
{
    // Change the current working directory, and restore it at the end
    std::filesystem::path file(filepath);
    if (!std::filesystem::exists(file))
    {
        MessageBox(NULL, L"Could not load the animation: file does not exists.", L"Error!", NULL);
        return;
    }

    // Read the data from the file
    p_Scene = new gef::Scene();
    p_Scene->ReadSceneFromFile(platform, filepath);
    if (!p_Scene->mesh_data.size() || !p_Scene->skeletons.size())
    {
        MessageBox(NULL, L"Could not load the animation: file does not contain any mesh or skeleton.", L"Error!", NULL);
        return;
    }

    // Read the filename
    s_Filename = file.filename().replace_extension("").string();

    // Initialise the animation data from the scene data
    p_Scene->CreateMaterials(platform);
    p_Mesh = p_Scene->CreateMesh(platform, p_Scene->mesh_data.front());
    gef::Skeleton* skeleton = p_Scene->skeletons.front();
    p_MeshInstance = new gef::SkinnedMeshInstance(*skeleton);
    p_MeshInstance->set_mesh(p_Mesh);
    gef::Matrix44 identity;
    identity.SetIdentity();
    p_MeshInstance->set_transform(identity);
    for (uint32_t i = 0u; i < m_AnimationPlayers.size(); ++i)
    {
        m_AnimationPlayers[i].Init(p_MeshInstance->bind_pose());
        m_AnimationPlayers[i].set_looping(true);
        m_AnimationPlayers[i].set_anim_time(0.f);
    }
    m_BlendedPose = p_MeshInstance->bind_pose();

    // Load all the animations for that filname
    for (const auto& entry : std::filesystem::directory_iterator(file.parent_path()))
    {
        // Get the current file in the directory
        std::filesystem::path animationFile = entry.path().filename();
        size_t temp = animationFile.string().find(s_Filename + "@");
        if (temp == std::string::npos) continue;

        // If the file contains the same name as the scene file and '@', this is a valid animation file and all the information it contains should be loaded
        gef::Scene tempScene;
        tempScene.ReadSceneFromFile(platform, entry.path().string().c_str());
        for (auto& animIterator : tempScene.animations)
        {
            //Determine the type of this clip if possible
            ClipType clipType = ClipType::Clip_Type_Undefined;
            std::string name;
            //Brut force, very bad
            if(animationFile.string().find("idle", s_Filename.size() + 1) != std::string::npos)         {   clipType = ClipType::Clip_Type_Idle;    name = "idle";     }
            else if(animationFile.string().find("walking", s_Filename.size() + 1) != std::string::npos) {   clipType = ClipType::Clip_Type_Walk;    name = "walking";  }
            else if(animationFile.string().find("walk", s_Filename.size() + 1) != std::string::npos)    {   clipType = ClipType::Clip_Type_Walk;    name = "walk";     }
            else if(animationFile.string().find("running", s_Filename.size() + 1) != std::string::npos) {   clipType = ClipType::Clip_Type_Run;     name = "running";  }
            else if(animationFile.string().find("run", s_Filename.size() + 1) != std::string::npos)     {   clipType = ClipType::Clip_Type_Run;     name = "run";      }
            else if(animationFile.string().find("jump", s_Filename.size() + 1) != std::string::npos)    {   clipType = ClipType::Clip_Type_Jump;    name = "jump";     }
            else if(animationFile.string().find("fall", s_Filename.size() + 1) != std::string::npos)    {   clipType = ClipType::Clip_Type_Fall;    name = "fall";     }

            // Create a new clip
            Clip clip{
                new gef::Animation(std::move(*animIterator.second)),
                clipType,
                name
            };
            v_Clips.push_back(std::move(clip));
            //v_AvailableAnimations.push_back(tempScene.string_id_table.table().at(animIterator.first));    // This won't work cause the gef loader only saves one animation
            v_AvailableClips.push_back(entry.path().filename().replace_extension("").string());             // Save the filename instead
        }
    }

    // TODO: Handle the case of no animation loaded, should only display 3d mesh and maybe a message box saying no animation was loaded
    // Initialise the first clip player on the first loaded animation
    p_CurrentAnimation = &v_Clips.front();
    m_AnimationPlayers[0].set_clip(p_CurrentAnimation->clip);

    // Init blend tree
    p_BlendTree = new BlendTree(p_MeshInstance->bind_pose());
    uint32_t idleNodeID = p_BlendTree->AddNode(NodeType_::NodeType_Clip);
    ClipNode* idleNode = reinterpret_cast<ClipNode*>(p_BlendTree->GetNode(idleNodeID));
    idleNode->SetClip(p_CurrentAnimation);
    p_BlendTree->ConnectToRoot(idleNodeID);
}

void AsdfAnim::Animation3D::LoadRagdoll(btDiscreteDynamicsWorld* pbtDynamicWorld, const char* filepath)
{
    p_Ragdoll = new Ragdoll;
    p_Ragdoll->Init(p_BlendTree->GetBindPose(), pbtDynamicWorld, filepath);
}

void AsdfAnim::Animation3D::Update(float frameTime)
{
    m_NeedsPhysicsUpdate = false;
    p_BlendTree->Update(frameTime, m_NeedsPhysicsUpdate);
    p_MeshInstance->UpdateBoneMatrices(p_BlendTree->GetOutputPose());
}

void AsdfAnim::Animation3D::Draw(gef::Renderer3D* renderer) const
{
    renderer->DrawSkinnedMesh(*p_MeshInstance, p_MeshInstance->bone_matrices());
}

void AsdfAnim::Animation3D::TransitionToAnimation(const size_t animIndex, float transitionTime, TransitionType transitionType)
{
    // Signal the transition
    m_BlendInfo.blend = true;
    m_BlendInfo.blendEndTime = transitionTime;
    m_BlendInfo.blendType = transitionType;
    ++m_BlendInfo.blendClipPlayerIndex;
    m_BlendInfo.blendClipPlayerIndex *= static_cast<uint32_t>(m_BlendInfo.blendClipPlayerIndex < m_AnimationPlayers.size());

    p_CurrentAnimation = &v_Clips[animIndex];
    m_AnimationPlayers[m_BlendInfo.blendClipPlayerIndex].set_clip(p_CurrentAnimation->clip);
    m_AnimationPlayers[m_BlendInfo.blendClipPlayerIndex].set_anim_time(0.f);
}

const AsdfAnim::Clip * AsdfAnim::Animation3D::GetClip(const size_t animIndex) const
{
    return &v_Clips[animIndex];
}

const gef::Matrix44& AsdfAnim::Animation3D::GetMeshTransform()
{
    return p_MeshInstance->transform();
}

void AsdfAnim::Animation3D::SetMeshTransform(const gef::Matrix44& transform)
{
    p_MeshInstance->set_transform(transform);
}

const std::string& AsdfAnim::Animation3D::GetFileName()
{
    return s_Filename;
}
