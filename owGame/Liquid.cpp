#include "stdafx.h"

// Includes
#include "WMO/Wmo_Material.h"

// General
#include "liquid.h"

// Additional
#include "Environment/EnvironmentManager.h"

#include "../shared/pack_begin.h"

struct MH20_Attributes
{
	uint64_t fishable; // seems to be usable as visibility information.
	uint64_t deep;
};

struct MH2O_Instance
{
	uint16 liquidType;
	uint16 liquidObjectOrLVF;

	/*
	float min;
	The minimum height from all vertices if flags == 5
	*/
	float minHeightLevel;

	/*
	float max;
	The maximum height from all vertices if flags == 5
	*/
	float maxHeightLevel;

	/*
	uint8 x, y, w, h;
	These 4 values are used to define the size of the sub mask and
	the count of height and alpha values needed.

	x and y can be 0-7
	w and h can be 1-8

	Lets say we have a VisibilityMask like this:
	00000000
	00000000
	00000000
	00000000
	00111000
	11111100
	11111111
	11111111

	And the given Values for x y w h are 0 4 8 4 it means there MAYBE more info for this part (marked with X):
	00000000
	00000000
	xy 00000000
	\00000000_
	XXXXXXXX|
	XXXXXXXX| h
	XXXXXXXX|
	XXXXXXXX_
	|---w--|
	*/
	uint8 xOffset;
	uint8 yOffset;
	uint8 width;
	uint8 height;

	/*
	This is a more detailed version of the VisibilityMask. Its an uint8[w*h/8] data block.
	The grid is created as before but every line only contains w bits
	If the offset is 0 then every quad marked by x y w h is displayed
	*/
	uint32 offsetExistsBitmap;


	/*
	uint32 ofsHeigthAlpha;

	This offset points to an array of heights and after that there is an array of alpha values.

	the size of both arrays is (w+1)*(h+1)
	8*8 quads -> 9*9 vertices so if w or h is 8 we need 9 values this explains the "+1"

	the heights array float[(w+1)*(h+1)] is only present if the flags == 5 otherwise (2) its not required

	the alpha array uint8[(w+1)*(h+1)] seems tobe always present if the offset is given and comes always
	after the heights array and if heights are not given it is directly at ofsHeigthAlpha
	*/
	uint32 offsetVertexData;
};

struct MH2O_Vertex
{
	vec3 position;
	vec3 textureCoord;
	vec3 normal;
};

#include "../shared/pack_end.h"

//

Liquid::Liquid(uint32 x, uint32 y, vec3 base, float tilesize) : m_TilesX(x), m_TilesY(y), m_Position(base), tilesize(tilesize), shader(-1), ydir(1.0f)
{
	m_TilesCount = (m_TilesX + 1) * (m_TilesY + 1);
}

Liquid::~Liquid()
{
	for (size_t i = 0; i < textures.size(); i++)
	{
		_TexturesMgr->Delete(textures[i]);
	}
}

//

