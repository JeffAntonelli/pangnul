#pragma once

#include <SFML/System/Time.hpp>

#include "game_globals.h"

namespace game
{
	struct Background
	{
		
	};

	class GameManager;
	class BackgroundManager : public core::ComponentManager<Background, core::EntityMask(ComponentType::BACKGROUND)>
	{
	public:
		explicit BackgroundManager(core::EntityManager& entityManager, GameManager& gameManager);
		void FixedUpdate(sf::Time dt);
	private:
		GameManager& gameManager_;
	};
}
//#include <SFML/Graphics/VertexArray.hpp>
//
//#include "graphics/graphics.h"
//
//namespace sf
//{
//class RenderTarget;
//}
//
//namespace game
//{
//
//class StarBackground : public core::DrawInterface
//{
//public:
//    void Init();
//    void Draw(sf::RenderTarget& window) override;
//private:
//    static constexpr std::size_t starCount = 1024;
//    sf::VertexArray vertexArray_;
//};
//
//}
