#include "stdafx.h"

// Additional
#include "WMO.h"
#include "WMO_Base_Instance.h"

// General
#include "WMO_Group.h"

// Include 
#include "WMO_Doodad_Instance.h"
#include "WMO_Liquid_Instance.h"

WMO_Group::WMO_Group(const WMO* _parentWMO, const uint32 _groupIndex, string _groupName, IFile* _groupFile) :
	m_ParentWMO(_parentWMO),
	m_GroupName(_groupName),
	m_GroupIndex(_groupIndex),
	m_F(_groupFile),
	m_IsMOCVExists(false),
	m_PortalsVis(true),
	m_Calculated(false),
	m_Quality(GetSettingsGroup<CGroupQuality>())
{
	m_WMOLiqiud = nullptr;
}

WMO_Group::~WMO_Group()
{
}

void WMO_Group::CreateInsances(CWMO_Base_Instance* _parent)
{
	if (m_WMOLiqiud != nullptr)
	{
		vec3 realPos = m_LiquidHeader.pos.toXZmY();
		realPos.y = 0.0f; // why they do this???

		CWMO_Liquid_Instance* liquid = new CWMO_Liquid_Instance(_parent, m_WMOLiqiud, realPos, this);
		_parent->AddWMOLiquidInstance(liquid);
	}

	for (auto& index : m_DoodadsPlacementIndexes)
	{
		const WMO_Doodad_PlacementInfo& placement = m_ParentWMO->m_DoodadsPlacementInfos[index];

		SmartPtr<M2> mdx = (M2*)GetManager<IM2Manager>()->Add(m_ParentWMO->m_DoodadsFilenames + placement.flags.nameIndex);
		if (mdx)
		{
			CWMO_Doodad_Instance* instance = new CWMO_Doodad_Instance(_parent, mdx, index, placement, this);
			_parent->AddDoodadInstance(instance);
		}
	}
}