void Liquid::initFromTerrainMH2O(File& f, MH2O_Header * _header)
{
	texRepeats = 0.5f;
	ydir = 1.0f;

	//

	for (uint32 j = 0; j < _header->layersCount; j++)
	{
		MH2O_Instance* mh2o_instance = new MH2O_Instance;
		mh2o_instance = (MH2O_Instance*)(f.GetDataFromCurrent() + _header->offsetInstances + sizeof(MH2O_Instance) * j);

		if (mh2o_instance->minHeightLevel - mh2o_instance->maxHeightLevel > 0.001f)
		{
			Debug::Green("MinHeight %f:", mh2o_instance->minHeightLevel);
			Debug::Green("MaxHeight %f:", mh2o_instance->maxHeightLevel);
			Debug::Error("MIN WATER != MAX_WATER!!!!");
			fail1();
		}

		// Init liquid
		DBC_LiquidTypeRecord* liquidType = DBC_LiquidType[mh2o_instance->liquidType];
		assert1(liquidType != nullptr);
		uint32 vertexFormat = liquidType->Get_LiquidMaterialID()->Get_LiquidVertexFormat();
		InitTextures(liquidType);
		Debug::Warn("Liquid is [%s]", liquidType->Get_Name());

		// Fix ocean shit
		if (mh2o_instance->liquidType == 2) 
		{
			if (mh2o_instance->offsetVertexData == 0)
			{
				vertexFormat = UINT32_MAX; // Is ocean and hasn't depths
			}
			else
			{
				vertexFormat = 2;
			}
		}

		// Create layer

		MH2O_WaterLayer waterLayer;

		waterLayer.x = mh2o_instance->xOffset;
		waterLayer.y = mh2o_instance->yOffset;
		waterLayer.Width = mh2o_instance->width;
		waterLayer.Height = mh2o_instance->height;

		waterLayer.LiquidType = mh2o_instance->liquidType;
		waterLayer.MinHeightLevel = mh2o_instance->minHeightLevel;
		waterLayer.MaxHeightLevel = mh2o_instance->maxHeightLevel;
		waterLayer.LiquidObjectOrLVF = mh2o_instance->liquidObjectOrLVF; // FIXME Send format to create buffer

		waterLayer.hasmask = mh2o_instance->offsetExistsBitmap != 0;

		if (waterLayer.hasmask)
		{
			unsigned co = mh2o_instance->width * mh2o_instance->height / 8;

			if (mh2o_instance->width * mh2o_instance->height % 8 != 0)
			{
				co++;
			}

			memcpy(waterLayer.mask, f.GetDataFromCurrent() + mh2o_instance->offsetExistsBitmap, co);

			for (uint32_t k = 0; k < (uint32_t)(waterLayer.Width * waterLayer.Height); k++)
			{
				waterLayer.renderTiles.push_back(getBitL2H(waterLayer.mask, (uint32)k));
			}
		}

		//----------------------------------------------------------------//
		//----------------------------------------------------------------//
		//----------------------------------------------------------------//

		struct uv_map_entry
		{
			uint16 x; // divided by 8 for shaders
			uint16 y;
		};

		const uint32 vertexDataSize = (mh2o_instance->width + 1) * (mh2o_instance->height + 1);

		if (vertexFormat == 0)         // Case 0, Height and Depth data
		{
			float* pHeights = (float*)(f.GetDataFromCurrent() + mh2o_instance->offsetVertexData);
			uint8* pDepths = (uint8*)(f.GetDataFromCurrent() + mh2o_instance->offsetVertexData + (sizeof(float) * vertexDataSize));

			for (uint32 g = 0; g < vertexDataSize; g++)
			{
				waterLayer.heights.push_back(pHeights[g]);
				waterLayer.depths.push_back(pDepths[g]);
			}
		}
		else if (vertexFormat == 1)    //Case 1, Height and Texture Coordinate data
		{
			float* pHeights = (float*)(f.GetDataFromCurrent() + mh2o_instance->offsetVertexData);
			uv_map_entry* pUVMap = (uv_map_entry*)(f.GetDataFromCurrent() + mh2o_instance->offsetVertexData + (sizeof(float) * vertexDataSize));

			for (uint32 g = 0; g < vertexDataSize; g++)
			{
				waterLayer.heights.push_back(pHeights[g]);
				// push textcoords
			}
		}
		else if (vertexFormat == 2)         // Case 2, Depth only data (OCEAN)
		{
			uint8* pDepths = (uint8*)(f.GetDataFromCurrent() + mh2o_instance->offsetVertexData);

			for (uint32 g = 0; g < vertexDataSize; g++)
			{
				waterLayer.depths.push_back(pDepths[g]);
			}
		}
		else if (vertexFormat == 3)         //Case 3, Height, Depth and Texture Coordinates
		{
			fail1();

			float* pHeights = (float*)(f.GetDataFromCurrent() + mh2o_instance->offsetVertexData);
			uv_map_entry* pUVMap = (uv_map_entry*)(f.GetDataFromCurrent() + mh2o_instance->offsetVertexData + (sizeof(float) * vertexDataSize));
			uint8* pDepths = (uint8*)(f.GetDataFromCurrent() + mh2o_instance->offsetVertexData + (sizeof(float) * vertexDataSize) + (sizeof(uv_map_entry) * vertexDataSize));

			for (uint32 g = 0; g < vertexDataSize; g++)
			{
				waterLayer.heights.push_back(pHeights[g]);
				// push textcoords
				waterLayer.depths.push_back(pDepths[g]);
			}
		}

		m_WaterLayers.push_back(waterLayer);
	}

	m_WaterColorLight = _EnvironmentManager->GetSkyColor(RIVER_COLOR_LIGHT);
	m_WaterColorDark = _EnvironmentManager->GetSkyColor(RIVER_COLOR_DARK);
}

