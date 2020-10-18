#include "WorldRaycastRenderer.h"

#include <array>
#include <vector>

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/System/Vector2.hpp>

#include <Box2D/Common/b2Math.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Dynamics/b2WorldCallbacks.h>

#include <spdlog/spdlog.h>

#include "Quiver/Graphics/Camera3D.h"
#include "Quiver/Graphics/FixtureRenderData.h"
#include "Quiver/Graphics/RenderSettings.h"
#include "Quiver/World/World.h"

namespace {

void glCheckError(const char* file, unsigned int line, const char* expression)
{
	// Get the last error
	GLenum errorCode = glGetError();

	if (errorCode != GL_NO_ERROR)
	{
		std::string fileString = file;
		std::string error = "Unknown error";
		std::string description = "No description";

		// Decode the error code
		switch (errorCode)
		{
		case GL_INVALID_ENUM:
		{
			error = "GL_INVALID_ENUM";
			description = "An unacceptable value has been specified for an enumerated argument.";
			break;
		}

		case GL_INVALID_VALUE:
		{
			error = "GL_INVALID_VALUE";
			description = "A numeric argument is out of range.";
			break;
		}

		case GL_INVALID_OPERATION:
		{
			error = "GL_INVALID_OPERATION";
			description = "The specified operation is not allowed in the current state.";
			break;
		}

		case GL_STACK_OVERFLOW:
		{
			error = "GL_STACK_OVERFLOW";
			description = "This command would cause a stack overflow.";
			break;
		}

		case GL_STACK_UNDERFLOW:
		{
			error = "GL_STACK_UNDERFLOW";
			description = "This command would cause a stack underflow.";
			break;
		}

		case GL_OUT_OF_MEMORY:
		{
			error = "GL_OUT_OF_MEMORY";
			description = "There is not enough memory left to execute the command.";
			break;
		}

		//case GLEXT_GL_INVALID_FRAMEBUFFER_OPERATION:
		//{
		//	error = "GL_INVALID_FRAMEBUFFER_OPERATION";
		//	description = "The object bound to FRAMEBUFFER_BINDING is not \"framebuffer complete\".";
		//	break;
		//}
		}

		auto log = spdlog::get("console");

		if (!log) {
			assert(log);
			return;
		}

		log->error(
			"An internal OpenGL call failed in {}({}).\n"
			"Expression:\n   {} \n"
			"Error description:\n   {} \n   {}",
			fileString.substr(fileString.find_last_of("\\/") + 1),
			line,
			expression,
			error,
			description);
	}
}

#ifndef DO_GL_CHECK
#define DO_GL_CHECK (DEBUG)
#endif

#ifdef DO_GL_CHECK

// The do-while loop is needed so that glCheck can be used as a single statement in if/else branches
#define glCheck(expr) do { expr; glCheckError(__FILE__, __LINE__, #expr); } while (false)

#else

// Else, we don't add any overhead
#define glCheck(expr) (expr)

#endif

inline sf::Vector2f B2VecToSFVec(const b2Vec2& b2vec) {
	return sf::Vector2f(b2vec.x, b2vec.y);
}

}

namespace qvr {

class WorldRaycastRendererImpl {

	struct RayIntersection
	{
		b2Fixture* m_fixture;
		b2Vec2 m_point;
		b2Vec2 m_normal;
		b2Vec2 m_backPoint;
		float32 m_fraction;
		int m_screenX;
		enum class Facing
		{
			Towards,
			Away
		};
		Facing m_facing;
	};

	struct IntersectionCollection
	{
		static const unsigned sm_MaxNumIntersections = 32;

		std::array<RayIntersection, sm_MaxNumIntersections> m_Intersections;

		int m_IntersectionCount = 0;
		int m_Index = 0;
	};

	class ForwardRaycastCallback : public b2RayCastCallback
	{
	public:
		IntersectionCollection* m_Collection;

		float32 ReportFixture(
			b2Fixture* fixture,
			const b2Vec2& point,
			const b2Vec2& normal,
			float32 fraction)
			override;
	};

	class BackwardRaycastCallback : public b2RayCastCallback
	{
	public:
		IntersectionCollection* m_Collection;

		float32 ReportFixture(
			b2Fixture* fixture,
			const b2Vec2& point,
			const b2Vec2& normal,
			float32 fraction)
			override;
	};

