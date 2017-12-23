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
	class RaycastCallback : public b2RayCastCallback
	{
	public:
		struct RayIntersection
		{
			b2Fixture* m_fixture;
			b2Vec2 m_point;
			b2Vec2 m_normal;
			float32 m_fraction;
			int m_screenX;
		};

		float32 ReportFixture(
			b2Fixture* fixture,
			const b2Vec2& point,
			const b2Vec2& normal,
			float32 fraction)
			override;

		static const unsigned sm_MaxNumIntersections = 32;

		std::array<RayIntersection, sm_MaxNumIntersections> m_Intersections;

		unsigned m_IntersectionCount = 0;
		unsigned m_Index = 0;
	};

	std::vector<RaycastCallback> m_RaycastCallbacks;

	std::vector<RaycastCallback::RayIntersection> m_AllIntersections;

	struct Column {
		float m_Top;
		float m_Bottom;
		float m_X;
		float m_Distance;
		float m_U;
		float m_VTop;
		float m_VBottom;
		sf::Color m_BlendColor;
		sf::Vector2f m_Normal;
		const sf::Texture* m_Texture;
	};

	std::vector<Column> m_AllColumns;

	sf::Shader mShader;

	void LoadShader();

public:
	WorldRaycastRendererImpl()
	{
		LoadShader();
	}
	void Render(const World& world, const Camera3D& camera, const RenderSettings& settings, sf::RenderTarget& target);
};

