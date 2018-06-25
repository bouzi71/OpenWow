#include "stdafx.h"

// Include
#include "M2.h"

// General
#include "M2_Skin_Batch.h"


M2_Skin_Batch::M2_Skin_Batch(const M2* _parentM2) :
	m_ParentM2(_parentM2),

	color(nullptr),
	material(nullptr),
	texture_WeightIndex(nullptr),
	texture_Transform(nullptr),

	m_IsBilldoard(false),
	m_QualitySettings(GetSettingsGroup<CGroupQuality>())
{}

void M2_Skin_Batch::Render()
{
	material->Set();

	// Bind textures
	for (uint32 i = 0; i < m_Textures.size(); i++)
	{
		uint16 sampler = m_QualitySettings.Texture_Sampler;

		if (m_Textures[i]->isWrapX())
		{
			sampler |= SS_ADDRU_WRAP;
		}

		if (m_Textures[i]->isWrapY())
		{
			sampler |= SS_ADDRV_WRAP;
		}

		_Render->r.setTexture(Material::C_DiffuseTextureIndex + i, m_Textures[i]->getTexture(), sampler, 0);
	}

	_Render->getTechniquesMgr()->M2_Pass->SetShader(newShader);

	// Billboard
	_Render->getTechniquesMgr()->M2_Pass->SetBillboard(m_IsBilldoard);

	// Model color
	bool isTextureWeightEnable = (texture_WeightIndex != nullptr);
	_Render->getTechniquesMgr()->M2_Pass->SetTextureAnimEnable(isTextureWeightEnable);
	if (color != nullptr)
	{
		_Render->getTechniquesMgr()->M2_Pass->SetColor(color->getValue());
	}

	// Texture alpha
	bool isTextureWeightEnable = (texture_WeightIndex != nullptr);
	_Render->getTechniquesMgr()->M2_Pass->SetTextureAnimEnable(isTextureWeightEnable);
	if (isTextureWeightEnable)
	{
		_Render->getTechniquesMgr()->M2_Pass->SetTextureWeight(texture_WeightIndex->getValue());
	}

	// Texture transform
	bool isTextureTransformEnable = (texture_Transform != nullptr);
	_Render->getTechniquesMgr()->M2_Pass->SetTextureAnimEnable(isTextureTransformEnable);
	if (isTextureTransformEnable)
	{
		_Render->getTechniquesMgr()->M2_Pass->SetTextureAnimMatrix(texture_Transform->getValue());
	}

	//--

	_Render->r.drawIndexed
	(
		PRIM_TRILIST,
		m_SkinSection.indexStart,
		m_SkinSection.indexCount,
		m_SkinSection.vertexStart,
		m_SkinSection.vertexCount,
		false
	);
}