	std::vector<IntersectionCollection> m_IntersectionsPerColumn;

	std::vector<ForwardRaycastCallback> m_ForwardRaycastCallbacks;
	std::vector<BackwardRaycastCallback> m_BackwardRaycastCallbacks;

	std::vector<RayIntersection> m_AllIntersections;

	struct Drawable {
		float m_Top;
		float m_Bottom;
		float m_X;
		float m_DistanceFar;
		float m_DistanceNear;
		float m_U;
		float m_VTop;
		float m_VBottom;
		float m_FarY;
		sf::Color m_BlendColor;
		sf::Vector3f m_Normal;
		const sf::Texture* m_Texture;
		bool m_DrawTop;
		bool m_DrawBottom;
	};

	std::vector<Drawable> m_AllColumns;

	sf::Shader mShader;

	void LoadShader();

public:
	WorldRaycastRendererImpl()
	{
		LoadShader();
	}

	void Render(
		const World& world, 
		const Camera3D& camera, 
		const RenderSettings& settings, 
		sf::RenderTarget& target);
};

void WorldRaycastRendererImpl::Render(
	const World & world, 
	const Camera3D & camera, 
	const RenderSettings& settings, 
	sf::RenderTarget & target)
{
	assert(world.GetPhysicsWorld());

	const auto targetWidth = target.getSize().x;

	if (m_ForwardRaycastCallbacks.size() != targetWidth)
	{
		m_IntersectionsPerColumn.resize(targetWidth);

		m_ForwardRaycastCallbacks.resize(targetWidth);
		m_BackwardRaycastCallbacks.resize(targetWidth);

		m_AllIntersections.reserve(targetWidth * IntersectionCollection::sm_MaxNumIntersections);

		m_AllColumns.reserve(m_AllIntersections.size());
	}

	m_AllIntersections.resize(0);
	m_AllColumns.resize(0);

	for (unsigned i = 0; i < targetWidth; ++i)
	{
		m_IntersectionsPerColumn[i].m_Index = i;
		m_IntersectionsPerColumn[i].m_IntersectionCount = 0;
	}

	for (unsigned i = 0; i < targetWidth; ++i)
	{
		m_ForwardRaycastCallbacks[i].m_Collection = &m_IntersectionsPerColumn[i];
		m_BackwardRaycastCallbacks[i].m_Collection = &m_IntersectionsPerColumn[i];
	}

	auto DoRaycastForwards = [&](ForwardRaycastCallback& cb)
	{
		const b2World& physicsWorld = *world.GetPhysicsWorld();

		const auto cameraPosition = camera.GetPosition();
		const auto cameraForwards = camera.GetForwards();
		const float screenXDelta = 2.0f / (float)targetWidth;

		const float viewPlaneWidthModifier = camera.GetViewPlaneWidthModifier();
		// The 'view plane' vector is the camera's right-vector, stretched/squashed a bit:
		const b2Vec2 viewPlane(
			cameraForwards.y * viewPlaneWidthModifier * (-1),
			cameraForwards.x * viewPlaneWidthModifier);

		// Cheeky wee lambda to calculate the end point of the ray.
		const auto rayEnd = [&]()
		{
			const auto screenX = -1.0f + screenXDelta * cb.m_Collection->m_Index;
			auto rayDir = (cameraForwards + (screenX * viewPlane));
			rayDir.Normalize();
			return cameraPosition + (settings.m_RayLength * rayDir);
		}();

		physicsWorld.RayCast(&cb, cameraPosition, rayEnd);
	};

	std::for_each(
		m_ForwardRaycastCallbacks.begin(),
		m_ForwardRaycastCallbacks.end(),
		DoRaycastForwards);

	auto DoRaycastBackwards = [&](BackwardRaycastCallback& cb)
	{
		const b2World& physicsWorld = *world.GetPhysicsWorld();

		const auto cameraPosition = camera.GetPosition();
		const auto cameraForwards = camera.GetForwards();
		const float screenXDelta = 2.0f / (float)targetWidth;

		const float viewPlaneWidthModifier = camera.GetViewPlaneWidthModifier();
		// The 'view plane' vector is the camera's right-vector, stretched/squashed a bit:
		const b2Vec2 viewPlane(
			cameraForwards.y * viewPlaneWidthModifier * (-1),
			cameraForwards.x * viewPlaneWidthModifier);

		// Cheeky wee lambda to calculate the end point of the ray.
		const auto rayEnd = [&]()
		{
			const auto screenX = -1.0f + screenXDelta * cb.m_Collection->m_Index;
			auto rayDir = (cameraForwards + (screenX * viewPlane));
			rayDir.Normalize();
			return cameraPosition + (settings.m_RayLength * rayDir);
		}();

		physicsWorld.RayCast(&cb, rayEnd, cameraPosition);
	};

	std::for_each(
		m_BackwardRaycastCallbacks.begin(),
		m_BackwardRaycastCallbacks.end(),
		DoRaycastBackwards);

	std::for_each(
		m_IntersectionsPerColumn.begin(),
		m_IntersectionsPerColumn.end(),
		[](IntersectionCollection& collection)
		{
			// sort by distance such that further away intersections come first
			std::sort(
				collection.m_Intersections.begin(),
				collection.m_Intersections.begin() + collection.m_IntersectionCount,
				[](const RayIntersection& a, const RayIntersection& b)
				{
					return (a.m_fraction > b.m_fraction);
				});

			for (int index = 0; index < collection.m_IntersectionCount - 1; index++)
			{
				auto& thisIntersection = collection.m_Intersections[index];

				// If it's a back face
				if (thisIntersection.m_facing == RayIntersection::Facing::Away)
				{
					b2Vec2 point = thisIntersection.m_point;

					// Get the corresponding front face.
					for (int followingIndex = index + 1; followingIndex < collection.m_IntersectionCount; followingIndex++)
					{
						auto& thatIntersection = collection.m_Intersections[followingIndex];

						if (thatIntersection.m_fixture == thisIntersection.m_fixture)
						{
							thatIntersection.m_backPoint = point;
							break;
						}
						else 
						{
							point = thatIntersection.m_point;
						}
					}
				}
			}

			// Remove back faces
			auto it = std::remove_if(
				collection.m_Intersections.begin(),
				collection.m_Intersections.begin() + collection.m_IntersectionCount,
				[](RayIntersection& intersection)
				{
					return intersection.m_facing == RayIntersection::Facing::Away;
				});
				
			collection.m_IntersectionCount = it - collection.m_Intersections.begin();
		});

	// shove all intersections into one big array
	for (const auto& collection : m_IntersectionsPerColumn)
	{
		const auto begin = std::begin(collection.m_Intersections);
		const auto end = begin + collection.m_IntersectionCount;

		m_AllIntersections.insert(
			std::end(m_AllIntersections),
			begin,
			end);
	}

	// sort by distance such that further away intersections come first
	std::sort(
		m_AllIntersections.begin(),
		m_AllIntersections.end(),
		[](const RayIntersection& a, const RayIntersection& b)
	{
		return (a.m_fraction > b.m_fraction);
	});

	const auto targetSize = target.getSize();

	auto Prepare = [targetSize, &camera](const RayIntersection& intersection) -> Drawable
	{
		const auto& renderData =
			*(qvr::FixtureRenderData*)(intersection.m_fixture->GetUserData());

		const b2Vec2 displacementNear = intersection.m_point - camera.GetPosition();
		const float  distanceNear = b2Dot(displacementNear, camera.GetForwards());

		const b2Vec2 displacementFar = intersection.m_backPoint - camera.GetPosition();
		const float  distanceFar = b2Dot(displacementFar, camera.GetForwards());

		const bool drawTopOrBottom = distanceFar != distanceNear;

		// At a distance of 1 metre, a vertical metre is enough pixels in height to fill the screen.
		const float oneMetreInPixelsNear = abs(targetSize.y / distanceNear);
		const float oneMetreInPixelsFar = abs(targetSize.y / distanceFar);

		auto CalculateLineOffset = [&renderData, &camera](float const oneMetreInPixels)
		{
			const float groundOffset = renderData.GetGroundOffset() * oneMetreInPixels;
			const float heightOffset = (renderData.GetHeight() - 1.0f) * oneMetreInPixels;
			const float cameraHeightOffset = camera.GetHeightOffset() * 2.0f * oneMetreInPixels;

			return -groundOffset - heightOffset - cameraHeightOffset;
		};

		const float lineOffsetNear = CalculateLineOffset(oneMetreInPixelsNear);

		const float cameraPitchOffset = (float)GetPitchOffsetInPixels(camera, targetSize.y);

		const float lineHeightNear = renderData.GetHeight() * oneMetreInPixelsNear;
		const float lineStartYNear = ((targetSize.y - lineHeightNear + lineOffsetNear) / 2) + cameraPitchOffset;
		const float lineEndYNear = ((targetSize.y + lineHeightNear + lineOffsetNear) / 2) + cameraPitchOffset;

		const float lineOffsetFar = CalculateLineOffset(oneMetreInPixelsFar);

		const float lineHeightFar = renderData.GetHeight() * oneMetreInPixelsFar;
		const float lineStartYFar = ((targetSize.y - lineHeightFar + lineOffsetFar) / 2) + cameraPitchOffset;
		const float lineEndYFar = ((targetSize.y + lineHeightFar + lineOffsetFar) / 2) + cameraPitchOffset;

		const Animation::Rect textureRect = 
			renderData.GetViews().viewCount <= 1 ?
			renderData.GetViews().views[0] :
			CalculateView(
				renderData.GetViews(),
				renderData.GetObjectAngle(),
				[&camera, &renderData]() {
					const b2Vec2 disp = renderData.GetSpritePosition() - camera.GetPosition();
					return b2Atan2(disp.y, disp.x) + b2_pi;
				}());

		auto CalculateU = [&camera](
			const b2Vec2& hitPoint,
			const b2Vec2& flatSpritePos,
			const float flatSpriteRadius,
			const Animation::Rect& textureRect)
		{
			const b2Vec2 perp(-camera.GetForwards().y, camera.GetForwards().x);

			const b2Vec2 left = flatSpritePos - (flatSpriteRadius * perp);

			float u = (hitPoint - left).Length() / (flatSpriteRadius * 2);

			// Convert to texels.
			u *= textureRect.right - textureRect.left;
			u += textureRect.left;

			return u;
		};

		const float u = CalculateU(
			intersection.m_point,
			renderData.GetSpritePosition(),
			renderData.GetSpriteRadius(),
			textureRect);

		Drawable output;
		output.m_BlendColor = renderData.GetColor();
		output.m_Texture = renderData.GetTexture();
		output.m_X = (float)intersection.m_screenX;
		output.m_Top = lineStartYNear;
		output.m_Bottom = lineEndYNear;
		output.m_DistanceNear = distanceNear;
		output.m_DistanceFar = distanceNear;
		output.m_U = u;
		output.m_VTop = (float)textureRect.top;
		output.m_VBottom = (float)textureRect.bottom;
		output.m_Normal = sf::Vector3f(intersection.m_normal.x, intersection.m_normal.y, 0.0f);
		output.m_DrawTop = drawTopOrBottom && lineStartYFar < lineStartYNear;
		output.m_DrawBottom = drawTopOrBottom && lineEndYFar > lineEndYNear;
		output.m_FarY = output.m_DrawTop ? lineStartYFar : output.m_DrawBottom ? lineEndYFar : output.m_Top;
		return output;
	};

	std::transform(
		m_AllIntersections.begin(),
		m_AllIntersections.end(),
		std::back_inserter(m_AllColumns),
		Prepare);

	class Drawer {
	public:
		Drawer(sf::RenderTarget& target, sf::Shader& shader, const World& world)
			: m_Target(target)
			, m_Shader(shader)
		{
			m_DefaultTexture.create(1, 1);
			// Make it white.
			{
				auto c = sf::Color::White;
				m_DefaultTexture.update(&c.r);
			}

			sf::Shader::bind(&m_Shader);
			shader.setUniform("ambientLightColor", sf::Glsl::Vec4(world.GetAmbientLight().mColor));

			shader.setUniform("directionalLightDirection", B2VecToSFVec(world.GetDirectionalLight().GetDirection()));
			shader.setUniform("directionalLightColor", sf::Glsl::Vec4(world.GetDirectionalLight().GetColor()));

			shader.setUniform("fogColor", sf::Glsl::Vec4(world.GetFog().GetColor()));
			shader.setUniform("fogMaxIntensity", world.GetFog().GetMaxIntensity());
			shader.setUniform("fogMaxDistance", world.GetFog().GetMaxDistance());
			shader.setUniform("fogMinDistance", world.GetFog().GetMinDistance());

			sf::Texture::bind(&m_DefaultTexture, sf::Texture::CoordinateType::Pixels);
			shader.setUniform("texture", sf::Shader::CurrentTexture);

			glCheck(glEnableClientState(GL_VERTEX_ARRAY));
			glCheck(glEnableClientState(GL_COLOR_ARRAY));
			glCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
			glCheck(glEnableClientState(GL_NORMAL_ARRAY));

			glCheck(glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &m_Line[0].position));
			glCheck(glNormalPointer(GL_FLOAT, sizeof(Vertex), &m_Line[0].normal));
			glCheck(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &m_Line[0].color));
			glCheck(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &m_Line[0].texCoords));
		}

		void operator()(const Drawable& drawable) {
			if (drawable.m_Texture != m_LastTexture) {
				m_LastTexture = drawable.m_Texture;

				const sf::Texture* textureToBind;

				if (drawable.m_Texture)
				{
					textureToBind = drawable.m_Texture;
				}
				else
				{
					textureToBind = &m_DefaultTexture;
				}

				sf::Texture::bind(textureToBind, sf::Texture::CoordinateType::Pixels);
				m_Shader.setUniform("texture", sf::Shader::CurrentTexture);
			}

			m_Line[0].position.x = drawable.m_X;
			m_Line[1].position.x = drawable.m_X;

			m_Line[0].color = drawable.m_BlendColor;
			m_Line[1].color = drawable.m_BlendColor;

			DrawTop(drawable);
			DrawBottom(drawable);
			DrawFront(drawable);
		}

		~Drawer()
		{
			glCheck(glDisableClientState(GL_VERTEX_ARRAY));
			glCheck(glDisableClientState(GL_COLOR_ARRAY));
			glCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
			glCheck(glDisableClientState(GL_NORMAL_ARRAY));

			m_Target.resetGLStates();
		}

	private:
		void DrawLine()
		{
			glCheck(glDrawArrays(GL_LINES, 0, 2));
		}

		void DrawFront(const Drawable& drawable)
		{
			m_Line[0].position.y = drawable.m_Top;
			m_Line[1].position.y = drawable.m_Bottom;
			m_Line[0].position.z = drawable.m_DistanceNear;
			m_Line[1].position.z = drawable.m_DistanceNear;

			m_Line[0].normal = drawable.m_Normal;
			m_Line[1].normal = drawable.m_Normal;

			m_Line[0].texCoords.x = drawable.m_U;
			m_Line[0].texCoords.y = drawable.m_VTop;
			m_Line[1].texCoords.x = drawable.m_U;
			m_Line[1].texCoords.y = drawable.m_VBottom;

			DrawLine();
		}

		void DrawTop(const Drawable& drawable)
		{
			if (!drawable.m_DrawTop)
			{
				return;
			}

			m_Line[0].position.y = drawable.m_FarY;
			m_Line[1].position.y = drawable.m_Top;
			m_Line[0].position.z = drawable.m_DistanceFar;
			m_Line[1].position.z = drawable.m_DistanceNear;

			m_Line[0].normal = sf::Vector3f(0.0f, 0.0f, 1.0f);
			m_Line[1].normal = sf::Vector3f(0.0f, 0.0f, 1.0f);

			DrawLine();
		}

		void DrawBottom(const Drawable& drawable)
		{
			if (!drawable.m_DrawBottom)
			{
				return;
			}

			m_Line[0].position.y = drawable.m_Bottom;
			m_Line[1].position.y = drawable.m_FarY;
			m_Line[0].position.z = drawable.m_DistanceNear;
			m_Line[1].position.z = drawable.m_DistanceFar;

			m_Line[0].normal = sf::Vector3f(0.0f, 0.0f, -1.0f);
			m_Line[1].normal = sf::Vector3f(0.0f, 0.0f, -1.0f);

			DrawLine();
		}

		sf::RenderTarget& m_Target;
		sf::Shader& m_Shader;

		const sf::Texture* m_LastTexture = nullptr;

		// Flat white, like a coffee.
		sf::Texture m_DefaultTexture;

		struct Vertex {
			sf::Vector3f position;
			sf::Vector3f normal;
			sf::Vector2f texCoords;
			sf::Color color;
		};

		Vertex m_Line[2];
	};

	std::for_each(
		m_AllColumns.begin(),
		m_AllColumns.end(),
		Drawer(target, mShader, world));
}

