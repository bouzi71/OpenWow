#include "stdafx.h"

// Include
#include "WMO_Base_Instance.h"

// General
#include "WMO.h"

CWMO::CWMO(IBaseManager& BaseManager, IRenderDevice& RenderDevice, const std::string& Filename)
	: m_FileName(Filename)
	, m_TexturesNames(nullptr)
	, m_DoodadsFilenames(nullptr)
	, m_BaseManager(BaseManager)
	, m_RenderDevice(RenderDevice)
{
	m_ChunkReader = std::make_unique<WoWChunkReader>(m_BaseManager, m_FileName);

	// Version
	{
		auto buffer = m_ChunkReader->OpenChunk("MVER");
		uint32 version;
		buffer->readBytes(&version, 4);
		_ASSERT(version == 17);
	}

	// Header
	{
		auto buffer = m_ChunkReader->OpenChunk("MOHD");
		buffer->readBytes(&m_Header, sizeof(SWMO_HeaderDef));

		m_Bounds = m_Header.bounding_box.Convert();
	}
}

CWMO::~CWMO()
{
}

//
// ISceneNodeProvider
//
void CWMO::CreateInsances(const std::shared_ptr<CWMO_Base_Instance>& Parent) const
{
	for (const auto& it : m_Groups)
	{
		auto groupInstance = Parent->CreateSceneNode<CWMO_Group_Instance>(it);
		Parent->AddGroupInstance(groupInstance);
		if (it->m_GroupHeader.flags.IS_OUTDOOR)
			Parent->AddOutdoorGroupInstance(groupInstance);

		groupInstance->Load();
		groupInstance->SetState(ILoadable::ELoadableState::Loaded);
		//m_BaseManager.GetManager<ILoader>()->AddToLoadQueue(groupInstance);
	}

#ifdef USE_WMO_PORTALS_CULLING
	for (const auto& groupPtr : Parent->getGroupInstances())
	{
		if (auto group = groupPtr.lock())
		{
			group->CreatePortals(Parent);
		}
		else _ASSERT(false);
	}
#endif

	for (auto& it : m_Lights)
	{
		//Parent->AddLight(it);
	}
}

