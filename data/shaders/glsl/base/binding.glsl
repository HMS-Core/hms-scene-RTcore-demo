/** Scene set**/
#define SET_SCENE_INDEX 0
// Scene and model

#define B_CPR 0
#define B_TRIANGLE 1
#define B_BLAS 2
#define B_CBLAS_COUNT_INNER 3

#define B_CAMERA_MATRICES 4
#define B_SCENE_PARAMS 5
#define B_GLTF_TEXTURES 6
#define B_VERTEX 7
#define B_INDEX 8
#define B_AO 9

/**Input attachment set**/
#define SET_ATTACHMENT_INDEX 1
#define B_SUBPASS_COLOR_AO 0
#define B_SUBPASS_POSITION_ROUGHNESS 1
#define B_SUBPASS_NORMAL_METALNESS 2

/**IBL texture set**/
#define SET_IBL_INDEX 2
// ibl cube textures resources
#define B_IRRRADIANCE_CUBE_TEXTURE 0
#define B_PREFILTER_CUBE_TEXTURE 1
#define B_BRDFFLUT_CUBE_TEXTURE 2
/**GLTF Node uniform buffer set*/

#define SET_GLTF_NODE_INDEX 3
#define B_GLTF_NODE_BUFFER 0

#define INDEX_COLOR_AO 0
#define INDEX_POSITION_ROUGHNESS 1
#define INDEX_NORMAL_METALNESS 2

#define INPUT_ATTACHMENT_COLOR_AO 1
#define INPUT_ATTACHMENT_POSITION_ROUGHNESS 2
#define INPUT_ATTACHMENT_NORMAL_METALNESS 3
#define MAX_TEXTURE_NUM 20