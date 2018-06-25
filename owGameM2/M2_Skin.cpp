#include "stdafx.h"

// Include
#include "M2.h"

// General
#include "M2_Skin.h"

// Additional
#include "M2_Part_Material.h"

CM2_Skin::CM2_Skin(M2* _model) :
	m_ParentM2(_model),
	__geom(nullptr),
	m_QualitySettings(GetSettingsGroup<CGroupQuality>())
{}

CM2_Skin::~CM2_Skin()
{}

//

void CM2_Skin::Draw()
{
	_Render->r.setGeometry(__geom);

	for (auto& p : m_Batches)
	{

			p->Render();
		//p->__material.Restore();
	}
}


/*void CM2_Skin::RenderNormals(cmat4 _worldMatrix)
{
	_Render->getTechniquesMgr()->DebugNormal_Pass->Bind();
	_Render->getTechniquesMgr()->DebugNormal_Pass->SetWorldMatrix(_worldMatrix);

	_Render->r.setGeometry(__geomDebugNormals);

	for (auto p : m_Batches)
	{
		if (m_VisibleSubmeshes[p->m2SkinIndex])
		{
			_Render->r.drawIndexed
			(
				PRIM_TRILIST,
				p->m_SkinSection.indexStart,
				p->m_SkinSection.indexCount,
				p->m_SkinSection.vertexStart,
				p->m_SkinSection.vertexCount,
				false
			);
		}
	}

	_Render->getTechniquesMgr()->DebugNormal_Pass->Unbind();
}*/
