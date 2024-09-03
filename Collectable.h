#pragma once

#include "modelclass.h"
#include "Terrain.h"
#include <random>

using namespace DirectX;

class Collectable
{
public:
	Collectable();
	~Collectable();

	void	Initialise(ID3D11Device* device, Terrain* terrain, int id, int totalCollectables);
	void	SetSpawnPosition();
	void	CheckCollision(SimpleMath::Vector3 targetSphereCenter, float targetSphereRadius, float scalingFactor);
	int		NewRandomInt(int min, int max);

	ModelClass				m_collectableSphere;
	SimpleMath::Vector3		m_spawnPosition;
	SimpleMath::Vector3		m_lastPosition;
	bool					m_isHit;
	int						m_id;
	float					m_radius;
	float					m_distanceFromCamera;

private:
	Terrain*				m_terrain;
	int						m_totalCollectables;
};

#pragma once