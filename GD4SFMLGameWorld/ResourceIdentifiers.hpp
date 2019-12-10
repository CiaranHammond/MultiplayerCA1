#pragma once
#include "TextureID.hpp"
#include "FontID.hpp"
#include "ShaderID.hpp"

namespace sf
{
	class Texture;
	class Font;
	class Shader;
}

template<typename Resource, typename Identifier>
class ResourceHolder;

typedef ResourceHolder<sf::Texture, TextureID> TextureHolder;
typedef ResourceHolder<sf::Font, FontID> FontHolder;
typedef ResourceHolder<sf::Shader, ShaderID> ShaderHolder;