float32 WorldRaycastRendererImpl::ForwardRaycastCallback::ReportFixture(
	b2Fixture* fixture, 
	b2Vec2 const& point, 
	b2Vec2 const& normal, 
	float32 fraction)
{
	if (fixture->GetUserData() == nullptr)
	{
		return 1;
	}

	if (m_Collection->m_IntersectionCount >= (int)m_Collection->m_Intersections.size())
	{
		return 0;
	}

	RayIntersection& newIntersection = m_Collection->m_Intersections[m_Collection->m_IntersectionCount++];

	newIntersection.m_fixture = fixture;
	newIntersection.m_point = point;
	newIntersection.m_normal = normal;
	newIntersection.m_fraction = fraction;
	newIntersection.m_screenX = (int)m_Collection->m_Index;
	newIntersection.m_facing = RayIntersection::Facing::Towards;

	return 1;
}

float32 WorldRaycastRendererImpl::BackwardRaycastCallback::ReportFixture(
	b2Fixture* fixture,
	b2Vec2 const& point,
	b2Vec2 const& normal,
	float32 fraction)
{
	if (fixture->GetUserData() == nullptr)
	{
		return 1;
	}

	if (m_Collection->m_IntersectionCount >= (int)m_Collection->m_Intersections.size())
	{
		return 0;
	}

	RayIntersection& newIntersection = m_Collection->m_Intersections[m_Collection->m_IntersectionCount++];

	newIntersection.m_fixture = fixture;
	newIntersection.m_point = point;
	newIntersection.m_normal = -normal; // flip the normal
	newIntersection.m_fraction = 1.0f - fraction; // flip the fraction
	newIntersection.m_screenX = m_Collection->m_Index;
	newIntersection.m_facing = RayIntersection::Facing::Away;

	return 1;
}

