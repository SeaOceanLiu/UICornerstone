#ifndef RAYLIBCOMPAT_H
#define RAYLIBCOMPAT_H

// RaylibCompat.h
// Renames raylib types that conflict with UIControls abstractions
// before including raylib.h, then provides friendly aliases.

#define Font        rlFont_
#define Texture     rlTexture_
#define MouseButton rlMouseButton_
#define BlendMode   rlBlendMode_

#include "raylib.h"

#undef Font
#undef Texture
#undef MouseButton
#undef BlendMode

using rlFont       = rlFont_;
using rlTexture    = rlTexture_;
using rlMouseButton = int;

#endif
