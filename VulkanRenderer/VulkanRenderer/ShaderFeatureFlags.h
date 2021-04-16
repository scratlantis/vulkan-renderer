#pragma once
enum SHADER_FEATURE_FLAGS
{
    ENABLE_RAY_QUERY_DECALS         = 0x00000001,
    ENABLE_RAY_QUERY_LIGHTS         = 0x00000002,
    ENABLE_RAY_QUERY_DECAL_AABBS    = 0x00000004,
    ENABLE_RAY_QUERY_SHADOWS        = 0x00000008,
    ENABLE_RAY_QUERY_REFLECTIONS    = 0x00000010,
    ENABLE_AMBIENT_TERM             = 0x00000020,
    ENABLE_SCENE_POINT_LIGHTS       = 0x00000040,
    ENABLE_PRIMARY_POINT_LIGHT      = 0x00000080,
    ENABLE_EMISSIVE_TERM            = 0x00000100,
    ENABLE_EDITOR                   = 0x00000200
};

static std::string createShaderFeatureFlagsComlileArguments(uint32_t flags)
{
    std::string args = "";
    // Create argument string
    if (flags & ENABLE_AMBIENT_TERM)
    {
        args.append(" -DENABLE_AMBIENT_TERM");
    }
    if (flags & ENABLE_RAY_QUERY_DECALS)
    {
        args.append(" -DENABLE_RAY_QUERY_DECALS");
    }
    if (flags & ENABLE_RAY_QUERY_DECAL_AABBS)
    {
        args.append(" -DENABLE_RAY_QUERY_DECAL_AABBS");
    }
    if (flags & ENABLE_RAY_QUERY_LIGHTS)
    {
        args.append(" -DENABLE_RAY_QUERY_LIGHTS");
    }
    if (flags & ENABLE_RAY_QUERY_REFLECTIONS)
    {
        args.append(" -DENABLE_RAY_QUERY_REFLECTIONS");
    }
    if (flags & ENABLE_RAY_QUERY_SHADOWS)
    {
        args.append(" -DENABLE_RAY_QUERY_SHADOWS");
    }
    if (flags & ENABLE_SCENE_POINT_LIGHTS)
    {
        args.append(" -DENABLE_SCENE_POINT_LIGHTS");
    }
    if (flags & ENABLE_PRIMARY_POINT_LIGHT)
    {
        args.append(" -DENABLE_PRIMARY_POINT_LIGHT");
    }
    if (flags & ENABLE_EMISSIVE_TERM)
    {
        args.append(" -DENABLE_EMISSIVE_TERM");
    }
    if (flags & ENABLE_EDITOR)
    {
        args.append(" -DENABLE_EDITOR");
    }
    return args;
}


static std::string createShaderFeatureFlagsWindowName(uint32_t flags)
{
    std::string args = "";
    // Create argument string
    args.append(" (1)RQ_DECALS:");
    args.append((flags & ENABLE_RAY_QUERY_DECALS) ? "[ON]" : "[OFF]");

    args.append(" (2)RQ_LIGHTS:");
    args.append((flags & ENABLE_RAY_QUERY_LIGHTS) ? "[ON]" : "[OFF]");

    args.append(" (3)RQ_AABBS:");
    args.append((flags & ENABLE_RAY_QUERY_DECAL_AABBS) ? "[ON]" : "[OFF]");

    args.append(" (4)RQ_SHADOWS:");
    args.append((flags & ENABLE_RAY_QUERY_SHADOWS) ? "[ON]" : "[OFF]");

    args.append(" (5)RQ_REFLECTIONS:");
    args.append((flags & ENABLE_RAY_QUERY_REFLECTIONS) ? "[ON]" : "[OFF]");

    args.append(" (6)AMBIENT:");
    args.append((flags & ENABLE_AMBIENT_TERM) ? "[ON]" : "[OFF]");

    args.append(" (7)SCENE_LIGHTS:");
    args.append((flags & ENABLE_SCENE_POINT_LIGHTS) ? "[ON]" : "[OFF]");

    args.append(" (8)PRIMARY_LIGHT:");
    args.append((flags & ENABLE_PRIMARY_POINT_LIGHT) ? "[ON]" : "[OFF]");

    args.append(" (9)EMISSIVE:");
    args.append((flags & ENABLE_EMISSIVE_TERM) ? "[ON]" : "[OFF]");

    args.append(" (0)EDITOR:");
    args.append((flags & ENABLE_EDITOR) ? "[ON]" : "[OFF]");
    return args;
}