void Liquid::initFromTerrainMCLQ(File& f, MCNK_MCLQ_LiquidType _liquidType)
{
	texRepeats = 4.0f;
	ydir = 1.0f;

	initGeometry(f);

	switch (_liquidType)
	{
		case lq_river:
		initTextures("XTextures\\river\\lake_a", 1, 30);
		break;

		case lq_ocean:
		initTextures("XTextures\\ocean\\ocean_h", 1, 30);
		break;

		case lq_magma:
		initTextures("XTextures\\lava\\lava", 1, 30);
		break;

		case lq_slime:
		initTextures("XTextures\\slime\\slime", 1, 30);
		break;
	}

	//m_WaterColorLight = _EnvironmentManager->GetSkyColor(RIVER_COLOR_LIGHT); // FIXME!!!
	//m_WaterColorDark = _EnvironmentManager->GetSkyColor(RIVER_COLOR_DARK);

	m_WaterColorLight = vec3(1.0f, 1.0f, 1.0f);
	m_WaterColorDark = vec3(0.7f, 0.7f, 0.7f);
}

void Liquid::initFromWMO2(File& f, WMOMaterial* _material, DBC_LiquidTypeRecord* _liquidType, bool _indoor)
{
	// Magic for WMO
	texRepeats = 4.0f;
	ydir = -1.0f;

	initGeometry(f);
	InitTextures(_liquidType);

	if (_indoor)
	{
		m_WaterColorLight = fromRGB(_material->GetDiffuseColor());
		m_WaterColorDark = m_WaterColorLight;
	}
	else
	{
		m_WaterColorLight = _EnvironmentManager->GetSkyColor(RIVER_COLOR_LIGHT);
		m_WaterColorDark = _EnvironmentManager->GetSkyColor(RIVER_COLOR_DARK);
	}
}

//