void WorldRaycastRendererImpl::Render(const World & world, const Camera3D & camera, const RenderSettings& settings, sf::RenderTarget & target)
{
	assert(world.GetPhysicsWorld());

	const auto targetWidth = target.getSize().x;

	if (m_RaycastCallbacks.size() != targetWidth)
	{
		m_RaycastCallbacks.resize(targetWidth);

		m_AllIntersections.reserve(targetWidth * RaycastCallback::sm_MaxNumIntersections);

		m_AllColumns.reserve(m_AllIntersections.size());
	}

	m_AllIntersections.resize(0);
	m_AllColumns.resize(0);

	// This is a bit grim.
	for (unsigned i = 0; i < m_RaycastCallbacks.size(); ++i)
	{
		m_RaycastCallbacks[i].m_Index = i;
		m_RaycastCallbacks[i].m_IntersectionCount = 0;
	}

	auto DoRaycast = [&](RaycastCallback& cb)
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
			const auto screenX = -1.0f + screenXDelta * cb.m_Index;
			auto rayDir = (cameraForwards + (screenX * viewPlane));
			rayDir.Normalize();
			return cameraPosition + (settings.m_RayLength * rayDir);
		}();

		physicsWorld.RayCast(&cb, cameraPosition, rayEnd);
	};

	std::for_each(
		m_RaycastCallbacks.begin(),
		m_RaycastCallbacks.end(),
		DoRaycast);

	using RayIntersection = RaycastCallback::RayIntersection;

	for (const auto& raycastCallback : m_RaycastCallbacks)
	{
		const auto begin = std::begin(raycastCallback.m_Intersections);
		const auto end = begin + raycastCallback.m_IntersectionCount;

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

	auto Prepare = [targetSize, &camera](const RayIntersection& intersection) -> Column
	{
		const auto& renderData =
			*(qvr::FixtureRenderData*)(intersection.m_fixture->GetUserData());

		const b2Vec2 displacement = intersection.m_point - camera.GetPosition();
		const float  distance = b2Dot(displacement, camera.GetForwards());

		// At a distance of 1 metre, a vertical metre is enough pixels in height to fill the screen.
		const float oneMetreInPixels = abs(targetSize.y / distance);

		const float lineOffset = [&renderData, &camera, oneMetreInPixels]()
		{
			const float groundOffset = renderData.GetGroundOffset()    * oneMetreInPixels;
			const float heightOffset = (renderData.GetHeight() - 1.0f) * oneMetreInPixels;
			const float cameraHeightOffset = camera.GetHeightOffset() * 2.0f * oneMetreInPixels;

			return -groundOffset - heightOffset - cameraHeightOffset;
		}();

		const float cameraPitchOffset = (float)GetPitchOffsetInPixels(camera, targetSize.y);

		const float lineHeight = renderData.GetHeight() * oneMetreInPixels;
		const float lineStartY = ((targetSize.y - lineHeight + lineOffset) / 2) + cameraPitchOffset;
		const float lineEndY = ((targetSize.y + lineHeight + lineOffset) / 2) + cameraPitchOffset;

		const Animation::Rect textureRect = renderData.GetViews().views[0];

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

		Column output;
		output.m_BlendColor = renderData.GetColor();
		output.m_Texture = renderData.GetTexture();
		output.m_X = (float)intersection.m_screenX;
		output.m_Top = lineStartY;
		output.m_Bottom = lineEndY;
		output.m_Distance = distance;
		output.m_U = u;
		output.m_VTop = (float)textureRect.top;
		output.m_VBottom = (float)textureRect.bottom;
		output.m_Normal = sf::Vector2f(intersection.m_normal.x, intersection.m_normal.y);
		return output;
	};

	std::transform(
		m_AllIntersections.begin(),
		m_AllIntersections.end(),
		std::back_inserter(m_AllColumns),
		Prepare);

	class ColumnDrawer {
	public:
		ColumnDrawer(sf::RenderTarget& target, sf::Shader& shader, const World& world)
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

			glCheck(glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &line[0].position));
			glCheck(glNormalPointer(GL_FLOAT, sizeof(Vertex), &line[0].normal));
			glCheck(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &line[0].color));
			glCheck(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &line[0].texCoords));
		}

		void operator()(const Column& column) {
			if (column.m_Texture != m_LastTexture) {
				m_LastTexture = column.m_Texture;

				const sf::Texture* textureToBind;

				if (column.m_Texture)
				{
					textureToBind = column.m_Texture;
				}
				else
				{
					textureToBind = &m_DefaultTexture;
				}

				sf::Texture::bind(textureToBind, sf::Texture::CoordinateType::Pixels);
				m_Shader.setUniform("texture", sf::Shader::CurrentTexture);
			}

			line[0].position.x = column.m_X;
			line[1].position.x = column.m_X;
			line[0].position.y = column.m_Top;
			line[1].position.y = column.m_Bottom;
			line[0].position.z = column.m_Distance;
			line[1].position.z = column.m_Distance;

			line[0].normal = column.m_Normal;
			line[1].normal = column.m_Normal;

			line[0].color = column.m_BlendColor;
			line[1].color = column.m_BlendColor;

			line[0].texCoords.x = column.m_U;
			line[0].texCoords.y = column.m_VTop;
			line[1].texCoords.x = column.m_U;
			line[1].texCoords.y = column.m_VBottom;

			// Draw the line.
			glCheck(glDrawArrays(GL_LINES, 0, 2));
		}

		~ColumnDrawer()
		{
			m_Target.resetGLStates();
		}

	private:
		sf::RenderTarget& m_Target;
		sf::Shader& m_Shader;

		const sf::Texture* m_LastTexture = nullptr;

		// Flat white, like a coffee.
		sf::Texture m_DefaultTexture;

		struct Vertex {
			sf::Vector3f position;
			sf::Vector2f normal;
			sf::Vector2f texCoords;
			sf::Color color;
		};

		// Maybe use a custom vertex type in future.
		Vertex line[2];
	};

	std::for_each(
		m_AllColumns.begin(),
		m_AllColumns.end(),
		ColumnDrawer(target, mShader, world));
}

float32 WorldRaycastRendererImpl::RaycastCallback::ReportFixture(b2Fixture * fixture, const b2Vec2 & point, const b2Vec2 & normal, float32 fraction)
{
	if (fixture->GetUserData() == nullptr)
	{
		return 1;
	}

	m_Intersections[m_IntersectionCount++] =
	{
		fixture,
		point,
		normal,
		fraction,
		(int)m_Index
	};

	if (m_IntersectionCount >= m_Intersections.size())
	{
		return 0;
	}

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