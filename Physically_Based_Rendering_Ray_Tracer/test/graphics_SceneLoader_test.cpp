#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

// 打印节点树结构
void printNodeTree(aiNode* node, int depth, std::map<int, int>& levelCount) {
    if (!node) return;
    
    levelCount[depth]++;
    
    std::string indent(depth * 2, ' ');
    std::cout << indent << "|- " << node->mName.C_Str() 
              << " (meshes: " << node->mNumMeshes << ") [";
    
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        std::cout << node->mMeshes[i];
        if (i < node->mNumMeshes - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        printNodeTree(node->mChildren[i], depth + 1, levelCount);
    }
}

void testAssimpLoader(const std::string& filepath) {
    std::cout << "========================================" << std::endl;
    std::cout << "Testing Assimp Loader: " << filepath << std::endl;
    std::cout << "========================================" << std::endl;
    
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType
    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return;
    }
    
    // 基本信息
    std::cout << "\n[Basic Info]" << std::endl;
    std::cout << "  Material Count: " << scene->mNumMaterials << std::endl;
    std::cout << "  aiMesh Count:   " << scene->mNumMeshes << std::endl;
    
    // 打印每个 aiMesh 的信息
    std::cout << "\n[aiMesh Details]" << std::endl;
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        std::cout << "  aiMesh[" << i << "]: \"" << mesh->mName.C_Str() << "\""
                  << " | vertices: " << mesh->mNumVertices
                  << " | faces: " << mesh->mNumFaces
                  << " | materialIndex: " << mesh->mMaterialIndex
                  << std::endl;
    }
    
    // 打印材质信息
    std::cout << "\n[Material Details]" << std::endl;
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        aiMaterial* mat = scene->mMaterials[i];
        aiString name;
        mat->Get(AI_MATKEY_NAME, name);
        
        aiColor4D baseColor(1, 1, 1, 1);
        mat->Get(AI_MATKEY_BASE_COLOR, baseColor);
        
        float metallic = 0.0f, roughness = 1.0f;
        mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
        mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
        
        std::cout << "  Material[" << i << "]: \"" << name.C_Str() << "\""
                  << " | baseColor: (" << baseColor.r << ", " << baseColor.g << ", " << baseColor.b << ")"
                  << " | metallic: " << metallic
                  << " | roughness: " << roughness
                  << std::endl;
    }
    
    // 打印节点树
    std::cout << "\n[Node Tree Structure]" << std::endl;
    std::map<int, int> levelCount;
    printNodeTree(scene->mRootNode, 0, levelCount);
    
    // 打印每层节点数量
    std::cout << "\n[Nodes Per Level]" << std::endl;
    for (const auto& pair : levelCount) {
        std::cout << "  Level " << pair.first << ": " << pair.second << " node(s)" << std::endl;
    }
    
    std::cout << "\n========================================" << std::endl;
}

//int main() {
//    testAssimpLoader("resource/cornell_box/scene.gltf");
//    return 0;
//}