void WMO_Group::Load()
{
	// Buffer
	R_Buffer*	indexBuffer = nullptr;
	R_Buffer*	VB_Vertexes = nullptr;
	vector<SmartBufferPtr>	VB_TextureCoords;
	R_Buffer*	VB_Normals = nullptr;
	R_Buffer*	VB_Colors = nullptr;

	// Read file
	char fourcc[5];
	uint32 size = 0;
	while (!m_F->isEof())
	{
		memset(fourcc, 0, 4);
		size = 0;
		m_F->readBytes(fourcc, 4);
		m_F->readBytes(&size, 4);
		flipcc(fourcc);
		fourcc[4] = 0;
		if (size == 0)	continue;
		uint32_t nextpos = m_F->getPos() + size;

		if (strcmp(fourcc, "MVER") == 0)
		{
			uint32 version;
			m_F->readBytes(&version, 4);
			assert1(version == 17);
		}
		else if (strcmp(fourcc, "MOGP") == 0)
		{
			// The MOGP chunk size will be way more than the header variables!
			nextpos = m_F->getPos() + sizeof(WMO_Group_HeaderDef);

			m_F->readBytes(&m_Header, sizeof(WMO_Group_HeaderDef));

			// Bounds
			m_Bounds.set(m_Header.boundingBox.min, m_Header.boundingBox.max, true);
		}
		else if (strcmp(fourcc, "MOPY") == 0) // Material info for triangles
		{
			uint32 materialsInfoCount = size / sizeof(WMO_Group_MaterialDef);
			WMO_Group_MaterialDef* materialsInfo = (WMO_Group_MaterialDef*)m_F->getDataFromCurrent();
			for (uint32 i = 0; i < materialsInfoCount; i++)
			{
				m_MaterialsInfo.push_back(materialsInfo[i]);
			}
		}
		else if (strcmp(fourcc, "MOVI") == 0) // Indices
		{
			assert1(indexBuffer == nullptr);
			uint32 indicesCount = size / sizeof(uint16);
			uint16* indices = (uint16*)m_F->getDataFromCurrent();
			// Buffer
			indexBuffer = _Render->r.createIndexBuffer(indicesCount * sizeof(uint16), indices, false);
		}
		else if (strcmp(fourcc, "MOVT") == 0) // Vertices chunk.
		{
			assert1(VB_Vertexes == nullptr);
			uint32 vertexesCount = size / sizeof(vec3);
			vec3* vertexes = (vec3*)m_F->getDataFromCurrent();
			// Convert
			for (uint32 i = 0; i < vertexesCount; i++)
			{
				vertexes[i] = vertexes[i].toXZmY();
			}
			// Buffer
			VB_Vertexes = _Render->r.createVertexBuffer(vertexesCount * sizeof(vec3), vertexes, false);
			VB_Colors = _Render->r.createVertexBuffer(vertexesCount * sizeof(vec4), nullptr, false);
			//m_Bounds.calculate(vertexes, vertexesCount);
		}
		else if (strcmp(fourcc, "MONR") == 0) // Normals
		{
			assert1(VB_Normals == nullptr);
			uint32 normalsCount = size / sizeof(vec3);
			vec3* normals = (vec3*)m_F->getDataFromCurrent();
			// Convert
			for (uint32 i = 0; i < normalsCount; i++)
			{
				normals[i] = normals[i].toXZmY();
			}
			// Buffer
			VB_Normals = _Render->r.createVertexBuffer(normalsCount * sizeof(vec3), normals, false);
		}
		else if (strcmp(fourcc, "MOTV") == 0) // R_Texture coordinates
		{
			uint32 textureCoordsCount = size / sizeof(vec2);
			vec2* textureCoords = (vec2*)m_F->getDataFromCurrent();
			VB_TextureCoords.push_back(_Render->r.createVertexBuffer(textureCoordsCount * sizeof(vec2), textureCoords, false));
		}
		else if (strcmp(fourcc, "MOBA") == 0) // WMO_Group_Batch
		{
			for (uint32 i = 0; i < size / sizeof(WMO_Group_BatchDef); i++)
			{
				WMO_Group_BatchDef batchDef;
				m_F->readBytes(&batchDef, sizeof(WMO_Group_BatchDef));

				SmartPtr<WMO_Group_Part_Batch> batch = new WMO_Group_Part_Batch(m_ParentWMO, batchDef);
				m_WMOBatchIndexes.push_back(batch);
			}

			std::sort(m_WMOBatchIndexes.begin(), m_WMOBatchIndexes.end(), WMO_Group_Part_BatchCompare());
		}
		else if (strcmp(fourcc, "MOLR") == 0) // Light references
		{
			assert1(m_Header.flags.HAS_LIGHTS);
			uint32 lightsIndexesCount = size / sizeof(uint16);
			uint16* lightsIndexes = (uint16*)m_F->getDataFromCurrent();
			for (uint32 i = 0; i < lightsIndexesCount; i++)
			{
				m_WMOLightsIndexes.push_back(lightsIndexes[i]);
			}
		}
		else if (strcmp(fourcc, "MODR") == 0) // Doodad references
		{
			assert1(m_Header.flags.HAS_DOODADS);
			uint32 doodadsIndexesCount = size / sizeof(uint16);
			uint16* doodadsIndexes = (uint16*)m_F->getDataFromCurrent();
			for (uint32 i = 0; i < doodadsIndexesCount; i++)
			{
				uint16 index = doodadsIndexes[i];
				m_DoodadsPlacementIndexes.push_back(index);
			}
		}
		else if (strcmp(fourcc, "MOBN") == 0)
		{
			assert1(m_Header.flags.HAS_COLLISION);
		}
		else if (strcmp(fourcc, "MOBR") == 0)
		{
			assert1(m_Header.flags.HAS_COLLISION);
		}
		else if (strcmp(fourcc, "MOCV") == 0) // Vertex colors
		{
			assert1(m_Header.flags.HAS_VERTEX_COLORS);
			uint32 vertexColorsCount = size / sizeof(CBgra);
			CBgra* vertexColors = (CBgra*)m_F->getDataFromCurrent();
			// Convert
			vector<vec4> vertexColorsConverted;
			for (uint32 i = 0; i < vertexColorsCount; i++)
			{
				vertexColorsConverted.push_back(vec4
				(
					static_cast<float>(vertexColors[i].r) / 255.0f,
					static_cast<float>(vertexColors[i].g) / 255.0f,
					static_cast<float>(vertexColors[i].b) / 255.0f,
					static_cast<float>(vertexColors[i].a) / 255.0f
				));
			}
			// Buffer
			VB_Colors = _Render->r.createVertexBuffer(vertexColorsConverted.size() * sizeof(vec4), vertexColorsConverted.data(), false);
			m_IsMOCVExists = vertexColorsCount > 0;
		}
		else if (strcmp(fourcc, "MLIQ") == 0) // Liquid
		{
			m_F->readBytes(&m_LiquidHeader, sizeof(WMO_Group_MLIQDef));

			if (m_Header.liquidType > 0)
			{
				Log::Green("WMO[%s]: LiquidType CHUNK = [%d]", m_ParentWMO->m_FileName.c_str(), m_LiquidHeader.type);

				//assert1(m_Header.liquidType == 15);
				m_WMOLiqiud = new CWMO_Liquid(m_LiquidHeader.A, m_LiquidHeader.B);
				m_WMOLiqiud->CreateFromWMO(m_F, m_ParentWMO->m_Materials[m_LiquidHeader.type], DBC_LiquidType[m_Header.liquidType & 3], m_Header.flags.IS_INDOOR);
			}
		}
		else
		{
			Log::Fatal("WMO_Group[]: No implement group chunk %s [%d].", fourcc, size);
		}
		m_F->seek(nextpos);
	}

	//

	/*WMO_Part_Fog* wf = m_ParentWMO->m_Fogs[m_Header.m_Fogs[0]];
	if (wf->fogDef.largerRadius <= 0)
		fog = -1; // default outdoor fog..?
	else
		fog = m_Header.m_Fogs[0];*/

		//

	assert1(VB_TextureCoords.size() < 3);

	__geom = _Render->r.beginCreatingGeometry(_Render->getRenderStorage()->__layoutWMO_VC);
	__geom->setGeomVertexParams(VB_Vertexes, R_DataType::T_FLOAT, 0, 0);
	__geom->setGeomVertexParams(VB_TextureCoords[0], R_DataType::T_FLOAT, 0, 0);

	if (VB_TextureCoords.size() == 2)
	{
		__geom->setGeomVertexParams(VB_TextureCoords[1], R_DataType::T_FLOAT, 0, 0);
	}
	else
	{
		__geom->setGeomVertexParams(VB_TextureCoords[0], R_DataType::T_FLOAT, 0, 0);
	}
	__geom->setGeomVertexParams(VB_Normals, R_DataType::T_FLOAT, 0, 0);
	__geom->setGeomVertexParams(VB_Colors, R_DataType::T_FLOAT, 0, 0);
	__geom->setGeomIndexParams(indexBuffer, R_IndexFormat::IDXFMT_16);
	__geom->finishCreatingGeometry();
}