void WorldRaycastRendererImpl::LoadShader() {
	static const char* vertexShaderRawText = R"(
	
	#version 130

	uniform vec4 ambientLightColor;

	uniform vec4 directionalLightColor;
	uniform vec2 directionalLightDirection;

	uniform vec4 fogColor;
	uniform float fogMaxIntensity;
	uniform float fogMaxDistance;
	uniform float fogMinDistance;
	
	out vec4 appliedFogColor;
	out vec4 appliedDirectionalLightColor;

	void main() {
		float fogIntensity = 
			min(
				((min(
					max(
						gl_Vertex.z, 
						fogMinDistance), 
					fogMaxDistance) 
				- fogMinDistance) 
				/ (fogMaxDistance - fogMinDistance)),
				fogMaxIntensity);
		appliedFogColor = fogColor * fogIntensity;

		appliedDirectionalLightColor = 
			directionalLightColor * 
			clamp(dot(vec2(gl_Normal), -directionalLightDirection), 0.0f, 1.0f);

		gl_Position = ftransform();
		gl_Position.z = 0.0f;
		
		gl_FrontColor = ambientLightColor * gl_Color;
		
		gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	}

	)";

	static const char* fragmentShaderRawText = R"(
	
	#version 130	

	uniform sampler2D texture;
	
	in vec4 appliedFogColor;
	in vec4 appliedDirectionalLightColor;

	void main() {
		vec4 blendColor = gl_Color;
	
		vec4 textureColor = texture2D(texture, gl_TexCoord[0].xy);
	
		gl_FragColor = (blendColor * textureColor) + appliedFogColor + appliedDirectionalLightColor;
	}
	
	)";

	bool result = mShader.loadFromMemory(vertexShaderRawText, fragmentShaderRawText);
	assert(result);
}

WorldRaycastRenderer::WorldRaycastRenderer()
	: m_Impl(std::make_unique<WorldRaycastRendererImpl>())
{}

WorldRaycastRenderer::~WorldRaycastRenderer() = default;

void WorldRaycastRenderer::Render(
	const World & world,
	const Camera3D & camera,
	const RenderSettings& settings,
	sf::RenderTarget & target)
{
	m_Impl->Render(world, camera, settings, target);
}

}