bool CWMO::Load()
{
	// Textures
	if (auto buffer = m_ChunkReader->OpenChunk("MOTX"))
	{
		m_TexturesNames = std::unique_ptr<char[]>(new char[buffer->getSize() + 1]);
		buffer->readBytes(m_TexturesNames.get(), buffer->getSize());
		m_TexturesNames[buffer->getSize()] = 0x00;
	}

	// Materials
	size_t cntr = 0;
	{
		for (const auto& mat : m_ChunkReader->OpenChunkT<SWMO_MaterialDef>("MOMT"))
		{
			std::shared_ptr<WMO_Part_Material> material = std::make_shared<WMO_Part_Material>(m_RenderDevice, *this, mat);
			material->SetName(m_FileName + "_Material_" + std::to_string(cntr++));
			m_Materials.push_back(material);
		}
		_ASSERT(m_Materials.size() == m_Header.nTextures);
		//Log::Info("WMO: Materials count = '%d'", m_Materials.size());
	}

	// Group names
	if (auto buffer = m_ChunkReader->OpenChunk("MOGN"))
	{
		m_GroupNames = std::unique_ptr<char[]>(new char[buffer->getSize() + 1]);
		buffer->readBytes(m_GroupNames.get(), buffer->getSize());
		m_GroupNames[buffer->getSize()] = 0x00;
	}

	// Skybox
	
	if (auto buffer = m_ChunkReader->OpenChunk("MOSB"))
	{
		if (buffer->getSize() > 4)
		{
			std::unique_ptr<char[]> skyboxFilename = std::unique_ptr<char[]>(new char[buffer->getSize() + 1]);
			buffer->readBytes(skyboxFilename.get(), buffer->getSize());
			skyboxFilename[buffer->getSize()] = 0x00;

			Log::Error("WMO[%s]: Skybox [%s]", m_FileName.c_str(), skyboxFilename.get());
		}
	}

	// Portal vertices
	std::vector<glm::vec3> portalVertices;
	{
		for (const auto& pv : m_ChunkReader->OpenChunkT<glm::vec3>("MOPV"))
		{
			portalVertices.push_back(Fix_XZmY(pv));
		}
	}

	// Portal defs
	std::vector<CWMO_Part_Portal> portals;
	for (const auto& pt : m_ChunkReader->OpenChunkT<SWMO_PortalDef>("MOPT"))
	{
		portals.push_back(CWMO_Part_Portal(m_RenderDevice, portalVertices, pt));
	}

	// Portal references
	std::vector<SWMO_PortalReferencesDef> portalsReferences;
	{
		for (const auto& pr : m_ChunkReader->OpenChunkT<SWMO_PortalReferencesDef>("MOPR"))
		{
			_ASSERT(pr.portalIndex >= 0 && pr.portalIndex < portals.size());
			auto& portal = portals[pr.portalIndex];
			portal.setGroup(pr.groupIndex, pr.side);
			portalsReferences.push_back(pr);
		}
	}

	// Visible vertices
	for (const auto& vv : m_ChunkReader->OpenChunkT<glm::vec3>("MOVV"))
	{
		m_VisibleBlockVertices.push_back(Fix_XZmY(vv));
	}

	// Visible blocks
	for (const auto& vb : m_ChunkReader->OpenChunkT<SWMO_VisibleBlockListDef>("MOVB"))
	{
		m_VisibleBlockList.push_back(vb);
	}

	// Lights
	for (const auto& lt : m_ChunkReader->OpenChunkT<SWMO_LightDef>("MOLT"))
	{
		m_Lights.push_back(std::make_shared<WMO_Part_Light>(lt));
	}

	// Doodads set
	for (const auto& ds : m_ChunkReader->OpenChunkT<SWMO_Doodad_SetInfo>("MODS"))
	{
		m_DoodadsSetInfos.push_back(ds);
	}
	//Log::Info("WMO: Doodads count = '%d'", m_DoodadsSetInfos.size());

	// Doodads filenames
	if (auto buffer = m_ChunkReader->OpenChunk("MODN"))
	{
		m_DoodadsFilenames = std::unique_ptr<char[]>(new char[buffer->getSize() + 1]);
		buffer->readBytes(m_DoodadsFilenames.get(), buffer->getSize());
		m_DoodadsFilenames[buffer->getSize()] = 0x00;
	}

	// Doodads placemnts
	for (const auto& dd : m_ChunkReader->OpenChunkT<SWMO_Doodad_PlacementInfo>("MODD"))
	{
		m_DoodadsPlacementInfos.push_back(dd);
	}

	// HACK! INCORRECT SIZE
	//m_Header.nDoodadDefs = m_DoodadsPlacementInfos.size();

	// Fog
	for (const auto& fog : m_ChunkReader->OpenChunkT<SWMO_FogDef>("MFOG"))
	{
		m_Fogs.push_back(std::make_shared<WMO_Part_Fog>(fog));
	}

	// Group info
	{
		uint32 cntr = 0;
		for (const auto& groupInfo : m_ChunkReader->OpenChunkT<SWMO_GroupInfoDef>("MOGI"))
		{
			std::shared_ptr<WMO_Group> group = std::make_shared<WMO_Group>(m_BaseManager, m_RenderDevice, std::dynamic_pointer_cast<CWMO>(shared_from_this()), cntr++, groupInfo);
			group->Load();
			group->SetState(ELoadableState::Loaded);
			//m_BaseManager.GetManager<ILoader>()->AddToLoadQueue(group);
			m_Groups.push_back(group);

			if (group->m_GroupHeader.flags.IS_OUTDOOR)
			{
				m_OutdoorGroups.push_back(group);
			}
		}
	}

	m_ChunkReader.reset();

	// Setup group portals
	{
		for (const auto& ref : portalsReferences)
		{
			auto portal = portals[ref.portalIndex];
			auto group = m_Groups[ref.groupIndex];

			if (ref.groupIndex == group->GetGroupIndex())
			{
				group->AddPortal(portal);
			}
			else _ASSERT(false);
		}
	}

	// Create portal controller
	if (portals.size() > 0)
	{
#ifdef USE_WMO_PORTALS_CULLING
		m_PortalController = std::make_shared<CWMO_PortalsController>();

		for (const auto& it : portalsReferences)
		{
			_ASSERT(it.portalIndex < portals.size());
			_ASSERT(it.groupIndex < m_Groups.size());
}
#endif
	}

	return true;
}