void WMO_Group::initLighting()
{
	/*if (m_Header.flags.FLAG_IS_INDOOR && m_Header.flags.FLAG_HAS_VERTEX_COLORS)
	{
		vec3 dirmin(1, 1, 1);
		float lenmin;
		int lmin;
		for (uint32 i = 0; i < m_DoodadsIndexesCount; i++)
		{
			lenmin = 999999.0f * 999999.0f;
			lmin = 0;
			DoodadInstance* mi = m_ParentWMO->m_M2Instances[m_PlacementInfoIndexes[i]];
			for (uint32 j = 0; j < m_ParentWMO->m_Header.nLights; j++)
			{
				WMO_Part_Light* l = m_ParentWMO->m_Lights[j];
				vec3 dir = l->lightDef.pos - mi->placementInfo->position;
				float ll = dir.length2();
				if (ll < lenmin)
				{
					lenmin = ll;
					dirmin = dir;
					lmin = j;
				}
			}
			mi->light = lmin;
			mi->ldir = dirmin;
		}
		m_EnableOutdoorLights = false;
	}
	else
	{
		m_EnableOutdoorLights = true;
	}*/
}

//



void WMO_Group::Render(cmat4 _worldMatrix, const WMO_Doodad_SetInfo& _doodadSet)
{
	if (!m_PortalsVis)
	{
		return;
	}

	// GROUPS
	_Render->r.setGeometry(__geom);

	CWMO_GeomertyPass* pass = _Render->getTechniquesMgr()->m_WMO_GeometryPass;
	{
		// Vertex colors
		pass->SetHasMOCV(m_IsMOCVExists && m_Quality.WMO_MOCV);

		// Ambient color
		pass->SetUseAmbColor(m_Quality.WMO_AmbColor);
		pass->SetAmbColor(m_ParentWMO->m_Header.getAmbColor());

		for (auto it : m_WMOBatchIndexes)
		{
			it->Render();
		}
	}
}