void Liquid::createBuffer(cvec3 _position)
{
	vector<MH2O_Vertex> mh2oVertices;

	for (unsigned l = 0; l < m_WaterLayers.size(); l++)
	{
		MH2O_WaterLayer& layer = m_WaterLayers[l];

		for (uint8 y = layer.y; y < layer.Height + layer.y; y++)
		{
			for (uint8 x = layer.x; x < layer.Width + layer.x; x++)
			{

				unsigned tx = x - layer.x;
				unsigned ty = y - layer.y;

				// p1--p4
				// |    |  // this is GL_QUADS 
				// p2--p3
				unsigned p1 = tx + ty           * (layer.Width + 1);
				unsigned p2 = tx + (ty + 1)     * (layer.Width + 1);
				unsigned p3 = tx + 1 + (ty + 1) * (layer.Width + 1);
				unsigned p4 = tx + 1 + ty       * (layer.Width + 1);

				// alpha values helper
				float a1, a2, a3, a4;
				a1 = a2 = a3 = a4 = 1.0f;
				if (layer.depths.size() != 0)
				{
					a1 = (float)layer.depths[p1] / 255.f * 4.0f + 0.0f; // whats the magic formular here ???
					a2 = (float)layer.depths[p2] / 255.f * 4.0f + 0.0f;
					a3 = (float)layer.depths[p3] / 255.f * 4.0f + 0.0f;
					a4 = (float)layer.depths[p4] / 255.f * 4.0f + 0.0f;
				}

				// height values helper
				float h1, h2, h3, h4;
				h1 = h2 = h3 = h4 = 0.0f;
				if (layer.heights.size() != 0)
				{
					h1 = layer.heights[p1];
					h2 = layer.heights[p2];
					h3 = layer.heights[p3];
					h4 = layer.heights[p4];
				}

				// Skip hidden water tile
				if (layer.renderTiles.size() != 0)
				{
					if (!layer.renderTiles[tx + ty * layer.Width])
					{
						continue;
					}
				}

				// Insert vertex

				mh2oVertices.push_back
				({
					vec3(_position.x + C_UnitSize * static_cast<float>(x), h1, _position.z + ydir * (C_UnitSize * static_cast<float>(y))),
					vec3(0.0f, 0.0f, a1),
					defaultNormal
				});

				mh2oVertices.push_back
				({
					vec3(_position.x + C_UnitSize * static_cast<float>(x), h2, _position.z + ydir * (C_UnitSize +  C_UnitSize * static_cast<float>(y))),
					vec3(0.0f, texRepeats, a2),
					defaultNormal
				});

				mh2oVertices.push_back
				({
					vec3(_position.x + C_UnitSize + C_UnitSize * static_cast<float>(x), h3, _position.z + ydir * (C_UnitSize + C_UnitSize * static_cast<float>(y))),
					vec3(texRepeats, texRepeats, a3),
					defaultNormal
				});

				mh2oVertices.push_back
				({
					vec3(_position.x + C_UnitSize + C_UnitSize * static_cast<float>(x), h4, _position.z + ydir * (C_UnitSize * static_cast<float>(y))),
					vec3(texRepeats, 0.0f, a4),
					defaultNormal
				});
			}
		}
	}

	globalBufferSize = static_cast<uint32>(mh2oVertices.size());

	if (globalBufferSize == 0)
	{
		return;
	}

	glGenBuffers(1, &globalBufferWater);
	glBindBuffer(GL_ARRAY_BUFFER, globalBufferWater);

	glBufferData(GL_ARRAY_BUFFER, mh2oVertices.size() * sizeof(MH2O_Vertex), mh2oVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Liquid::draw()
{
	if (globalBufferSize == 0)
	{
		return;
	}

	_TechniquesMgr->m_Water->Bind();
	_TechniquesMgr->m_Water->SetPVW();

	glBindBuffer(GL_ARRAY_BUFFER, globalBufferWater);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (const GLvoid*)(0));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (const GLvoid*)(3 * sizeof(float)));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (const GLvoid*)(6 * sizeof(float)));

	size_t texidx = (size_t)(_EnvironmentManager->animtime / 60.0f) % textures.size();
	textures[texidx]->Bind();

	_TechniquesMgr->m_Water->SetWaterColorLight(_EnvironmentManager->GetSkyColor(LIGHT_GLOBAL_DIFFUSE));
	_TechniquesMgr->m_Water->SetWaterColorDark(_EnvironmentManager->GetSkyColor(LIGHT_GLOBAL_DIFFUSE));

	glDrawArrays(GL_QUADS, 0, globalBufferSize);
	PERF_INC(PERF_MAP_CHUNK_MH20);

	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	_TechniquesMgr->m_Water->Unbind();

	//const float tcol = trans ? 0.9f : 1.0f;
	//if (trans)
	//{
	//	glEnable(GL_BLEND);
	//	glDepthMask(GL_FALSE);
	//}

	/*if (Settings::useshadersfalse && (shader >= 0))
	{
	// SHADER-BASED
	vec3 col2;
	waterShaders[shader]->bind();
	if (type == 2)
	{
	col = _EnvironmentManager->GetSkyColor(RIVER_COLOR_LIGHT);
	col2 = _EnvironmentManager->GetSkyColor(RIVER_COLOR_DARK);
	}
	else
	{
	col2 = col;
	}

	glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, col.x, col.y, col.z, tcol);
	glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, col2.x, col2.y, col2.z, tcol);

	glCallList(m_OGLList);

	waterShaders[shader]->unbind();
	}
	else
	{
	// FIXED-FUNCTION

	if (type == 0)
	{
	glColor4f(1, 1, 1, tcol);
	}
	else
	{
	if (type == 2)
	{
	col = _EnvironmentManager->GetSkyColor(RIVER_COLOR_LIGHT); // TODO: add variable water colo
	}
	glColor4f(col.x, col.y, col.z, tcol);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD); // TODO: check if ARB_texture_env_add is supported? :(
	}
	glCallList(m_OGLList);

	if (type != 0) glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	//glDepthFunc(GL_LEQUAL);
	//glColor4f(1, 1, 1, 1);
	//if (trans)
	//{
	//	glDepthMask(GL_TRUE);
	//	glDisable(GL_BLEND);
	//}
	*/
}

