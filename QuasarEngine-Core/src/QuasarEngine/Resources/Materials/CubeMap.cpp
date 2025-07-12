#include "qepch.h"
#include "CubeMap.h"

//#include <glad/glad.h>
#include <stb_image.h>

namespace QuasarEngine
{
	CubeMap::CubeMap() : m_ID(0)
	{
		
	}
	void CubeMap::Load(std::array<std::string, 6> paths)
	{
		/*glGenTextures(1, &m_ID);

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		for (unsigned int i = 0; i < paths.size(); i++)
		{
			stbi_set_flip_vertically_on_load(false);

			int width, height, nbChannels;
			unsigned char* data;

			data = stbi_load(paths[i].c_str(), &width, &height, &nbChannels, 0);

			if (data)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			else
				std::cout << "Failed to load cubemap texture at " << paths[i] << " : " << stbi_failure_reason() << std::endl;

			stbi_image_free(data);
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);*/
	}

	void CubeMap::Load(std::string hdr_path)
	{
		/*stbi_set_flip_vertically_on_load(true);
		int width, height, nrComponents;
		float* data = stbi_loadf(hdr_path.c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			glGenTextures(1, &m_ID);
			glBindTexture(GL_TEXTURE_2D, m_ID);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Failed to load HDR image." << std::endl;
		}*/
	}

	void CubeMap::ActiveTexture()
	{
		//glActiveTexture(GL_TEXTURE0);
	}

	void CubeMap::BeginDrawModel()
	{
		//glDepthFunc(GL_LEQUAL);
	}

	void CubeMap::EndDrawModel()
	{
		//glDepthFunc(GL_LESS);
	}

	void CubeMap::Bind()
	{
		//glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);
	}
}