//

void Liquid::initGeometry(File& f)
{
	Liquid_Vertex* map = (Liquid_Vertex*)f.GetDataFromCurrent();
	Liquid_Flag* flags = (Liquid_Flag*)(f.GetDataFromCurrent() + m_TilesCount * sizeof(Liquid_Vertex));

	// generate vertices
	//vec3* verts = new vec3[m_TilesCount];
	//float* col = new float[m_TilesCount];

	MH2O_WaterLayer waterLayer;

	waterLayer.x = 0;
	waterLayer.y = 0;
	waterLayer.Width = m_TilesX;
	waterLayer.Height = m_TilesY;

	waterLayer.LiquidType = 0;
	waterLayer.MinHeightLevel = 0;
	waterLayer.MaxHeightLevel = 0;
	waterLayer.LiquidObjectOrLVF = 0; // FIXME Send format to create buffer

	for (uint32 j = 0; j < m_TilesY + 1; j++)
	{
		for (uint32 i = 0; i < m_TilesX + 1; i++)
		{
			uint32 p = j * (m_TilesX + 1) + i;

			if (flags[p].liquid & 0x08)
			{
				waterLayer.renderTiles.push_back(false);
			}
			else
			{
				waterLayer.renderTiles.push_back(true);
			}

			waterLayer.heights.push_back(map[p].magmaVert.height);
			//waterLayer.depths.push_back((map[p].magmaVert.s / 255.0f) * 0.5f + 0.5f);
		}
	}

	m_WaterLayers.push_back(waterLayer);

	createBuffer(m_Position);

	/*vector<MH2O_Vertex> mh2oVertices;

	// draw tiles
	for (int j = 0; j < m_TilesY; j++)
	{
		for (int i = 0; i < m_TilesX; i++)
		{
			Liquid_Flag f = flags[j * m_TilesX + i];
			if (f.liquid & 0x08)
			{
				continue;
			}

			// 15 seems to be "don't draw"
			uint32 p = j * (m_TilesX + 1) + i;

			mh2oVertices.push_back({
				verts[p],
				vec3(i / texRepeats, j / texRepeats, col[p]),
				defaultNormal
			});

			mh2oVertices.push_back({
				verts[p + 1],
				vec3((i + 1) / texRepeats, j / texRepeats, col[p + 1]),
				defaultNormal
			});

			mh2oVertices.push_back({
				verts[p + m_TilesX + 1 + 1],
				vec3((i + 1) / texRepeats, (j + 1) / texRepeats, col[p + m_TilesX + 1 + 1]),
				defaultNormal
			});

			mh2oVertices.push_back({
				verts[p + m_TilesX + 1],
				vec3(i / texRepeats, (j + 1) / texRepeats, col[p + m_TilesX + 1]),
				defaultNormal
			});
		}
	}

	globalBufferSize = static_cast<uint32>(mh2oVertices.size());

	if (globalBufferSize == 0)
	{
		return;
	}

	glGenBuffers(1, &globalBufferWater);
	glBindBuffer(GL_ARRAY_BUFFER, globalBufferWater);

	glBufferData(GL_ARRAY_BUFFER, mh2oVertices.size() * sizeof(MH2O_Vertex), mh2oVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	delete[] verts;
	delete[] col;*/
}

void Liquid::InitTextures(DBC_LiquidTypeRecord * _liquidType)
{
	assert1(_liquidType != nullptr);

	char buf[256];
	uint32_t counter = 1;
	while (true)
	{
		const char* filenameFormatted = _liquidType->Get_TextureName();
		sprintf_s(buf, filenameFormatted, counter);

		if (!MPQFile::IsFileExists(buf))
		{
			break;
		}

		textures.push_back(_TexturesMgr->Add(buf));
		counter++;
	}
}

void Liquid::initTextures(const char* basename, int first, int last)
{
	char buf[256];
	for (int i = first; i <= last; i++)
	{
		sprintf_s(buf, "%s.%d.blp", basename, i);
		if (!MPQFile::IsFileExists(buf))
		{
			break;
		}
		textures.push_back(_TexturesMgr->Add(buf));
	}
}



